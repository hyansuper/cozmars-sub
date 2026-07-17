#pragma once

typedef enum {
    SERVO_CMD_SET_VALUE_IN_DURATION,
    SERVO_CMD_SET_VALUE_AT_SPEED,
    SERVO_CMD_POWEROFF,
    _SERVO_CMD_SET_DUTY,            // danger! only for configuration
} servo_cmd_type_t;

typedef struct {
    servo_cmd_type_t type;
    union {
        struct {
            int value;
            uint32_t hold;
            union {
                int speed;       /* abs value per second, 0 means instant */
                uint32_t duration; /* ms, 0 means instant */
            };
        } set_value;
        struct {
            uint32_t duty;          // 0~16383 (14-bit LEDC)
            uint32_t hold;          // clamped to [50, 200]
        } _set_duty;                // danger! only for configuration
    };
} servo_cmd_t;
