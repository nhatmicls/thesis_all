/**
 * @file Ethernet.h
 * @author Micls
 * @brief
 * @version 0.1
 * @date 2023-03-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef INC_ETHERNET_LIB_H
#define INC_ETHERNET_LIB_H

#include "esp_eth_driver.h"

esp_err_t eth_init(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out);

#endif /* INC_ETHERNET_LIB_H */
