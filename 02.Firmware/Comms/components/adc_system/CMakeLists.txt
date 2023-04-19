idf_component_register(SRCS "adc_system.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_adc nvs_flash esp_event
                    )
