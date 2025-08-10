idf_component_register(SRCS "source/i2c_pn532.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver freertos i2c_mgmt_driver")