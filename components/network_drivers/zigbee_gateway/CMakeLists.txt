idf_component_register( SRCS "source/zigbee_gateway.c"
                        INCLUDE_DIRS "include"
                        REQUIRES
                            # Headers que usamos desde el .c
                            esp_timer
                            nvs_flash
                            esp_event
                            ieee802154
                            # Stacks Zigbee (managed components)
                            espressif__esp-zigbee-lib
                            espressif__esp-zboss-lib
                            # Básicos del sistema
                            esp_system
                            log
                            )