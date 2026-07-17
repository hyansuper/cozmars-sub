#pragma once

#include "sub_state.h"

// battery level can only be read when robot is not on charging dock

void battery_init(void);
int battery_read_adc_mv(void); // voltage (mv) read at adc pin, not battery actual battery voltage
battery_state_t battery_to_state(int mv);
