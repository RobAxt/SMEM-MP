#ifndef AO_FSM_H
#define AO_FSM_H

/**
 * @file ao_fsm.h
 * @brief Header file for the AO FSM (Active Object Finite State Machine) module.
 * @details This file contains the declarations and definitions for the AO FSM module,
 *          which is responsible for managing finite state machines in active objects.
 * @author Roberto Axt
 * @date 2025-10-13
 * @version 0.1
 * 
 * @par License
 * This file is part of the AO FSM module and is licensed under the MIT License.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ao_core.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_err.h"

/** 
 * @brief Forward declaration of the ao_fsm_t structure.
 * @details This is a forward declaration of the ao_fsm_t structure, which represents a finite state machine.
 */
typedef struct ao_fsm_s ao_fsm_t;

/**
 * @brief Definition of the state type for the finite state machine.
 * @details This type represents the state of the finite state machine.
 * It is defined as an 8-bit unsigned integer.
 */
typedef uint8_t ao_fsm_state_t;

/**
 * @brief Definition of the event structure for the finite state machine.
 * @details This structure represents an event that can be processed by the finite state machine.
 * It contains the event type, length, and a pointer to the event data.
 */
typedef ao_evt_t ao_fsm_evt_t;

/**
 * @brief Definition of the event type for the finite state machine.
 * @details This type represents the event type that can trigger state transitions
 * in the finite state machine. It is defined as an 8-bit unsigned integer.
 */
typedef ao_evt_type_t ao_fsm_evt_type_t;

/**
 * @brief Definition of the action handler function type for state transitions.
 * @details This type defines the signature of the action handler function for state transitions
 * in the finite state machine. The handler takes a pointer to the FSM and a pointer to the event
 * that triggered the transition, and returns the next state of the FSM.
 */
typedef ao_fsm_state_t (*ao_fsm_action_handler_t)(ao_fsm_t* fsm, const ao_fsm_evt_t* evt);

/**
 * @brief Definition of the state transition structure.
 * @details This structure represents a state transition in the finite state machine,
 * including the current state and the action handler function to be executed during the transition.
 */
typedef struct {
    ao_fsm_state_t          state;
    ao_fsm_evt_type_t       event_type;
    ao_fsm_action_handler_t action;
} ao_fsm_transition_t;

/**
 * @brief Creates a new finite state machine (FSM) for an active object.
 * @details This function creates a new FSM with the specified name and initial state.
 * @param name The name of the FSM.
 * @param initial_state The initial state of the FSM.
 * @param transitions An array of state transition definitions for the FSM.
 * @param transitions_count The number of state transitions in the transitions array.
 * @return A pointer to the created FSM, or NULL if creation fails.  
 */
ao_fsm_t* ao_fsm_create(const char* name, ao_fsm_state_t initial_state, const ao_fsm_transition_t* transitions, size_t transitions_count);

/**
 * @brief Starts the finite state machine (FSM) associated with an active object.
 * @details This function starts the FSM by starting its underlying active object with the specified
 * priority and stack size.
 * @param fsm A pointer to the FSM to start.
 * @param prio The priority of the FSM's active object's task.
 * @param stack_words The stack size of the FSM's active object's task in words.
 * @return ESP_OK if the FSM is started successfully, or an error code otherwise.
 * @note The FSM must be created using ao_fsm_create() before calling this function.
 */
esp_err_t ao_fsm_start(ao_fsm_t* fsm, UBaseType_t prio, uint32_t stack_words);

/**
 * @brief Posts an event to the finite state machine (FSM) associated with an active object.
 * @details This function posts an event with the specified type and payload to the FSM's
 * underlying active object's event queue. If the queue is full, it will wait for a default timeout duration.
 * @param fsm A pointer to the FSM to which the event is posted.
 * @param type The type of the event.
 * @param payload A pointer to the event payload data.
 * @param len The length of the event payload data.
 * @return ESP_OK if the event is posted successfully, or an error code otherwise.
 * @note The FSM must be started using ao_fsm_start() before posting events to it.
 */
esp_err_t ao_fsm_post(ao_fsm_t* fsm, ao_fsm_evt_type_t type, const void* payload, uint8_t len);

/**
 * @brief Destroys the finite state machine (FSM) and its associated active object.
 * @details This function destroys the FSM and frees all associated resources, including
 * its underlying active object.
 * @param fsm A pointer to the FSM to destroy.
 */
void ao_fsm_destroy(ao_fsm_t* fsm);

/**
 * @brief Starts a periodic timer that posts events to the FSM.
 * @details This function creates and starts a FreeRTOS timer that periodically posts
 * events of the specified type to the FSM's event queue at the specified interval.
 * @param fsm A pointer to the FSM to which events will be posted.
 * @param event_type The type of event to post when the timer expires.
 * @param period_ms The period of the timer in milliseconds.
 * @return A handle to the created timer, or NULL if creation or starting fails.
 * @note The caller is responsible for stopping and deleting the timer using ao_fsm_timer_stop().
 */
TimerHandle_t ao_fsm_timer_start(ao_fsm_t* fsm, ao_fsm_evt_type_t event_type, uint32_t period_ms);

/**
 * @brief Stops and deletes a previously started timer.
 * @details This function stops the specified FreeRTOS timer and deletes it,
 * freeing all associated resources.
 * @param timer A handle to the timer to stop and delete.
 * @note The timer must have been created using ao_fsm_timer_start().
 */
void ao_fsm_timer_stop(TimerHandle_t timer);

#endif // AO_FSM_H