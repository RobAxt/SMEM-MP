#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_err.h"

#include "security_module.h"
#include "security_ao_fsm.h"
#include "security_watcher.h"
#include "ao_core.h"
#include "ao_fsm.h"


#define SEC_TAGREAD_TIMER_MS 20000
#define SEC_WORKING_TIMER_MS 60000

static const char *TAG = "security_ao_fsm";

static TimerHandle_t tagReadTimerHandle = NULL;
static TimerHandle_t workingTimerHandle = NULL;

/**
 * @brief Stops and deletes a timer.
 * @details This function stops the specified timer and sets its handle to NULL.
 * @param timerHandle Pointer to the timer handle to stop and delete.
 */
static void security_stop_timer(TimerHandle_t timerHandle)
{
    if(timerHandle != NULL)
        ao_fsm_timer_stop(timerHandle);
}

/**
 * @brief Starts the tag read timer.
 * @details This function starts a timer that will post a READ_TAG_TIMEOUT_EVENT
 *          to the FSM after SEC_TAGREAD_TIMER_MS milliseconds.
 * @param fsm Pointer to the finite state machine instance.
 * @note If the timer is already running, it will be restarted.
 */
static void security_start_tagRead_timer(ao_fsm_t *fsm)
{
    if(fsm != NULL) 
    {
        if(tagReadTimerHandle != NULL) 
        {
            ESP_LOGI(TAG, "Tag read timer already created. Restarting it.");
            ao_fsm_timer_reset(tagReadTimerHandle, fsm, READ_TAG_TIMEOUT_EVENT, SEC_TAGREAD_TIMER_MS);
        }
        else
        {
            ESP_LOGI(TAG, "Tag read timer created.");
            tagReadTimerHandle = ao_fsm_timer_start(fsm, READ_TAG_TIMEOUT_EVENT, SEC_TAGREAD_TIMER_MS);
        }
    
        if(tagReadTimerHandle == NULL) 
            ESP_LOGE(TAG, "Failed to start tag read timer");
    }
}

/**
 * @brief Starts the working timer.
 * @details This function starts a timer that will post a WORKING_TIMEOUT_EVENT
 *          to the FSM after SEC_WORKING_TIMER_MS milliseconds.
 * @param fsm Pointer to the finite state machine instance.
 * @note If the timer is already running, it will be restarted.
 */
static void security_start_working_timer(ao_fsm_t *fsm)
{
    if(fsm != NULL) 
    {
        if(workingTimerHandle != NULL) 
        {
            ESP_LOGI(TAG, "Working timer already created. Restarting it.");
            ao_fsm_timer_reset(workingTimerHandle, fsm, WORKING_TIMEOUT_EVENT, SEC_WORKING_TIMER_MS);
        }
        else
        {
            ESP_LOGI(TAG, "Working timer created.");
            workingTimerHandle = ao_fsm_timer_start(fsm, WORKING_TIMEOUT_EVENT, SEC_WORKING_TIMER_MS);
        }
    
        if(workingTimerHandle == NULL) 
            ESP_LOGE(TAG, "Failed to start working timer");
    }
}

// ---------------------------------------------------------------------------------------------------------

// Monitoring State Function Definition

ao_fsm_state_t security_monitoringState_intrusionDetected_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != INTRUSION_DETECTED_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_monitoringState_intrusionDetected_action");
        return SEC_MONITORING_STATE;
    }
    ESP_LOGI(TAG, "Intrusion detected! Transitioning to VALIDATION_STATE.");

    // Start tag read timer
    security_start_tagRead_timer(fsm);

    // Notify intrusion detected event
    if(security_onEvent_callbacks[INTRUSION_DETECTED_EVENT] != NULL)
        security_onEvent_callbacks[INTRUSION_DETECTED_EVENT]();

    return SEC_VALIDATION_STATE;
}

ao_fsm_state_t security_monitoringState_panicButtonPressed_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != PANIC_BUTTON_PRESSED_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_monitoringState_panicButtonPressed_action");
        return SEC_MONITORING_STATE;
    }
    ESP_LOGI(TAG, "Panic button pressed! Transitioning to VALIDATION_STATE.");

    // Start tag read timer
    security_start_tagRead_timer(fsm);

    // Notify panic button pressed event
    if(security_onEvent_callbacks[PANIC_BUTTON_PRESSED_EVENT] != NULL)
        security_onEvent_callbacks[PANIC_BUTTON_PRESSED_EVENT]();

    return SEC_VALIDATION_STATE;
}

ao_fsm_state_t security_monitoringState_turnLightsOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_LIGHTS_ON_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_monitoringState_turnLightsOn_action");
        return SEC_MONITORING_STATE;
    }
    ESP_LOGI(TAG, "Turn lights on command received in MONITORING_STATE.");

    // Turn lights on.
    security_turnLights_on();

    // Notify turn lights on event
    if(security_onEvent_callbacks[TURN_LIGHTS_ON_EVENT] != NULL)
        security_onEvent_callbacks[TURN_LIGHTS_ON_EVENT]();
    
    return SEC_MONITORING_STATE;
}


ao_fsm_state_t security_monitoringState_turnLightsOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_LIGHTS_OFF_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_monitoringState_turnLightsOff_action");
        return SEC_MONITORING_STATE;
    }
    ESP_LOGI(TAG, "Turn lights off command received in MONITORING_STATE.");

    // Turn lights off.
    security_turnLights_off();

    // Notify turn lights off event
    if(security_onEvent_callbacks[TURN_LIGHTS_OFF_EVENT] != NULL)
        security_onEvent_callbacks[TURN_LIGHTS_OFF_EVENT]();

    return SEC_MONITORING_STATE;
}


ao_fsm_state_t security_monitoringState_turnSirenOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_SIREN_ON_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_monitoringState_turnSirenOn_action");
        return SEC_MONITORING_STATE;
    }
    ESP_LOGI(TAG, "Turn siren on command received in MONITORING_STATE.");

    //Turn siren on.
    security_turnSiren_on();

    // Notify turn siren on event
    if(security_onEvent_callbacks[TURN_SIREN_ON_EVENT] != NULL)
        security_onEvent_callbacks[TURN_SIREN_ON_EVENT]();
    
    return SEC_MONITORING_STATE;
}

ao_fsm_state_t security_monitoringState_turnSirenOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_SIREN_OFF_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_monitoringState_turnSirenOff_action");
        return SEC_MONITORING_STATE;
    }
    ESP_LOGI(TAG, "Turn siren off command received in MONITORING_STATE.");

    //Turn siren off.
    security_turnSiren_off();

    // Notify turn siren off event
    if(security_onEvent_callbacks[TURN_SIREN_OFF_EVENT] != NULL)
        security_onEvent_callbacks[TURN_SIREN_OFF_EVENT]();
    
    return SEC_MONITORING_STATE;
}

// ---------------------------------------------------------------------------------------------------------

// Validation State Function Definition

ao_fsm_state_t security_validationState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != INVALID_TAG_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_validationState_invalidTagEvent_action");
        return SEC_VALIDATION_STATE;
    }
    ESP_LOGW(TAG, "Invalid tag read. Transitioning to ALARM_STATE.");

    // Stop all timers
    security_stop_timer(tagReadTimerHandle);
    security_stop_timer(workingTimerHandle);

    // Activate siren and lights.
    security_turnLights_on();
    security_turnSiren_on();

    // Notify invalid tag event
    if(security_onEvent_callbacks[INVALID_TAG_EVENT] != NULL)
        security_onEvent_callbacks[INVALID_TAG_EVENT]();

    return SEC_ALARM_STATE;
}

ao_fsm_state_t security_validationState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != VALID_TAG_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_validationState_validTagEvent_action");
        return SEC_VALIDATION_STATE;
    }
    ESP_LOGI(TAG, "Valid tag read. Transitioning to NORMAL_STATE.");

    // Stop tag read timer
    security_stop_timer(tagReadTimerHandle);

    // Deactivate siren and lights.
    security_turnLights_off();
    security_turnSiren_off();

    // Start working timer
    security_start_working_timer(fsm);

    // Notify valid tag event
    if(security_onEvent_callbacks[VALID_TAG_EVENT] != NULL)
        security_onEvent_callbacks[VALID_TAG_EVENT]();

    return SEC_NORMAL_STATE;
}

ao_fsm_state_t security_validationState_tagReadTimeoutEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != READ_TAG_TIMEOUT_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_validationState_tagReadTimeoutEvent_action");
        return SEC_VALIDATION_STATE;
    }
    ESP_LOGW(TAG, "Tag read timeout. Transitioning to ALARM_STATE.");
    
    // Stop tag read timer
    security_stop_timer(tagReadTimerHandle);

    // Activate siren and lights.
    security_turnLights_on();
    security_turnSiren_on();

    // Notify tag read timeout event
    if(security_onEvent_callbacks[READ_TAG_TIMEOUT_EVENT] != NULL)
        security_onEvent_callbacks[READ_TAG_TIMEOUT_EVENT]();

    return SEC_ALARM_STATE;
}

// ---------------------------------------------------------------------------------------------------------

// Alarm State Function Definition

ao_fsm_state_t security_alarmState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != INVALID_TAG_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_alarmState_invalidTagEvent_action");
        return SEC_ALARM_STATE;
    }
    ESP_LOGI(TAG, "Invalid tag event received in ALARM_STATE. Staying in ALARM_STATE.");

    // Notify invalid tag event
    if(security_onEvent_callbacks[INVALID_TAG_EVENT] != NULL)
        security_onEvent_callbacks[INVALID_TAG_EVENT]();

    return SEC_ALARM_STATE;
}

ao_fsm_state_t security_alarmState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != VALID_TAG_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_alarmState_validTagEvent_action");
        return SEC_ALARM_STATE;
    }
    ESP_LOGI(TAG, "Valid tag event received in ALARM_STATE. Transitioning to NORMAL_STATE.");

    // Deactivate siren and lights.
    security_turnLights_off();
    security_turnSiren_off();

    // Start working timer
    security_start_working_timer(fsm);

    // Notify valid tag event
    if(security_onEvent_callbacks[VALID_TAG_EVENT] != NULL)
        security_onEvent_callbacks[VALID_TAG_EVENT]();

    return SEC_NORMAL_STATE;
}

ao_fsm_state_t security_alarmState_turnLightsOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_LIGHTS_OFF_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_alarmState_turnLightsOff_action");
        return SEC_ALARM_STATE;
    }
    ESP_LOGI(TAG, "Turn lights off command received in ALARM_STATE.");

    // Turn lights off.
    security_turnLights_off();

    // Notify turn lights off event
    if(security_onEvent_callbacks[TURN_LIGHTS_OFF_EVENT] != NULL)
        security_onEvent_callbacks[TURN_LIGHTS_OFF_EVENT]();
    
    return SEC_ALARM_STATE;
}

ao_fsm_state_t security_alarmState_turnSirenOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_SIREN_OFF_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_alarmState_turnSirenOff_action");
        return SEC_ALARM_STATE;
    }
    ESP_LOGI(TAG, "Turn siren off command received in ALARM_STATE.");

    // Turn siren off.
    security_turnSiren_off();

    // Notify turn siren off event
    if(security_onEvent_callbacks[TURN_SIREN_OFF_EVENT] != NULL)
        security_onEvent_callbacks[TURN_SIREN_OFF_EVENT]();
    
    return SEC_ALARM_STATE;
}
// ---------------------------------------------------------------------------------------------------------

// Normal State Function Definition

ao_fsm_state_t security_normalState_workingTimeoutEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != WORKING_TIMEOUT_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_normalState_workingTimeoutEvent_action");
        return SEC_NORMAL_STATE;
    }
    ESP_LOGI(TAG, "Working timeout event received. Transitioning to MONITORING_STATE.");
    
    security_stop_timer(workingTimerHandle);

    // Notify working timeout event
    if(security_onEvent_callbacks[WORKING_TIMEOUT_EVENT] != NULL)
        security_onEvent_callbacks[WORKING_TIMEOUT_EVENT]();
    
    return SEC_MONITORING_STATE;
}

ao_fsm_state_t security_normalState_panicButtonPressed_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != PANIC_BUTTON_PRESSED_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_normalState_panicButtonPressed_action");
        return SEC_NORMAL_STATE;
    }
    ESP_LOGI(TAG, "Panic button pressed! Transitioning to VALIDATION_STATE.");

    // Stop working timer
    security_stop_timer(workingTimerHandle);
    // Start tag read timer
    security_start_tagRead_timer(fsm);

    // Notify panic button pressed event
    if(security_onEvent_callbacks[PANIC_BUTTON_PRESSED_EVENT] != NULL)
        security_onEvent_callbacks[PANIC_BUTTON_PRESSED_EVENT]();
    
    return SEC_VALIDATION_STATE;
}

ao_fsm_state_t security_normalState_turnLightsOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_LIGHTS_ON_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_normalState_turnLightsOn_action");
        return SEC_NORMAL_STATE;
    }
    ESP_LOGI(TAG, "Turn lights on command received in NORMAL_STATE.");

    // Turn lights on.
    security_turnLights_on();
    
    // Notify turn lights on event
    if(security_onEvent_callbacks[TURN_LIGHTS_ON_EVENT] != NULL)
        security_onEvent_callbacks[TURN_LIGHTS_ON_EVENT]();
    
    return SEC_NORMAL_STATE;
}
ao_fsm_state_t security_normalState_turnLightsOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_LIGHTS_OFF_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_normalState_turnLightsOff_action");
        return SEC_NORMAL_STATE;
    }
    ESP_LOGI(TAG, "Turn lights off command received in NORMAL_STATE.");

    // Turn lights off.
    security_turnLights_off();

    // Notify turn lights off event
    if(security_onEvent_callbacks[TURN_LIGHTS_OFF_EVENT] != NULL)
        security_onEvent_callbacks[TURN_LIGHTS_OFF_EVENT]();
    
    return SEC_NORMAL_STATE;
}
ao_fsm_state_t security_normalState_turnSirenOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_SIREN_ON_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_normalState_turnSirenOn_action");
        return SEC_NORMAL_STATE;
    }
    ESP_LOGI(TAG, "Turn siren on command received in NORMAL_STATE.");

    // Turn siren on.
    security_turnSiren_on();

    // Notify turn siren on event
    if(security_onEvent_callbacks[TURN_SIREN_ON_EVENT] != NULL)
        security_onEvent_callbacks[TURN_SIREN_ON_EVENT]();
    
    return SEC_NORMAL_STATE;
}

ao_fsm_state_t security_normalState_turnSirenOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TURN_SIREN_OFF_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_normalState_turnSirenOff_action");
        return SEC_NORMAL_STATE;
    }
    ESP_LOGI(TAG, "Turn siren off command received in NORMAL_STATE.");

    // Turn siren off.
    security_turnSiren_off();

    // Notify turn siren off event
    if(security_onEvent_callbacks[TURN_SIREN_OFF_EVENT] != NULL)
        security_onEvent_callbacks[TURN_SIREN_OFF_EVENT]();
    
    return SEC_NORMAL_STATE;
}