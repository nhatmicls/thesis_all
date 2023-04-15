idf_component_register(SRCS "modbus.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_netif esp_wifi nvs_flash espressif__esp-modbus modbus_params
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")