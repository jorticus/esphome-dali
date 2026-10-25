#pragma once

#include <driver/gpio.h>
#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DALI_ADDRESS_TYPE_SHORT,
    DALI_ADDRESS_TYPE_GROUP,
    DALI_ADDRESS_TYPE_BROADCAST,
    DALI_ADDRESS_TYPE_SPECIAL_CMD
} dali_addressType_t;

#define DALI_TX_TIMEOUT_DEFAULT_MS 50
#define DALI_RESULT_NO_REPLY (-1)

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t dali_init(gpio_num_t dali_rx_gpio, gpio_num_t dali_tx_gpio);
esp_err_t dali_init_with_inversion(gpio_num_t dali_rx_gpio, gpio_num_t dali_tx_gpio, bool invert_tx, bool invert_rx);
esp_err_t dali_transaction(
    dali_addressType_t address_type,
    uint8_t address,
    bool is_cmd,
    uint8_t command,
    bool send_twice,
    int timeout_ms,
    int *result);

#ifdef __cplusplus
}
#endif
