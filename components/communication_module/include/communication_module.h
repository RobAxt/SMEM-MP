#ifndef COMMUNICATION_MODULE_H
#define COMMUNICATION_MODULE_H

/**
 * @file communication_module.h
 * @brief Communication Module Interface
 * This module provides functions to initialize and manage network communication,
 * including Ethernet setup, SNTP client, and MQTT client.
 * 
 * @author Roberto Axt
 * @version 1.0
 * @date 2024-10-31
 *  
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

#include "esp_err.h"
#include "esp_netif_types.h"

#define MQTT_BASE_TOPIC_SIZE 42

/**
 * @brief Base MQTT topic for the communication module.
 */
extern const char MQTT_BASE_TOPIC[MQTT_BASE_TOPIC_SIZE];

/**
 * @brief Initialize the communication module with network parameters.
 * @details This function sets up the Ethernet network interface, SNTP client for time synchronization,
 * and MQTT client for message publishing using the provided IP addresses.
 * @param ip     The static IP address to assign to the device.
 * @param gw     The gateway IP address.
 * @param mask   The subnet mask.
 * @param ntp    The NTP server IP address for time synchronization.
 * @param broker The MQTT broker IP address for message publishing.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t communication_module_start(esp_ip4_addr_t ip, esp_ip4_addr_t gw, esp_ip4_addr_t mask, esp_ip4_addr_t ntp, esp_ip4_addr_t broker);

#endif // COMMUNICATION_MODULE_H