#ifndef ETH_DRIVER_H
#define ETH_DRIVER_H

#include "esp_eth_driver.h"

/**
 * @brief Initialize Ethernet driver based on Espressif IoT Development Framework Configuration
 *
 * @param[out] eth_handles_out initialized Ethernet driver handles
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_ARG when passed invalid pointers
 *          - ESP_ERR_NO_MEM when there is no memory to allocate for Ethernet driver handles array
 *          - ESP_FAIL on any other failure
 */
esp_err_t esp_eth_init(esp_eth_handle_t *eth_handles_out);

#endif