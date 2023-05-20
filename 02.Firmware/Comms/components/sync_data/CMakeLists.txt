idf_component_register(SRCS "sync_data.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver modbus_params esp_adc nvs_flash esp_event gpio_system adc_system spi_adc
                    )
