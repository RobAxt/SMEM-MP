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
#include "energy_module.h"
#include "communication_module.h"
#include "communication_suscriber.h"
#include "mqtt_driver.h"
#include "sntp_driver.h"

static const char *TAG = "communication_publisher";

static const char *ALARM_STATUS_TOPIC  = "SECURITY/STATUS";
static const char *SIREN_STATUS_TOPIC  = "SECURITY/Siren";
static const char *LIGHTS_STATUS_TOPIC = "SECURITY/Lights";

static const char *ALARM_JSON_PAYLOAD  = "{\"TimeStamp\":\"%s\",\"Status\":\"%s\",\"State\":\"%s\"}";
static const char *STATUS_JSON_PAYLOAD = "{\"TimeStamp\":\"%s\",\"Status\":\"%s\"}";

static const char *AMBIENTAL_EXTERNAL_TOPIC = "AMBIENTAL/Temperature/External";
static const char *AMBIENTAL_INTERNAL_TOPIC = "AMBIENTAL/Temperature/Internal";
static const char *TEMPERATURE_JSON_PAYLOAD = "{\"TimeStamp\":\"%s\",\"SensorID\":\"%016llX\",\"Value\":%.2f,\"Unit\":\"°C\"}";

static const char *VOLTAGE_AC_TOPIC = "ENERGY/AC/Voltage";
static const char *CURRENT_AC_TOPIC = "ENERGY/AC/Current";
static const char *POWER_AC_TOPIC = "ENERGY/AC/Power";
static const char *FREQUENCY_AC_TOPIC = "ENERGY/AC/Frequency";
static const char *POWERFACTOR_AC_TOPIC = "ENERGY/AC/PowerFactor";
static const char *VOLTAGE_DC_TOPIC = "ENERGY/DC/Voltage";
static const char *CURRENT_DC_TOPIC = "ENERGY/DC/Current";
static const char *POWER_DC_TOPIC = "ENERGY/DC/Power";
static const char *VALUE_PAYLOAD = "{\"TimeStamp\":\"%s\",\"Value\":%.2f,\"Unit\":\"%s\"}";

static const char *ENERGY_PROVIDER_STATUS_TOPIC = "ENERGY/STATUS/Provider";
static const char *ENERGY_PROTECTION_STATUS_TOPIC = "ENERGY/STATUS/Protection";
static const char *ENERGY_TAMPERING_STATUS_TOPIC = "ENERGY/STATUS/Tampering";
static const char *ENERGY_STATUS_NORMAL = "NORMAL";
static const char *ENERGY_STATUS_FAULT = "FAULT";
static const char *ENERGY_STATUS_TAMPERED = "TAMPERED";

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
            snprintf(payload, MQTT_PAYLOAD_SIZE, paylaodFormat, timeString, status, state);
        else
            snprintf(payload, MQTT_PAYLOAD_SIZE, paylaodFormat, timeString, status);

        err = mqtt_client_publish(topic, payload, QOS0);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get ISO timestamp for %s Event", status);

        if(state != NULL)
            snprintf(payload, MQTT_PAYLOAD_SIZE, paylaodFormat, "1981-01-04T10:00:00-03.00", status, state);
        else
            snprintf(payload, MQTT_PAYLOAD_SIZE, paylaodFormat, "1981-01-04T10:00:00-03.00", status);

        err = mqtt_client_publish(topic, payload, QOS0);
    }

    if( err == ESP_OK )
        ESP_LOGI(TAG, "Published %s Event: %s", status, payload);
    
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

        snprintf(payload, MQTT_PAYLOAD_SIZE, TEMPERATURE_JSON_PAYLOAD, timeString, tempData->sensor_id, tempData->temperature_celsius);

        if( mqtt_client_publish(topic, payload, QOS0) != ESP_OK )
            ESP_LOGE(TAG, "Fail to publish MQTT Temperature message");
        else
            ESP_LOGI(TAG, "Published Temperature Reading: %s", payload);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get ISO timestamp for Temperature Read Event");
    }
}

//-------------------------------------------------------------------

//------------------------- Energy Publisher ------------------------
void generic_publish_energy_read(const char* timeString, const char* subTopic, const char* payloadFormat, float value, const char* unit)
{
    char topic[MQTT_FULL_TOPIC_SIZE] = {0};
    char payload[MQTT_PAYLOAD_SIZE] = {0};

    snprintf(topic, MQTT_FULL_TOPIC_SIZE, "%s%s", MQTT_BASE_TOPIC, subTopic);
    snprintf(payload, MQTT_PAYLOAD_SIZE, payloadFormat, timeString, value, unit );

    if( mqtt_client_publish(topic, payload, QOS0) != ESP_OK)
        ESP_LOGE(TAG, "Fail to publish MQTT DC message");
    else
        ESP_LOGI(TAG, "Published DC Reading: %s", payload);
    
}

static void publish_energy_read_event(energy_data_t *data)
{
    if( data !=  NULL )
    {
        char timeString[ISO_TIMESTAMP_SIZE] = {0};

        if( sntp_client_isotime(timeString, sizeof(timeString)) == ESP_OK )
        {
            generic_publish_energy_read(timeString, VOLTAGE_AC_TOPIC, VALUE_PAYLOAD, data->ac_voltage, "V");
            generic_publish_energy_read(timeString, CURRENT_AC_TOPIC, VALUE_PAYLOAD, data->ac_current, "A");
            generic_publish_energy_read(timeString, POWER_AC_TOPIC, VALUE_PAYLOAD, data->ac_power, "W");
            generic_publish_energy_read(timeString, FREQUENCY_AC_TOPIC, VALUE_PAYLOAD, data->ac_frequency, "Hz");
            generic_publish_energy_read(timeString, POWERFACTOR_AC_TOPIC, VALUE_PAYLOAD, data->ac_power_factor, "#");
            generic_publish_energy_read(timeString, VOLTAGE_DC_TOPIC, VALUE_PAYLOAD, data->dc_voltage, "V");
            generic_publish_energy_read(timeString, CURRENT_DC_TOPIC, VALUE_PAYLOAD, data->dc_current, "A");
            generic_publish_energy_read(timeString, POWER_DC_TOPIC, VALUE_PAYLOAD, data->dc_power, "W");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get ISO timestamp for Temperature Read Event");
        }
   }
}

static void publish_energy_state_event(energy_data_t *data)
{
    if( data !=  NULL )
    {
        char timeString[ISO_TIMESTAMP_SIZE] = {0};

        if( sntp_client_isotime(timeString, sizeof(timeString)) == ESP_OK )
        {
            ESP_LOGI(TAG, "ZIGBEE Device State changed= 0x%02X", data->zigbee_device_state);
            publish_generic_event(ENERGY_PROVIDER_STATUS_TOPIC,   STATUS_JSON_PAYLOAD, data->zigbee_device_state & 0x01? ENERGY_STATUS_NORMAL : ENERGY_STATUS_FAULT, NULL);
            publish_generic_event(ENERGY_PROTECTION_STATUS_TOPIC, STATUS_JSON_PAYLOAD, data->zigbee_device_state & 0x02? ENERGY_STATUS_NORMAL : ENERGY_STATUS_FAULT, NULL);
            publish_generic_event(ENERGY_TAMPERING_STATUS_TOPIC,  STATUS_JSON_PAYLOAD, data->zigbee_device_state & 0x0C? ENERGY_STATUS_NORMAL : ENERGY_STATUS_TAMPERED, NULL);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get ISO timestamp for Temperature Read Event");
        }
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

    energy_set_hookCallback_onEnergyRead(publish_energy_read_event);
    energy_set_hookCallback_onEnergyState(publish_energy_state_event);

    ESP_LOGI(TAG, "Communication publisher started");
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