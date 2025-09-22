#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_err.h"

#include "security_ao_fsm.h"
#include "ao_core.h"
#include "ao_fsm.h"

#define TAG_SIZE 4
#define MAX_VALID_TAGS 3
#define SEC_TAGREAD_TIMER_MS 10000
#define SEC_WORKING_TIMER_MS 60000

static const char *TAG = "security_ao_fsm";

char valid_tags[MAX_VALID_TAGS][TAG_SIZE] = {
                                               { 0xFF, 0xFF, 0xFF, 0xFF },
                                               { 0xEA, 0xEE, 0x85, 0x6A },
                                               { 0x40, 0x8B, 0xE6, 0x30 }
                                            };

typedef struct 
{
    ao_fsm_t *fsm;
    ao_fsm_evt_type_t event_type;
} security_timer_t;

static security_timer_t tagRead_timer = {0};
static security_timer_t working_timer = {0};

static TimerHandle_t tagReadTimerHandle = NULL;
static TimerHandle_t workingTimerHandle = NULL;

static void timer_cb(TimerHandle_t xTimer)
{
    security_timer_t *ctx = (security_timer_t*) pvTimerGetTimerID(xTimer);
    if (ctx && ctx->fsm) 
        ao_fsm_post(ctx->fsm, ctx->event_type, NULL, 0);
}

static void security_start_tagRead_timer(ao_fsm_t *fsm)
{
    if(fsm == NULL) 
    {
        ESP_LOGE(TAG, "Invalid FSM pointer in security_start_tagRead_timer");
        return;
    }

    tagRead_timer.fsm = fsm;
    tagRead_timer.event_type = READ_TAG_TIMEOUT_EVENT;

    tagReadTimerHandle = xTimerCreate("TagReadTimer", pdMS_TO_TICKS(SEC_TAGREAD_TIMER_MS), pdFALSE, (void*)&tagRead_timer, timer_cb);

    if(tagReadTimerHandle != NULL)
    {
        if(xTimerStart(tagReadTimerHandle, 0) == pdPASS)
            ESP_LOGI(TAG, "Tag Read Timer started for 10 seconds."); 
    }
    else 
    {
        ESP_LOGE(TAG, "Failed to create Tag Read Timer");
    }
}

static void security_start_working_timer(ao_fsm_t *fsm)
{
    if(fsm == NULL) 
    {
        ESP_LOGE(TAG, "Invalid FSM pointer in security_start_working_timer");
        return;
    }

    working_timer.fsm = fsm;
    working_timer.event_type = WORKING_TIMEOUT_EVENT;

    workingTimerHandle = xTimerCreate("WorkingTimer", pdMS_TO_TICKS(SEC_WORKING_TIMER_MS), pdFALSE, (void*)&working_timer, timer_cb);

    if(workingTimerHandle != NULL) 
    {
        if(xTimerStart(workingTimerHandle, 0) == pdPASS)
            ESP_LOGI(TAG, "Working Timer started for 60 seconds."); 
    }
    else 
    {
        ESP_LOGE(TAG, "Failed to create Working Timer");
    }
}

static void security_stop_timer(TimerHandle_t *timerHandle)
{
    if(timerHandle != NULL && *timerHandle != NULL) 
    {
        if(xTimerStop(*timerHandle, 0) == pdPASS) 
        {
            ESP_LOGI(TAG, "Timer stopped successfully.");
        } 
        else 
        {
            ESP_LOGE(TAG, "Failed to stop timer.");
        }
        if(xTimerDelete(*timerHandle, 0) == pdPASS) 
        {
            ESP_LOGI(TAG, "Timer deleted successfully.");
        } 
        else 
        {
            ESP_LOGE(TAG, "Failed to delete timer.");
        }
        *timerHandle = NULL;
    }
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

ao_fsm_state_t security_validationState_tagReadEvent_action(ao_fsm_t *fsm, const ao_evt_t *event)
{
    if(fsm == NULL || event == NULL || event->type != TAG_READ_EVENT || event->len != TAG_SIZE) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_validationState_tagReadEvent_action");
        return SEC_VALIDATION_STATE;
    }

    ESP_LOGI(TAG, "Tag read event received. Validating tag: %02X%02X%02X%02X", event->data[0], event->data[1], event->data[2], event->data[3]);
    
    for(int i = 0; i < MAX_VALID_TAGS; i++) 
    {
        if(memcmp(event->data, valid_tags[i], TAG_SIZE) == 0) 
        {
            ESP_LOGI(TAG, "Valid tag detected.");
            security_stop_timer(&tagReadTimerHandle);
            ao_fsm_post(fsm, VALID_TAG_EVENT, NULL, 0);
            return SEC_VALIDATION_STATE;
        }
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
    
    for(int i = 0; i < MAX_VALID_TAGS; i++) 
    {
        if(memcmp(event->data, valid_tags[i], TAG_SIZE) == 0) 
        {
            ESP_LOGI(TAG, "Valid tag detected in ALARM_STATE.");
            ao_fsm_post(fsm, VALID_TAG_EVENT, NULL, 0);
            return SEC_VALIDATION_STATE;
        }
    }
    ESP_LOGW(TAG, "Invalid tag detected in ALARM_STATE. Remaining in ALARM_STATE.");
    return SEC_ALARM_STATE;
}