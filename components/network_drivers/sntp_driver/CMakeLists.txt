include($ENV{IDF_PATH}/tools/cmake/version.cmake)

idf_component_register(SRCS "source/sntp_driver.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_event lwip esp_netif)
