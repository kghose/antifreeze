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
  State state;
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  while (true) {
    state = get_state();

    gpio_set_level(LED_PIN, 1);
    vTaskDelay(LED_ON_PERIOD_MS);
    gpio_set_level(LED_PIN, 0);

    // The lag in switching state indication is acceptable
    if (state.relay_activated) {
      vTaskDelay(RELAY_ACTIVATED_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
    } else if (state.freeze_danger) {
      vTaskDelay(FREEZE_DANGER_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
    } else {
      vTaskDelay(NORMAL_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
    }
  }
}

time_t get_next_relay_activation_time(State *state) {
  float relay_activation_interval_s =
      60.0 *
      (60.0 - state->k * (state->t_freeze_danger_c - state->temperature_c));
  uint64_t next_relay_activation_s;
  if (relay_activation_interval_s > 0) {
    next_relay_activation_s =
        state->relay_activated_time_s + relay_activation_interval_s;
  } else {
    next_relay_activation_s = 0;  // Do it now
  }
  return next_relay_activation_s;
}

void relay_task() {
  setenv("TZ", TIME_ZONE, 1);
  tzset();

  State state;
  time_t current_time_s;

  while (true) {
    vTaskDelay(RELAY_SAMPLE_PERIOD_MS);

    state = get_state();
    if (state.temperature_c > state.t_freeze_danger_c) {
      continue;
    }

    time(&current_time_s);
    if (!state.relay_activated) {
      uint64_t next_relay_activation_s = get_next_relay_activation_time(&state);
      if (current_time_s > next_relay_activation_s) {
        gpio_set_level(RELAY_PIN, 1);
        state.relay_activated_time_s = current_time_s;
        state.relay_activated = true;
      }
    } else {
      if (current_time_s > state.relay_activated_time_s + RELAY_ON_TIME_S) {
        gpio_set_level(RELAY_PIN, 0);
        state.relay_activated = false;
      }
    }
  }
}

void app_main() {
  wifi_init_sta();

  ESP_ERROR_CHECK(initialize_state());

  TaskHandle_t heartbeat_task_h;
  TaskHandle_t relay_task_h;

  xTaskCreate(heartbeat_task, "Heartbeat", 1024, NULL, tskIDLE_PRIORITY,
              &heartbeat_task_h);
  xTaskCreate(relay_task, "Relay", 1024, NULL, tskIDLE_PRIORITY, &relay_task_h);
}
