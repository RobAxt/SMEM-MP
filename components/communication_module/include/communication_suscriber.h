#ifndef COMMUNICATION_SUSCRIBER_H
#define COMMUNICATION_SUSCRIBER_H

/**
 * @file communication_suscriber.c
 * @brief Communication Subscriber Module
 * This module provides functions to initialize and manage MQTT subscriptions.
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
 * @brief Initialize the communication subscriber module.
 * @details This function sets up MQTT subscriptions for the communication module.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t communication_suscriber_start(void);

#endif // COMMUNICATION_SUSCRIBER_H