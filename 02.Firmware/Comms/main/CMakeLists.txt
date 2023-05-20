idf_component_register(SRCS "Comms.c"
                    INCLUDE_DIRS "."
                    REQUIRES Wifi driver Ethernet esp_netif modbus sync_data gpio_system spi_adc
                    )
                    