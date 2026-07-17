#pragma once

#include "wled_cmd.h"
#include "motor_cmd.h"
#include "servo_cmd.h"
#include "config_cmd.h"
#include "power_cmd.h"
#include "sub_state.h"
#include "sub_msg_version.h"

/* I2C slave address */
#define SUB_I2C_ADDR                 (0x50)

typedef enum __attribute__((packed)) {
    SUB_MSG_UNKNOWN,
    
    SUB_MSG_RD_VERSION,
    SUB_MSG_RD_STATE, /* read state */

    SUB_MSG_RD_SETTINGS, /* read settings */
    SUB_MSG_WR_SAVE_SETTINGS, /* save settings to NVS */

    SUB_MSG_WR_STOP, /* put servos and motors to powerless mode (pwm duty=0), can be useful in emergency */
    SUB_MSG_WR_HEAD,
    SUB_MSG_WR_LIFT,
    SUB_MSG_WR_MOTOR,
    SUB_MSG_WR_LED,

    SUB_MSG_WR_CONFIG, /* change config */

    SUB_MSG_WR_POWER,  /* power command: sleep or reboot */
    
} sub_msg_type_t;

typedef struct {
    sub_msg_type_t type;
    union {
        wled_cmd_t wled_cmd;
        motors_cmd_t motor_cmd;
        servo_cmd_t servo_cmd;
        config_cmd_t config_cmd;
        power_cmd_t power_cmd;
    };
} sub_msg_t;
