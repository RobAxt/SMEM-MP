
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "ao_core.h"
#include "ao_fsm.h"
#include "ao_fsm_watcher.h"
#include "ao_evt_mpool.h"

/*
 * Propósito de este main:
 * - Mostrar el uso "público" completo del componente ao_fsm y piezas relacionadas.
 *   API cubierta: ao_fsm_create/start/post/destroy, ao_fsm_timer_start/stop,
 *   ao_fsm_watcher_start/add_callback/stop.
 * - FSM de ejemplo: Luz ON/OFF con eventos TICK y SENSOR.
 */

static const char *TAG = "SMEM-MP";

// ----------------- Definiciones de estados y eventos -----------------

typedef enum {
    OFF_STATE = 0,
    ON_STATE  = 1,
} demo_state_t;

typedef enum {
    TURN_ON_EVT  = 1,
    TURN_OFF_EVT = 2,
    TICK_EVT     = 3,
    SENSOR_EVT   = 4,
} demo_evt_t;

// ----------------- Prototipos de handlers de acción ------------------

static ao_fsm_state_t fsm_onState_onEvt_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt);
static ao_fsm_state_t fsm_offState_onEvt_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt);
static ao_fsm_state_t fsm_anyState_onTick_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt);
static ao_fsm_state_t fsm_anyState_onSensor_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt);

// ----------------- Tabla de transiciones -----------------------------

static const ao_fsm_transition_t g_transitions[] = {
    // OFF -> TURN_ON -> ON
    { OFF_STATE, TURN_ON_EVT,  fsm_offState_onEvt_action },
    // ON -> TURN_OFF -> OFF
    { ON_STATE,  TURN_OFF_EVT, fsm_onState_onEvt_action  },
    // Any -> TICK -> stay
    { OFF_STATE, TICK_EVT,     fsm_anyState_onTick_action },
    { ON_STATE,  TICK_EVT,     fsm_anyState_onTick_action },
    // Any -> SENSOR -> stay
    { OFF_STATE, SENSOR_EVT,   fsm_anyState_onSensor_action },
    { ON_STATE,  SENSOR_EVT,   fsm_anyState_onSensor_action  },
};

// ----------------- funcion auxiliar  ---------------------------------

void print_pool_status(void) 
{
    ESP_LOGI(TAG, "Pool: cap=%u libres=%u block=%uB overhead=%uB max_payload=%uB",
             (unsigned int)mpool_capacity(),
             (unsigned int)mpool_free_count(),
             (unsigned int)mpool_block_size(),
             (unsigned int)ao_evt_overhead(),
             (unsigned int)ao_evt_max_payload());
}

// --------------- Implementación de handlers de acción ----------------

static ao_fsm_state_t fsm_onState_onEvt_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt)
{
    if (evt->type != TURN_OFF_EVT) {
        ESP_LOGD(TAG, "Evento inválido en ON_STATE (%u)", (unsigned)evt->type);
        return ON_STATE;
    }
    ESP_LOGI(TAG, "Apagando luz desde ON_STATE");
    return OFF_STATE;
}

static ao_fsm_state_t fsm_offState_onEvt_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt)
{
    if (evt->type != TURN_ON_EVT) {
        ESP_LOGD(TAG, "Evento inválido en OFF_STATE (%u)", (unsigned)evt->type);
        return OFF_STATE;
    }
    ESP_LOGI(TAG, "Encendiendo luz desde OFF_STATE");
    return ON_STATE;
}

static ao_fsm_state_t fsm_anyState_onTick_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt)
{
    ESP_LOGI(TAG, "TICK_EVT: ejecutando latido ");
    return ON_STATE;
}

static ao_fsm_state_t fsm_anyState_onSensor_action(ao_fsm_t* fsm, const ao_fsm_evt_t* evt)
{
    // evt->data contiene bytes arbitrarios — en este demo interpretamos un uint16
    uint32_t sample = 0;
    if (evt->len <= sizeof(sample)) {
        memcpy(&sample, evt->data, evt->len);
    }
    ESP_LOGI(TAG, "SENSOR_EVT: lectura=0x%08" PRIx32, sample);
    return ON_STATE;
}
// ----------------- Watcher: callbacks de ejemplo ---------------------

static void watcher_cb_sensor_fast(ao_fsm_t* fsm)
{
    // Simular lectura “rápida”: postea un valor chico
    uint32_t val = 0x98765432;
    (void)ao_fsm_post(fsm, SENSOR_EVT, &val, sizeof(val));
    vTaskDelay(pdMS_TO_TICKS(600));
}

static void watcher_cb_sensor_slow(ao_fsm_t* fsm)
{
    // Simular lectura “lenta”: postea un valor mayor
    uint16_t val = 0x1234;
    (void)ao_fsm_post(fsm, SENSOR_EVT, &val, sizeof(val));
    vTaskDelay(pdMS_TO_TICKS(500));
}

// ----------------- app_main: demo integral ---------------------------

void app_main(void)
{
    ESP_LOGI(TAG, "Demo ao_fsm: inicio");
    
    // Estado inicial del pool de eventos
    print_pool_status();
    
    // 1) Crear la FSM (cubre ao_fsm_create)
    ao_fsm_t* fsm = ao_fsm_create("fsm_demo", OFF_STATE, g_transitions, sizeof(g_transitions)/sizeof(g_transitions[0]));
    if (!fsm) {
        ESP_LOGE(TAG, "No se pudo crear la FSM");
        return;
    }

    // 2) Arrancar la FSM (cubre ao_fsm_start)
    if (ao_fsm_start(fsm, tskIDLE_PRIORITY + 2, 4 * 1024 / sizeof(StackType_t)) != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo iniciar la FSM");
        ao_fsm_destroy(fsm);
        return;
    }

    // 3) Postear eventos manuales (cubre ao_fsm_post)
    ESP_LOGI(TAG, "Publicando TURN_ON_EVT");
    (void)ao_fsm_post(fsm, TURN_ON_EVT, NULL, 0);

    // 4) Timer periódico generando TICK_EVT (cubre ao_fsm_timer_start/stop)
    ESP_LOGI(TAG, "Iniciando timer de TICK en 500 ms");
    TimerHandle_t tick_timer = ao_fsm_timer_start(fsm, TICK_EVT, 500);

    // 5) Watcher para callbacks de sensores (cubre watcher_start/add_callback/stop)
    ESP_LOGI(TAG, "Iniciando watcher cada 1000 ms con dos callbacks");
    ao_fsm_watcher_t* watcher = ao_fsm_watcher_start(fsm, 1000);
    if (watcher) {
        (void)ao_fsm_watcher_add_callback(watcher, watcher_cb_sensor_fast);
        (void)ao_fsm_watcher_add_callback(watcher, watcher_cb_sensor_slow);
    } else {
        ESP_LOGW(TAG, "No se inició watcher; se continúa sin él");
    }

    // 6) Mantener la demo corriendo un rato
    vTaskDelay(pdMS_TO_TICKS(4000));

    // 7) Forzar apagado
    ESP_LOGI(TAG, "Publicando TURN_OFF_EVT");
    (void)ao_fsm_post(fsm, TURN_OFF_EVT, NULL, 0);

    // 8) Parar utilitarios
    if (tick_timer) {
        ESP_LOGI(TAG, "Deteniendo timer de TICK");
        ao_fsm_timer_stop(tick_timer);
    }
    if (watcher) {
        ESP_LOGI(TAG, "Deteniendo watcher");
        ao_fsm_watcher_stop(watcher);
    }

    // 9) Destruir FSM (cubre ao_fsm_destroy); a efectos de demo, esperar un poco
    ESP_LOGI(TAG, "Destruyendo FSM y finalizando demo");
    ao_fsm_destroy(fsm);
}