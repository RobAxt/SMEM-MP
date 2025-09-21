idf_component_register(SRCS "SMEM-MP.c"
                    INCLUDE_DIRS "."
                    REQUIRES ao_core i2c_mgmt_driver i2c_pn532 i2c_mcp23017 i2c_ads1115)
