idf_component_register(SRCS "source/security_module.c" "source/security_watcher.c" "source/security_ao.c"
                    INCLUDE_DIRS "include"
                    REQUIRES i2c_mgmt_driver i2c_pn532 i2c_mcp23017)
