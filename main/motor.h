#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hal/gpio_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "motor_cmd.h"

#define MOTOR_NOTIFY_NEW_CMD_MASK            (1 << 0)
#define MOTOR_NOTIFY_DISTANCE_COMPLETE_MASK  (1 << 1)
#define MOTOR_NOTIFY_CLIFF_DETECTED_MASK     (1 << 2)

typedef struct {
    gpio_num_t gpio_a;
    gpio_num_t gpio_b;
    ledc_channel_t ch_a;
    ledc_channel_t ch_b;
} motor_init_arg_t;

typedef struct {
    motor_init_arg_t left, right;
} motor_pair_init_arg_t;

typedef struct {
    struct {
        ledc_channel_t ch_a;
        ledc_channel_t ch_b;
    } ctx;
    struct {
        bool swap;
    } config;
    int current_throttle;
} motor_t;

typedef struct {
    TaskHandle_t task_handle;
    volatile motors_cmd_t cmd;
    int accel_throttle_per_sec; 
    motor_t left, right;
    volatile uint8_t idle;
} motor_pair_t;

void motors_init(motor_pair_t* motors, motor_pair_init_arg_t *arg);
void motors_set_swap(motor_pair_t* motors, bool swap_left, bool swap_right);
void motors_set_accel(motor_pair_t* motors, int throttle_per_sec);
