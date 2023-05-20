#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "./../components/Wifi/include/Wifi.h"
#include "./../components/Ethernet/include/Ethernet.h"
#include "./../components/modbus/include/modbus.h"
#include "./../components/sync_data/include/sync_data.h"
#include "./../components/gpio_system/include/gpio_system.h"
#include "./../components/adc_system/include/adc_system.h"

#define BLINK_GPIO INTERNAL_LED_1
#define CONFIG_BLINK_PERIOD 1000

static uint8_t s_led_state = 0;
static const char *TAG = "MODBUS Simulate";

static void blink_led(void)
{
    while (1)
    {
        // ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        gpio_set_level(BLINK_GPIO, s_led_state);
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    TaskHandle_t ledHandle = NULL;
    TaskHandle_t syncHandle = NULL;

    esp_event_loop_args_t loop_wifi_args = {
        .queue_size = 10,
        .task_name = "wifi",
        .task_priority = 10,
        .task_stack_size = 2048,
        .task_core_id = 0,
    };

    esp_event_loop_args_t loop_args_modbus = {
        .queue_size = 10,
        .task_name = "modbus",
        .task_priority = 11,
        .task_stack_size = 2048,
        .task_core_id = 0,
    };

    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    init_adc();
    init_gpio();
    set_gpio_status(INTERNAL_LED_1, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    printf("Wifi init\n");
    wifi_init_squence(SSID, PASS, true, &loop_wifi_args);
    set_gpio_status(INTERNAL_LED_2, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    printf("Modbus init\n");
    init_modbus_system(&loop_args_modbus);
    // ethernet_init_main();
    set_gpio_status(INTERNAL_LED_3, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    set_gpio_status(INTERNAL_LED_1, 1);
    set_gpio_status(INTERNAL_LED_2, 1);
    set_gpio_status(INTERNAL_LED_3, 1);

    xTaskCreate(blink_led, "Led_Task", 2048, NULL, 10, &ledHandle);
    xTaskCreate(task_sync_data, "sync_Task", 2048, NULL, 10, &syncHandle);
    xTaskCreate(slave_operation_func, "modbus_task", 2048 * 2, NULL, 6, NULL);

    while (1)
    {
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
