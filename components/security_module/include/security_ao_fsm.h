#ifndef SECURITY_AO_FSM_H
#define SECURITY_AO_FSM_H

/**
 * @file security_ao_fsm.h
 * @brief Header file for the Security AO FSM (Finite State Machine) module.
 * @details This module defines the states and events for the FSM used in the security application.
 *          It provides the necessary structures and function prototypes to manage the FSM.
 * 
 * @author Roberto Axt
 * @version 1.0
 * @date 2024-09-21
 *  
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

#include "ao_fsm.h"

// Define states and events for the Security AO FSM

enum { SEC_MONITORING_STATE = 0, SEC_VALIDATION_STATE = 1, SEC_ALARM_STATE = 2, SEC_NORMAL_STATE = 3 };
enum { INTRUSION_DETECTED_EVENT = 0, PANIC_BUTTON_PRESSED_EVENT = 1, TAG_READ_EVENT = 2, VALID_TAG_EVENT = 3, 
       INVALID_TAG_EVENT = 4, READ_TAG_TIMEOUT_EVENT = 5, WORKING_TIMEOUT_EVENT = 6 };

// Function prototypes for action handlers

/**
 * @brief Action function for handling intrusion detected event in monitoring state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when an intrusion is detected while in the monitoring state.
 *      It transitions the FSM to the validation state.
 */
ao_fsm_state_t security_monitoringState_intrusionDetected_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling panic button pressed event in monitoring state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when the panic button is pressed while in the monitoring state.
 *      It transitions the FSM to the validation state.
 */
ao_fsm_state_t security_monitoringState_panicButtonPressed_action(ao_fsm_t *fsm, const ao_evt_t *event);
/**
 * @brief Action function for handling tag read event in validation state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when a tag is read while in the validation state.
 *      It processes the tag and determines if it is valid or invalid.
 */
ao_fsm_state_t security_validationState_tagReadEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling valid tag event in validation state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when a valid tag is detected while in the validation state.
 *      It transitions the FSM to the normal state and launch the working timer.
 */
ao_fsm_state_t security_validationState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling invalid tag event in validation state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when an invalid tag is detected while in the validation state.
 *      It transitions the FSM to the alarm state.
 */
ao_fsm_state_t security_validationState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling tag read timeout event in validation state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when a tag read operation times out while in the validation state.
 *      It transitions the FSM back to the alarm state.
 */
ao_fsm_state_t security_validationState_tagReadTimeoutEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling tag read event in alarm state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when a tag is read while in the alarm state.
 *      It processes the tag and determines if it is valid or invalid.
 *      It transitions the FSM back to the validation state.
 */
ao_fsm_state_t security_alarmState_tagReadEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling working timeout event in normal state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note This function is called when a working timeout occurs while in the normal state.
 *      It transitions the FSM back to the monitoring state.
 */
ao_fsm_state_t security_normalState_workingTimeoutEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief State transition table for the Security AO FSM.
 * @details This table defines the state transitions for the Security AO FSM,
 *          including the current state, event type, and corresponding action handler.
 */
static ao_fsm_transition_t security_fsm_transitions[] = 
{
    { SEC_MONITORING_STATE, INTRUSION_DETECTED_EVENT,   security_monitoringState_intrusionDetected_action },
    { SEC_MONITORING_STATE, PANIC_BUTTON_PRESSED_EVENT, security_monitoringState_panicButtonPressed_action },
    { SEC_VALIDATION_STATE, TAG_READ_EVENT,             security_validationState_tagReadEvent_action },
    { SEC_VALIDATION_STATE, VALID_TAG_EVENT,            security_validationState_validTagEvent_action },
    { SEC_VALIDATION_STATE, INVALID_TAG_EVENT,          security_validationState_invalidTagEvent_action },
    { SEC_VALIDATION_STATE, READ_TAG_TIMEOUT_EVENT,     security_validationState_tagReadTimeoutEvent_action },
    { SEC_ALARM_STATE,      TAG_READ_EVENT,             security_alarmState_tagReadEvent_action },
    { SEC_NORMAL_STATE,     WORKING_TIMEOUT_EVENT,      security_normalState_workingTimeoutEvent_action }
};

#endif // SECURITY_AO_FSM_H