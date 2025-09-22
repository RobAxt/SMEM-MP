#ifndef AO_CORE_H
#define AO_CORE_H

/**
 * @file ao_core.h
 * @brief Header file for the AO (Active Object) module.
 * @details This file contains the declarations and definitions for the AO module,
 *          which is responsible for managing active objects in the system.
 * @author Roberto Axt
 * @date 2025-10-13
 * @version 0.0
 * 
 * @par License
 * This file is part of the AO module and is licensed under the MIT License.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

/**
 * @brief Definition of the ao_evt_type_t type.
 * @details This type represents the type of an event.
 * @note The type is defined as a uint8_t, allowing for 256 different event types.
 */
typedef uint8_t ao_evt_type_t;

/**
 * @brief Definition of the ao_evt_len_t type.
 * @details This type represents the length of the event data.
 * @note The length is defined as a uint8_t, allowing for event data lengths up to 255 bytes.
 */
typedef uint8_t ao_evt_len_t;

/**
 * @brief Definition of the ao_evt_data_t type.
 * @details This type represents a pointer to the event data.
 * @note The data is defined as a pointer to uint8_t, allowing for flexible event data storage.
 */
typedef uint8_t ao_evt_data_t;

/** 
 * @brief Definition of an event structure.
 * @details This structure represents an event that can be processed by an active object.
 * @note The data field is a flexible array member, allowing for variable-length event data.
 */
typedef struct {
    ao_evt_type_t type;
    ao_evt_len_t  len;
    ao_evt_data_t data[];
} ao_evt_t;

/**
 * @brief Definition of the event context type.
 * @details This type represents the context in which an event is processed.
 * It can be used to pass additional information to the event handler.
 */
typedef void* evt_cntx_t;

/** 
 * @brief Forward declaration of the ao_t structure.
 * @details This is a forward declaration of the ao_t structure, which represents an active object.
 */
typedef struct ao_s ao_t;

/** 
 * @brief Definition of the event handler function type.
 * @details This type defines the signature of the event handler function for an active object.
 * The handler takes a pointer to the event context and a pointer to the event to be processed.
 */
typedef void (*ao_handler_t)(evt_cntx_t evt_cntx, const ao_evt_t* evt);

/**
 * @brief Creates a new active object.
 * @details This function creates a new active object with the specified name, queue length, and
 * event handler function.
 * @param name The name of the active object.
 * @param queue_len The length of the event queue for the active object.
 * @param evt_cntx The context to be passed to the event handler.
 * @param handler The event handler function for the active object.
 * @return A pointer to the newly created active object, or nullptr if creation fails.
 * @note The created active object must be started using ao_start() before it can process events.
 */
ao_t* ao_create(const char* name, size_t queue_len, evt_cntx_t evt_cntx, ao_handler_t handler);

/**
 * @brief Starts the active object.
 * @details This function starts the active object, creating its task with the specified priority
 * and stack size.
 * @param self A pointer to the active object to start.
 * @param prio The priority of the active object's task.
 * @param stack_words The stack size of the active object's task in words.
 * @return ESP_OK if the active object is started successfully, or an error code otherwise.
 * @note The active object must be created using ao_create() before calling this function.
 */
esp_err_t ao_start(ao_t* self, UBaseType_t prio, uint32_t stack_words);

/**
 * @brief Posts an event to the active object's event queue.
 * @details This function posts an event with the specified type and payload to the active object's
 * event queue. If the queue is full, it will wait for the specified timeout duration.
 * @param self A pointer to the active object to which the event is posted.
 * @param type The type of the event.
 * @param payload A pointer to the event payload data.
 * @param len The length of the event payload data.
 * @param to_ticks The timeout duration in ticks to wait if the queue is full.
 * @return ESP_OK if the event is posted successfully, or an error code otherwise.
 * @note The active object must be started using ao_start() before posting events to it.
 */
esp_err_t ao_post(ao_t* self, uint8_t type, const void* payload, uint8_t len, TickType_t to_ticks);

/**
 * @brief Stops the active object.
 * @details This function stops the active object's task and cleans up its resources.
 * @param self A pointer to the active object to stop.
 * @note The active object must be started using ao_start() before calling this function.
 */
void ao_stop(ao_t* self);

/**
 * @brief Destroys the active object.
 * @details This function destroys the active object, freeing its resources.
 * @param self A pointer to the active object to destroy.
 * @note The active object must be stopped using ao_stop() before calling this function.
 */
void ao_destroy(ao_t* self);

/* helpers */
/**
 * @brief Retrieves the overhead size of an event.
 * @details This function returns the overhead size in bytes of an event, which includes
 * the event header and any additional metadata.
 * @return The overhead size in bytes of an event.
 * @note This value is used to calculate the maximum payload size for events.
 */
size_t ao_evt_overhead(void);

/**
 * @brief Retrieves the maximum payload size allowed for an event.
 * @details This function returns the maximum payload size in bytes that can be included
 * in an event, taking into account the overhead size.
 * @return The maximum payload size in bytes for an event.
 * @note The maximum payload size is calculated as (MP_BLOCK_SIZE - overhead).
 */
size_t ao_evt_max_payload(void);
#endif // AO_CORE_H