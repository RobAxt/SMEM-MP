#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ao_evt_mpool.h"
#include "ao_core.h"
#include "ao_fsm.h"

static const char *TAG = "SMEM-MP";

// Definición de eventos y estados
enum { TURN_ON_EVT = 0, TURN_DIMMER_EVT, TURN_OFF_EVT };
enum { ON_STATE = 0, DIMMER_STATE, OFF_STATE };

// Prototipos de acciones
ao_fsm_state_t fsm_onState_action(ao_fsm_t* fsm, const ao_evt_t* evt);
ao_fsm_state_t fsm_dimmerState_action(ao_fsm_t* fsm, const ao_evt_t* evt);
ao_fsm_state_t fsm_offState_action(ao_fsm_t* fsm, const ao_evt_t* evt);

// Definición de transiciones
ao_fsm_transition_t transitions[] = {
    { ON_STATE,     fsm_onState_action },
    { DIMMER_STATE, fsm_dimmerState_action },
    { OFF_STATE,    fsm_offState_action }
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
    ao_fsm_post(fsm, TURN_ON_EVT, NULL, 0);
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    uint8_t dimmer_level = 5; // Ejemplo de nivel de dimmer
    ao_fsm_post(fsm, TURN_DIMMER_EVT, &dimmer_level, sizeof(dimmer_level));
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    dimmer_level = 10; // Cambiar nivel de dimmer
    ao_fsm_post(fsm, TURN_DIMMER_EVT, &dimmer_level, sizeof(dimmer_level));
    print_pool_status();
    vTaskDelay(pdMS_TO_TICKS(500));

    ao_fsm_post(fsm, TURN_OFF_EVT, NULL, 0);  
    print_pool_status();   
    vTaskDelay(pdMS_TO_TICKS(2000));
    print_pool_status();
    ao_fsm_destroy(fsm);
    ESP_LOGI(TAG, "Fin demo");
}


ao_fsm_state_t fsm_onState_action(ao_fsm_t* fsm, const ao_evt_t* evt) 
{
   ESP_LOGI(TAG, "Estado ON: Recibido evento %d", evt->type);
   switch (evt->type) 
   {
        case TURN_DIMMER_EVT:
            if (evt->len == sizeof(uint8_t)) 
            {
                uint8_t level = evt->data[0];
                ESP_LOGI(TAG, "Ajustando dimmer al nivel %d", level);
                return DIMMER_STATE; 
            }
            break;
        case TURN_OFF_EVT:
            ESP_LOGI(TAG, "Apagando");
            return OFF_STATE;
        default:
            ESP_LOGW(TAG, "Evento desconocido en estado ON");
            break;
    }
    return ON_STATE;
}

ao_fsm_state_t fsm_dimmerState_action(ao_fsm_t* fsm, const ao_evt_t* evt) 
{
    ESP_LOGI(TAG, "Estado DIMMER: Recibido evento %d", evt->type);
    switch (evt->type) 
    {
        case TURN_DIMMER_EVT:
            if (evt->len == sizeof(uint8_t)) 
            {
                uint8_t level = evt->data[0];
                ESP_LOGI(TAG, "Ajustando dimmer al nivel %d", level);
                return DIMMER_STATE; 
            }
            break;
        case TURN_OFF_EVT:
            ESP_LOGI(TAG, "Apagando");
            return OFF_STATE;
        case TURN_ON_EVT:
            ESP_LOGI(TAG, "Encendiendo");
            return ON_STATE;
        default:
            ESP_LOGW(TAG, "Evento desconocido en estado DIMMER");
            break;
    }
    return DIMMER_STATE;
}

ao_fsm_state_t fsm_offState_action(ao_fsm_t* fsm, const ao_evt_t* evt) 
{
    ESP_LOGI(TAG, "Estado OFF: Recibido evento %d", evt->type);
    switch (evt->type) 
    {
        case TURN_ON_EVT:
            ESP_LOGI(TAG, "Encendiendo");
            return ON_STATE;
        default:
            ESP_LOGW(TAG, "Evento desconocido en estado OFF");
            break;
    }   
    return OFF_STATE;    
}