#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "ao_fsm_watcher.h"
#include "ao_fsm.h"

#ifndef AO_FSM_WATCHER_CBS
#define AO_FSM_WATCHER_CBS CONFIG_AO_FSM_WATCHER_CBS
#endif

static const char *TAG = "ao_fsm_watcher";

/**
 * @brief Definition of the FSM watcher structure.
 * @details This structure represents a watcher that holds the callbacks to be called periodically
 * for a specific FSM.
 */
struct ao_fsm_watcher_s 
{
    ao_fsm_t* fsm;
    ao_fsm_watcher_cb_t callbacksList [AO_FSM_WATCHER_CBS];
    uint8_t callbacksCount;
    uint32_t intervalMs;
    SemaphoreHandle_t mtx;
    volatile bool running;
};

/**
 * @brief Internal function to lock the watcher mutex.
 * @details This function locks the mutex of the FSM watcher to ensure thread-safe access
 * to its internal data structures.
 * @param w A pointer to the FSM watcher to lock.
 */
static inline void lock(ao_fsm_watcher_t *w)   { xSemaphoreTake(w->mtx, portMAX_DELAY); }
static inline void unlock(ao_fsm_watcher_t *w) { xSemaphoreGive(w->mtx); }

/**
 * @brief The main task function for the FSM watcher.
 * @details This function runs in a separate FreeRTOS task and periodically calls the registered
 * callbacks to read sensors and post events to the FSM.
 * @param arg A pointer to the FSM watcher instance.
 */
static void watcher_task(void *arg)
{
    ao_fsm_watcher_t *watcher = (ao_fsm_watcher_t*)arg;
    configASSERT(watcher && watcher->fsm);

    while (watcher->running) 
    {
        TickType_t start = xTaskGetTickCount();
        
        lock(watcher);
        for (int i = 0; i < watcher->callbacksCount; i++) 
        {
            if (watcher->callbacksList[i] != NULL) 
                watcher->callbacksList[i](watcher->fsm);
        }
        unlock(watcher);


        TickType_t elapsed = xTaskGetTickCount() - start;
        if (pdTICKS_TO_MS(elapsed) < watcher->intervalMs)
            vTaskDelay(pdMS_TO_TICKS(watcher->intervalMs - pdTICKS_TO_MS(elapsed)));
        else 
            ESP_LOGW(TAG, "Overrun: ciclo tardo %u ms (> periodo %u ms)", 
                    (unsigned)pdTICKS_TO_MS(elapsed),(unsigned)watcher->intervalMs);
    }

}

ao_fsm_watcher_t* ao_fsm_watcher_start(ao_fsm_t* fsm, uint32_t interval_ms) 
{
    if (!fsm) return NULL;

    ao_fsm_watcher_t* watcher = (ao_fsm_watcher_t*)calloc(1, sizeof(ao_fsm_watcher_t));
    if (!watcher) return NULL;

    watcher->fsm = fsm;
    watcher->callbacksCount = 0;
    watcher->intervalMs = interval_ms;
    watcher->running = true;

    watcher->mtx = xSemaphoreCreateMutex();
    if (!watcher->mtx) 
    {
        free(watcher);
        ESP_LOGE(TAG, "Failed to create mutex");
        return NULL;
    }

    BaseType_t ok = xTaskCreate(watcher_task, TAG, 2048, (void*) watcher, tskIDLE_PRIORITY + 1, NULL);

    if (ok != pdPASS)
    {
        vSemaphoreDelete(watcher->mtx);
        free(watcher);
        ESP_LOGE(TAG, "Failed to create watcher task");
        return NULL;
    }

    return watcher;
}

void ao_fsm_watcher_stop(ao_fsm_watcher_t* watcher)
{
    if (watcher)
    {
        watcher->running = false;       
        vTaskDelay(pdMS_TO_TICKS(watcher->intervalMs));
        vSemaphoreDelete(watcher->mtx);
        free(watcher);
    }
}

esp_err_t ao_fsm_watcher_add_callback(ao_fsm_watcher_t* watcher, ao_fsm_watcher_cb_t cb)
{
    if (!watcher || !cb) return ESP_ERR_INVALID_ARG;

    lock(watcher);
    for (int i = 0; i < AO_FSM_WATCHER_CBS; ++i) {
        if (watcher->callbacksList[i] == NULL) 
        {
            watcher->callbacksCount++;
            watcher->callbacksList[i] = cb;
            unlock(watcher);
            return ESP_OK;
        }
    }
    unlock(watcher);

    ESP_LOGW(TAG, "No space to add new callback");
    return ESP_ERR_NO_MEM; // No space for new callback
}