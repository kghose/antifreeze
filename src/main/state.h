/*
 * Defines a state struct and manages thread safe access to it.
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
  float freeze_danger_temp_c;
  float outside_temp_c;

  bool relay_on;
  time_t relay_activated_time_s;

} State;

esp_err_t initialize_state();

void set_freeze_danger_temp_c(float);
float get_freeze_danger_temp_c();

void set_outside_temp_c(float);
float get_outside_temp_c();

bool freeze_danger_present();

void set_relay_activated(time_t);
time_t get_relay_activated_time_s();
bool get_relay_activated();
void set_relay_deactivated();

State get_state();

#endif  //_STATE_H_
