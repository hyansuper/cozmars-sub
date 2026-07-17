#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "wled_cmd.h"


typedef struct {
    gpio_num_t gpio;
} wled_init_arg_t;


typedef struct {
    TaskHandle_t task_handle;
    volatile wled_cmd_t cmd;
    struct {
        rmt_channel_handle_t rmt_chan;
        rmt_encoder_handle_t rmt_encoder;
    } ctx;
    volatile uint8_t idle;
} wled_t;

void wled_init(wled_t* wled, wled_init_arg_t* cfg);