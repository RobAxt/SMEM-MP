idf_component_register(SRCS "source/security_module.c" "source/security_ao_fsm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ao_core i2c_mgmt_driver i2c_pn532 i2c_mcp23017)
