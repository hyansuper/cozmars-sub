# cozmars-sub

ESP-IDF I2C slave controller for ESP32-C3. LEDC PWM motors+servos, WS2812 via RMT,
battery ADC, 74HC165 input, bootloader safety hooks, USB Serial/JTAG console.

## Build

```bash
idf.py reconfigure   # required after sdkconfig changes
idf.py build
```

Do NOT build unless explicitly asked. Only make source edits.

## Architecture

- `app_main` (`main.c`) calls `state_monitor_init()` first, then `components_init()`, then
  installs I2C slave callbacks, creates `i2c_write_task`, and self-deletes.
- `state_monitor_task` (pri 15, 20 ms period) reads HC165 and samples battery every 60 s
  (only when motors+servos idle).
- WLED (pri 1), motor (pri 10), servo (pri 10) tasks: notification-driven
  (`ulTaskNotifyTake(pdTRUE, portMAX_DELAY)`). ISR callbacks dispatch directly via
  `vTaskNotifyGiveFromISR` / `xTaskNotifyFromISR` — no FreeRTOS queues.
- `i2c_write_task` (pri 5) writes `sub_state` on master read.
- Settings (`main/kv.c`): global `kv_settings_t settings`, persisted to NVS as a blob
  (namespace/key `"settings"`) via `kv_save_settings()`; loaded by `kv_init()` at boot.
- Bootloader hook (`bootloader_components/boot_safe/`) sets servo pins then motor pins
  output-low via ROM functions (no BSS/console init).
- `sub_config.h` shared with bootloader component (`PRIV_INCLUDE_DIRS`).
- `sub_i2c_msg/` is a header-only shared protocol bundle (no component registration),
  included via `INCLUDE_DIRS` in `main/CMakeLists.txt`.

## I2C Protocol (`sub_i2c_msg/`)

| Dir | Type | Payload |
|-----|------|---------|
| Write | `SUB_MSG_WR_LED` | `wled_cmd_t` |
| Write | `SUB_MSG_WR_HEAD` | `servo_cmd_t` |
| Write | `SUB_MSG_WR_LIFT` | `servo_cmd_t` |
| Write | `SUB_MSG_WR_MOTOR` | `motors_cmd_t` |
| Write | `SUB_MSG_WR_CONFIG` | `config_cmd_t` |
| Write | `SUB_MSG_WR_STOP` | — |
| Write | `SUB_MSG_WR_POWER` | `power_cmd_t` (sleep / reboot) |
| Write | `SUB_MSG_WR_SAVE_SETTINGS` | — (persists `settings` to NVS) |
| Read | `SUB_MSG_RD_STATE` | `sub_state_resp_t` |
| Read | `SUB_MSG_RD_VERSION` | `sub_msg_version_resp_t` |
| Read | `SUB_MSG_RD_SETTINGS` | `kv_settings_t` (current settings) |

Read is two-phase: master writes the request type (1 byte), then reads the response.

Command envelope: `sub_msg_t` — union of `wled_cmd_t`, `motors_cmd_t`, `servo_cmd_t`, `config_cmd_t`, `power_cmd_t`.

## Battery States (`battery_state_t` enum)

`BATTERY_TOO_LOW=0`, `BATTERY_LVL_1..5=1..5`, `BATTERY_CHARGING=6`, `BATTERY_STANDBY=7`.

## Pin Configuration (`main/sub_config.h`)

| Function | GPIO | Notes |
|---|---|---|
| I2C SDA/SCL | 2/5 | addr 0x28 |
| WLED | 21 | WS2812 via RMT, GRB order |
| L/R Motor A/B | 6/7, 3/4 | LEDC PWM 1 kHz 8-bit |
| Head/Lift Servo | 1/20 | LEDC PWM 50 Hz 14-bit |
| Battery ADC | 0 | ADC1_CH0, 12 dB atten |
| HC165 PL/CP/Q7 | 9/8/10 | 9,8 are strapping pins |

## MCP Servers (pre-configured in `opencode.jsonc`)

- `espressif-docs` — Espressif documentation search
- `esp-idf-eim` — local IDF build/flash/target
- `esp-component-registry` — component search

External directory access allowed for `{env:IDF_PATH}/**` (read-only).

Refer to `$IDF_PATH/examples` for ESP-IDF API usage patterns.
