#include "state.h"

#include "constants.h"

State state;
SemaphoreHandle_t state_mutex;

esp_err_t initialize_state() {
  state_mutex = xSemaphoreCreateMutex();
  if (state_mutex == NULL) {
    printf(
        "[ERROR] Could not initialize state "
        "mutex.");
    return ESP_ERR_INVALID_STATE;
  }
  state.freeze_danger_temp_c = DEFAULT_FREEZE_DANGER_TEMP_C;
  state.outside_temp_c = 25;  // Takes us a moment to get the temperature and we
                              // don't want to trigger the relay
  state.relay_activated_time_s = 0;
  state.relay_on = false;

  return ESP_OK;
}

void set_freeze_danger_temp_c(float t) {
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  state.freeze_danger_temp_c = t;
  xSemaphoreGive(state_mutex);
}

float get_freeze_danger_temp_c() {
  float t;
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  t = state.freeze_danger_temp_c;
  xSemaphoreGive(state_mutex);
  return t;
}

void set_outside_temp_c(float t) {
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  state.outside_temp_c = t;
  xSemaphoreGive(state_mutex);
}

float get_outside_temp_c() {
  float t;
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  t = state.outside_temp_c;
  xSemaphoreGive(state_mutex);
  return t;
}

bool freeze_danger_present() {
  bool d;
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  d = state.outside_temp_c < state.freeze_danger_temp_c;
  xSemaphoreGive(state_mutex);
  return d;
}

void set_relay_activated(time_t t) {
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  state.relay_activated_time_s = t;
  state.relay_on = true;
  xSemaphoreGive(state_mutex);
}

time_t get_relay_activated_time_s() {
  time_t t;
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  t = state.relay_activated_time_s;
  xSemaphoreGive(state_mutex);
  return t;
}

time_t get_relay_activated_elapsed_time_s() {
  time_t t = get_relay_activated_time_s();
  time_t now;
  time(&now);
  return now - t;
}

bool get_relay_activated() {
  bool r;
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  r = state.relay_on;
  xSemaphoreGive(state_mutex);
  return r;
}

void set_relay_deactivated() {
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  state.relay_on = false;
  xSemaphoreGive(state_mutex);
}

State get_state() {
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  State state_copy = state;
  xSemaphoreGive(state_mutex);
  return state_copy;
}
