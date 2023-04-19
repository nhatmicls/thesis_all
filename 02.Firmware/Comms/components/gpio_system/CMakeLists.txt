idf_component_register(SRCS "gpio_system.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver nvs_flash esp_event
                    )
