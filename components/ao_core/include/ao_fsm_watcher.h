#ifndef AO_FSM_WATCHER_H
#define AO_FSM_WATCHER_H

/**
 * @file ao_fsm_watcher.h
 * @brief Header file for AO FSM Watcher module.
 * @details This file contains the declarations and definitions for the AO FSM Watcher module,
 *          which is responsible for sending periodic events to an FSM. The user will be able to
 *          register callbacks to be called periodically in order to read a sensor and perform 
 *          the posting of events to the FSM. 
 *         
 * @author Roberto Axt
 * @date 2025-09-23
 * @version 0.0
 * 
 * @par License
 * This file is part of the AO module and is licensed under the MIT License.
 */

#include "esp_err.h"
#include "ao_fsm.h"

/**
 * @brief Opaque type for the FSM watcher.
 * @details This type represents a watcher that holds the callbacks to be called periodically
 * for a specific FSM.
 */
typedef struct ao_fsm_watcher_s ao_fsm_watcher_t;

/**
 * @brief Callback function type for FSM state changes.
 * @details This type defines the signature of the callback function that is called periodically
 *          for the user to read sensors and post events to the FSM.
 * The callback takes a pointer to the FSM as its parameter.
 */
typedef void (*ao_fsm_watcher_cb_t)(ao_fsm_t* fsm);

/**
 * @brief Starts a watcher for the specified FSM.
 * @details This function creates and starts a watcher for the given FSM. The watcher will allow
 * the user to register callbacks that will be called periodically to read sensors and post events.
 * @param fsm A pointer to the FSM to be watched.
 * @return A pointer to the created FSM watcher, or nullptr if creation fails.
 * @note The returned watcher must be stopped using ao_fsm_watcher_stop() to free resources.
 */
ao_fsm_watcher_t* ao_fsm_watcher_start(ao_fsm_t* fsm, uint32_t interval_ms);

/**
 * @brief Stops the specified FSM watcher.
 * @details This function stops the given FSM watcher and frees its resources.
 * @param watcher A pointer to the FSM watcher to be stopped.
 * @note The watcher must have been created using ao_fsm_watcher_start().
 */
void ao_fsm_watcher_stop(ao_fsm_watcher_t* watcher);

/**
 * @brief Adds a callback to the FSM watcher.
 * @details This function adds a callback to the specified FSM watcher. The callback will be called
 * periodically at the specified interval to allow the user to read sensors and post events to the FSM
 * @param watcher A pointer to the FSM watcher.
 * @param cb The callback function to be called on state changes.
 * @param period_ms The period in milliseconds at which the callback should be called.
 * @return ESP_OK if the callback was added successfully, or an error code otherwise.
 * @note The watcher must have been created using ao_fsm_watcher_start().
 */
esp_err_t ao_fsm_watcher_add_callback(ao_fsm_watcher_t* watcher, ao_fsm_watcher_cb_t cb);

#endif //AO_FSM_WATCHER_H