#include "motor.h"
#include "state_monitor.h"
#include "sub_config.h"
#include "util.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "sub_state.h"

static const char *TAG = "motor";

#define MOTOR_PWM_FREQ_HZ     (1000)
#define MOTOR_PWM_RESOLUTION  LEDC_TIMER_8_BIT
#define MOTOR_PWM_MAX_DUTY    (255)
/* With too low PWM motor cannot move but still draws current and heats up.
   [MOTOR_PWM_MIN_DUTY, MOTOR_PWM_MAX_DUTY] will be mapped to [1,100] % throttle. 0 duty is still mapped to 0% throttle. */
#define MOTOR_PWM_MIN_DUTY    (10) 


static void motor_set_throttle_raw(motor_t *m, int throttle)
{
    uint32_t duty;
    throttle = clamp_int(throttle, -100, 100);

    if (throttle > 0) {
        duty = (uint32_t)throttle * (MOTOR_PWM_MAX_DUTY - MOTOR_PWM_MIN_DUTY) / 100 + MOTOR_PWM_MIN_DUTY;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_a, duty);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_b, 0);
    } else if (throttle < 0) {
        duty = (uint32_t)(-throttle) * (MOTOR_PWM_MAX_DUTY - MOTOR_PWM_MIN_DUTY) / 100 + MOTOR_PWM_MIN_DUTY;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_a, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_b, duty);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_a, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_b, 0);
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_a);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, m->ctx.ch_b);

    m->current_throttle = throttle;

}


/* 
  New command will overwrite old one.
  When cliff is detected by 4 ir sensors at the bottom, motors will immediately turn off.
  Motors can accelerate from current throttle (optional)
*/
static void motor_pair_task(void *arg)
{
    motor_pair_t *motors = (motor_pair_t *)arg;

    for (;;) {
        motors->idle = IDLE_MOTORS_MASK;
        uint32_t notif = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (!(notif & MOTOR_NOTIFY_NEW_CMD_MASK)) continue;
        motors_cmd_t cmd = motors->cmd;
    process:
        motors->idle = 0;
        switch (cmd.type) {
        case MOTOR_CMD_SET_THROTTLE:
            if (cmd.set_throttle.accel) {
                int left_start = motors->left.current_throttle;
                int right_start = motors->right.current_throttle;
                int left_delta = cmd.set_throttle.throttle[0] - left_start;
                int right_delta = cmd.set_throttle.throttle[1] - right_start;
                int left_steps = abs_int(left_delta) * 1000 / (motors->accel_throttle_per_sec * MOTOR_UPDATE_STEP_MS);
                int right_steps = abs_int(right_delta) * 1000 / (motors->accel_throttle_per_sec * MOTOR_UPDATE_STEP_MS);
                int steps = left_steps > right_steps ? left_steps : right_steps;
                uint32_t elapsed = 0;

                for (int i = 0; i < steps; i++) {
                    motor_set_throttle_raw(&motors->left, left_start + left_delta * i / steps);
                    motor_set_throttle_raw(&motors->right, right_start + right_delta * i / steps);
                    notif = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MOTOR_UPDATE_STEP_MS));
                    elapsed += MOTOR_UPDATE_STEP_MS;
                    if (notif || (cmd.set_throttle.duration > 0 && elapsed >= cmd.set_throttle.duration))
                        goto set_throttle_done;
                }
                motor_set_throttle_raw(&motors->left, cmd.set_throttle.throttle[0]);
                motor_set_throttle_raw(&motors->right, cmd.set_throttle.throttle[1]);

                notif = ulTaskNotifyTake(pdTRUE, cmd.set_throttle.duration>0 ? pdMS_TO_TICKS(cmd.set_throttle.duration-elapsed) : portMAX_DELAY);
            } else {
                motor_set_throttle_raw(&motors->left, cmd.set_throttle.throttle[0]);
                motor_set_throttle_raw(&motors->right, cmd.set_throttle.throttle[1]);
                notif = ulTaskNotifyTake(pdTRUE, cmd.set_throttle.duration > 0 ? pdMS_TO_TICKS(cmd.set_throttle.duration) : portMAX_DELAY);
            }
        set_throttle_done:
            if ((notif & MOTOR_NOTIFY_CLIFF_DETECTED_MASK) || (notif == 0)) {
                motor_set_throttle_raw(&motors->left, 0);
                motor_set_throttle_raw(&motors->right, 0);
            }
            if (notif & MOTOR_NOTIFY_NEW_CMD_MASK) {
                cmd = motors->cmd;
                goto process;
            }
            break;
        case MOTOR_CMD_GO_DISTANCE: {
            if (cmd.go_distance.distance == 0) {
                notif = MOTOR_NOTIFY_DISTANCE_COMPLETE_MASK;
                goto go_distance_interrupted;
            }
            state_monitor_set_target_dist(cmd.go_distance.distance);
            int abs_th = clamp_int(abs_int(cmd.go_distance.throttle), 1, 100); // 0 throttle can't move motors
            int sign = cmd.go_distance.distance > 0 ? 1 : -1;
            int target_throttle = sign * abs_th;

            if (cmd.go_distance.no_accel) {
                motor_set_throttle_raw(&motors->left, target_throttle);
                motor_set_throttle_raw(&motors->right, target_throttle);
            } else {
                int left_start = motors->left.current_throttle;
                int right_start = motors->right.current_throttle;
                int left_delta = target_throttle - left_start;
                int right_delta = target_throttle - right_start;
                int left_steps = abs_int(left_delta) * 1000 / (motors->accel_throttle_per_sec * MOTOR_UPDATE_STEP_MS);
                int right_steps = abs_int(right_delta) * 1000 / (motors->accel_throttle_per_sec * MOTOR_UPDATE_STEP_MS);
                int steps = left_steps > right_steps ? left_steps : right_steps;

                for (int i = 0; i < steps; i++) {
                    int left_cur = left_start + left_delta * i / steps;
                    int right_cur = right_start + right_delta * i / steps;
                    motor_set_throttle_raw(&motors->left, left_cur);
                    motor_set_throttle_raw(&motors->right, right_cur);

                    notif = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MOTOR_UPDATE_STEP_MS));
                    if (notif) goto go_distance_interrupted;
                }

                motor_set_throttle_raw(&motors->left, target_throttle);
                motor_set_throttle_raw(&motors->right, target_throttle);
            }
            notif = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
go_distance_interrupted:
            if (notif & (MOTOR_NOTIFY_CLIFF_DETECTED_MASK | MOTOR_NOTIFY_DISTANCE_COMPLETE_MASK)) {
                motor_set_throttle_raw(&motors->left, 0);
                motor_set_throttle_raw(&motors->right, 0);
            }
            if (notif & MOTOR_NOTIFY_NEW_CMD_MASK) {
                cmd = motors->cmd;
                goto process;
            }
            break;
        }
        case MOTOR_CMD_POWEROFF:
            motor_set_throttle_raw(&motors->left, 0);
            motor_set_throttle_raw(&motors->right, 0);
            break;
        default:
            break;
        }
    }
}

static void channel_init(gpio_num_t gpio, ledc_channel_t ch)
{
    ledc_channel_config_t ch_cfg = {
        .gpio_num = gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = ch,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);
}

static void init_one_motor(motor_t *m, motor_init_arg_t *arg)
{
    channel_init(arg->gpio_a, arg->ch_a);
    channel_init(arg->gpio_b, arg->ch_b);
    m->ctx.ch_a = arg->ch_a;
    m->ctx.ch_b = arg->ch_b;
}

void motors_init(motor_pair_t *motors, motor_pair_init_arg_t *arg)
{
    static bool timer_init = false;

    if (!timer_init) {
        ledc_timer_config_t timer_cfg = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = MOTOR_PWM_RESOLUTION,
            .timer_num = MOTOR_PWM_TIMER,
            .freq_hz = MOTOR_PWM_FREQ_HZ,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));
        timer_init = true;
    }

    init_one_motor(&motors->left, &arg->left);
    init_one_motor(&motors->right, &arg->right);

    motors_set_accel(motors, MOTOR_ACCEL_THROTTLE_PER_SEC_DEFAULT);
    xTaskCreate(motor_pair_task, "motor_pair_task", 4096, motors, MOTOR_TASK_PRIORITY, &motors->task_handle);

    ESP_LOGI(TAG, "init left=(%d,%d) right=(%d,%d)",
             arg->left.gpio_a, arg->left.gpio_b, arg->right.gpio_a, arg->right.gpio_b);
}

static void swap_motor_chan(motor_t* m, bool swap) {
    if(swap != m->config.swap) {
        m->config.swap = swap;
        ledc_channel_t tmp = m->ctx.ch_a;
        m->ctx.ch_a = m->ctx.ch_b;
        m->ctx.ch_b = tmp;
    }
}

void motors_set_swap(motor_pair_t *motors, bool swap_left, bool swap_right)
{
    swap_motor_chan(&motors->left, swap_left);
    swap_motor_chan(&motors->right, swap_right);
}


void motors_set_accel(motor_pair_t* motors, int throttle_per_sec) 
{
    motors->accel_throttle_per_sec = clamp_int(throttle_per_sec, 1, 100);
}