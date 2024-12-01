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
  state.t_freeze_danger_c = T_FREEZE_DANGER_DEFAULT_C;
  state.k = K;

  state.freeze_danger = false;
  state.relay_activated = false;

  return ESP_OK;
}

State get_state() {
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  State state_copy = state;
  xSemaphoreGive(state_mutex);
  return state_copy;
}

void set_state(State new_state) {
  xSemaphoreTake(state_mutex, portMAX_DELAY);  // block
  state = new_state;
  xSemaphoreGive(state_mutex);
}
