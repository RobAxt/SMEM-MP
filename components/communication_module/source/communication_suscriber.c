#include "esp_log.h"

#include "communication_module.h"
#include "communication_suscriber.h"
#include "mqtt_driver.h"

static const char *TAG = "communication_suscriber";

//------------------------------------------------------------------------------
// MQTT TIME SUBSCRIPTION
//------------------------------------------------------------------------------

/**
 * @brief MQTT message callback for time topic.
 * @param topic The topic on which the message was received.
 * @param payload The payload of the received message.
 */
static void mqtt_time_callback(const char *topic, const char *payload)
{
    ESP_LOGI(TAG, "Received message on topic: %s, payload: %s", topic, payload);
}

//------------------------------------------------------------------------------

static esp_err_t mqtt_generic_suscription(char* subTopic, mqtt_msg_handler_t callback)
{
    char full_topic[MQTT_FULL_TOPIC_SIZE]; 
    snprintf(full_topic, MQTT_FULL_TOPIC_SIZE, "%s%s", MQTT_BASE_TOPIC, subTopic);

    esp_err_t ret = mqtt_client_subscribe(full_topic, callback, QOS0);

    return ret;
}

esp_err_t communication_suscriber_start(void)
{   
    ESP_LOGI(TAG, "Starting communication subscriber");

    esp_err_t ret = mqtt_generic_suscription("TIME", mqtt_time_callback);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to set up MQTT time subscription: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}