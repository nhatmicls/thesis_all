#include <stdio.h>
#include "modbus.h"

/*
 * SPDX-FileCopyrightText: 2016-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// FreeModbus Slave Example ESP32

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_netif.h"

#include "mbcontroller.h"                             // for mbcontroller defines and api
#include "./../modbus_params/include/modbus_params.h" // for modbus parameters structures
#include "./../Wifi/include/Wifi.h"

#define MB_TCP_PORT_NUMBER (CONFIG_FMB_TCP_PORT_DEFAULT)
#define MB_MDNS_PORT (502)

// Defines below are used to define register start address for each type of Modbus registers
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) >> 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) >> 1))
#define MB_REG_DISCRETE_INPUT_START (0x0000)
#define MB_REG_COILS_START (0x0000)
#define MB_REG_INPUT_START_AREA0 (INPUT_OFFSET(input_float_data_block[0])) // register offset input area 0
#define MB_REG_INPUT_START_AREA1 (INPUT_OFFSET(input_int_data_block[0]))   // register offset input area 1
#define MB_REG_HOLDING_START_AREA0 (HOLD_OFFSET(holding_float_data_block[0]))
#define MB_REG_HOLDING_START_AREA1 (HOLD_OFFSET(holding_int_data_block[0]))

#define MB_PAR_INFO_GET_TOUT (10) // Timeout for get parameter info
#define MB_CHAN_DATA_MAX_VAL (10)
#define MB_CHAN_DATA_OFFSET (1.1f)

#define MB_READ_MASK (MB_EVENT_INPUT_REG_RD | MB_EVENT_HOLDING_REG_RD | MB_EVENT_DISCRETE_RD | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK (MB_READ_MASK | MB_WRITE_MASK)

static const char *TAG = "SLAVE_TEST";

static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

void slave_operation_func(void *arg)
{
    mb_param_info_t reg_info; // keeps the Modbus registers access information

    ESP_LOGI(TAG, "Modbus slave stack initialized.");
    ESP_LOGI(TAG, "Start modbus test...");
    // The cycle below will be terminated when parameter holding_data0
    // incremented each access cycle reaches the CHAN_DATA_MAX_VAL value.
    while (1)
    {
        // Check for read/write events of Modbus master for certain events
        mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);
        const char *rw_str = (event & MB_READ_MASK) ? "READ" : "WRITE";
        // Filter events and process them accordingly
        if (event & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD))
        {
            // Get parameter information from parameter queue
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(TAG, "HOLDING %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                     rw_str,
                     (uint32_t)reg_info.time_stamp,
                     (uint32_t)reg_info.mb_offset,
                     (uint32_t)reg_info.type,
                     (uint32_t)reg_info.address,
                     (uint32_t)reg_info.size);
        }
        else if (event & MB_EVENT_INPUT_REG_RD)
        {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(TAG, "INPUT READ (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                     (uint32_t)reg_info.time_stamp,
                     (uint32_t)reg_info.mb_offset,
                     (uint32_t)reg_info.type,
                     (uint32_t)reg_info.address,
                     (uint32_t)reg_info.size);
        }
        else if (event & MB_EVENT_DISCRETE_RD)
        {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(TAG, "DISCRETE READ (%u us): ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                     (uint32_t)reg_info.time_stamp,
                     (uint32_t)reg_info.mb_offset,
                     (uint32_t)reg_info.type,
                     (uint32_t)reg_info.address,
                     (uint32_t)reg_info.size);
        }
        else if (event & (MB_EVENT_COILS_RD | MB_EVENT_COILS_WR))
        {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(TAG, "COILS %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                     rw_str,
                     (uint32_t)reg_info.time_stamp,
                     (uint32_t)reg_info.mb_offset,
                     (uint32_t)reg_info.type,
                     (uint32_t)reg_info.address,
                     (uint32_t)reg_info.size);
        }
        vTaskDelay(100);
    }
    // Destroy of Modbus controller on alarm
    ESP_LOGI(TAG, "Modbus controller destroyed.");
    vTaskDelay(100);
}

esp_netif_t *get_example_netif_from_desc(const char *desc)
{
    esp_netif_t *netif = NULL;
    while ((netif = esp_netif_next(netif)) != NULL)
    {
        if (strcmp(esp_netif_get_desc(netif), desc) == 0)
        {
            return netif;
        }
    }
    return netif;
}

static esp_err_t init_services(void)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "nvs_flash_init fail, returns(0x%x).",
                       (uint32_t)result);
    result = esp_netif_init();
    MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "esp_netif_init fail, returns(0x%x).",
                       (uint32_t)result);
    result = esp_event_loop_create_default();
    MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "esp_event_loop_create_default fail, returns(0x%x).",
                       (uint32_t)result);

    // This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
    // Read "Establishing Wi-Fi or Ethernet Connection" section in
    // examples/protocols/README.md for more information about this function.
    // result = example_connect();
    MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "example_connect fail, returns(0x%x).",
                       (uint32_t)result);
    return ESP_OK;
}

// Modbus slave initialization
static esp_err_t slave_init(mb_communication_info_t *comm_info)
{
    mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure

    void *slave_handler = NULL;

    // Initialization of Modbus controller
    esp_err_t err = mbc_slave_init_tcp(&slave_handler);
    MB_RETURN_ON_FALSE((err == ESP_OK && slave_handler != NULL), ESP_ERR_INVALID_STATE,
                       TAG,
                       "mb controller initialization fail.");

    comm_info->ip_addr = NULL; // Bind to any address
    comm_info->ip_netif_ptr = (void *)get_wifi_netif();

    // Setup communication parameters and start stack
    err = mbc_slave_setup((void *)comm_info);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "mbc_slave_setup fail, returns(0x%x).",
                       (uint32_t)err);

    // The code below initializes Modbus register area descriptors
    // for Modbus Holding Registers, Input Registers, Coils and Discrete Inputs
    // Initialization should be done for each supported Modbus register area according to register map.
    // When external master trying to access the register in the area that is not initialized
    // by mbc_slave_set_descriptor() API call then Modbus stack
    // will send exception response for this register area.
    reg_area.type = MB_PARAM_HOLDING;                                           // Set type of register area
    reg_area.start_offset = MB_REG_HOLDING_START_AREA0;                         // Offset of register area in Modbus protocol
    reg_area.address = (void *)&holding_reg_params.holding_float_data_block[0]; // Set pointer to storage instance
    reg_area.size = (sizeof(float) + sizeof(int)) * SIZE_HOLDING;               // Set the size of register storage instance
    printf("%d\n", reg_area.size);
    err = mbc_slave_set_descriptor(reg_area);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "mbc_slave_set_descriptor fail, returns(0x%x).",
                       (uint32_t)err);

    // reg_area.type = MB_PARAM_HOLDING;                                         // Set type of register area
    // reg_area.start_offset = MB_REG_HOLDING_START_AREA1;                       // Offset of register area in Modbus protocol
    // reg_area.address = (void *)&holding_reg_params.holding_int_data_block[0]; // Set pointer to storage instance
    // reg_area.size = SIZE_HOLDING;                                             // Set the size of register storage instance
    // err = mbc_slave_set_descriptor(reg_area);
    // MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
    //                    TAG,
    //                    "mbc_slave_set_descriptor fail, returns(0x%x).",
    //                    (uint32_t)err);

    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = MB_REG_INPUT_START_AREA0;
    reg_area.address = (void *)&input_reg_params.input_float_data_block[0];
    reg_area.size = (sizeof(float) + sizeof(int)) * SIZE_INPUT;
    err = mbc_slave_set_descriptor(reg_area);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "mbc_slave_set_descriptor fail, returns(0x%x).",
                       (uint32_t)err);
    // reg_area.type = MB_PARAM_INPUT;
    // reg_area.start_offset = MB_REG_INPUT_START_AREA1;
    // reg_area.address = (void *)&input_reg_params.input_int_data_block[0];
    // reg_area.size = sizeof(int) * SIZE_INPUT;
    // err = mbc_slave_set_descriptor(reg_area);
    // MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
    //                    TAG,
    //                    "mbc_slave_set_descriptor fail, returns(0x%x).",
    //                    (uint32_t)err);

    // Initialization of Coils register area
    reg_area.type = MB_PARAM_COIL;
    reg_area.start_offset = MB_REG_COILS_START;
    reg_area.address = (void *)&coil_reg_params;
    reg_area.size = sizeof(coil_reg_params);
    err = mbc_slave_set_descriptor(reg_area);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "mbc_slave_set_descriptor fail, returns(0x%x).",
                       (uint32_t)err);

    // Initialization of Discrete Inputs register area
    reg_area.type = MB_PARAM_DISCRETE;
    reg_area.start_offset = MB_REG_DISCRETE_INPUT_START;
    reg_area.address = (void *)&discrete_reg_params;
    reg_area.size = sizeof(discrete_reg_params);
    err = mbc_slave_set_descriptor(reg_area);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "mbc_slave_set_descriptor fail, returns(0x%x).",
                       (uint32_t)err);

    // Starts of modbus controller and stack
    err = mbc_slave_start();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                       TAG,
                       "mbc_slave_start fail, returns(0x%x).",
                       (uint32_t)err);
    vTaskDelay(5);
    return err;
}

void init_modbus_system(esp_event_loop_args_t *loop_args)
{

    // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);

    mb_communication_info_t comm_info = {0};

    comm_info.ip_addr_type = MB_IPV4;
    comm_info.ip_mode = MB_MODE_TCP;

    comm_info.ip_port = MB_TCP_PORT_NUMBER;

    ESP_ERROR_CHECK(slave_init(&comm_info));
}
