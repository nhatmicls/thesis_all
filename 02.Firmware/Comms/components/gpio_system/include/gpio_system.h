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

#define INTERNAL_LED_1 40
#define INTERNAL_LED_2 41
#define INTERNAL_LED_3 42

#define EXTERNAL_INPUT_1 14
#define EXTERNAL_INPUT_2 15
#define EXTERNAL_INPUT_3 16
#define EXTERNAL_INPUT_4 19
#define EXTERNAL_INPUT_5 20

#define EXTERNAL_OUTPUT_1 2
#define EXTERNAL_OUTPUT_2 3
#define EXTERNAL_OUTPUT_3 4
#define EXTERNAL_OUTPUT_4 5
#define EXTERNAL_OUTPUT_5 6

#define CONFIG_BLINK_PERIOD 1000

    void init_gpio(void);
    int get_gpio_status(gpio_num_t gpio_pin);
    void set_gpio_status(gpio_num_t gpio_pin, uint8_t status);

#ifdef __cplusplus
}
#endif

#endif /* INC_GPIO_SYSTEM_H */
