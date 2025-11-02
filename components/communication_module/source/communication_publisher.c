#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "cJSON.h"

#include "security_module.h"
#include "security_ao_fsm.h"
#include "ambiental_module.h"
#include "communication_module.h"
#include "communication_suscriber.h"
#include "mqtt_driver.h"
#include "sntp_driver.h"

static const char *TAG = "communication_publisher";

static const char *ALARM_STATUS_TOPIC  = "SECURITY/STATUS";
static const char *SIREN_STATUS_TOPIC  = "SECURITY/Siren/STATUS";
static const char *LIGHTS_STATUS_TOPIC = "SECURITY/Lights/STATUS";

static const char *ALARM_JSON_PAYLOAD  = "{\"TimeStamp\":\"%s\",\"Status\":\"%s\",\"State\":\"%s\"}";
static const char *STATUS_JSON_PAYLOAD = "{\"TimeStamp\":\"%s\",\"Status\":\"%s\"}";

static const char *AMBIENTAL_EXTERNAL_TOPIC = "AMBIENTAL/Temperature/External";
static const char *AMBIENTAL_INTERNAL_TOPIC = "AMBIENTAL/Temperature/Internal";
static const char *TEMPERATURE_JSON_PAYLOAD = "{\"TimeStamp\":\"%s\",\"Temperature\":{\"SensorID\":\"%016llX\",\"Value\":%.2f, \"Unit\":\"Celsius\"}}";

static const uint64_t EXTERNAL = 0xA079510087D31C28;
static const uint64_t INTERNAL = 0x9624300087EFEF28;

static esp_err_t publish_generic_event(const char *subTopic, const char *paylaodFormat, const char *status, const char *state)
{
    char topic[MQTT_FULL_TOPIC_SIZE] = {0}; 
    char timeString[ISO_TIMESTAMP_SIZE] = {0};
    char payload[MQTT_PAYLOAD_SIZE] = {0};
    esp_err_t err = ESP_OK;
    
    if( subTopic == NULL || paylaodFormat == NULL || status == NULL )
    {
        ESP_LOGE(TAG, "Invalid parameters in publish_generic_event");
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(topic, MQTT_FULL_TOPIC_SIZE, "%s%s", MQTT_BASE_TOPIC, subTopic);

    if( sntp_client_isotime(timeString, sizeof(timeString)) == ESP_OK )
    {
        if(state != NULL)
            snprintf(payload, sizeof(payload), paylaodFormat, timeString, status, state);
        else
            snprintf(payload, sizeof(payload), paylaodFormat, timeString, status);

        err = mqtt_client_publish(topic, payload, QOS0);

        ESP_LOGI(TAG, "Published %s Event: %s", status, payload);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get ISO timestamp for %s Event", status);

        if(state != NULL)
            snprintf(payload, sizeof(payload), paylaodFormat, "1981-01-04T10:00:00-03.00", status, state);
        else
            snprintf(payload, sizeof(payload), paylaodFormat, "1981-01-04T10:00:00-03.00", status);

        err = mqtt_client_publish(topic, payload, QOS0);
    }

    return err;
}

//--------------- INTRUSION_DETECTED_EVENT Publisher ---------------

static void publish_intrusion_detected_event(void)
{
    publish_generic_event(ALARM_STATUS_TOPIC, ALARM_JSON_PAYLOAD, "INTRUSION_DETECTED", "VALIDATING");
}

//--------------- PANIC_BUTTON_PRESSED_EVENT Publisher -------------

static void publish_panic_button_pressed_event(void)
{
    publish_generic_event(ALARM_STATUS_TOPIC, ALARM_JSON_PAYLOAD, "PANIC_BUTTON_PRESSED", "VALIDATING");   
}

//--------------------- VALID_TAG_EVENT Publisher ------------------

static void publish_valid_tag_event(void)
{
    publish_generic_event(ALARM_STATUS_TOPIC, ALARM_JSON_PAYLOAD, "VALID_TAG", "NORMAL");
    publish_generic_event(SIREN_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "SIREN_OFF", NULL);
    publish_generic_event(LIGHTS_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "LIGHTS_OFF", NULL);        
}

//-------------------- INVALID_TAG_EVENT Publisher ------------------

static void publish_invalid_tag_event(void)
{
    publish_generic_event(ALARM_STATUS_TOPIC, ALARM_JSON_PAYLOAD, "INVALID_TAG", "ALARM_TRIGGERED");
    publish_generic_event(SIREN_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "SIREN_ON", NULL);
    publish_generic_event(LIGHTS_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "LIGHTS_ON", NULL);  
}

//----------------- READ_TAG_TIMEOUT_EVENT Publisher ----------------

static void publish_read_tag_timeout_event(void)
{
    publish_generic_event(ALARM_STATUS_TOPIC, ALARM_JSON_PAYLOAD, "READ_TAG_TIMEOUT","ALARM_TRIGGERED");
    publish_generic_event(SIREN_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "SIREN_ON", NULL);
    publish_generic_event(LIGHTS_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "LIGHTS_ON", NULL);
}

//----------------- WORKING_TIMEOUT_EVENT Publisher -----------------
//-------------------------------------------------------------------
static void publish_working_timeout_event(void)
{
    publish_generic_event(ALARM_STATUS_TOPIC, ALARM_JSON_PAYLOAD, "WORKING_TIMEOUT", "MONITORING");
    publish_generic_event(SIREN_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "SIREN_OFF", NULL);
    publish_generic_event(LIGHTS_STATUS_TOPIC, STATUS_JSON_PAYLOAD, "LIGHTS_OFF", NULL);
}

//-------------------------------------------------------------------

//------------------ Ambiental Temperature Publisher ----------------
static void publish_temperature_read_event(void *data, ssize_t size)
{
    if( data == NULL || size != sizeof(ambiental_callback_data_t) )
    {
        ESP_LOGE(TAG, "Invalid data in publish_temperature_read");
        return;
    }

    ambiental_callback_data_t *tempData = (ambiental_callback_data_t *)data;

    char topic[MQTT_FULL_TOPIC_SIZE] = {0}; 
    char timeString[ISO_TIMESTAMP_SIZE] = {0};
    char payload[MQTT_PAYLOAD_SIZE] = {0};
    esp_err_t err = ESP_OK;
    if( sntp_client_isotime(timeString, sizeof(timeString)) == ESP_OK )
    {
        if( tempData->sensor_id == EXTERNAL )
        {
            snprintf(topic, MQTT_FULL_TOPIC_SIZE, "%s%s", MQTT_BASE_TOPIC, AMBIENTAL_EXTERNAL_TOPIC);
        }
        else if( tempData->sensor_id == INTERNAL )
        {
            snprintf(topic, MQTT_FULL_TOPIC_SIZE, "%s%s", MQTT_BASE_TOPIC, AMBIENTAL_INTERNAL_TOPIC);
        }
        else
        {
            ESP_LOGE(TAG, "Unknown sensor ID: %016llX", tempData->sensor_id);
            return;
        }

        snprintf(payload, sizeof(payload), TEMPERATURE_JSON_PAYLOAD, timeString, tempData->sensor_id, tempData->temperature_celsius);

        err = mqtt_client_publish(topic, payload, QOS0);

        ESP_LOGI(TAG, "Published Temperature Read: %s", payload);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get ISO timestamp for Temperature Read Event");
    }

}

//-------------------------------------------------------------------

esp_err_t communication_publisher_start(void)
{
    publish_generic_event(ALARM_STATUS_TOPIC, ALARM_JSON_PAYLOAD, "SYSTEM_STARTUP", "MONITORING");

    security_set_hookCallbak_OnEvent(INTRUSION_DETECTED_EVENT, publish_intrusion_detected_event);
    security_set_hookCallbak_OnEvent(PANIC_BUTTON_PRESSED_EVENT, publish_panic_button_pressed_event);
    security_set_hookCallbak_OnEvent(VALID_TAG_EVENT, publish_valid_tag_event);
    security_set_hookCallbak_OnEvent(INVALID_TAG_EVENT, publish_invalid_tag_event);
    security_set_hookCallbak_OnEvent(READ_TAG_TIMEOUT_EVENT, publish_read_tag_timeout_event);
    security_set_hookCallbak_OnEvent(WORKING_TIMEOUT_EVENT, publish_working_timeout_event);

    ambiental_set_hookCallback_onTemperatureRead(publish_temperature_read_event);

    ESP_LOGI(TAG, "Time publisher started");
    return ESP_OK;
}

esp_err_t communication_lights_status_publish(const char* status)
{
    return publish_generic_event(LIGHTS_STATUS_TOPIC, STATUS_JSON_PAYLOAD, status, NULL);
}

esp_err_t communication_siren_status_publish(const char* status)
{
    return publish_generic_event(SIREN_STATUS_TOPIC, STATUS_JSON_PAYLOAD, status, NULL);
}