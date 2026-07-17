#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hal/gpio_types.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo_cmd.h"
#include "config_cmd.h"

#define HEAD_ANGLE_MIN           (-20)
#define HEAD_ANGLE_MAX           (45)

#define SERVO_PWM_FREQ_HZ     (50)
#define SERVO_PWM_RESOLUTION  LEDC_TIMER_14_BIT
#define SERVO_PWM_MAX_DUTY       (16383)  /* 14-bit LEDC max */


typedef struct {
	gpio_num_t gpio;
	ledc_channel_t chan;
	int value_start;
	int value_end;
	servo_config_t cfg;
    uint8_t idle_mask;
} servo_init_arg_t;

typedef struct {
    TaskHandle_t task_handle;
    volatile servo_cmd_t cmd;
    uint32_t current_duty;
    struct {
        ledc_channel_t ch;
        int value_start;
        int value_end;
    } ctx;
    servo_config_t cfg;
    volatile uint8_t idle;
    uint8_t idle_mask;
} servo_t;

void servo_init(servo_t* servo, servo_init_arg_t* arg);
void servo_config(servo_t* servo, servo_config_t* cfg);
