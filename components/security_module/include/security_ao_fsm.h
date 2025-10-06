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
enum { INTRUSION_DETECTED_EVENT = 0, PANIC_BUTTON_PRESSED_EVENT = 1, TURN_LIGHTS_ON_EVENT = 2, 
       TURN_LIGHTS_OFF_EVENT = 3, TURN_SIREN_ON_EVENT = 4, TURN_SIREN_OFF_EVENT = 5, VALID_TAG_EVENT = 6,
       INVALID_TAG_EVENT = 7, READ_TAG_TIMEOUT_EVENT = 8, WORKING_TIMEOUT_EVENT = 9, MAX_EVENT = 10 };

// Function prototypes for action handlers

// Monitoring State Function Prototipe

/**
 * @brief Action function for handling intrusion detected event in monitoring state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event: Validation
 * @note This function is called when an intrusion is detected with the PIR sensor while in 
 *       the monitoring state. Also must start the tag read timer.
 */
ao_fsm_state_t security_monitoringState_intrusionDetected_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling panic button pressed event in monitoring state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event: Validation
 * @note This function is called when the panic button is pressed while in the monitoring state.
 *       Also must start the tag read timer.
 */
ao_fsm_state_t security_monitoringState_panicButtonPressed_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event:
 * @note This function is called when
 */
ao_fsm_state_t security_monitoringState_turnLightsOn_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event:
 * @note This function is called when
 */
ao_fsm_state_t security_monitoringState_turnLightsOff_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event:
 * @note This function is called when
 */
ao_fsm_state_t security_monitoringState_turnSirenOn_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event:
 * @note This function is called when
 */
ao_fsm_state_t security_monitoringState_turnSirenOff_action(ao_fsm_t *fsm, const ao_evt_t *event);

// ---------------------------------------------------------------------------------------------------------

// Validation State Function Prototipe

/**
 * @brief Action function for handling invalid tag read event in validation state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event: Alarm
 * @note This function is called when an invalid read of a RFID Tag occurs. Also must stop the read
 *       tag timer and turn on the lights and siren.
 */
ao_fsm_state_t security_validationState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling valid tag read event in validation state
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event: Normal
 * @note his function is called when a valid read of a RFID Tag occurs. Also must stop the timer,
 *       turn off the lights and siren and start the working timer.
 */
ao_fsm_state_t security_validationState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling tag read timeout event in validation state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event: Alarm
 * @note This function is called when a tag read operation times out while in the validation state.
 *       Also must stop the read tag timer and turn on the lights and siren.
 */
ao_fsm_state_t security_validationState_tagReadTimeoutEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

// ---------------------------------------------------------------------------------------------------------

// Alarm State Function Prototipe

/**
 * @brief
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note
 */
ao_fsm_state_t security_alarmState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event: Alarm
 * @note
 */
ao_fsm_state_t security_alarmState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

// ---------------------------------------------------------------------------------------------------------

// Normal State Function Prototipe

/**
 * @brief Action function for handling working timeout event in normal state.
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event: Alarm
 * @note This function is called when a working timeout occurs while in the normal state.
 *      It transitions the FSM back to the monitoring state.
 */
ao_fsm_state_t security_normalState_workingTimeoutEvent_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event.
 * @note
 */
ao_fsm_state_t security_normalState_panicButtonPressed_action(ao_fsm_t *fsm, const ao_evt_t *event);

/**
 * @brief Action function for handling
 * @param fsm Pointer to the finite state machine instance.
 * @param event Pointer to the event that triggered the action.
 * @return The next state of the FSM after handling the event:
 * @note This function is called when
 */
ao_fsm_state_t security_normalState_turnLightsOn_action(ao_fsm_t *fsm, const ao_evt_t *event);
ao_fsm_state_t security_normalState_turnLightsOff_action(ao_fsm_t *fsm, const ao_evt_t *event);
ao_fsm_state_t security_normalState_turnSirenOn_action(ao_fsm_t *fsm, const ao_evt_t *event);
ao_fsm_state_t security_normalState_turnSirenOff_action(ao_fsm_t *fsm, const ao_evt_t *event);
/**
 * @brief State transition table for the Security AO FSM.
 * @details This table defines the state transitions for the Security AO FSM,
 *          including the current state, event type, and corresponding action handler.
 */
static ao_fsm_transition_t security_fsm_transitions[] = 
{
    // Monitoring State
    { SEC_MONITORING_STATE, INTRUSION_DETECTED_EVENT,   security_monitoringState_intrusionDetected_action },
    { SEC_MONITORING_STATE, PANIC_BUTTON_PRESSED_EVENT, security_monitoringState_panicButtonPressed_action },

    // Global Commands on Monitoring
    { SEC_MONITORING_STATE, TURN_LIGHTS_ON_EVENT,       security_monitoringState_turnLightsOn_action  },     
    { SEC_MONITORING_STATE, TURN_LIGHTS_OFF_EVENT,      security_monitoringState_turnLightsOff_action },     
    { SEC_MONITORING_STATE, TURN_SIREN_ON_EVENT,        security_monitoringState_turnSirenOn_action   },     
    { SEC_MONITORING_STATE, TURN_SIREN_OFF_EVENT,       security_monitoringState_turnSirenOff_action  },    

    // Validation State
    { SEC_VALIDATION_STATE, INVALID_TAG_EVENT,          security_validationState_invalidTagEvent_action },
    { SEC_VALIDATION_STATE, VALID_TAG_EVENT,            security_validationState_validTagEvent_action },
    { SEC_VALIDATION_STATE, READ_TAG_TIMEOUT_EVENT,     security_validationState_tagReadTimeoutEvent_action },

    // Alarm State
    { SEC_ALARM_STATE,      INVALID_TAG_EVENT,          security_alarmState_invalidTagEvent_action },
    { SEC_ALARM_STATE,      VALID_TAG_EVENT,            security_alarmState_validTagEvent_action },

    // Normal    
    { SEC_NORMAL_STATE,     WORKING_TIMEOUT_EVENT,      security_normalState_workingTimeoutEvent_action },

    // Silent Alarm - return to Normal State
    { SEC_NORMAL_STATE,     PANIC_BUTTON_PRESSED_EVENT, security_normalState_panicButtonPressed_action },

    // Global Commands on Normal
    { SEC_NORMAL_STATE,     TURN_LIGHTS_ON_EVENT,       security_normalState_turnLightsOn_action  },
    { SEC_NORMAL_STATE,     TURN_LIGHTS_OFF_EVENT,      security_normalState_turnLightsOff_action },
    { SEC_NORMAL_STATE,     TURN_SIREN_ON_EVENT,        security_normalState_turnSirenOn_action   },
    { SEC_NORMAL_STATE,     TURN_SIREN_OFF_EVENT,       security_normalState_turnSirenOff_action  }   

};

#endif // SECURITY_AO_FSM_H