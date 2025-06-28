#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

#include "mqtt_driver.h"

static const char *TAG = "mqtt_driver";

// MQTT publish message structure to be used in the queue
typedef struct {
    char topic[MAX_TOPIC_SIZE];
    char payload[MAX_PAYLOAD_SIZE];
    int qos;
} mqtt_publish_msg_t;

static QueueHandle_t mqtt_publish_queue = NULL;
static TaskHandle_t mqtt_publisher_task_handle = NULL;

// Data structure for MQTT message handlers
typedef struct {
    char topic[MAX_TOPIC_SIZE];
    int qos;
    mqtt_msg_handler_t handler;
} mqtt_topic_handler_t;

static mqtt_topic_handler_t mqtt_handlers[MAX_SUBSCRIBE_MSG] = {0};

static bool mqtt_connected = false;

static esp_mqtt_client_handle_t client = NULL;

static void mqtt_publisher_task(void *arg);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

esp_err_t mqtt_client_start(void)
{
    if (client != NULL) {
        ESP_LOGE(TAG, "MQTT client already started");
        return ESP_ERR_INVALID_STATE;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .broker.address.port = CONFIG_BROKER_PORT,
        .credentials.username = CONFIG_BROKER_USERNAME,
        .credentials.client_id = CONFIG_BROKER_CLIENT_ID,
        .credentials.authentication.password = CONFIG_BROKER_PASSWORD,
        .network.reconnect_timeout_ms = CONFIG_MQTT_RECONNECT_TIMEOUT_MS
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_ERR_NO_MEM;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t ret = esp_mqtt_client_start(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ret));
        return ret;
    }

    mqtt_publish_queue = xQueueCreate(MAX_PUBLISH_MSG, sizeof(mqtt_publish_msg_t));
    if (mqtt_publish_queue == NULL) {
        ESP_LOGE(TAG, "Error to create MQTT publish queue");
        return ESP_ERR_NO_MEM;
    }
    BaseType_t result = xTaskCreate(mqtt_publisher_task, "mqtt_pub_task", 4096, NULL, 5, &mqtt_publisher_task_handle);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create MQTT publisher task");
        vQueueDelete(mqtt_publish_queue);
        mqtt_publish_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t mqtt_client_publish(const char *topic, const char *payload, int qos)
{
    if (!mqtt_publish_queue || !topic || !payload) {
        return ESP_ERR_INVALID_ARG;
    }

    mqtt_publish_msg_t msg;
    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
    msg.qos = qos;

    if (xQueueSend(mqtt_publish_queue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Queue is full, message not sent");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t mqtt_client_suscribe(const char *topic, mqtt_msg_handler_t handler, int qos)
{
    for (int i = 0; i < MAX_SUBSCRIBE_MSG; ++i)
    {
        if (mqtt_handlers[i].handler == NULL) 
        {
            strncpy(mqtt_handlers[i].topic, topic, sizeof(mqtt_handlers[i].topic) - 1);
            mqtt_handlers[i].handler = handler;
            mqtt_handlers[i].qos = qos;

            ESP_LOGI(TAG, "Handler register for topic: %s", topic);

            if (mqtt_connected)
            {
                int msg_id = esp_mqtt_client_subscribe(client, topic, qos);
                ESP_LOGI(TAG, "Subscribed to topic: %s, msg_id=%d", topic, msg_id);
            }
            else
            {                
                ESP_LOGW(TAG, "MQTT not connected, deferring subscription for topic: %s", topic);
            }

            return ESP_OK;
        }
    }
    return ESP_ERR_NO_MEM;
}

/**
 * @brief MQTT publisher task
 *
 * This task listens for messages in the MQTT publish queue and publishes them
 * to the MQTT broker using the esp_mqtt_client_publish function.
 *
 * @param arg Unused argument
 */
static void mqtt_publisher_task(void *arg) {
    mqtt_publish_msg_t msg;

    while (1)
    {
        if (xQueuePeek(mqtt_publish_queue, &msg, pdMS_TO_TICKS(100))) 
        {
            if(mqtt_connected)
            {
                xQueueReceive(mqtt_publish_queue, &msg, portMAX_DELAY);
                esp_mqtt_client_publish(client, msg.topic, msg.payload, 0, msg.qos, 0);
                ESP_LOGI(TAG, "Message sent with topic: %s", msg.topic);
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

/**
 * @brief MQTT event handler
 *
 * This function handles various MQTT events such as connection, disconnection,
 * subscription, unsubscription, publication, and data reception.
 *
 * @param handler_args Unused argument
 * @param base Event base
 * @param event_id Event ID
 * @param event_data Pointer to event data
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            for (int i = 0; i < MAX_SUBSCRIBE_MSG; ++i)
            {
                if (mqtt_handlers[i].handler != NULL)
                {
                    int msg_id = esp_mqtt_client_subscribe(client, mqtt_handlers[i].topic, mqtt_handlers[i].qos);
                    ESP_LOGI(TAG, "Subscribed to topic: %s, msg_id=%d", mqtt_handlers[i].topic, msg_id);
                }
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", (int)event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", (int)event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", (int)event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Message received with topic: %.*s", event->topic_len, event->topic);

            for (int i = 0; i < MAX_SUBSCRIBE_MSG; ++i)
            {
                if (mqtt_handlers[i].handler &&
                    strncmp(mqtt_handlers[i].topic, event->topic, event->topic_len) == 0)
                {
                    
                    char topic[MAX_TOPIC_SIZE], payload[MAX_PAYLOAD_SIZE];
                    strncpy(topic, event->topic, event->topic_len);
                    topic[event->topic_len] = '\0';
                    
                    strncpy(payload, event->data, event->data_len);
                    payload[event->data_len] = '\0';
                    
                    mqtt_handlers[i].handler(topic, payload);
                }
            }
            break;

        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled event id: %d", (int)event_id);
            break;
    }
}