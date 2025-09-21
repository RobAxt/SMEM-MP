#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ao_evt_mpool.h"
#include "ao_core.h"
#include "ao_fsm.h"

static const char *TAG = "SMEM-MP";

// Definición de eventos y estados
enum { TURN_ON_EVT = 0, TURN_DIMMER_EVT = 1, TURN_OFF_EVT = 2 };
enum { ON_STATE = 0, DIMMER_STATE = 1, OFF_STATE = 2 };

// Prototipos de acciones
ao_fsm_state_t fsm_onState_dimmerEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt);
ao_fsm_state_t fsm_onState_offEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt);
ao_fsm_state_t fsm_dimmerState_onEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt);
ao_fsm_state_t fsm_dimmerState_dimmerEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt);
ao_fsm_state_t fsm_dimmerState_offEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt);
ao_fsm_state_t fsm_offState_onEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt);

// Definición de transiciones
ao_fsm_transition_t transitions[] = {
    { ON_STATE,     TURN_DIMMER_EVT,    fsm_onState_dimmerEvt_action },
    { ON_STATE,     TURN_OFF_EVT,       fsm_onState_offEvt_action },
    { DIMMER_STATE, TURN_ON_EVT,        fsm_dimmerState_onEvt_action },
    { DIMMER_STATE, TURN_DIMMER_EVT,    fsm_dimmerState_dimmerEvt_action },
    { DIMMER_STATE, TURN_OFF_EVT,       fsm_dimmerState_offEvt_action },
    { OFF_STATE,    TURN_ON_EVT,        fsm_offState_onEvt_action }
};

void print_pool_status(void) 
{
    ESP_LOGI(TAG, "Pool: cap=%u libres=%u block=%uB overhead=%uB max_payload=%uB",
             (unsigned int)mpool_capacity(),
             (unsigned int)mpool_free_count(),
             (unsigned int)mpool_block_size(),
             (unsigned int)ao_evt_overhead(),
             (unsigned int)ao_evt_max_payload());
}

void app_main(void) 
{
    esp_log_level_set("ao_core", ESP_LOG_DEBUG);

    char demoName[] = "Switch FSM Demo";

    ESP_LOGI(TAG, "Iniciando demo: %s", demoName);
    
    ao_fsm_t* fsm = ao_fsm_create(demoName, OFF_STATE, transitions, sizeof(transitions)/sizeof(ao_fsm_transition_t));
    
    if (!fsm)
    {
        ESP_LOGE(TAG, "No se pudo crear FSM");
        return;
    }

    esp_err_t err = ao_fsm_start(fsm, tskIDLE_PRIORITY + 1, 2048);
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "No se pudo iniciar FSM. err=%s (0x%x)", esp_err_to_name(err), err);
        return;
    }

    // Enviar eventos de prueba
    ao_fsm_post(fsm, TURN_OFF_EVT, NULL, 0); // transicion invalida
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    ao_fsm_post(fsm, TURN_ON_EVT, NULL, 0);
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    ao_fsm_post(fsm, TURN_ON_EVT, NULL, 0); // transicion invalida
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    uint32_t dimmer_level = 65525; // Ejemplo de nivel de dimmer
    ao_fsm_post(fsm, TURN_DIMMER_EVT, &dimmer_level, sizeof(dimmer_level));
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    dimmer_level = 305419896; // Cambiar nivel de dimmer
    ao_fsm_post(fsm, TURN_DIMMER_EVT, &dimmer_level, sizeof(dimmer_level));
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    ao_fsm_post(fsm, TURN_OFF_EVT, NULL, 0);  
    print_pool_status();   
    vTaskDelay(pdMS_TO_TICKS(500));

    ao_fsm_post(fsm, TURN_DIMMER_EVT, NULL, 0);  // transicion invalida
    print_pool_status();   
    vTaskDelay(pdMS_TO_TICKS(2000));

    print_pool_status();
    ao_fsm_destroy(fsm);
    ESP_LOGI(TAG, "Fin demo");
}

// Implementación de acciones

ao_fsm_state_t fsm_onState_dimmerEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt)
{   
    if(evt->type != TURN_DIMMER_EVT)
    {
        ESP_LOGD(TAG, "Evento invalido en ON_STATE");
        return ON_STATE;
    }

    if (evt->len == sizeof(uint32_t)) 
    {
        char level[4]; memcpy(level, evt->data, sizeof(level)); 
        ESP_LOGI(TAG, "Ajustando dimmer a nivel %lu desde ON_STATE", (unsigned long)*(uint32_t*)level);
    } 
    else 
    {
        ESP_LOGW(TAG, "Evento TURN_DIMMER_EVT recibido con payload inválido");
    }

    return DIMMER_STATE;
}

ao_fsm_state_t fsm_onState_offEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt)
{
    if(evt->type != TURN_OFF_EVT)
    {
        ESP_LOGD(TAG, "Evento invalido en ON_STATE");
        return ON_STATE;
    }

    ESP_LOGI(TAG, "Apagando luz desde ON_STATE");
    return OFF_STATE;
}

ao_fsm_state_t fsm_dimmerState_onEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt)
{
    if(evt->type != TURN_ON_EVT)
    {
        ESP_LOGD(TAG, "Evento invalido en DIMMER_STATE");
        return DIMMER_STATE;
    }

    ESP_LOGI(TAG, "Luz al 100%% desde DIMMER_STATE");
    return ON_STATE;
}

ao_fsm_state_t fsm_dimmerState_dimmerEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt)
{
    if(evt->type != TURN_DIMMER_EVT)
    {
        ESP_LOGD(TAG, "Evento invalido en DIMMER_STATE");
        return DIMMER_STATE;
    }

    if (evt->len == sizeof(uint32_t)) 
    {
        uint32_t level = *((uint32_t *) evt->data);
        ESP_LOGI(TAG, "Ajustando dimmer a nivel %lu desde DIMMER_STATE", (long unsigned int)level);
    } 
    else 
    {
        ESP_LOGW(TAG, "Evento TURN_DIMMER_EVT recibido con payload inválido");
    }

    return DIMMER_STATE;
}

ao_fsm_state_t fsm_dimmerState_offEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt)
{
    if(evt->type != TURN_OFF_EVT)
    {
        ESP_LOGD(TAG, "Evento invalido en DIMMER_STATE");
        return DIMMER_STATE;
    }

    ESP_LOGI(TAG, "Apagando luz desde DIMMER_STATE");
    return OFF_STATE;
}

ao_fsm_state_t fsm_offState_onEvt_action(ao_fsm_t* fsm, const ao_evt_t* evt)
{
    if(evt->type != TURN_ON_EVT)
    {
        ESP_LOGD(TAG, "Evento invalido en OFF_STATE");
        return OFF_STATE;
    }

    ESP_LOGI(TAG, "Encendiendo luz desde OFF_STATE");
    return ON_STATE;
}
// Fin de las acciones