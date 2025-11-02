#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "security_watcher.h"
#include "communication_module.h"
#include "communication_suscriber.h"
#include "communication_publisher.h"
#include "mqtt_driver.h"

static const char *TAG = "communication_suscriber";

static const char *SIREN_CMND_SUBTOPIC  = "ALARM/Siren/CMND";
static const char *LIGHTS_CMND_SUBTOPIC = "ALARM/Lights/CMND";

//------------------------------------------------------------------------------
// MQTT TIME SUBSCRIPTION
//------------------------------------------------------------------------------

/**
 * @brief MQTT message callback for Siren Command topic.
 * @param topic The topic on which the message was received.
 * @param payload The payload of the received message.
 */
static void mqtt_siren_callback(const char *topic, const char *payload)
{
    ESP_LOGI(TAG, "Received message on topic: %s, payload: %s", topic, payload);
    if (strcmp(payload, "ON") == 0 || strcmp(payload, "on") == 0)
    {
        security_turnSiren_on();
        communication_siren_status_publish("SIREN_ON");
    }
    else if (strcmp(payload, "OFF") == 0 || strcmp(payload, "off") == 0)
    {
        security_turnSiren_off();
        communication_siren_status_publish("SIREN_OFF");
    }
}

/**
 * @brief MQTT message callback for Lights Command topic.
 * @param topic The topic on which the message was received.
 * @param payload The payload of the received message.
 */
static void mqtt_lights_callback(const char *topic, const char *payload)
{
    ESP_LOGI(TAG, "Received message on topic: %s, payload: %s", topic, payload);
    if (strcmp(payload, "ON") == 0 || strcmp(payload, "on") == 0)
    {
        security_turnLights_on();
        communication_lights_status_publish("LIGHTS_ON");
    }
    else if (strcmp(payload, "OFF") == 0 || strcmp(payload, "off") == 0)
    {
        security_turnLights_off();
        communication_lights_status_publish("LIGHTS_OFF");
    }
}

//------------------------------------------------------------------------------

static esp_err_t mqtt_generic_suscription(const char* subTopic, mqtt_msg_handler_t callback)
{
    char full_topic[MQTT_FULL_TOPIC_SIZE]; 
    snprintf(full_topic, MQTT_FULL_TOPIC_SIZE, "%s%s", MQTT_BASE_TOPIC, subTopic);

    esp_err_t ret = mqtt_client_subscribe(full_topic, callback, QOS0);

    return ret;
}

esp_err_t communication_suscriber_start(void)
{   
    ESP_LOGI(TAG, "Starting communication subscriber");

    esp_err_t ret = mqtt_generic_suscription(SIREN_CMND_SUBTOPIC, mqtt_siren_callback);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to set up MQTT Siren Command Subscription: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = mqtt_generic_suscription(LIGHTS_CMND_SUBTOPIC, mqtt_lights_callback);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to set up MQTT Lights Command Subscription: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}