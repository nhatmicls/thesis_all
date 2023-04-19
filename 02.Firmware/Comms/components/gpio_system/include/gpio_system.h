#ifndef INC_GPIO_SYSTEM_H
#define INC_GPIO_SYSTEM_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_event.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define INTERNAL_INPUT_1 0
#define INTERNAL_LED_1 1
#define INTERNAL_LED_2 45
#define INTERNAL_LED_3 40
#define EXTERNAL_INPUT_1 12
#define EXTERNAL_INPUT_2 13
#define EXTERNAL_INPUT_3 14
#define EXTERNAL_INPUT_4 15
#define EXTERNAL_INPUT_5 16
#define EXTERNAL_OUTPUT_1 19
#define EXTERNAL_OUTPUT_2 20
#define EXTERNAL_OUTPUT_3 21
#define EXTERNAL_OUTPUT_4 26
#define EXTERNAL_OUTPUT_5 33

#define CONFIG_BLINK_PERIOD 1000

    void init_gpio(void);
    int get_gpio_status(gpio_num_t gpio_pin);
    void set_gpio_status(gpio_num_t gpio_pin, uint8_t status);

#ifdef __cplusplus
}
#endif

#endif /* INC_GPIO_SYSTEM_H */
