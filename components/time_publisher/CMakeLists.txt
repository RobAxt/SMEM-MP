idf_component_register(SRCS "source/time_publisher.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES mqtt_driver sntp_driver esp_netif json)
