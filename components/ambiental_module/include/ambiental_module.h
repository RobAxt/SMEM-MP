#ifndef AMBIENTAL_MODULE_H
#define AMBIENTAL_MODULE_H

/**
 * @file ambiental_module.h
 * @brief Header file for the Ambiental Module component.
 * @details This file declares the interface for the Ambiental Module component,
 *          which provides functionalities related to environmental sensing using
 *          DS18B20 temperature sensors over a 1-Wire bus.
 * 
 * @author Roberto Axt
 * @version 1.0
 * @date 2025-11-01
 *
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

/**
 * @brief Callback type for temperature notifications.
 * @details This callback is invoked when a temperature reading is available.
 * @param data Pointer to the temperature structure data.
 * @param size Size of the temperature structure data.
 * @return void No return value.
 */
typedef void (*hookCallback_onTemperatureRead)(void *data, ssize_t size);

/**
 * @brief Data structure for temperature callback data.
 * @details This structure holds information about a DS18B20 sensor and its
 *          latest temperature reading.
 */
typedef struct {
    uint64_t sensor_id;        /**< Unique identifier for the DS18B20 sensor */
    float temperature_celsius; /**< Temperature in Celsius */
} ambiental_callback_data_t;

/**
 * @brief Set the callback function for temperature read events.
 * @details This function allows the user to register a callback that will be
 *          called whenever a temperature reading is obtained from a DS18B20 sensors.
 * @param cb The callback function to be set.
 * @return void No return value.
 */
void ambiental_set_hookCallback_onTemperatureRead(hookCallback_onTemperatureRead cb);

/**
 * @brief Start the Ambiental Module component.
 * @details This function initializes and starts the Ambiental Module component,
 *          setting up necessary resources and configurations.
 * @return esp_err_t Returns ESP_OK on success or an error code on failure.
 */
esp_err_t ambiental_module_start(void);

#endif // AMBIENTAL_MODULE_H