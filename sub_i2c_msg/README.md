# sub_i2c_msg

Shared I2C protocol definitions between the sub (ESP32-C3, slave) and main
(ESP32-S3, master). Include this directory in your main controller project and `#include "sub_i2c_msg.h`
to use the same structs, enums, and bit masks.

## Message Format

Each I2C write transaction sends a `sub_msg_t` envelope with this layout:

```
sub_cmd + [action_cmd] + [parameters]
```

| Field        | Description |
|--------------|-------------|
| `sub_cmd`    | `sub_msg_type_t` — selects the target subsystem (motor, servo, LED, config, etc.) |
| `action_cmd` | Subsystem-specific command type (e.g. `motors_cmd_type_t`, `servo_cmd_type_t`, `wled_cmd_type_t`) — indicates the action to perform |
| `parameters` | Command-specific data fields (e.g. throttle values, target position, RGB color) |

Both `action_cmd` and `parameters` are optional — some sub commands (e.g. `SUB_MSG_WR_STOP`) carry no payload.

## Write commands

| Sub Cmd                    | Action Cmd                       | Parameters                        |
|----------------------------|----------------------------------|-----------------------------------|
| `SUB_MSG_WR_LED`          | `wled_cmd_type_t`                | `grb_color_t` / blink / fade args |
| `SUB_MSG_WR_HEAD`         | `servo_cmd_type_t`               | target value, speed/duration, hold |
| `SUB_MSG_WR_LIFT`         | `servo_cmd_type_t`               | target value, speed/duration, hold |
| `SUB_MSG_WR_MOTOR`        | `motors_cmd_type_t`              | throttle / distance               |
| `SUB_MSG_WR_CONFIG`       | `config_cmd_type_t`              | servo range / cliff detection flag |
| `SUB_MSG_WR_STOP`| —                                | —                                 |
| `SUB_MSG_WR_POWER`        | `power_cmd_type_t`               | — (sleep / reboot)                |

A new command can overwrite uncompleted command of the same componets.

## Read

The master first writes a `sub_msg_type_t` to select which read response to get,
then reads the response:

| Request Type          | Response                          |
|-----------------------|-----------------------------------|
| `SUB_MSG_RD_STATE`   | `sub_state_resp_t` (74hc165 + battery_state + battery_adc_mv) |
| `SUB_MSG_RD_VERSION` | `sub_msg_version_resp_t` (major + minor + patch + reserved) |

HC165 bit masks are defined in `sub_state.h`.
