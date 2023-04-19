/*
 * SPDX-FileCopyrightText: 2016-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*=====================================================================================
 * Description:
 *   The Modbus parameter structures used to define Modbus instances that
 *   can be addressed by Modbus protocol. Define these structures per your needs in
 *   your application. Below is just an example of possible parameters.
 *====================================================================================*/
#ifndef _DEVICE_PARAMS
#define _DEVICE_PARAMS

#include <stdint.h>

#define SIZE_DISCRETE 10
#define SIZE_COIL 10
#define SIZE_INPUT 50
#define SIZE_HOLDING 50

// This file defines structure of modbus parameters which reflect correspond modbus address space
// for each modbus register type (coils, discreet inputs, holding registers, input registers)
#pragma pack(push, 1)
typedef struct
{
    uint8_t discrete_data_block[SIZE_DISCRETE];
} discrete_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint8_t coils_data_block[SIZE_COIL];
} coil_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    float input_float_data_block[SIZE_INPUT];
    uint16_t input_int_data_block[SIZE_INPUT];
} input_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    float holding_float_data_block[SIZE_HOLDING];
    uint16_t holding_int_data_block[SIZE_HOLDING];
} holding_reg_params_t;
#pragma pack(pop)

extern holding_reg_params_t holding_reg_params;
extern input_reg_params_t input_reg_params;
extern coil_reg_params_t coil_reg_params;
extern discrete_reg_params_t discrete_reg_params;

#endif // !defined(_DEVICE_PARAMS)
