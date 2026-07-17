#pragma once

typedef enum {
    POWER_CMD_SLEEP,
    POWER_CMD_REBOOT,
} power_cmd_type_t;

typedef struct {
    power_cmd_type_t type;
} power_cmd_t;
