/*
 * Antifreeze: A ESP32 program to periodically run baseboard heat during
 * periods of cold weather. See https://github.com/kghose/antifreeze
 *
 * (c) 2024 Kaushik Ghose
 *
 * Released under the terms of the MIT License
 */

#include <esp_log.h>

#include "constants.h"
#include "driver/gpio.h"
#include "ds18b20.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "httpserver.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "owb.h"
#include "owb_rmt.h"
#include "state.h"
#include "wifi.h"

static const char* TAG = "antifreeze";

void pulse_led(uint32_t led_on_ticks, uint32_t period_ticks) {
  gpio_set_level(LED_PIN, 1);
  vTaskDelay(led_on_ticks);
  gpio_set_level(LED_PIN, 0);
  vTaskDelay(period_ticks - led_on_ticks);
}

void heartbeat_task() {
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  while (true) {
    // The lag in switching state indication is acceptable
    if (get_relay_activated()) {
      pulse_led(RELAY_ACTIVATED_LED_ON_TICKS, RELAY_ACTIVATED_HEARTBEAT_TICKS);
    } else if (freeze_danger_present()) {
      pulse_led(LED_ON_TICKS, FREEZE_DANGER_HEARTBEAT_TICKS);
    } else {
      pulse_led(LED_ON_TICKS, NORMAL_HEARTBEAT_TICKS);
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
         MAX_CIRC_INTERVAL_S / (1 + RELAY_PERIOD_SCALING_CONSTANT * delta_t);
}

void relay_activate_task() {
  gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);
  time_t now_s;
  while (true) {
    time_t next_relay_activation_time_s = get_next_relay_activation_time_s();
    time(&now_s);
    if (now_s < next_relay_activation_time_s) {
      vTaskDelay(RELAY_TASK_INTERVAL_TICKS);
      continue;
    }
    gpio_set_level(RELAY_PIN, 1);
    set_relay_activated(now_s);
    vTaskDelay(CIRC_ON_TICKS);
    gpio_set_level(RELAY_PIN, 0);
    set_relay_deactivated();
  }
}

// TODO: Clean up this function
void temperature_sample_task(void* pvParameter) {
  // There is something sensitive here, possibly task related:
  // If the initialization lines are moved to a separate function the ds18b20
  // stops responding.

  OneWireBus* owb = NULL;
  // Stable readings require a brief period before communication
  vTaskDelay(2000.0 / portTICK_PERIOD_MS);

  owb_rmt_driver_info rmt_driver_info;
  owb = owb_rmt_initialize(&rmt_driver_info, TEMP_SENSOR_PIN, RMT_CHANNEL_1,
                           RMT_CHANNEL_0);
  owb_use_crc(owb, true);  // enable CRC check for ROM code

  OneWireBus_SearchState search_state = {0};
  bool found = false;
  while (!found) {
    ESP_LOGI(TAG, "Looking for DS18B20 outdoor temp probe.");
    owb_search_first(owb, &search_state, &found);
    vTaskDelay(TEMP_SAMPLE_PERIOD_TICKS);
  }

  char rom_code_s[OWB_ROM_CODE_STRING_LENGTH];
  owb_string_from_rom_code(search_state.rom_code, rom_code_s,
                           sizeof(rom_code_s));
  ESP_LOGI(TAG, "Probe found. ROM Code:  %s\n", rom_code_s);

  // Create DS18B20 device on the 1-Wire bus
  DS18B20_Info* ds18b20_info = ds18b20_malloc();  // heap allocation
  ds18b20_init_solo(ds18b20_info, owb);           // only one device on bus
  ds18b20_use_crc(ds18b20_info, true);  // enable CRC check on all reads
  ds18b20_set_resolution(ds18b20_info,
                         DS18B20_RESOLUTION_12_BIT);  // The counterfeit ones
                                                      // make trouble with 9bit

  float t_c = 0;
  while (true) {
    DS18B20_ERROR err = ds18b20_convert_and_read_temp(ds18b20_info, &t_c);
    set_outside_temp_c(t_c);
    vTaskDelay(TEMP_SAMPLE_PERIOD_TICKS);
  }
}

void init_flash() {
  // NVS required for WiFi
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

void init_time() {
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
  esp_netif_sntp_init(&config);
  if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update system time within 10s timeout");
  } else {
    ESP_LOGI(TAG, "Obtained time from " NTP_SERVER);
  }
  setenv("TZ", TIME_ZONE, 1);
  tzset();

  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "Antifreeze thinks the time is: %s\n", strftime_buf);
}

void start_mdns_service() {
  // initialize mDNS service
  esp_err_t err = mdns_init();
  if (err) {
    printf("MDNS Init failed: %d\n", err);
    return;
  }

  mdns_hostname_set(MDNS_HOSTNAME);
  mdns_instance_name_set(MDNS_INSTANCE_NAME);

  mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
  mdns_service_add(NULL, MDNS_SERVICENAME, "_tcp", 80, NULL, 0);
}

void app_main() {
  init_flash();
  wifi_init_sta();
  init_time();
  start_mdns_service();

  // TODO: error check inside the function
  ESP_ERROR_CHECK(initialize_state());

  TaskHandle_t heartbeat_task_h;
  TaskHandle_t relay_activate_task_h;
  TaskHandle_t temperature_sample_task_h;
  TaskHandle_t http_server_task_h;

  xTaskCreate(heartbeat_task, "Heartbeat", 1024, NULL, tskIDLE_PRIORITY,
              &heartbeat_task_h);
  xTaskCreate(relay_activate_task, "Relay Activate", 1024, NULL,
              tskIDLE_PRIORITY, &relay_activate_task_h);
  xTaskCreate(temperature_sample_task, "Temp Sample", 4096, NULL,
              tskIDLE_PRIORITY, &temperature_sample_task_h);
  xTaskCreate(http_server_task, "HTTP server", 4096, NULL, tskIDLE_PRIORITY,
              &http_server_task_h);

  while (true) {
    State state = get_state();
    ESP_LOGI(TAG, "Temperature: %0.1f", state.outside_temp_c);
    ESP_LOGI(TAG, "Relay activated %llds ago",
             get_relay_activated_elapsed_time_s());
    vTaskDelay(5 * configTICK_RATE_HZ);
  }
}
