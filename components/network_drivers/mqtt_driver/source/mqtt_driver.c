#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "lwip/ip4_addr.h"
#include "lwip/inet.h"
#include "sdkconfig.h"

#include "mqtt_driver.h"

#define MAX_TASK_PRIORITY 24 // Maximum priority for the MQTT task
#define MIN_URI_SIZE 32 // Minimum size for "mqtt://
#define MAX_IP_SIZE  16 // Maximum size for an IPv4 address string

static const char *TAG = "mqtt_driver";

// MQTT publish message structure to be used in the queue
typedef struct {
    char topic[MAX_TOPIC_SIZE];
    char payload[MAX_PAYLOAD_SIZE];
    int qos;
} mqtt_publish_msg_t;

static QueueHandle_t mqtt_publish_queue = NULL;
static TaskHandle_t mqtt_publisher_task_handle = NULL;
static SemaphoreHandle_t mqtt_handlers_mutex = NULL;

// Data structure for MQTT message handlers
typedef struct {
    char topic[MAX_TOPIC_SIZE];
    int qos;
    mqtt_msg_handler_t handler;
    int msg_id;
} mqtt_topic_handler_t;

static mqtt_topic_handler_t mqtt_handlers[MAX_SUBSCRIBE_MSG] = {0};

static bool mqtt_connected = false;

static esp_mqtt_client_handle_t client = NULL;

static void mqtt_publisher_task(void *arg);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static esp_err_t esp_ip4_to_mqtt_uri(const esp_ip4_addr_t *ip, char *uri_buf, size_t buf_size);


esp_err_t mqtt_client_start(esp_ip4_addr_t broker)
{
    char mqtt_uri[MIN_URI_SIZE];

    if (client != NULL) {
        ESP_LOGE(TAG, "MQTT client already started");
        return ESP_ERR_INVALID_STATE;
    }

    if (esp_ip4_to_mqtt_uri(&broker, mqtt_uri, sizeof(mqtt_uri)) == ESP_OK)
    {
        ESP_LOGI(TAG, "Broker URI: %s", mqtt_uri);
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_uri,
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

    mqtt_handlers_mutex = xSemaphoreCreateMutex();
    assert(mqtt_handlers_mutex != NULL);
    xSemaphoreGive(mqtt_handlers_mutex);

    mqtt_publish_queue = xQueueCreate(MAX_PUBLISH_MSG, sizeof(mqtt_publish_msg_t));
    if (mqtt_publish_queue == NULL) {
        ESP_LOGE(TAG, "Error to create MQTT publish queue");
        return ESP_ERR_NO_MEM;
    }
    BaseType_t result = xTaskCreate(mqtt_publisher_task, "mqtt_pub_task", 4096, NULL, MAX_TASK_PRIORITY, &mqtt_publisher_task_handle);
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

esp_err_t mqtt_client_subscribe(const char *topic, mqtt_msg_handler_t handler, int qos)
{
    for (int i = 0; i < MAX_SUBSCRIBE_MSG; ++i)
    {
        if (mqtt_handlers[i].handler == NULL) 
        {
            xSemaphoreTake(mqtt_handlers_mutex, portMAX_DELAY);
            strncpy(mqtt_handlers[i].topic, topic, sizeof(mqtt_handlers[i].topic) - 1);
            mqtt_handlers[i].handler = handler;
            mqtt_handlers[i].qos = qos;
            mqtt_handlers[i].msg_id = 0;
            
            ESP_LOGI(TAG, "Handler register for topic: %s", topic);
            
            if (mqtt_connected)
            {
                mqtt_handlers[i].msg_id  = esp_mqtt_client_subscribe(client, topic, qos);
                ESP_LOGI(TAG, "Subscribed to topic[%d]: %s, msg_id=%d", i, topic, mqtt_handlers[i].msg_id );
            }
            else
            {                
                ESP_LOGW(TAG, "MQTT not connected, deferring subscription for topic: %s", topic);
            }
            xSemaphoreGive(mqtt_handlers_mutex);
            return ESP_OK;
        }
        
    }
    return ESP_ERR_NO_MEM;
}

/**
 * @brief Convert an IPv4 address to an MQTT URI format
 *
 * This function converts an IPv4 address to a string in the format "mqtt://<ip_address>/"
 * and stores it in the provided buffer.
 *
 * @param ip Pointer to the IPv4 address structure
 * @param uri_buf Buffer to store the resulting URI string
 * @param buf_size Size of the buffer
 * @return ESP_OK on success, or an error code on failure
 */
static esp_err_t esp_ip4_to_mqtt_uri(const esp_ip4_addr_t *ip, char *uri_buf, size_t buf_size)
{
    if (!ip || !uri_buf || buf_size < MIN_URI_SIZE) 
        return ESP_ERR_INVALID_ARG;

    char ip_str[MAX_IP_SIZE];
    if (!ip4addr_ntoa_r((const ip4_addr_t *)ip, ip_str, sizeof(ip_str))) 
        return ESP_FAIL;

    snprintf(uri_buf, buf_size, "mqtt://%s/", ip_str);
    return ESP_OK;
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

                UBaseType_t items_in_queue = uxQueueMessagesWaiting(mqtt_publish_queue);
                ESP_LOGI(TAG, "Queue size: %u", (unsigned int)items_in_queue);
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
            xSemaphoreTake(mqtt_handlers_mutex, portMAX_DELAY);
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            for (int i = 0; i < MAX_SUBSCRIBE_MSG; ++i)
            {
                if (mqtt_handlers[i].handler != NULL && mqtt_handlers[i].msg_id <= 0)
                {
                    mqtt_handlers[i].msg_id  = esp_mqtt_client_subscribe(client, mqtt_handlers[i].topic, mqtt_handlers[i].qos);
                    ESP_LOGI(TAG, "Subscribed to topic(%d): %s, msg_id=%d", i, mqtt_handlers[i].topic, mqtt_handlers[i].msg_id );
                }
            }
            xSemaphoreGive(mqtt_handlers_mutex);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = false;
            for (int i = 0; i < MAX_SUBSCRIBE_MSG; ++i)
                mqtt_handlers[i].msg_id = 0; // Reset msg_id on disconnect
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
            ESP_LOGD(TAG, "Message received with topic: %.*s", event->topic_len, event->topic);

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