#include "esp_log.h"

#include "ao_evt_mpool.h"
#include "ao_core.h"
#include "ao_fsm.h"

#ifndef CONFIG_AO_QUEUE_LEN
#define CONFIG_AO_QUEUE_LEN 8
#endif
#ifndef CONFIG_AO_POST_TIMEOUT_MS
#define CONFIG_AO_POST_TIMEOUT_MS 100
#endif
#define AO_QUEUE_LEN     (CONFIG_AO_QUEUE_LEN)
#define AO_EVT_POST_TO   pdMS_TO_TICKS(CONFIG_AO_POST_TIMEOUT_MS)

static const char *TAG = "ao_fsm";

/**
 * @brief Definition of the finite state machine (FSM) structure.
 * @details This structure represents a finite state machine associated with an active object.
 * It contains a pointer to the active object, the current state, and the state transition table.
 */
struct ao_fsm_s 
{
    ao_t* owner;
    const ao_fsm_transition_t* transitions;
    size_t transitions_count;
    ao_fsm_state_t current_state;
};

/**
 * @brief Definition of the FSM timer context structure.
 * @details This structure holds the context for the FSM timer, including a pointer to the FSM
 * and the event type to be posted when the timer expires.
 */
typedef struct
{
    ao_fsm_t *fsm;
    ao_fsm_evt_type_t event_type;
} ao_fsm_timer_t;

/**
 * @brief Internal event handler for the finite state machine (FSM).
 * @details This function is called by the active object when an event is posted to it.
 * It processes the event according to the current state and the state transition table,
 * executing the corresponding action handler and updating the current state.
 * @param evt_ctx The context of the event, which is a pointer to the FSM instance.
 * @param evt A pointer to the event being processed.
 */
static void ao_fsm_handler(evt_cntx_t evt_ctx, const ao_fsm_evt_t* evt) 
{
    size_t i;
    ao_fsm_t* fsm = (ao_fsm_t*)evt_ctx;
    if (!fsm || !fsm->owner || !fsm->transitions) return;

    ao_fsm_state_t next_state = fsm->current_state;
    
    for(i = 0; i < fsm->transitions_count; i++) 
    {
        if (fsm->transitions[i].state == fsm->current_state &&
            fsm->transitions[i].event_type == evt->type) 
        {
            if (fsm->transitions[i].action)
                next_state = fsm->transitions[i].action(fsm, evt);
            break;
        }
    }
    if(i == fsm->transitions_count)
    {
        ESP_LOGW(TAG, "No transition found for state %d and event %d", fsm->current_state, evt->type);
    }
    fsm->current_state = next_state;
}

ao_fsm_t* ao_fsm_create(const char* name, ao_fsm_state_t initial_state, const ao_fsm_transition_t* transitions, size_t transitions_count) 
{
    ao_fsm_t* fsm = (ao_fsm_t*)calloc(1, sizeof(ao_fsm_t));
    if (!fsm) return NULL;

    fsm->owner = ao_create(name, AO_QUEUE_LEN, (evt_cntx_t)fsm, ao_fsm_handler);
    if (!fsm->owner)
    {
        ESP_LOGE(TAG, "No se pudo crear AO");
        free(fsm);
        return NULL;
    }

    fsm->transitions = transitions;
    fsm->transitions_count = transitions_count;
    fsm->current_state = initial_state;

    return fsm;
}

esp_err_t ao_fsm_start(ao_fsm_t* fsm, UBaseType_t prio, uint32_t stack_words) 
{
    if (!fsm || !fsm->owner) return ESP_ERR_INVALID_ARG;
    return ao_start(fsm->owner, prio, stack_words);
}

esp_err_t ao_fsm_post(ao_fsm_t* fsm, ao_fsm_evt_type_t type, const void* payload, uint8_t len) 
{
    if (!fsm || !fsm->owner) return ESP_ERR_INVALID_ARG;
    esp_err_t err = ao_post(fsm->owner, type, payload, len, AO_EVT_POST_TO);
    if (err != ESP_OK)  
        ESP_LOGW(TAG, "No se pudo postear evento %d al FSM. err=%s (0x%x)", type, esp_err_to_name(err), err);
    return err;
}

void ao_fsm_destroy(ao_fsm_t* fsm) 
{
    if (!fsm) return;
    if (fsm->owner) ao_destroy(fsm->owner);
    free(fsm);
}

static void timer_cb(TimerHandle_t xTimer)
{
    ao_fsm_timer_t *ctx = (ao_fsm_timer_t*) pvTimerGetTimerID(xTimer);
    if (ctx && ctx->fsm) 
        ao_fsm_post(ctx->fsm, ctx->event_type, NULL, 0);
    ESP_LOGD(TAG, "Timer callback posting event %d", ctx ? ctx->event_type : -1);
    mpool_free(ctx);
}

TimerHandle_t ao_fsm_timer_start(ao_fsm_t* fsm, ao_fsm_evt_type_t event_type, uint32_t period_ms)
{
    if (!fsm || !fsm->owner) return NULL;

    ao_fsm_timer_t *ctx = (ao_fsm_timer_t*)mpool_alloc(sizeof(ao_fsm_timer_t));
    if (!ctx) return NULL;

    ctx->fsm = fsm;
    ctx->event_type = event_type;

    TimerHandle_t timer = xTimerCreate("fsm_timer", pdMS_TO_TICKS(period_ms), pdFALSE, (void*)ctx, timer_cb);
    if (!timer) 
    {
        free(ctx);
        return NULL;
    }

    if (xTimerStart(timer, 0) != pdPASS) 
    {
        xTimerDelete(timer, 0);
        free(ctx);
        return NULL;
    }

    return timer;
}

void ao_fsm_timer_stop(TimerHandle_t timer)
{
    if (!timer) return;

    ao_fsm_timer_t *ctx = (ao_fsm_timer_t*) pvTimerGetTimerID(timer);
    
    if (ctx) mpool_free(ctx);
    xTimerStop(timer, 0);
    xTimerDelete(timer, 0);
}