idf_component_register(SRCS "SMEM-MP.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash 
                             esp_netif 
                             ambiental_module 
                             security_module 
                             energy_module
                             communication_module
                    )

