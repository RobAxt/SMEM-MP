#ifndef ZIGBEE_GATEWAY_H
#define ZIGBEE_GATEWAY_H

/**
 * @file zigbee_gateway.h
 * @brief Zigbee Gateway Driver Interface
 * @details This header file defines the interface for the Zigbee Gateway driver,
 * including initialization, configuration, and data transmission functions.
 * 
 * @author Roberto Axt
 * @date 2025-12-26
 * 
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize the Zigbee Gateway
 * @details  This function initializes the Zigbee Gateway hardware and prepares it for operation.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t zigbee_gateway_start(void);

/**
 * @brief Recieve data from Zigbee End Devices
 * @details This function receives data from Zigbee end devices through the gateway.
 * 
 * @param data Pointer to the buffer where received data will be stored.
 * @param length Length of the data to be received.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t zigbee_gateway_data_receive(uint8_t *data, size_t length);

#endif // ZIGBEE_GATEWAY_H