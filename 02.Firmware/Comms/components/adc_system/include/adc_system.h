#ifndef INC_ADC_SYSTEM_H
#define INC_ADC_SYSTEM_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_adc/adc_continuous.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ADC_2_0 11

#define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ADC_GET_CHANNEL(p_data) ((p_data)->type1.channel)
#define ADC_GET_DATA(p_data) ((p_data)->type1.data)

#define READ_LEN 256

#define ADC_UNIT ADC_UNIT_1
#define ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_0
#define ADC_BIT_WIDTH SOC_ADC_DIGI_MAX_BITWIDTH

    void init_adc(void);
    int *get_adc_value();

#ifdef __cplusplus
}
#endif

#endif /* INC_ADC_SYSTEM_H */
