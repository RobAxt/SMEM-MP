idf_component_register(SRCS "SMEM-MP.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_netif security_module communication_module)
