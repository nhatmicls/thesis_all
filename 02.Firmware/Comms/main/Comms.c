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

#define BLINK_GPIO 45
#define CONFIG_BLINK_PERIOD 1000

static uint8_t s_led_state = 0;
static const char *TAG = "MODBUS Simulate";

static void blink_led(void)
{
    while (1)
    {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        gpio_set_level(BLINK_GPIO, s_led_state);
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
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

    printf("Wifi init\n");
    wifi_init_squence("PIF_CLUB", "chinsochin", true, &loop_wifi_args);
    printf("Modbus init\n");
    init_modbus_system(&loop_args_modbus);
    // ethernet_init_main();
    configure_led();

    xTaskCreate(blink_led, "Led_Task", 1024, NULL, 10, &ledHandle);

    while (1)
    {
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
