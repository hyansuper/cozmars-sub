#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "config_cmd.h"

#define KV_SETTINGS "settings"

typedef struct {
	servo_config_t lift, head;
	struct {
		bool swap;
	} left_motor, right_motor;
} kv_settings_t;

extern kv_settings_t settings;

esp_err_t kv_init();
esp_err_t kv_save_settings(); // save settings to storage by the name of KV_SETTIGNS
esp_err_t kv_read_settings(); // read settings from storage