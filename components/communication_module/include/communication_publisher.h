#ifndef COMMUNICATION_PUBLISHER_H
#define COMMUNICATION_PUBLISHER_H

/**
 * @file communication_publisher.h
 * @brief Communication Publisher Module
 * This module provides functions to initialize and manage MQTT publishing.
 * 
 * @author Roberto Axt
 * @version 1.0
 * @date 2024-10-31
 *  
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

#include "esp_err.h"

/**
 * @brief Initialize the communication publisher module.
 * @details This function sets up MQTT publishing for the communication module.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t communication_publisher_start(void);

#endif // COMMUNICATION_PUBLISHER_H