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

#define TAG_SIZE 4
#define MAX_VALID_TAGS 3
#define SEC_TAGREAD_TIMER_MS 10000
#define SEC_WORKING_TIMER_MS 60000

static const char *TAG = "security_ao_fsm";

uint8_t valid_tags[MAX_VALID_TAGS][TAG_SIZE] = {
    { 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xEA, 0xEE, 0x85, 0x6A },
    { 0x40, 0x8B, 0xE6, 0x30 }
};

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

/**
 * @brief Validates a tag against the list of valid tags.
 * @details This function checks if the provided tag matches any of the valid tags.
 * @param tag Pointer to the tag data to validate.
 * @param len Length of the tag data. Must be equal to TAG_SIZE.
 * @return true if the tag is valid, false otherwise.
 */
static bool security_tagValidation(uint8_t* tag, size_t len)
{
    if(tag == NULL || len != TAG_SIZE) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_tagValidation");
        return false;
    }

    for(int i = 0; i < MAX_VALID_TAGS; i++) 
        if(memcmp(tag, valid_tags[i], TAG_SIZE) == 0) 
            return true;

    return false;
}

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

ao_fsm_state_t security_validationState_tagReadEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TAG_READ_EVENT || event->len != TAG_SIZE) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_validationState_tagReadEvent_action");
        return SEC_VALIDATION_STATE;
    }

    ESP_LOGI(TAG, "Tag read event received. Validating tag: %02X%02X%02X%02X", event->data[0], event->data[1], event->data[2], event->data[3]);
    
    if(security_tagValidation(event->data, TAG_SIZE)) 
    {
        ESP_LOGI(TAG, "Valid tag detected.");
        security_stop_timer(&tagReadTimerHandle);
        ao_fsm_post(fsm, VALID_TAG_EVENT, NULL, 0);
        return SEC_VALIDATION_STATE;
    }    
    
    ESP_LOGW(TAG, "Invalid tag detected.");
    security_stop_timer(&tagReadTimerHandle);
    ao_fsm_post(fsm, INVALID_TAG_EVENT, NULL, 0);
    return SEC_VALIDATION_STATE;
}

ao_fsm_state_t security_validationState_validTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != VALID_TAG_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_validationState_validTagEvent_action");
        return SEC_VALIDATION_STATE;
    }
    ESP_LOGI(TAG, "Valid Tag... Transitioning to NORMAL_STATE.");
    
    ESP_LOGI(TAG, "Working timer activated.");
    security_start_working_timer(fsm);
    
    //TODO: turn off siren and lights if they were active.
    
    return SEC_NORMAL_STATE;
}

ao_fsm_state_t security_validationState_invalidTagEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != INVALID_TAG_EVENT) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_validationState_invalidTagEvent_action");
        return SEC_VALIDATION_STATE;
    }
    ESP_LOGI(TAG, "Invalid Tag... Transitioning to ALARM_STATE.");
    
    //TODO: Activate siren and lights.
    
    return SEC_ALARM_STATE;
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

ao_fsm_state_t security_alarmState_tagReadEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TAG_READ_EVENT || event->len != TAG_SIZE) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_alarmState_tagReadEvent_action");
        return SEC_ALARM_STATE;
    }
    ESP_LOGI(TAG, "Tag read event received in ALARM_STATE. Validating tag: %02X%02X%02X%02X", event->data[0], event->data[1], event->data[2], event->data[3]);
    
    if(security_tagValidation(event->data, TAG_SIZE)) 
    {
        ESP_LOGI(TAG, "Valid tag detected in ALARM_STATE.");
        ao_fsm_post(fsm, VALID_TAG_EVENT, NULL, 0);
        return SEC_VALIDATION_STATE;
    }

    ESP_LOGW(TAG, "Invalid tag detected in ALARM_STATE. Remaining in ALARM_STATE.");
    return SEC_ALARM_STATE;
}

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