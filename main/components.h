#pragma once

#include "wled.h"
#include "motor.h"
#include "servo.h"

extern wled_t wled;
extern motor_pair_t motors;
extern servo_t head_servo, lift_servo;

void components_init();