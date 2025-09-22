#include <string.h>

#include "esp_log.h"

#include "i2c_mgmt_driver.h"
#include "i2c_mcp23017.h"

// Dirección de registros (BANK=0 por defecto)
#define MCP23017_IODIRA   0x00
#define MCP23017_IODIRB   0x01
#define MCP23017_IPOLA    0x02
#define MCP23017_IPOLB    0x03
#define MCP23017_GPINTENA 0x04
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALA  0x06
#define MCP23017_DEFVALB  0x07
#define MCP23017_INTCONA  0x08
#define MCP23017_INTCONB  0x09
#define MCP23017_IOCON    0x0A  // (mirror en 0x0B)
#define MCP23017_GPPUA    0x0C
#define MCP23017_GPPUB    0x0D
#define MCP23017_INTFA    0x0E
#define MCP23017_INTFB    0x0F
#define MCP23017_INTCAPA  0x10
#define MCP23017_INTCAPB  0x11
#define MCP23017_GPIOA    0x12
#define MCP23017_GPIOB    0x13
#define MCP23017_OLATA    0x14
#define MCP23017_OLATB    0x15

static const int MCP23017_I2C_ADDRESS = 0x20; // Dirección I2C del MCP23017 (0x20 a 0x27)
static const char *TAG = "i2c_mcp23017";

static uint8_t mcp23017_i2c_addr = MCP23017_I2C_ADDRESS;
static int mcp23017_timeout_ms = 1000; // Timeout por defecto en milisegundos

static esp_err_t write_reg(uint8_t dev_addr, uint8_t reg, uint8_t val, int timeout_ms)
{
    uint8_t frame[2] = { reg, val };
    return i2c_mgmt_write(dev_addr, frame, sizeof(frame), timeout_ms);
}

static esp_err_t read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *val, int timeout_ms)
{
    esp_err_t err;

    // Setea el puntero de registro
    err = i2c_mgmt_write(dev_addr, &reg, 1, timeout_ms);
    if (err != ESP_OK)
        return err;

    size_t len = 1;
    err = i2c_mgmt_read(dev_addr, val, &len, timeout_ms);

    if (err != ESP_OK) 
        return err;
    return (len == 1) ? ESP_OK : ESP_FAIL;
}

esp_err_t i2c_mcp23017_start(uint8_t i2c_addr, int timeout_ms)
{
    mcp23017_i2c_addr = i2c_addr;
    mcp23017_timeout_ms = timeout_ms;

    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) 
        return err;

    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_IODIRA, IODIRA_VALUE, mcp23017_timeout_ms)) != ESP_OK) goto out;
    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_IODIRB, IODIRB_VALUE, mcp23017_timeout_ms)) != ESP_OK) goto out;

    // Limpia OLAT/ GPIO para que salidas arranquen en 0 sin glitch
    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_OLATA, 0x00, mcp23017_timeout_ms)) != ESP_OK) goto out;
    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_OLATB, 0x00, mcp23017_timeout_ms)) != ESP_OK) goto out;

out:
    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MCP23017(0x%02X) init: IODIRA=0x%02X, IODIRB=0x%02X",
                 mcp23017_i2c_addr, IODIRA_VALUE, IODIRB_VALUE);
    }
    return err;
}

esp_err_t i2c_mcp23017_set_pullups(uint8_t gppua_mask, uint8_t gppub_mask)
{
    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) 
        return err;

    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_GPPUA, gppua_mask, mcp23017_timeout_ms)) != ESP_OK) goto out;
    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_GPPUB, gppub_mask, mcp23017_timeout_ms)) != ESP_OK) goto out;


out:
    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }
    
    if (err == ESP_OK) 
    {
        ESP_LOGI(TAG, "MCP23017(0x%02X) pulll-ups: GPPUA=0x%02X, GPPUB=0x%02X",
                 mcp23017_i2c_addr, gppua_mask, gppub_mask); 
    }
    return err;
}

esp_err_t i2c_mcp23017_write_gpioa_outputs(uint8_t value)
{
    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) return err;

    // Leer OLATA para preservar bits de entrada (0..3)
    uint8_t olat = 0;
    if ((err = read_reg(mcp23017_i2c_addr, MCP23017_OLATA, &olat, mcp23017_timeout_ms)) != ESP_OK) goto out;

    uint8_t new_olat = (olat & ~GPIOA_OUT_MASK) | (value & GPIOA_OUT_MASK);

    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_OLATA, new_olat, mcp23017_timeout_ms)) != ESP_OK) goto out;

out:
    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }
    return err;
}

esp_err_t i2c_mcp23017_write_gpiob_outputs(uint8_t value)
{
    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) return err;

    // Preserva entradas (0..3 y 6..7), sólo modifica B4..B5
    uint8_t olat = 0;
    if ((err = read_reg(mcp23017_i2c_addr, MCP23017_OLATB, &olat, mcp23017_timeout_ms)) != ESP_OK) goto out;

    uint8_t new_olat = (olat & ~GPIOB_OUT_MASK) | (value & GPIOB_OUT_MASK);

    if ((err = write_reg(mcp23017_i2c_addr, MCP23017_OLATB, new_olat, mcp23017_timeout_ms)) != ESP_OK) goto out;

out:
    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }
    return err;
}

esp_err_t i2c_mcp23017_read_gpioa_inputs(uint8_t *value)
{
    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) return err;

    uint8_t gpioa = 0;
    if ((err = read_reg(mcp23017_i2c_addr, MCP23017_GPIOA, &gpioa, mcp23017_timeout_ms)) != ESP_OK) goto out;

    *value = (gpioa & 0x0F);  // sólo entradas A0..A3

out:
    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }
    return err;
}

esp_err_t i2c_mcp23017_read_gpiob_inputs(uint8_t *value)
{
    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) return err;

    uint8_t gpiob = 0;
    if ((err = read_reg(mcp23017_i2c_addr, MCP23017_GPIOB, &gpiob, mcp23017_timeout_ms)) != ESP_OK) goto out;

    // Mantiene bits 0..3 y 6..7. (B4..B5 son salidas → se ignoran)
    *value = (uint8_t)((gpiob & 0x0F) | (gpiob & 0xC0));

out:
    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }
    return err;
}