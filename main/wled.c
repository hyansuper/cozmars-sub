#include "wled.h"
#include "sub_config.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "sub_state.h"

#define WLED_RESOLUTION_HZ 10000000

static const char *TAG = "wled";

static void set_color_raw(rmt_channel_handle_t chan, rmt_encoder_handle_t encoder, wled_color_t c)
{
    rmt_transmit_config_t tx_config = { .loop_count = 0 };
    ESP_ERROR_CHECK(rmt_transmit(chan, encoder, &c, 3, &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(chan, portMAX_DELAY));
}

static wled_color_t lerp_color(wled_color_t c1, wled_color_t c2, int step, int total)
{
    return (wled_color_t){
        .r = c1.r + (c2.r - c1.r) * step / total,
        .g = c1.g + (c2.g - c1.g) * step / total,
        .b = c1.b + (c2.b - c1.b) * step / total,
        .reserved = 0,
    };
}

static const TickType_t step_ticks = pdMS_TO_TICKS(WLED_FADE_STEP_MS);


static void wled_task(void *arg)
{
    wled_t *wled = (wled_t *)arg;

    for (;;) {
        wled->idle = IDLE_WLED_MASK;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        wled_cmd_t cmd = wled->cmd;

    process:
        wled->idle = 0;
        switch (cmd.type) {
        case WLED_CMD_SET_COLOR:
            set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, cmd.color_arg);
            break;

        case WLED_CMD_BLINK: {
            for (int i = 0; cmd.blink_arg.repeat == 0 || i < cmd.blink_arg.repeat; i++) {
                set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, cmd.blink_arg.color1);
                if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(cmd.blink_arg.dur1))) { cmd = wled->cmd; goto process; }
                set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, cmd.blink_arg.color2);
                if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(cmd.blink_arg.dur2))) { cmd = wled->cmd; goto process; }
            }
            break;
        }

        case WLED_CMD_FADE: {
            int up_steps = cmd.fade_arg.fade_up_dur / WLED_FADE_STEP_MS;
            int down_steps = cmd.fade_arg.fade_down_dur / WLED_FADE_STEP_MS;
            for (int i = 0; cmd.fade_arg.repeat == 0 || i < cmd.fade_arg.repeat; i++) {
                for (int s = 1; s <= up_steps; s++) {
                    set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, lerp_color(cmd.fade_arg.color1, cmd.fade_arg.color2, s, up_steps));
                    if (ulTaskNotifyTake(pdTRUE, step_ticks)) { cmd = wled->cmd; goto process; }
                }
                if (ulTaskNotifyTake(pdTRUE, step_ticks)) { cmd = wled->cmd; goto process; }
                set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, cmd.fade_arg.color2);
                for (int s = 1; s <= down_steps; s++) {
                    set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, lerp_color(cmd.fade_arg.color2, cmd.fade_arg.color1, s, down_steps));
                    if (ulTaskNotifyTake(pdTRUE, step_ticks)) { cmd = wled->cmd; goto process; }
                }
            }
            set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, cmd.fade_arg.color1);
            break;
        }
        }
    }
}

void wled_init(wled_t *wled, wled_init_arg_t *arg)
{
    rmt_tx_channel_config_t chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = arg->gpio,
        .mem_block_symbols = 64,
        .resolution_hz = WLED_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&chan_config, &wled->ctx.rmt_chan));

    led_strip_encoder_config_t enc_config = {
        .resolution = WLED_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&enc_config, &wled->ctx.rmt_encoder));

    ESP_ERROR_CHECK(rmt_enable(wled->ctx.rmt_chan));
    
    set_color_raw(wled->ctx.rmt_chan, wled->ctx.rmt_encoder, (wled_color_t){0});

    xTaskCreate(wled_task, "wled_task", 3072, wled, WLED_TASK_PRIORITY, &wled->task_handle);

    
    ESP_LOGI(TAG, "initialized on GPIO %d", arg->gpio);
}

