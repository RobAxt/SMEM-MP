#include <stdio.h>
#include "esp_log.h"
#include "DummyComponent.h"

static const char *TAG = "DummyComponent";

void func(void)
{
    ESP_LOGI(TAG, "DummyComponent function called");
}
