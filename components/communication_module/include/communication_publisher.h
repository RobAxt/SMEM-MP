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

/**
 * @brief Publish the current siren status.
 * @details This function publishes the siren status to the appropriate MQTT topic.
 * @param status A string representing the siren status.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t communication_siren_status_publish(const char* status);

/**
 * @brief Publish the current lights status.
 * @details This function publishes the lights status to the appropriate MQTT topic.
 * @param status A string representing the lights status.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t communication_lights_status_publish(const char* status);

#endif // COMMUNICATION_PUBLISHER_H