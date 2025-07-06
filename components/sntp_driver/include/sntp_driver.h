#ifndef SNTP_DRIVER_H
#define SNTP_DRIVER_H

/**
 * @file sntp_driver.h
 * @brief Header file for the SNTP driver component.
 * @details This file contains the declaration of the function used to initialize the SNTP driver.
 * 
 * @author  Roberto Axt
 * @date    2025-07-05
 * @version 0.0
 *
 * @par License
 * This project is licensed under the MIT License.
 */

#include "esp_err.h"
#include "esp_netif.h"

#define TIMESTAMP_STRING_SIZE 64  // Example  Sat Jul  5 20:06:30 2025
#define ISO_TIMESTAMP_SIZE    25  // "YYYY-MM-DDTHH:MM:SSZ" + null terminator

/**
 * @brief Start the SNTP client.
 * @details This function initializes the SNTP client and starts it to synchronize time with an NTP server.
 *
 * @param ip IPv4 address of the NTP server to synchronize with.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t sntp_client_start(esp_ip4_addr_t ip);

/**
 * @brief Obtain the current time as a formatted string.
 * @details This function retrieves the current time and formats it into a string.
 * 
 * @param datetime_string Pointer to a buffer where the formatted date and time will be stored.
 * @param size Size of the buffer pointed to by datetime_string.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t sntp_client_time(char *datetime_string, size_t size);


/**
 * @brief Obtain the current time in ISO 8601 format.
 * @details This function retrieves the current time and formats it into an ISO 8601 string.
 * 
 * @param datetime_string Pointer to a buffer where the ISO formatted date and time will be stored.
 * @param size Size of the buffer pointed to by datetime_string.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
esp_err_t sntp_client_isotime(char *datetime_string, size_t size);

#endif // SNTP_DRIVER_H
