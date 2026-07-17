#include "hc165.h"
#include "sub_config.h"
#include "driver/gpio.h"

void hc165_init(void)
{
    gpio_config_t out = {
        .pin_bit_mask = (1ULL << HC165_PL_GPIO) | (1ULL << HC165_CP_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&out);

    gpio_config_t in = {
        .pin_bit_mask = (1ULL << HC165_Q7_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&in);

    gpio_set_level(HC165_PL_GPIO, 1);
    gpio_set_level(HC165_CP_GPIO, 0);

}

uint8_t hc165_read(void)
{
    gpio_set_level(HC165_PL_GPIO, 0);
    gpio_set_level(HC165_PL_GPIO, 1);

    uint8_t data = 0;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(HC165_CP_GPIO, 0);
        if (gpio_get_level(HC165_Q7_GPIO)) {
            data |= (1 << i);
        }
        gpio_set_level(HC165_CP_GPIO, 1);
    }

    return data;
}

