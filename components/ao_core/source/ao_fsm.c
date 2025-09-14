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

struct ao_fsm_s 
{
    ao_t* owner;
    const fsm_transition_t* transitions;
    size_t transitions_count;
    fsm_state_t state;
};

static void ao_fsm_handler(evt_cntx_t evt_ctx, const ao_evt_t* evt) 
{
    ao_fsm_t* fsm = (ao_fsm_t*)evt_ctx;
    if (!fsm || !fsm->owner || !fsm->transitions) return;

    fsm_state_t next_state = fsm->state;
    
    for(size_t i = 0; i < fsm->transitions_count; i++) 
    {
        if (fsm->transitions[i].state == fsm->state) 
        {
            if (fsm->transitions[i].action)
                next_state = fsm->transitions[i].action(fsm, evt);
            break;
        }
    }
    fsm->state = next_state;
}

ao_fsm_t* ao_fsm_create(const char* name, fsm_state_t initial_state, const fsm_transition_t* transitions, size_t transitions_count) 
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
    fsm->state = initial_state;

    return fsm;
}

esp_err_t ao_fsm_start(ao_fsm_t* fsm, UBaseType_t prio, uint32_t stack_words) 
{
    if (!fsm || !fsm->owner) return ESP_ERR_INVALID_ARG;
    return ao_start(fsm->owner, prio, stack_words);
}

esp_err_t ao_fsm_post(ao_fsm_t* fsm, uint8_t type, const void* payload, uint8_t len) 
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