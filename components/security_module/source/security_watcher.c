#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "security_watcher.h"
#include "security_ao_fsm.h"
#include "i2c_mgmt_driver.h"
#include "i2c_mcp23017.h"
#include "i2c_pn532.h"

static const char *TAG = "security_watcher";

#define TAG_SIZE 4
#define MAX_VALID_TAGS 3
#define PANIC_BUTTON_MASK 0x01  // Assuming panic button is connected to GPA0
#define PIR_SENSOR_MASK   0x02  // Assuming PIR sensor is connected to GPA1
#define LIGHTS_MASK       0x04  // Assuming lights control is connected to GPA4
#define SIREN_MASK        0x05  // Assuming siren control is connected to GPA5 

uint8_t valid_tags[MAX_VALID_TAGS][TAG_SIZE] = {
    { 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xEA, 0xEE, 0x85, 0x6A },
    { 0x40, 0x8B, 0xE6, 0x30 }
};

/**
 * @brief Validates a tag against the list of valid tags.
 * @details This function checks if the provided tag matches any of the valid tags.
 * @param tag Pointer to the tag data to validate.
 * @param len Length of the tag data. Must be equal to TAG_SIZE.
 * @return true if the tag is valid, false otherwise.
 */
static bool security_tagValidation(uint8_t* tag, size_t len)
{
    if(tag == NULL || len != TAG_SIZE) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_tagValidation");
        return false;
    }

    for(int i = 0; i < MAX_VALID_TAGS; i++) 
        if(memcmp(tag, valid_tags[i], TAG_SIZE) == 0) 
            return true;

    return false;
}

esp_err_t security_watcher_devices_start(void)
{
    ESP_LOGI(TAG, "Initializing I2C manager for security watcher devices");
    esp_err_t err = i2c_mgmt_start(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start I2C manager. err=%s (0x%x)", esp_err_to_name(err), err);
        return err;
    }

    ESP_LOGI(TAG, "PN532 NFC module initialized");
    err = i2c_pn532_start();
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start PN532 RFID reader. err=%s (0x%x)", esp_err_to_name(err), err);
        return err;
    }

    ESP_LOGI(TAG, "MCP23017 I/O expander initialized");
    err = i2c_mcp23017_start(0x20, 1000);
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start MCP23017 I/O expander. err=%s (0x%x)", esp_err_to_name(err), err);
        return err;
    }
    err = i2c_mcp23017_set_pullups(IODIRA_VALUE, IODIRB_VALUE);
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to set pull-ups on MCP23017 I/O expander. err=%s (0x%x)", esp_err_to_name(err), err);
        return err;
    }

    return ESP_OK;
}

void security_tagReader(ao_fsm_t* fsm)
{
    uint8_t tag[TAG_SIZE] = {0};
    size_t tag_len = TAG_SIZE;
    esp_err_t err = i2c_pn532_read_passive_target(tag, &tag_len);

    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to read tag from RFID reader. err=%s (0x%x)", esp_err_to_name(err), err);
        return;
    }

    if(tag_len != 0 && tag_len == TAG_SIZE) 
    {
       bool is_valid = security_tagValidation(tag, TAG_SIZE);

       if(is_valid) 
       {
           ESP_LOGI(TAG, "Valid tag read: %02X %02X %02X %02X", tag[0], tag[1], tag[2], tag[3]);
           ao_fsm_post(fsm, VALID_TAG_EVENT, NULL, 0);
       } 
       else 
       {
           ESP_LOGW(TAG, "Invalid tag read: %02X %02X %02X %02X", tag[0], tag[1], tag[2], tag[3]);
           ao_fsm_post(fsm, INVALID_TAG_EVENT, NULL, 0);
       }
    }
}

void security_panicButtonReader(ao_fsm_t* fsm)
{
    // Read panic button state from MCP23017
    uint8_t gpioa_state;
    esp_err_t err = i2c_mcp23017_read_gpioa_inputs(&gpioa_state);
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to read GPIOA from MCP23017. err=%s (0x%x)", esp_err_to_name(err), err);
        return;
    }

    // Check if panic button is pressed (assuming active low)
    if((gpioa_state & PANIC_BUTTON_MASK) == 0) 
    {
        ESP_LOGI(TAG, "Panic button pressed");
        ao_fsm_post(fsm, PANIC_BUTTON_PRESSED_EVENT, NULL, 0);
    }
}

void security_pirSensorReader(ao_fsm_t* fsm)
{
    // Read PIR sensor state from MCP23017
    uint8_t gpioa_state;
    esp_err_t err = i2c_mcp23017_read_gpioa_inputs(&gpioa_state);
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to read GPIOA from MCP23017. err=%s (0x%x)", esp_err_to_name(err), err);
        return;
    }

    // Check if PIR sensor is Active (assuming active low)
    if((gpioa_state & PIR_SENSOR_MASK) == 0) 
    {
        ESP_LOGI(TAG, "Intrusion detected by PIR sensor");
        ao_fsm_post(fsm, INTRUSION_DETECTED_EVENT, NULL, 0);
    }
}

void security_turnLights_on(void)
{
    ESP_LOGI(TAG, "Turning on security lights");
    uint8_t gpioa4_state = 0;

    gpioa4_state |= (1<<LIGHTS_MASK); // Set lights bit high
    esp_err_t err = i2c_mcp23017_write_gpioa_outputs(gpioa4_state);

    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to turn on security lights. err=%s (0x%x)", esp_err_to_name(err), err);
    }
}

void security_turnLights_off(void)
{
    ESP_LOGI(TAG, "Turning off security lights");
    uint8_t gpioa4_state = 0;

    gpioa4_state &= ~(1<<LIGHTS_MASK); // Set lights bit low
    esp_err_t err = i2c_mcp23017_write_gpioa_outputs(gpioa4_state);

    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to turn off security lights. err=%s (0x%x)", esp_err_to_name(err), err);
    }
}

void security_turnSiren_on(void)
{
    ESP_LOGI(TAG, "Turning on security siren");
    uint8_t gpioa5_state = 0;

    gpioa5_state |= (1<<SIREN_MASK); // Set siren bit high
    esp_err_t err = i2c_mcp23017_write_gpioa_outputs(gpioa5_state);

    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to turn on security siren. err=%s (0x%x)", esp_err_to_name(err), err);
    }
}

void security_turnSiren_off(void)
{
    ESP_LOGI(TAG, "Turning off security siren");
    uint8_t gpioa5_state = 0;

    gpioa5_state &= ~(1<<SIREN_MASK); // Set siren bit low
    esp_err_t err = i2c_mcp23017_write_gpioa_outputs(gpioa5_state);

    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to turn off security siren. err=%s (0x%x)", esp_err_to_name(err), err);
    }
}


