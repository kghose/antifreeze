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
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
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

void app_main() {
  wifi_init_sta();

  setenv("TZ", TIME_ZONE, 1);
  tzset();

  ESP_ERROR_CHECK(initialize_state());

  TaskHandle_t heartbeat_task_h;
  TaskHandle_t relay_activate_task_h;

  xTaskCreate(heartbeat_task, "Heartbeat", 1024, NULL, tskIDLE_PRIORITY,
              &heartbeat_task_h);
  xTaskCreate(relay_activate_task, "Relay Activate", 1024, NULL,
              tskIDLE_PRIORITY, &relay_activate_task_h);
}
