#include "servo.h"
#include "sub_config.h"
#include "util.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "sub_state.h"

static const char *TAG = "servo";

static uint32_t value_to_duty(const servo_t *sv, int value)
{
    return sv->cfg.pwm_start +
           (uint32_t)(value - sv->ctx.value_start) * (sv->cfg.pwm_end - sv->cfg.pwm_start) 
           / (sv->ctx.value_end - sv->ctx.value_start); // the divisor won't be zero, coz they use defined value and won't change.
}

static void servo_task(void *arg)
{
    servo_t *sv = (servo_t *)arg;

    for (;;) {
        sv->idle = sv->idle_mask;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        servo_cmd_t cmd = sv->cmd;
    process:
        sv->idle = 0;
        switch (cmd.type) {
        case SERVO_CMD_SET_VALUE_AT_SPEED:
        case SERVO_CMD_SET_VALUE_IN_DURATION: {
            cmd.set_value.value = clamp_int(cmd.set_value.value, sv->ctx.value_start, sv->ctx.value_end);
            uint32_t target_duty = value_to_duty(sv, cmd.set_value.value);

            bool instant = cmd.type == SERVO_CMD_SET_VALUE_AT_SPEED
                           ? cmd.set_value.speed == 0
                           : cmd.set_value.duration == 0;

            if (instant) {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch, target_duty);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch);
                sv->current_duty = target_duty;
            } else {
                int diff_duty = (int)target_duty - (int)sv->current_duty;
                int abs_duty = abs_int(diff_duty);
                int count;

                if (cmd.type == SERVO_CMD_SET_VALUE_AT_SPEED) {
                    int pwm_range = (int)sv->cfg.pwm_end - (int)sv->cfg.pwm_start;
                    int value_range = sv->ctx.value_end - sv->ctx.value_start;
                    int step_duty = abs_int(cmd.set_value.speed) * pwm_range * SERVO_UPDATE_STEP_MS / (value_range * 1000);
                    if (step_duty < 1) step_duty = 1;
                    count = abs_duty / step_duty;
                } else {
                    count = cmd.set_value.duration / SERVO_UPDATE_STEP_MS;
                }

                uint32_t init_duty = sv->current_duty;

                for (int i = 1; i <= count; i++) {
                    sv->current_duty = init_duty + diff_duty * i / count;
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch, sv->current_duty);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch);

                    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SERVO_UPDATE_STEP_MS))) {
                        cmd = sv->cmd;
                        goto process;
                    }
                }
                sv->current_duty = target_duty;
                ledc_set_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch, target_duty);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch);
            }
            if (cmd.set_value.hold > 0) {
                if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(cmd.set_value.hold)) == pdFALSE) {
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch);
                }
            }
            break;
        }
        case _SERVO_CMD_SET_DUTY: {
            cmd._set_duty.duty = clamp_int(cmd._set_duty.duty, 0, SERVO_PWM_MAX_DUTY);
            cmd._set_duty.hold = clamp_int(cmd._set_duty.hold, 50, 200);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch, cmd._set_duty.duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch);
            sv->current_duty = cmd._set_duty.duty;
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(cmd._set_duty.hold)) == pdFALSE) {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch, 0);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch);
            }
            break;
        }
        case SERVO_CMD_POWEROFF:
            ledc_set_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, sv->ctx.ch);
            break;
        default:
            break;
        }
    }
}

void servo_init(servo_t* servo, servo_init_arg_t* arg)
{
    static bool ledc_timer_init = false;
    if (!ledc_timer_init)
    {
        ledc_timer_config_t timer_cfg = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = SERVO_PWM_RESOLUTION,
            .timer_num = SERVO_PWM_TIMER,
            .freq_hz = SERVO_PWM_FREQ_HZ,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));
        ledc_timer_init = true;
    }

    ledc_channel_config_t ch_cfg = {
        .gpio_num = arg->gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = arg->chan,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = SERVO_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);

    servo->ctx.ch = arg->chan;
    servo->ctx.value_start = arg->value_start;
    servo->ctx.value_end = arg->value_end;
    servo->cfg.pwm_start = arg->cfg.pwm_start;
    servo->cfg.pwm_end = arg->cfg.pwm_end;
    servo->current_duty = arg->cfg.pwm_start;
    servo->idle_mask = arg->idle_mask;

    xTaskCreate(servo_task, "servo_task", 4096, servo, SERVO_TASK_PRIORITY, &servo->task_handle);

    ESP_LOGI(TAG, "servo: gpio=%d ch=%d value=[%d,%d] pwm=[%lu,%lu]",
             arg->gpio, arg->chan, arg->value_start, arg->value_end,
             (unsigned long)arg->cfg.pwm_start, (unsigned long)arg->cfg.pwm_end);
}

void servo_config(servo_t* servo, servo_config_t* cfg) {
    servo->cfg = *cfg;
}
