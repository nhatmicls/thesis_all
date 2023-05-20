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

#include <stdbool.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ESP_MAXIMUM_RETRY 10
#define NETIF_DESC_STA "wifi_sta"

#define SSID_1 "PIF_CLUB"
#define PASS_1 "chinsochin"
#define SSID_2 "Morning"
#define PASS_2 "nowaytohide4520@#"
#define SSID_3 "Nyx"
#define PASS_3 "tamsotam"

#define SSID SSID_3
#define PASS PASS_3

#define CONFIG_ESP_WIFI_AUTH_WPA2_PSK 1

    ESP_EVENT_DECLARE_BASE(WIFI_EVENT);
    ESP_EVENT_DECLARE_BASE(IP_EVENT);

    esp_err_t wifi_init_squence(char *ssid, char *pwd, bool wait, esp_event_loop_args_t *loop_args);
    esp_netif_t *get_wifi_netif(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_WIFI_H */
