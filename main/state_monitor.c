#include "state_monitor.h"
#include "sub_config.h"
#include "hc165.h"
#include "battery.h"
#include "components.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

volatile sub_state_resp_t sub_state;
static bool detect_cliff = true;
static volatile int target_flip_count = 0;

static void state_monitor_task(void *arg)
{
    (void)arg;

    uint8_t prev_raw = sub_state.hc165_data;
    uint8_t enc_prev = 0;
    uint32_t lm_flip_count = 0;
    uint32_t rm_flip_count = 0;

    int bat_div = BATTERY_SAMPLE_INTERVAL_MS / STATE_MONITOR_INTERVAL_MS;
    int bat_cnt = bat_div;

    TickType_t last_wake = xTaskGetTickCount();

    while (true) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(STATE_MONITOR_INTERVAL_MS));
        
        /* sub_state.hc165_data is a "stable" value:
            only when a bit is measured new value twice will the same bit update in sub_state.hc165_data  */
        uint8_t curr_raw = hc165_read();
        uint8_t disagreed = (prev_raw ^ curr_raw);
        sub_state.hc165_data = (sub_state.hc165_data & disagreed) | (curr_raw & (~disagreed));
        prev_raw = curr_raw;

        if (!motors.idle) {
            if (detect_cliff && (sub_state.hc165_data & HC165_CLIFF_DETECT_MASK)) {
                xTaskNotify(motors.task_handle, MOTOR_NOTIFY_CLIFF_DETECTED_MASK, eSetBits);
                target_flip_count = lm_flip_count = rm_flip_count = 0;
                continue;
            }
            if (target_flip_count) {
                lm_flip_count = rm_flip_count = target_flip_count;
                target_flip_count = 0;
                enc_prev = sub_state.hc165_data;
            }
            if (lm_flip_count || rm_flip_count) {
                uint8_t enc_edge = sub_state.hc165_data ^ enc_prev;

                if ((enc_edge & HC165_LM_ENC_MASK) && lm_flip_count) lm_flip_count--;
                if ((enc_edge & HC165_RM_ENC_MASK) && rm_flip_count) rm_flip_count--;
                enc_prev = sub_state.hc165_data;

                if (!(lm_flip_count || rm_flip_count)) {
                    xTaskNotify(motors.task_handle, MOTOR_NOTIFY_DISTANCE_COMPLETE_MASK, eSetBits);
                    continue;
                }
            }
        }

        // measure battery state, moving motors can make battery level drop.
        if (++bat_cnt >= bat_div && motors.idle && head_servo.idle && lift_servo.idle) {
            sub_state.battery_state = battery_to_state(battery_read_adc_mv());
            bat_cnt = 0;
        }

    }
}

void state_monitor_set_target_dist(int dist) 
{
    target_flip_count = abs(dist) / MOTOR_IR_MM_PER_EDGE;
}

void state_monitor_init(void)
{
    battery_init();
    hc165_init();

    sub_state.hc165_data = hc165_read();
    
    // discard the first few reads
    battery_read_adc_mv();
    battery_read_adc_mv();
    sub_state.battery_state = battery_to_state(battery_read_adc_mv());

    xTaskCreate(state_monitor_task, "state_monitor", 3072, NULL, STATE_MONITOR_TASK_PRIORITY, NULL);
}

void state_monitor_disable_cliff_detection(bool dis) {
    detect_cliff = !dis;
}