#pragma once

typedef enum {
	CONFIG_CMD_HEAD_DUTY_RANGE,
	CONFIG_CMD_LIFT_DUTY_RANGE,
	CONFIG_CMD_MOTOR_SWAP,
	CONFIG_CMD_DISABLE_CLIFF_DETECTION,
	CONFIG_CMD_MOTOR_ACCEL
} config_cmd_type_t;

typedef struct {
	uint32_t pwm_start;
	uint32_t pwm_end;
} servo_config_t;

typedef struct {
    config_cmd_type_t type;
    union {
    	bool disable_cliff_detection;
    	servo_config_t servo_cfg;
    	int accel_throttle_per_sec;
    	bool swap_motors[2];
    };
} config_cmd_t;