#ifndef NET_DRIVER_H
#define NET_DRIVER_H

/**
 * @file net_driver.h
 * @brief Header file for the network driver component.
 * @details This file contains the declaration of the function used to start the network interface.
 * 
 * @author  Roberto Axt
 * @date    2025-06-24
 * @version 0.0
 *
 * @par License
 * This project is licensed under the MIT License.
*/

#include "esp_err.h"
#include "esp_netif_ip_addr.h"

/**
 * @brief Start the network interface with the specified IP, gateway, and mask.
 * 
 * @param ip IPv4 address structure for the device's IP address.
 * @param gw IPv4 address structure for the gateway address.
 * @param mask IPv4 address structure for the subnet mask.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t esp_net_start(esp_ip4_addr_t ip, esp_ip4_addr_t gw, esp_ip4_addr_t mask);

/**
 * @brief Wait until the network interface is ready.
 * 
 * @details This function blocks until the network interface is ready to use.
 * 
 * @return esp_err_t Returns ESP_OK if the network is ready, or an error code on failure.
 * 
 * @note This function should be called after esp_net_start() to ensure that the network interface is fully initialized.
 */
esp_err_t esp_net_ready(void);

#endif // NET_DRIVER_H