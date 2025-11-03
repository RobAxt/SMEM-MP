#ifndef ENERGY_MODULE_H
#define ENERGY_MODULE_H

#include "esp_err.h"

/**
 * @file energy_module.h
 * @brief Header file for the Energy Module.
 * @details This module handles energy management functionalities. 
 *          It provides interfaces for monitoring and controlling energy consumption.
 * 
 * @author Roberto Axt
 * @version 1.0
 * @date 2025-11-03
 * 
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

/**
 * @brief Callback function type for energy data read events.
 * @brief This callback is invoked when energy data is read.
 * @param data Pointer to the data read.
 * @param size Size of the data read.
 */
 typedef void (*hookCallback_onEnergyRead)(void *data, ssize_t size);

/**
 * @brief Data structure for energy callback data
 * @details This structure holds energy-related parameters such as AC/DC voltage, 
 *          current, power, frequency, and power factor.  
 */ 
 typedef struct {
    float ac_voltage;      /**< AC Voltage in Volts */
    float ac_current;      /**< AC Current in Amperes */
    float ac_power;        /**< AC Power in Watts */
    float ac_frequency;    /**< AC Frequency in Hertz */
    float ac_power_factor; /**< AC Power Factor (0 to 1) */
    float dc_voltage;      /**< DC Voltage in Volts */
    float dc_current;      /**< DC Current in Amperes */
    float dc_power;        /**< DC Power in Watts */
} energy_data_t;

/**
 * @brief Set the callback function for energy read events.
 * @details This function allows the user to set a custom callback function
 *          that will be invoked when energy data is read.
 * @param callback The callback function to be set.
 */
void energy_set_hookCallback_onEnergyRead(hookCallback_onEnergyRead callback); 

/**
 * @brief Start the energy module.
 * @details This function initializes and starts the energy module.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t energy_module_start(void);

 #endif // ENERGY_MODULE_H
