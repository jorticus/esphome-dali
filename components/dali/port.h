#pragma once

#include <esphome.h>

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void delayMilliseconds(uint32_t delay_ms) {
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}
#else
// Arduino provides delay() natively, but not delayMilliseconds.
static void delayMilliseconds(uint32_t delay_ms) {
    delay(delay_ms);
}
#endif
