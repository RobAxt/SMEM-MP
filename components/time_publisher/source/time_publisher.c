#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cJSON.h"

#include "mqtt_driver.h"
#include "sntp_driver.h"

#include "time_publisher.h"

#define TOPIC "tgn/seccion/comunicaciones/emyr/sitio/test"
#define PERIOD_MS 30000

static const char *TAG = "time_publisher";
static TaskHandle_t s_time_pub_handle = NULL;

static void time_publisher_task(void *arg)
{
    char json_buf[64];
    char time_str[ISO_TIMESTAMP_SIZE];

    while (1)
    {
        if (sntp_client_isotime(time_str, sizeof(time_str)) == ESP_OK)
        {
            snprintf(json_buf, sizeof(json_buf), "{\"timestamp\":\"%s\"}", time_str);
            mqtt_client_publish(TOPIC, json_buf, 0);
            ESP_LOGI(TAG, "Published: %s", json_buf);
        }
        else
        {
            ESP_LOGW(TAG, "Failed to get time");
        }

        vTaskDelay(pdMS_TO_TICKS(PERIOD_MS));
    }
}

static time_t parse_iso8601(const char *timestamp)
{
    struct tm tm = {0};
    strptime(timestamp, "%Y-%m-%dT%H:%M:%SZ", &tm);
    return mktime(&tm);
}

static void mqtt_time_callback(const char *topic, const char *payload)
{
    ESP_LOGI(TAG, "Received: %s", payload);

    cJSON *root = cJSON_Parse(payload);
    if (!root) {
        ESP_LOGW(TAG, "Failed to parse JSON");
        return;
    }

    const cJSON *ts = cJSON_GetObjectItem(root, "timestamp");
    if (!cJSON_IsString(ts) || (ts->valuestring == NULL)) {
        ESP_LOGW(TAG, "No valid timestamp found");
        cJSON_Delete(root);
        return;
    }

    time_t received = parse_iso8601(ts->valuestring);
    time_t now = time(NULL);
    long diff = (long)(now - received);

    ESP_LOGI(TAG, "Time difference: %ld seconds", diff);

    cJSON_Delete(root);
}

esp_err_t time_publisher_start(void)
{
    // Suscribirse al mismo topic
    esp_err_t ret = mqtt_client_subscribe(TOPIC, mqtt_time_callback, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to topic: %s", TOPIC);
        return ret;
    }
    if (s_time_pub_handle != NULL)
    {
        ESP_LOGW(TAG, "Task already running");
        return ESP_OK;
    }

    BaseType_t result = xTaskCreate(time_publisher_task, "time_pub", 4096, NULL, tskIDLE_PRIORITY + 1, &s_time_pub_handle);

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create time publisher task");
        s_time_pub_handle = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}
