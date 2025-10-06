#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_err.h"

#include "security_ao_fsm.h"
#include "security_watcher.h"
#include "ao_core.h"
#include "ao_fsm.h"


#define SEC_TAGREAD_TIMER_MS 10000
#define SEC_WORKING_TIMER_MS 60000

static const char *TAG = "security_ao_fsm";



static TimerHandle_t tagReadTimerHandle = NULL;
static TimerHandle_t workingTimerHandle = NULL;

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
            ESP_LOGW(TAG, "Tag read timer already running. Restarting it.");
            security_stop_timer(tagReadTimerHandle);
        }

        tagReadTimerHandle = ao_fsm_timer_start(fsm, READ_TAG_TIMEOUT_EVENT, SEC_TAGREAD_TIMER_MS);
    
        if(tagReadTimerHandle == NULL) ESP_LOGE(TAG, "Failed to start tag read timer");
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
            ESP_LOGW(TAG, "Working timer already running. Restarting it.");
            security_stop_timer(workingTimerHandle);
        }
        
        workingTimerHandle = ao_fsm_timer_start(fsm, WORKING_TIMEOUT_EVENT, SEC_WORKING_TIMER_MS);
    
        if(workingTimerHandle == NULL) ESP_LOGE(TAG, "Failed to start working timer");
    }
}

/**
 * @brief Stops and deletes a timer.
 * @details This function stops the specified timer and sets its handle to NULL.
 * @param timerHandle Pointer to the timer handle to stop and delete.
 */
static void security_stop_timer(TimerHandle_t timerHandle)
{
    ao_fsm_timer_stop(timerHandle);
    timerHandle = NULL;
}

// Monitoring State Function Definition

ao_fsm_state_t security_monitoringState_intrusionDetected_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != INTRUSION_DETECTED_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_monitoringState_intrusionDetected_action");
        return SEC_MONITORING_STATE;
    }
    ESP_LOGI(TAG, "Intrusion detected! Transitioning to VALIDATION_STATE.");
    
    ESP_LOGI(TAG, "Tag read timer activated.");
    security_start_tagRead_timer(fsm);

    return SEC_VALIDATION_STATE;
}

ao_fsm_state_t security_monitoringState_panicButtonPressed_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    //TODO: Implement panic button handling

    return SEC_MONITORING_STATE;
}

ao_fsm_state_t security_monitoringState_turnLightsOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_MONITORING_STATE;
}


ao_fsm_state_t security_monitoringState_turnLightsOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_MONITORING_STATE;
}


ao_fsm_state_t security_monitoringState_turnSirenOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_MONITORING_STATE;
}

ao_fsm_state_t security_monitoringState_turnSirenOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_MONITORING_STATE;
}

// ---------------------------------------------------------------------------------------------------------

// Validation State Function Definition

ao_fsm_state_t security_validationState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_ALARM_STATE;
}

ao_fsm_state_t security_validationState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
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
    
    //TODO: Activate siren and lights.

    return SEC_ALARM_STATE;
}

// ---------------------------------------------------------------------------------------------------------

// Alarm State Function Definition

ao_fsm_state_t security_alarmState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_ALARM_STATE;
}

ao_fsm_state_t security_alarmState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_NORMAL_STATE;
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
    
    security_stop_timer(&workingTimerHandle);

    //TODO: Implement working timeout handling
    
    return SEC_NORMAL_STATE;
}

ao_fsm_state_t security_normalState_panicButtonPressed_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_NORMAL_STATE;
}

ao_fsm_state_t security_normalState_turnLightsOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_NORMAL_STATE;
}
ao_fsm_state_t security_normalState_turnLightsOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_NORMAL_STATE;
}
ao_fsm_state_t security_normalState_turnSirenOn_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_NORMAL_STATE;
}

ao_fsm_state_t security_normalState_turnSirenOff_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    return SEC_NORMAL_STATE;
}