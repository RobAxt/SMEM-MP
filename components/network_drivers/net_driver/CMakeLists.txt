include($ENV{IDF_PATH}/tools/cmake/version.cmake)

idf_component_register(SRCS "source/net_driver.c" "source/eth_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "esp_driver_gpio esp_eth esp_event esp_netif driver")