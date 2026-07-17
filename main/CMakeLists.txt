idf_component_register(SRCS "main.c" "components.c" "wled.c" "led_strip_encoder.c" "battery.c" "hc165.c" "state_monitor.c" "motor.c" "servo.c" "util.c" "kv.c"
                    PRIV_REQUIRES esp_driver_rmt esp_driver_i2c esp_driver_gpio esp_adc esp_driver_ledc nvs_flash
                    INCLUDE_DIRS "." "../sub_i2c_msg")
