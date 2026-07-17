#pragma once

typedef enum {
    MOTOR_CMD_SET_THROTTLE,
    MOTOR_CMD_GO_DISTANCE,
    MOTOR_CMD_POWEROFF,
} motors_cmd_type_t;

typedef struct {
    motors_cmd_type_t type;
    union {
        struct {
            int throttle[2]; /* % */
            uint32_t duration; /* ms, 0 means non-stop */
            bool accel;  /* accelerate from current throttle */
        } set_throttle; /* set throttle of each motor for [duration] ms */
        struct {
            int distance; /* mm */
            int throttle; /* % */
            bool no_accel;
        } go_distance; /* move [distance] mm with both motors at throttle set to [throttle] % */
    };
} motors_cmd_t;
