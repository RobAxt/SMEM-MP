idf_component_register(SRCS "source/communication_module.c" "source/communication_suscriber.c" "source/communication_publisher.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES net_driver mqtt_driver sntp_driver esp_event lwip esp_netif json security_module ambiental_module energy_module)
