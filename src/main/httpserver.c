/*
 * Manages the simple http server for the program
 */

#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "state.h"

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)
#define IDLE_TICKS 5 * configTICK_RATE_HZ

static const char* TAG = "HTTP server";

// TOOD: Maybe this should be part of state ...
// Currently initialized when the http server is started
time_t boot_time;

// https://stackoverflow.com/a/16043969
char* root_page_template =
    "<head>"
    "<meta http-equiv=\"refresh\" content=\"30;url=/\"/>"
    "<style>"
    "body {"
    "margin: auto;"
    "width: 40em; height: 40em;"
    "font-family: Arial, sans-serif; font-size:1vw;"
    "}"
    "</style>"
    "</head>"
    "<body>"
    "<hr>"
    "Outside temp:         %0.1f C</br>"
    "Freeze danger temp:   %0.1f C</br>"
    "Relay last activated: %s </br>"
    "<hr>"
    "Antifreeze up since:  %s"
    "<p><a href=\"/relay_test\">Relay test</a></p>"
    "</body>";

/* Serves the info page */
static esp_err_t root_get_handler(httpd_req_t* req) {
  State state = get_state();

  struct tm timeinfo;

  char boot_time_buf[64] = "";
  localtime_r(&boot_time, &timeinfo);
  strftime(boot_time_buf, sizeof(boot_time_buf), "%c", &timeinfo);

  char relay_time_buf[64] = "";
  localtime_r(&state.relay_activated_time_s, &timeinfo);
  strftime(relay_time_buf, sizeof(relay_time_buf), "%c", &timeinfo);

  char* resp;
  // TODO check for error
  asprintf(&resp, root_page_template, state.outside_temp_c,
           state.freeze_danger_temp_c, relay_time_buf, boot_time_buf);

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

// TODO: Implement a proper temp override for testing
static esp_err_t relay_test_handler(httpd_req_t* req) {
  set_outside_temp_c(-100);

  return root_get_handler(req);
}

static const httpd_uri_t relay_test = {
    .uri = "/relay_test",
    .method = HTTP_GET,
    .handler = relay_test_handler,
};

static httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &relay_test);
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server) {
  // Stop the httpd server
  return httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
  httpd_handle_t* server = (httpd_handle_t*)arg;
  if (*server) {
    ESP_LOGI(TAG, "Stopping webserver");
    if (stop_webserver(*server) == ESP_OK) {
      *server = NULL;
    } else {
      ESP_LOGE(TAG, "Failed to stop http server");
    }
  }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data) {
  httpd_handle_t* server = (httpd_handle_t*)arg;
  if (*server == NULL) {
    ESP_LOGI(TAG, "Starting webserver");
    *server = start_webserver();
  }
}

void http_server_task() {
  time(&boot_time);

  static httpd_handle_t server = NULL;
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &connect_handler, &server));
  ESP_ERROR_CHECK(esp_event_handler_register(
      WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
  server = start_webserver();
  while (server) {
    vTaskDelay(IDLE_TICKS);
  }
}