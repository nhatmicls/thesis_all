/**
 * @file spi_adc.h
 * @author Micls
 * @brief
 * @version 0.1
 * @date 2023-05-20
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef INC_SPI_ADC_H
#define INC_SPI_ADC_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ADC_SPI_MISO 13
#define ADC_SPI_MOSI 11
#define ADC_SPI_CLK 12
#define ADC_SPI_CS 10

    enum
    {
        SPI_ADC_CH0,
        SPI_ADC_CH1,
        SPI_ADC_CH2,
        SPI_ADC_CH3,
    };

    typedef struct
    {
        uint8_t adc_channel;
        uint16_t raw_value;
    } SPI_ADC_DATA_t;

    void SPI_ADC_Init(SPI_ADC_DATA_t *spi_adc, uint8_t adc_channel);
    esp_err_t SPI_ADC_ReadValue(SPI_ADC_DATA_t *spi_adc);

#ifdef __cplusplus
}
#endif

#endif