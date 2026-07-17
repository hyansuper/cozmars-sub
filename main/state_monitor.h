#pragma once

#include <stdint.h>
#include "sub_i2c_msg.h"

extern volatile sub_state_resp_t sub_state;

void state_monitor_init(void);
void state_monitor_set_target_dist(int dist);
void state_monitor_disable_cliff_detection(bool dis);