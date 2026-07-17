#pragma once

typedef struct __attribute__((packed)) {
    uint8_t g;
    uint8_t r;
    uint8_t b;
    uint8_t reserved;
} wled_color_t;

typedef enum {
    WLED_CMD_SET_COLOR,
    WLED_CMD_BLINK,
    WLED_CMD_FADE,
} wled_cmd_type_t;

typedef struct {
    wled_color_t color1;
    uint32_t dur1; /* ms */
    wled_color_t color2;
    uint32_t dur2; /* ms */
    uint32_t repeat; /* 0 value means repeat forever */
} wled_blink_arg_t;

typedef struct {
    wled_color_t color1;
    uint32_t fade_up_dur; /* ms */
    wled_color_t color2;
    uint32_t fade_down_dur; /* ms */
    uint32_t repeat; /* 0 value means repeat forever */
} wled_fade_arg_t;


typedef struct {
    wled_cmd_type_t type;
    union {
        wled_color_t color_arg;
        wled_fade_arg_t fade_arg;
        wled_blink_arg_t blink_arg;
    };
} wled_cmd_t;