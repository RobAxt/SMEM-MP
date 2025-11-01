#ifndef SECURITY_WATCHER_H
#define SECURITY_WATCHER_H

/**
 * @file security_watcher.h
 * @brief Header file for the Security Watcher module.
 * @details This module provides the interface for the security watcher functionalities,
 *          including initialization and management of the security watcher.
 * 
 * @author Roberto Axt
 * @version 1.0
 * @date 2024-09-26
 *  
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

#include "esp_err.h"

#include "ao_fsm.h"

/**
 * @brief Starts the devices involved in the security watcher.
 * @details This function initializes and starts the devices involved in the security watcher,
 *          such as the PIR sensor, panic button, lights, siren and RFID reader. It sets up the necessary
 * @return ESP_OK on success, or an error code on failure.
 */ 
esp_err_t security_watcher_devices_start(void);

/**
 * @brief Reads the tag from the RFID reader and posts the appropriate event to the FSM.
 * @details This function reads the tag from the RFID reader and validates it against
 *          a list of valid tags. It then posts either a VALID_TAG_EVENT or INVALID_TAG_EVENT
 *          to the provided FSM based on the validation result.
 * @param fsm Pointer to the finite state machine instance.
 */
void security_tagReader(ao_fsm_t* fsm);

/**
 * @brief Reads the state of the panic button and posts an event to the FSM if pressed.
 * @details This function checks the state of the panic button connected to the system.
 *          If the button is pressed, it posts a PANIC_BUTTON_PRESSED_EVENT to the provided FSM.
 * @param fsm Pointer to the finite state machine instance.
 */
void security_panicButtonReader(ao_fsm_t* fsm);

/**
 * @brief Reads the state of the PIR sensor and posts an event to the FSM if intrusion is detected.
 * @details This function checks the state of the PIR sensor connected to the system.
 *          If an intrusion is detected, it posts an INTRUSION_DETECTED_EVENT to the provided FSM.
 * @param fsm Pointer to the finite state machine instance.
 */
void security_pirSensorReader(ao_fsm_t* fsm);

/**
 * @brief Turns on the security lights.
 * @details This function activates the security lights connected to the system.
 */
void security_turnLights_on(void);

/**
 * @brief Turns off the security lights.
 * @details This function deactivates the security lights connected to the system.
 */
void security_turnLights_off(void);

/**
 * @brief Turns on the security siren.
 * @details This function activates the security siren connected to the system.
 */
void security_turnSiren_on(void);

/**
 * @brief Turns off the security siren.
 * @details This function deactivates the security siren connected to the system.
 */
void security_turnSiren_off(void);

#endif // SECURITY_WATCHER_H