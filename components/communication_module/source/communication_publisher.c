#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cJSON.h"

#include "communication_module.h"
#include "communication_suscriber.h"
#include "mqtt_driver.h"
#include "sntp_driver.h"

#define PERIOD_MS 30000

static const char *TAG = "communication_publisher";
static TaskHandle_t s_time_pub_handle = NULL;

static void time_publisher_task(void *arg)
{
    char payload_buffer[MQTT_PAYLOAD_SIZE];
    char time_str[ISO_TIMESTAMP_SIZE];
    char full_topic[MQTT_FULL_TOPIC_SIZE];

    snprintf(full_topic, MQTT_FULL_TOPIC_SIZE, "%s%s", MQTT_BASE_TOPIC, "TIME");

    while (1)
    {
        if (sntp_client_isotime(time_str, sizeof(time_str)) == ESP_OK)
        {
            snprintf(payload_buffer, sizeof(payload_buffer), "{\"timestamp\":\"%s\"}", time_str);
            ESP_LOGI(TAG, "Publish: %s", payload_buffer);
            mqtt_client_publish(full_topic, payload_buffer, 0);
        }
        else
        {
            ESP_LOGW(TAG, "Failed to get time");
        }

        vTaskDelay(pdMS_TO_TICKS(PERIOD_MS));
    }
}

esp_err_t communication_publisher_start(void)
{
    if (s_time_pub_handle != NULL)
    {
        ESP_LOGW(TAG, "Task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(time_publisher_task, "time_publisher_task", 4096, NULL, tskIDLE_PRIORITY + 5, &s_time_pub_handle);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create time publisher task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Time publisher started");
    return ESP_OK;
}
