/*
 * Defines a state enum and provides thread safe setters and getters for it.
 * Part of the Antifreeze program. https://github.com/kghose/antifreeze
 *
 * (c) 2024 Kaushik Ghose
 *
 * Released under the MIT License
 */

#ifndef _STATE_H_
#define _STATE_H_

#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

typedef struct {
  float t_freeze_danger_c;
  float k;
  float temperature_c;

  bool freeze_danger;
  bool relay_activated;

  time_t relay_activated_time_s;

} State;

esp_err_t initialize_state();
State get_state();
void set_state(State);

#endif  //_STATE_H_
