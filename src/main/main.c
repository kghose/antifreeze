/*
 * Antifreeze: A ESP32 program to periodically run baseboard heat during
 * periods of cold weather. See https://github.com/kghose/antifreeze
 *
 * (c) 2024 Kaushik Ghose
 *
 * Released under the terms of the MIT License
 */

#include "constants.h"
#include "driver/gpio.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "httpserver.h"
#include "nvs_flash.h"
#include "state.h"
#include "wifi.h"

void heartbeat_task() {
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  while (true) {
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(LED_ON_PERIOD_MS);
    gpio_set_level(LED_PIN, 0);

    // The lag in switching state indication is acceptable
    if (get_relay_activated()) {
      vTaskDelay(RELAY_ACTIVATED_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
    } else if (freeze_danger_present()) {
      vTaskDelay(FREEZE_DANGER_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
    } else {
      vTaskDelay(NORMAL_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
    }
  }
}

time_t get_next_relay_activation_time_s() {
  float outside_temp_c = get_outside_temp_c();
  float freeze_danger_temp_c = get_freeze_danger_temp_c();

  if (outside_temp_c > freeze_danger_temp_c) {
    return THE_END_OF_TIME;
  }

  float delta_t = freeze_danger_temp_c - outside_temp_c;
  time_t relay_last_activated_s = get_relay_activated_time_s();
  return relay_last_activated_s +
         MAX_RELAY_PERIOD_S / (1 + RELAY_PERIOD_SCALING_CONSTANT * delta_t);
}

void relay_activate_task() {
  gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);
  time_t now_s;
  while (true) {
    vTaskDelay(SAMPLE_PERIOD_TICKS);
    time_t next_relay_activation_time_s = get_next_relay_activation_time_s();
    time(&now_s);
    if (now_s < next_relay_activation_time_s) continue;

    gpio_set_level(RELAY_PIN, 1);
    set_relay_activated(now_s);
    vTaskDelay(RELAY_ON_TICKS);
    gpio_set_level(RELAY_PIN, 0);
    set_relay_deactivated();
  }
}

void temperature_sample_task() {
  // For debugging
  float t = 0;
  while (true) {
    t -= 2;
    if (t < -20) {
      t = 4;
    }
    set_outside_temp_c(t);
    //    printf("Simulated T=%f\n", t);
    vTaskDelay(10 * configTICK_RATE_HZ);
  }
}

void app_main() {
  // NVS required for WiFi
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  wifi_init_sta();

  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
  esp_netif_sntp_init(&config);
  if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
    printf("Failed to update system time within 10s timeout");
  }
  setenv("TZ", TIME_ZONE, 1);
  tzset();

  ESP_ERROR_CHECK(initialize_state());

  TaskHandle_t heartbeat_task_h;
  TaskHandle_t relay_activate_task_h;
  TaskHandle_t temperature_sample_task_h;
  TaskHandle_t http_server_task_h;

  xTaskCreate(heartbeat_task, "Heartbeat", 1024, NULL, tskIDLE_PRIORITY,
              &heartbeat_task_h);
  xTaskCreate(relay_activate_task, "Relay Activate", 1024, NULL,
              tskIDLE_PRIORITY, &relay_activate_task_h);
  xTaskCreate(temperature_sample_task, "Temp Sample", 1024, NULL,
              tskIDLE_PRIORITY, &temperature_sample_task_h);
  xTaskCreate(http_server_task, "HTTP server", 4096, NULL, tskIDLE_PRIORITY,
              &http_server_task_h);
}
