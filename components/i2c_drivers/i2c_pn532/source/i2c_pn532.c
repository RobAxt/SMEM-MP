#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "i2c_mgmt_driver.h"
#include "i2c_pn532.h"

#define PN532_MAX_FRAME  21
#define PN532_NO_TAG_FOUND 0x80
#define PN532_UID_OFFSET  14
#define PN532_UID_SIZE_OFFSET  13
#define PN532_TIMEOUT_MS 100
#define PN532_DELAY_MS 50

static const uint8_t ACKNOWLEDGE[]           = {0x01, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
static const uint8_t NO_ACKNOWLEDGE[]        = {0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
//static const uint8_t FIRMWARE[]              = {0x00, 0x00, 0xFF, 0x02, 0xFE, 0xD4, 0x02, 0x2A, 0x00};
static const uint8_t SAMCONFIG[]             = {0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x17, 0x00};
static const uint8_t SAMCONFIG_RESPONSE[]    = {0x01, 0x00, 0x00, 0xFF, 0x02, 0xFE, 0xD5, 0x15, 0x016, 0x00};
static const uint8_t INLIST_PASSIVE_TARGET[] = {0x00, 0x00, 0xFF, 0x04, 0XFC, 0XD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};

static const int PN532_I2C_ADDRESS = 0x24; // Dirección I2C del PN532 (0x48 >> 1)

static const char *TAG = "i2c_pn532";

static esp_err_t pn532_transaction(const char *op_name,
                                  const uint8_t *cmd, size_t cmd_len,
                                  int inter_delay,              // p.ej. PN532_DELAY_MS
                                  int timeout,                  // p.ej. PN532_TIMEOUT_MS
                                  uint8_t *resp_buf, size_t *resp_len, // si resp_len==NULL no copia
                                  const uint8_t *expected_resp, size_t expected_len,
                                  bool match_as_prefix)                // true: expected es prefijo
{
    // 1) Iniciar transacción
    if (i2c_mgmt_begin_transaction() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to begin I2C transaction for %s", op_name);
        return ESP_FAIL;
    }

    // 2) Enviar comando
    esp_err_t err = i2c_mgmt_write(PN532_I2C_ADDRESS, cmd, cmd_len, timeout);
    if (err != ESP_OK) 
    {
        i2c_mgmt_end_transaction(); // Finalizar transacción en caso de error
        ESP_LOGE(TAG, "Failed to send %s command", op_name);
        return err;
    }

    if (inter_delay) vTaskDelay(inter_delay);

    // 3) Leer ACK
    size_t ack_len = sizeof(ACKNOWLEDGE);
    uint8_t ack[sizeof(ACKNOWLEDGE)] = {0};
    err = i2c_mgmt_read(PN532_I2C_ADDRESS, ack, &ack_len, timeout);

    ESP_LOGD(TAG, "%s: Ack Read bytes %02X, %02X, %02X, %02X, %02X, %02X, %02X",
             op_name, ack[0], ack[1], ack[2], ack[3], ack[4], ack[5], ack[6]);

    if (err != ESP_OK) 
    {
        i2c_mgmt_end_transaction(); // Finalizar transacción en caso de error
        ESP_LOGE(TAG, "Failed to read ACK for %s", op_name);
        return err;
    }
    if (memcmp(ack, ACKNOWLEDGE, sizeof(ACKNOWLEDGE)) != 0 ||
        memcmp(ack, NO_ACKNOWLEDGE, sizeof(NO_ACKNOWLEDGE)) == 0) 
    {
        i2c_mgmt_end_transaction(); // Finalizar transacción en caso de error
        ESP_LOGE(TAG, "Failed to receive valid ACK for %s", op_name);
        return ESP_FAIL;
    }

    if (inter_delay) vTaskDelay(inter_delay);

    // 4) Leer respuesta
    uint8_t local_buf[PN532_MAX_FRAME] = {0};
    uint8_t *dst = resp_buf ? resp_buf : local_buf;
    size_t   to_read = resp_len ? *resp_len : PN532_MAX_FRAME;

    if (to_read == 0 || to_read > PN532_MAX_FRAME) 
        to_read = PN532_MAX_FRAME;

    err = i2c_mgmt_read(PN532_I2C_ADDRESS, dst, &to_read, timeout);
    if (err != ESP_OK)
    {
        i2c_mgmt_end_transaction(); // Finalizar transacción en caso de error
        ESP_LOGE(TAG, "Failed to read response for %s", op_name);
        return err;
    }

    // Log de respuesta (acotado)
    for (size_t i = 0; i < to_read; ++i)
        ESP_LOGD(TAG, "%s: Resp[%02u]=%02X", op_name, (unsigned)i, dst[i]);

    // 5) Validar respuesta (opcional)
    if (expected_resp && expected_len > 0) {
        if (match_as_prefix) 
        {
            if (to_read < expected_len || memcmp(dst, expected_resp, expected_len) != 0) 
            {
                i2c_mgmt_end_transaction(); // Finalizar transacción en caso de error
                ESP_LOGE(TAG, "%s: response prefix mismatch", op_name);
                return ESP_FAIL;
            }
        } 
        else
        {
            if (to_read != expected_len || memcmp(dst, expected_resp, expected_len) != 0) 
            {
                i2c_mgmt_end_transaction(); // Finalizar transacción en caso de error
                ESP_LOGE(TAG, "%s: response mismatch", op_name);
                return ESP_FAIL;
            }
        }
    }

    // 6) Finalizar transacción
    if (i2c_mgmt_end_transaction() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to end I2C transaction for %s", op_name);
        return ESP_FAIL;
    }
    
    // 7) Copiar longitud leída
    if (resp_len) *resp_len = to_read;
    return ESP_OK;
}

esp_err_t i2c_pn532_start(void)
{
    ESP_LOGI(TAG, "Initializing PN532 NFC module over I2C");

    // Send SAMConfiguration command
    size_t resp_len = sizeof(SAMCONFIG_RESPONSE);
    uint8_t resp[sizeof(SAMCONFIG_RESPONSE)] = {0};

    esp_err_t err = pn532_transaction("SAMConfiguration",
                                     SAMCONFIG, sizeof(SAMCONFIG),
                                     PN532_DELAY_MS,
                                     PN532_TIMEOUT_MS,
                                     resp, &resp_len,
                                     SAMCONFIG_RESPONSE, sizeof(SAMCONFIG_RESPONSE),
                                     true);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "PN532 initialized successfully");
    return ESP_OK;
}

esp_err_t i2c_pn532_read_passive_target(uint8_t *uid, size_t *uid_len)
{
 // Aquí se pide la trama completa y luego se interpreta.
    uint8_t resp[PN532_MAX_FRAME] = {0};
    size_t  resp_len = PN532_MAX_FRAME;

    // En muchas implementaciones solo interesa chequear el prefijo de cabecera de respuesta.
    // Si se conoce un prefijo mínimo de respuesta para InListPassiveTarget, puede usarse.
    // Si no, se omite expected_resp (pase expected_resp=NULL).
    const uint8_t *expected_prefix = NULL;
    size_t expected_prefix_len = 0;

    esp_err_t err = pn532_transaction("InListPassiveTarget",
                                     INLIST_PASSIVE_TARGET, sizeof(INLIST_PASSIVE_TARGET),
                                     PN532_DELAY_MS,
                                     PN532_TIMEOUT_MS,
                                     resp, &resp_len,
                                     expected_prefix, expected_prefix_len,
                                     false);
    if (err != ESP_OK)
    {
        if (uid_len) *uid_len = 0;
        return err;
    }

    // Interpretación mínima del código “no tag”
    if (resp_len > 1 && resp[1] == PN532_NO_TAG_FOUND) 
    {
        ESP_LOGD(TAG, "No passive target found");
        if (uid_len) *uid_len = 0;
        return ESP_OK;
    }

    // Extraer UID según el formato de respuesta
    // https://www.nxp.com/docs/en/user-guide/141520.pdf
    if (uid && uid_len && *uid_len > 0)
    {
        size_t uid_sz  = resp[PN532_UID_SIZE_OFFSET]; // Longitud del UID en la respuesta
        if (uid_sz > *uid_len) uid_sz = *uid_len; // No exceder el buffer del usuario
        memcpy(uid, &resp[PN532_UID_OFFSET], uid_sz);
        *uid_len = uid_sz;
    }

    return ESP_OK;
}
