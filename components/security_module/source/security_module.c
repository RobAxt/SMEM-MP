#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "security_module.h"
#include "security_ao_fsm.h"
#include "ao_core.h"
#include "security_watcher.h"

static const char *TAG = "security_module";

/**
 * @brief Security AO FSM pointer
 * @details This pointer holds the instance of the security AO FSM.
 * It is initialized in the security_fsm_start function and used throughout
 * the security module to manage state transitions and handle events.
 */
static ao_fsm_t* security_fsm = NULL;

/**
 * @brief Starts the security AO FSM.
 * @details This function initializes and starts the security AO FSM.
 * It sets the initial state and configures the necessary parameters for operation.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t security_fsm_start(void);

/**
 * @brief Starts the watchers for the security module.
 * @details This function initializes and starts any necessary watchers
 * for the security module, such as monitoring sensors or other inputs.
 * @param security_fsm Pointer to the security AO FSM instance.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t security_watchers_start(ao_fsm_t* security_fsm);


esp_err_t security_module_start(void)
{
    esp_err_t err = ESP_OK;

    err = security_fsm_start();
    
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "No se pudo iniciar el módulo de seguridad. err=%s (0x%x)", esp_err_to_name(err), err);
        return err;
    }

    err = security_watchers_start(security_fsm);

    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "No se pudo iniciar los watchers del módulo de seguridad. err=%s (0x%x)", esp_err_to_name(err), err);
        return err;
    }
        
    return err;
}

esp_err_t security_fsm_start(void)
{
    if(security_fsm != NULL) 
    {
        ESP_LOGW(TAG, "Security FSM is already started.");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting security FSM...");

    security_fsm = ao_fsm_create(TAG, SEC_MONITORING_STATE, security_fsm_transitions, sizeof(security_fsm_transitions)/sizeof(ao_fsm_transition_t));
    
    return ao_fsm_start(security_fsm, tskIDLE_PRIORITY + 1, 2048);
}

static esp_err_t security_watchers_start(ao_fsm_t* security_fsm)
{
    ESP_LOGI(TAG, "Starting security watchers...");

    // TODO: Implement actual watchers as needed
    
    return ESP_OK;
}