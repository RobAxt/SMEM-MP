#include "esp_log.h"
#include "driver/uart.h"
#include "mbcontroller.h"

#include "uart_pzem004t.h"

#define MB_DEV_SPEED 9600
#define MB_RESPONSE_TIMEOUT_MS 1000
#define MB_INIT_WAIT_MS 100
#define PZEM_SLAVE_ADDR 1

static const char *TAG = "uart_pzem004t";
static void *mbc_master_handle = NULL;

static inline uint32_t u32_from_regs(uint16_t lo, uint16_t hi)
{
    return ((uint32_t)hi << 16) | lo;
}

// Parse and print one PZEM frame (20 data bytes = 10 registers)
static void pzem_parse_and_log(const uint8_t *rx, size_t len)
{
    // Expected reply: [slave][0x04][byte_count=20][20 data bytes][CRC lo][CRC hi]
    if (len < 3 + 20) {
        ESP_LOGE(TAG, "Unexpected length: %u", (unsigned)len);
        return;
    }
    if (rx[1] != 0x04) {
        ESP_LOGE(TAG, "Unexpected function: 0x%02X", rx[1]);
        return;
    }
    if (rx[2] != 20) {
        ESP_LOGE(TAG, "Unexpected byte count: %u", (unsigned)rx[2]);
        return;
    }

    const uint8_t *d = &rx[3]; // 20 data bytes start here
    // Unpack as 10 registers, big-endian per Modbus
    uint16_t reg[10];
    for (int i = 0; i < 10; ++i) {
        reg[i] = ((uint16_t)d[2*i] << 8) | d[2*i + 1];
    }

    // Map per datasheet
    // 0x0000 Voltage (0.1 V/LSB)
    float voltage_V = reg[0] * 0.1f;
    // 0x0001/0x0002 Current (0.001 A/LSB), 32-bit (low, high)
    float current_A = (float)u32_from_regs(reg[1], reg[2]) * 0.001f;
    // 0x0003/0x0004 Power (0.1 W/LSB), 32-bit
    float power_W = (float)u32_from_regs(reg[3], reg[4]) * 0.1f;
    // 0x0005/0x0006 Energy (1 Wh/LSB), 32-bit
    float energy_Wh = (float)u32_from_regs(reg[5], reg[6]) * 1.0f;
    // 0x0007 Frequency (0.1 Hz/LSB)
    float freq_Hz = reg[7] * 0.1f;
    // 0x0008 Power factor (0.01 /LSB)
    float pf = reg[8] * 0.01f;
    // 0x0009 Alarm (0xFFFF alarm, 0x0000 no alarm)
    bool alarm = (reg[9] == 0xFFFF);

    ESP_LOGI(TAG, "V=%.1f V, I=%.3f A, P=%.1f W, E=%.0f Wh, f=%.1f Hz, PF=%.2f, ALARM=%s",
             voltage_V, current_A, power_W, energy_Wh, freq_Hz, pf, alarm ? "YES" : "NO");
}

esp_err_t uart_pzem004t_start(uart_port_t uart_num, gpio_num_t tx_io_num, gpio_num_t rx_io_num)
{
    esp_err_t err = ESP_OK;

    mb_communication_info_t comm_info = {
        .ser_opts.port = uart_num,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = MB_DEV_SPEED,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.uid = 0,
        .ser_opts.response_tout_ms = MB_RESPONSE_TIMEOUT_MS,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1
    };

    // Initialize Modbus controller
    err = mbc_master_create_serial(&comm_info, &mbc_master_handle);
    if (err != ESP_OK || mbc_master_handle == NULL) 
    {
        ESP_LOGE(TAG, "Failed to create Modbus master controller: %s", esp_err_to_name(err));
        return ESP_ERR_INVALID_STATE;
    }

    // Set UART pins
    err = uart_set_pin(uart_num, tx_io_num, rx_io_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(err));
        return err;
    }

    err = mbc_master_start(mbc_master_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start Modbus master: %s", esp_err_to_name(err));
        return err;
    }
    
    err = uart_set_mode(uart_num, UART_MODE_UART);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set UART mode: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "UART PZEM004T initialized successfully at UART1, TX:%d RX:%d", tx_io_num, rx_io_num);

    vTaskDelay(pdMS_TO_TICKS(MB_INIT_WAIT_MS));
    return ESP_OK;
}

esp_err_t uart_pzem004t_read(void)
{
    mb_param_request_t req = {
        .slave_addr = PZEM_SLAVE_ADDR,
        .command    = 0x04,      // Read Input Registers
        .reg_start  = 0x0000,
        .reg_size   = 10         // 10 registers: 0x0000..0x0009
    };

    // Reply buffer: addr(1)+func(1)+bytecnt(1)+data(20)+crc(2) = 25 bytes
    uint8_t rx[32] = {0};
    esp_err_t err = mbc_master_send_request(mbc_master_handle, &req, rx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "send_request failed: %s (0x%x)", esp_err_to_name(err), err);
        return err;
    }
    pzem_parse_and_log(rx, sizeof(rx));
    return ESP_OK;
}