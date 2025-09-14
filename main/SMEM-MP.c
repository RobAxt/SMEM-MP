#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ao_evt_mpool.h"
#include "ao_core.h"

static const char *TAG = "SMEM-MP";

enum { EVT_HELLO = 1, EVT_BYTES = 2 };

static void my_handler(evt_cntx_t evt_ctx, const ao_evt_t* evt) 
{
    ESP_LOGI(TAG, "Event context: %s", (char*)evt_ctx);

    switch (evt->type) 
    {
        case EVT_HELLO:
            ESP_LOGI(TAG, "EVT_HELLO (len=%u)", evt->len);
            break;
        case EVT_BYTES:
            ESP_LOGI(TAG, "EVT_BYTES: %u bytes. Ej: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                     evt->len,
                     evt->len >= 1 ? evt->data[0] : 0,
                     evt->len >= 2 ? evt->data[1] : 0,
                     evt->len >= 3 ? evt->data[2] : 0,
                     evt->len >= 4 ? evt->data[3] : 0,
                     evt->len >= 5 ? evt->data[4] : 0,
                     evt->len >= 6 ? evt->data[5] : 0,
                     evt->len >= 7 ? evt->data[6] : 0,
                     evt->len >= 8 ? evt->data[7] : 0,
                     evt->len >= 9 ? evt->data[8] : 0);
            break;
        default:
            ESP_LOGW(TAG, "Evento desconocido %u", evt->type);
            break;
    }
}

void app_main(void) 
{
    char eventName[] = "Event demo object name";

    ao_t* ao = ao_create("AO_DEMO", 8, (evt_cntx_t)eventName, my_handler);
    if (!ao)
    {
        ESP_LOGE(TAG, "No se pudo crear AO");
        return;
    }
    if (ao_start(ao, tskIDLE_PRIORITY + 2, 2048) != ESP_OK) 
    {
        ESP_LOGE(TAG, "No se pudo iniciar AO");
        ao_destroy(ao);
        return;
    }

    /* Postear algunos eventos */
    (void) ao_post(ao, EVT_HELLO, NULL, 0, pdMS_TO_TICKS(100));
    uint8_t bytes[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE };
    (void) ao_post(ao, EVT_BYTES, bytes, sizeof(bytes), pdMS_TO_TICKS(100));
    uint8_t oversize[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0xBA, 0xBE };
    esp_err_t err = ao_post(ao, EVT_BYTES, oversize, sizeof(oversize), pdMS_TO_TICKS(100));
    if( err != ESP_OK )
    {
        ESP_LOGW(TAG, "No se pudo postear evento oversize (esperado). err=%s (0x%x)",
             esp_err_to_name(err), err);

    }
    
    /* Mostrar estado del pool (opcional) */
    ESP_LOGI(TAG, "Pool: cap=%u libres=%u block=%uB overhead=%uB max_payload=%uB",
             (unsigned int)mpool_capacity(),
             (unsigned int)mpool_free_count(),
             (unsigned int)mpool_block_size(),
             (unsigned int)ao_evt_overhead(),
             (unsigned int)ao_evt_max_payload());

    /* Ejecución de ejemplo por un rato y terminar ordenado */
    vTaskDelay(pdMS_TO_TICKS(2000));
    ao_stop(ao);
    ao_destroy(ao);
    ESP_LOGI(TAG, "Fin demo");
}
