#include "sync_data.h"

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

#include "./../modbus_params/include/modbus_params.h"
#include "./../gpio_system/include/gpio_system.h"
#include "./../adc_system/include/adc_system.h"

extern SPI_ADC_DATA_t spi_adc_0;
extern SPI_ADC_DATA_t spi_adc_1;
extern SPI_ADC_DATA_t spi_adc_2;
extern SPI_ADC_DATA_t spi_adc_3;

void get_data(void)
{
    int adc_channel = 0;

    input_reg_params.input_int_data_block[0] = get_gpio_status(EXTERNAL_INPUT_1);
    input_reg_params.input_int_data_block[1] = get_gpio_status(EXTERNAL_INPUT_2);
    input_reg_params.input_int_data_block[2] = get_gpio_status(EXTERNAL_INPUT_3);
    input_reg_params.input_int_data_block[3] = get_gpio_status(EXTERNAL_INPUT_4);
    input_reg_params.input_int_data_block[4] = get_gpio_status(EXTERNAL_INPUT_5);

    input_reg_params.input_int_data_block[5] = get_gpio_status(INTERNAL_INPUT_1);

    SPI_ADC_ReadValue(&spi_adc_0);
    input_reg_params.input_float_data_block[0] = spi_adc_0.raw_value;
    // printf("%d: %d\n", 0, spi_adc_0.raw_value);

    int *adc_pointer = get_adc_value();
    for (adc_channel = 0; adc_channel < 3; adc_channel++)
    {
        input_reg_params.input_float_data_block[adc_channel + 1] = *(adc_pointer + adc_channel);
        // printf("%d: %f\n", adc_channel + 1, input_reg_params.input_float_data_block[adc_channel + 1]);
    }

    // for (adc_channel = 0; adc_channel < 4; adc_channel++)
    // {
    //     SPI_ADC_ReadValue(&spi_adc[adc_channel]);
    //     input_reg_params.input_float_data_block[adc_channel] = spi_adc[adc_channel].raw_value;
    //     printf("%d: %d\n", adc_channel, spi_adc[adc_channel].raw_value);
    // }
}

void set_data(void)
{
    set_gpio_status(EXTERNAL_OUTPUT_1, (uint8_t)holding_reg_params.holding_int_data_block[0]);
    set_gpio_status(EXTERNAL_OUTPUT_2, (uint8_t)holding_reg_params.holding_int_data_block[1]);
    set_gpio_status(EXTERNAL_OUTPUT_3, (uint8_t)holding_reg_params.holding_int_data_block[2]);
    set_gpio_status(EXTERNAL_OUTPUT_4, (uint8_t)holding_reg_params.holding_int_data_block[3]);
    set_gpio_status(EXTERNAL_OUTPUT_5, (uint8_t)holding_reg_params.holding_int_data_block[4]);
}

void task_sync_data(void)
{
    while (1)
    {
        set_data();
        get_data();
        vTaskDelay(100);
    }
}
