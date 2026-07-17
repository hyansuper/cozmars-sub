#pragma once

#include <stdint.h>

/* HC165 bit masks in the state byte */
#define HC165_FL_IR_MASK   (1 << 5)  // front left ir reflective sensor at the bottom for cliff detection
#define HC165_FR_IR_MASK   (1 << 4)  // front right ir
#define HC165_RL_IR_MASK   (1 << 3)  // rare left ir
#define HC165_RR_IR_MASK   (1 << 2)  // rare right ir
#define HC165_TOUCH_MASK   (1 << 1)  // touch on the back
#define HC165_RM_ENC_MASK  (1 << 6)  // right motor encoder ir
#define HC165_LM_ENC_MASK  (1 << 7)  // left motor encoder ir
#define HC165_UNUSED_MASK   (1 << 0) 

#define HC165_CLIFF_DETECT_MASK (HC165_FL_IR_MASK | HC165_FR_IR_MASK | HC165_RL_IR_MASK | HC165_RR_IR_MASK)


#define IDLE_HEAD_MASK (1<<0)
#define IDLE_LIFT_MASK (1<<1)
#define IDLE_WLED_MASK (1<<2)
#define IDLE_MOTORS_MASK (1<<3)

/* battery */
typedef enum __attribute__((packed)) {
    BATTERY_TOO_LOW,
    BATTERY_LVL_1,  // low
    BATTERY_LVL_2,
    BATTERY_LVL_3,
    BATTERY_LVL_4,
    BATTERY_LVL_5,  // high
    BATTERY_CHARGING,  // when charging, battery level can't be measured correctly.
    BATTERY_STANDBY // fully changed and still attatched to charge dock

} battery_state_t;


/* State response sent from sub (ESP32-C3) to main (ESP32-S3) on I2C read */
typedef struct __attribute__((packed)) {
    uint8_t hc165_data;
    uint8_t idle_flags;
    battery_state_t battery_state;
    uint8_t reserved;   
} sub_state_resp_t;
