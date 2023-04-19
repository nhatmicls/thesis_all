#include "gpio_system.h"

#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

void init_gpio(void)
{
    // Init for internal input
    gpio_reset_pin(INTERNAL_INPUT_1);
    ESP_ERROR_CHECK(gpio_set_direction(INTERNAL_INPUT_1, GPIO_MODE_INPUT));

    // Init for internal output
    gpio_reset_pin(INTERNAL_LED_1);
    ESP_ERROR_CHECK(gpio_set_direction(INTERNAL_LED_1, GPIO_MODE_OUTPUT));
    gpio_reset_pin(INTERNAL_LED_2);
    ESP_ERROR_CHECK(gpio_set_direction(INTERNAL_LED_2, GPIO_MODE_OUTPUT));
    gpio_reset_pin(INTERNAL_LED_3);
    ESP_ERROR_CHECK(gpio_set_direction(INTERNAL_LED_3, GPIO_MODE_OUTPUT));

    // Init for external input
    gpio_reset_pin(EXTERNAL_INPUT_1);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_INPUT_1, GPIO_MODE_INPUT));
    gpio_reset_pin(EXTERNAL_INPUT_2);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_INPUT_2, GPIO_MODE_INPUT));
    gpio_reset_pin(EXTERNAL_INPUT_3);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_INPUT_3, GPIO_MODE_INPUT));
    gpio_reset_pin(EXTERNAL_INPUT_4);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_INPUT_4, GPIO_MODE_INPUT));
    gpio_reset_pin(EXTERNAL_INPUT_5);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_INPUT_5, GPIO_MODE_INPUT));

    // Init for external output
    gpio_reset_pin(EXTERNAL_OUTPUT_1);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_OUTPUT_1, GPIO_MODE_OUTPUT));
    gpio_reset_pin(EXTERNAL_OUTPUT_2);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_OUTPUT_2, GPIO_MODE_OUTPUT));
    gpio_reset_pin(EXTERNAL_OUTPUT_3);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_OUTPUT_3, GPIO_MODE_OUTPUT));
    gpio_reset_pin(EXTERNAL_OUTPUT_4);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_OUTPUT_4, GPIO_MODE_OUTPUT));
    gpio_reset_pin(EXTERNAL_OUTPUT_5);
    ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_OUTPUT_5, GPIO_MODE_OUTPUT));
}

int get_gpio_status(gpio_num_t gpio_pin)
{
    return gpio_get_level(gpio_pin);
}

void set_gpio_status(gpio_num_t gpio_pin, uint8_t status)
{
    ESP_ERROR_CHECK(gpio_set_level(gpio_pin, status));
}