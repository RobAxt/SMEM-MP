#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "ao_evt_mpool.h"
#include "ao_core.h"

static const char* TAG = "ao_core";

/**
 * @brief Definition of the active object structure.
 * @details This structure represents an active object, which includes its event queue,
 * task handle, event handler function, name, and running state.
 */
struct ao_s 
{
    QueueHandle_t q;
    TaskHandle_t  th;
    ao_handler_t  on_event;
    evt_cntx_t    on_event_context;
    char          name[16];
    bool          running;
};

static void ao_task(void* arg)
{
    ao_t* self = (ao_t*)arg;
    evt_cntx_t evt_ctx = self->on_event_context;
    ao_evt_t* evt = NULL;

    while (self->running) 
    {
        if (xQueueReceive(self->q, &evt, pdMS_TO_TICKS(250)) == pdTRUE) 
        {
            if (self->on_event) 
                self->on_event(evt_ctx, evt);
            mpool_free(evt);
            evt = NULL;

            ESP_LOGD(TAG, "Cola[%s]: ocupados=%u libres=%u", self->name,
                     (unsigned)uxQueueMessagesWaiting(self->q),
                     (unsigned)uxQueueSpacesAvailable(self->q));
        }
    }

    while (xQueueReceive(self->q, &evt, 0) == pdTRUE) 
        mpool_free(evt);

    vTaskDelete(NULL);
}

ao_t* ao_create(const char* name, size_t queue_len, evt_cntx_t evt_cntx, ao_handler_t handler) 
{
    mpool_start(); // idempotente

    ao_t* self = (ao_t*)calloc(1, sizeof(ao_t));
    if (!self) return NULL;

    self->q = xQueueCreate(queue_len, sizeof(ao_evt_t*));
    if (!self->q)
    {
        free(self); 
        return NULL; 
    }

    self->on_event_context = evt_cntx;
    self->on_event = handler;
    self->running = false;
 
    if (name) 
    {
        strncpy(self->name, name, sizeof(self->name)-1);
        self->name[sizeof(self->name)-1] = '\0';
    } 
    else 
    {
        strcpy(self->name, "AO");
    }
 
    return self;
}

esp_err_t ao_start(ao_t* self, UBaseType_t prio, uint32_t stack_words) 
{
    if (!self || self->running) 
        return ESP_ERR_INVALID_STATE;
    
    self->running = true;
    
    BaseType_t ok = xTaskCreate(ao_task, self->name, stack_words, self, prio, &self->th);
    
    if (ok != pdPASS) {
        self->running = false;
        return ESP_FAIL;
    }
 
    return ESP_OK;
}

esp_err_t ao_post(ao_t* self, uint8_t type, const void* payload, uint8_t len, TickType_t to_ticks)
{
    if (!self || !self->q) return ESP_ERR_INVALID_ARG;

    if (len > ao_evt_max_payload()) return ESP_ERR_NO_MEM;

    ao_evt_t* evt = (ao_evt_t*)mpool_alloc(sizeof(ao_evt_t) + len);
    if (!evt) return ESP_ERR_NO_MEM;

    evt->type = type;
    evt->len  = len;
    if (payload && len)
        memcpy(evt->data, payload, len);

    if (xQueueSend(self->q, &evt, to_ticks) != pdTRUE)
    {
        mpool_free(evt);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

void ao_stop(ao_t* self) 
{
    if (!self || !self->running) return;
    self->running = false;

    while (eTaskGetState(self->th) != eDeleted) 
        vTaskDelay(pdMS_TO_TICKS(10));

    self->th = NULL;
}

void ao_destroy(ao_t* self) 
{
    if (!self) return;
    ao_stop(self);
    if (self->q) vQueueDelete(self->q);
    free(self);
}

size_t ao_evt_overhead(void) 
{ 
    return sizeof(ao_evt_t); 
}

size_t ao_evt_max_payload(void) 
{
    size_t bs = mpool_block_size();
    return (bs > sizeof(ao_evt_t)) ? (bs - sizeof(ao_evt_t)) : 0;
}