/**
 * @file Wifi.h
 * @author Micls
 * @brief
 * @version 0.1
 * @date 2023-03-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef INC_WIFI_H
#define INC_WIFI_H

#define ESP_WIFI_SSID "PIF_CLUB"
#define ESP_WIFI_PASS "chinsochin"
#define ESP_MAXIMUM_RETRY 10

#define CONFIG_ESP_WIFI_AUTH_WPA2_PSK 1

void wifi_init_sta();

#endif /* INC_WIFI_H */
