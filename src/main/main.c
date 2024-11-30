/*
 * Antifreeze: A ESP32 program to periodically run baseboard heat during periods
 * of cold weather. See https://github.com/kghose/antifreeze
 *
 * (c) 2024 Kaushik Ghose
 *
 * Released under the terms of the MIT License
 */

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "constants.h"
#include "state.h"

void heartbeat_task() {
  enum OperationStatus operation_status;
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  while (true) {
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(LED_ON_PERIOD_MS);
    gpio_set_level(LED_PIN, 0);

    // The delay in switching state indication is acceptable
    operation_status = get_state().operation_status;
    switch (operation_status) {
    case NORMAL:
      vTaskDelay(NORMAL_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
      break;
    case FREEZE_DANGER:
      vTaskDelay(FREEZE_DANGER_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
      break;
    case RELAY_ACTIVATED:
      vTaskDelay(RELAY_ACTIVATED_HEARTBEAT_PERIOD_MS - LED_ON_PERIOD_MS);
      break;
    }
  }
}

void app_main() {
  ESP_ERROR_CHECK(initialize_state());

  TaskHandle_t heartbeat_task_h;

  xTaskCreate(heartbeat_task, "Heartbeat", 1024, NULL, tskIDLE_PRIORITY,
              &heartbeat_task_h);
}
