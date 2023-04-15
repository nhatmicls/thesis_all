idf_component_register(SRCS "Ethernet.c" "Ethernet_lib.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_eth esp_netif
                    )
