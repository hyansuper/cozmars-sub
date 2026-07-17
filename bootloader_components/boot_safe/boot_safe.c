#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"
#include "sub_config.h"

static void set_low(int pin)
{
    esp_rom_gpio_pad_select_gpio(pin);
    gpio_ll_func_sel(&GPIO, pin, PIN_FUNC_GPIO);
    gpio_ll_output_enable(&GPIO, pin);
    gpio_ll_set_level(&GPIO, pin, 0);
}

void bootloader_hooks_include(void)
{
}

void bootloader_before_init(void)
{
    set_low(HEAD_SERVO_GPIO);
    set_low(LIFT_SERVO_GPIO);
    set_low(LEFT_MOTOR_A_GPIO);
    set_low(LEFT_MOTOR_B_GPIO);
    set_low(RIGHT_MOTOR_A_GPIO);
    set_low(RIGHT_MOTOR_B_GPIO);

    // Debug output via USB Serial/JTAG (GPIO 18/19) — UART0 (GPIO 20/21) is unused
    // to keep WLED (GPIO 21) and lift servo (GPIO 20) free of bootloader noise.
    // Enable CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG in menuconfig for bootloader console.
}
