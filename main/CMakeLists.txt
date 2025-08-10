idf_component_register(SRCS "SMEM-MP.c"
                    INCLUDE_DIRS "."
                    REQUIRES i2c_mgmt_driver i2c_pn532 DummyComponent)
