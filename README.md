# cozmars-sub

I2C slave firmware for the **cozmars** robot sub controller (ESP32-C3FN4).
It controls motor/servo PWM, WS2812 LED, change configurations, reports battery
ADC and 74HC165 readings, in response to commands from the main controller (ESP32-S3)
over I2C.

## System Diagram

```
                         I2C (SDA=GPIO2, SCL=GPIO5)
┌──────────────┐                                    ┌──────────────────┐
│  MAIN        │◄─────────────────────────────────► │  SUB             │
│  ESP32-S3    │   addr 0x28                        │  ESP32-C3        │
│  (master)    │                                    │  (slave)         │
└──────────────┘                                    └──┬──┬──┬──┬──┬───┘
                                                       │  │  │  │  │
     ┌─────────────────────────────────────────────────┘  │  │  │  └──────────────┐
     │                                                    │  │  └─────────────────┐
┌────┴───────┐  ┌───────────┐  ┌───────────┐  ┌─────────┴──┴──┐          ┌──────┴─────┐
│  SERVOS    │  │ MOTORS    │  │ WLED      │  │ 74HC165       │          │ BATTERY    │
│ Head=GPIO1 │  │ L: 6/7    │  │ WS2812    │  │ PL=GPIO9      │          │ ADC        │
│ Lift=GPIO20│  │ R: 3/4    │  │ GPIO21    │  │ CP=GPIO8      │          │ GPIO0      │
│            │  │           │  │           │  │ Q7=GPIO10     │          └────────────┘
└────────────┘  └───────────┘  └───────────┘  └───────┬───────┘
                                                      │ Q7 (serial)
              ┌───────────────────────────────────────┼───────────────┐
              │  parallel inputs (8-bit)              │               │
              ▼         ▼         ▼         ▼         ▼         ▼     ▼
          ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌────────┐ ┌────────┐
          │Touch │ │RearR │ │RearL │ │FrontR│ │FrontL│ │R motor │ │L motor │
          │Sensor│ │IR    │ │IR    │ │IR    │ │IR    │ │encoder │ │encoder │
          │bit 1 │ │bit 2 │ │bit 3 │ │bit 4 │ │bit 5 │ │bit 6   │ │bit 7   │
          └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └────────┘ └────────┘
```

Complete pin map: see [sub_config.h](main/sub_config.h).


## Power Supply Diagram

```
       ┌─────────────────────────────────────────────────────────────────────────────┐
       │                                   USB / BATTERY                             │
       └──┬─────────────────┬─────────────────┬─────────────────┬─────────────────┬──┘
          │                 │                 │                 │                 │
          ▼                 ▼                 ▼                 ▼                 ▼
   ┌──────┴──────┐   ┌──────┴──────┐   ┌──────┴──────┐   ┌──────┴──────┐   ┌──────┴──────┐
   │   SERVOS    │   │   MOTORS    │   │   AUDIO AMP │   │    DCDC     │   │    DCDC     │
   │             │   │             │   │             │   │  reg 3.3V   │   │  reg 3.3V   │
   └─────────────┘   └─────────────┘   └─────────────┘   └──────┬──────┘   └──────┬──────┘
                                                                │                 │
                                                                ▼                 ▼
                                                          ┌─────┴───────┐   ┌─────┴───────┐
                                                          │   SUB       │   │   MAIN      │
                                                          │  ESP32-C3   │   │  ESP32-S3   │
                                                          └─────────────┘   └─────────────┘
```

## I2C Protocol

`sub_i2c_msg/` defines the I2C message format shared between the sub
(ESP32-C3, slave) and the main board (ESP32-S3, master). Both are **4-byte aligned, little endian**.

Include this directory in your project then `#include "sub_i2c_msg.h"` to use the same structs.


## Warnings

- Servos must be set to powerless mode(pwm duty=0) after reaching its desired position.
  If a servo stalls (e.g. driven beyond its range), cut power immediately.
- Cliff detection is for experimental use only, and can be turned off — do not rely on it for safety.

## Build and flash

Built with **ESP-IDF v6.0.2**. 

```bash
idf.py set-target esp32c3
idf.py build
idf.py flash
```
