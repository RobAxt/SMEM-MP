idf_component_register(SRCS "source/energy_module.c"
                    INCLUDE_DIRS "include"
                    REQUIRES uart_pzem004t
                             i2c_mgmt_driver
                             i2c_ads1115
                             zigbee_gateway
                    )

