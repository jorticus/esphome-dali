#pragma once

#include <esphome.h>

#if !defined(ARDUINO)
#include <esp_rom_sys.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void delayMicroseconds(uint32_t delay_us) {
    esp_rom_delay_us(delay_us);
}
static void delayMilliseconds(uint32_t delay_ms) {
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}
static uint32_t millis() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}
static void enterCritical() {
    portDISABLE_INTERRUPTS();
}
static void exitCritical() {
    portENABLE_INTERRUPTS();
}

#define LOW (0)
#define HIGH (1)
#else
// Arduino provides delayMicroseconds, millis, LOW, HIGH natively,
// but not delayMilliseconds — shim it with delay().
static void delayMilliseconds(uint32_t delay_ms) {
    delay(delay_ms);
}
#endif
