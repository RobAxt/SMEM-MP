include($ENV{IDF_PATH}/tools/cmake/version.cmake)

idf_component_register(SRCS "source/mqtt_driver.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "mqtt" )
