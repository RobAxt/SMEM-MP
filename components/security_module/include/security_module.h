#ifndef SECURITY_MODULE_H
#define SECURITY_MODULE_H

/**
 * @file security_module.h
 * @brief Header file for the Security Module.
 * @details This module provides the interface for the security functionalities,
 *          including initialization and management of the security AO FSM.
 * 
 * @author Roberto Axt
 * @version 1.0
 * @date 2024-09-21
 *  
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initializes and starts the security module.
 * @details This function sets up the necessary components for the security module,
 * including the security AO FSM and any required hardware interfaces. It also starts
 * the security monitoring process.
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called once during the system initialization phase.
 */
esp_err_t security_module_start(void);

#endif // SECURITY_MODULE_H
