#pragma once
#include "esp_err.h"

// Initializes the onboard WS2812 status LED (GPIO21, Waveshare ESP32-S3 boards).
// Call once at startup.
esp_err_t led_status_init(void);

// Solid green: both GPS and IMU confirmed present over I2C.
void led_status_ok(void);

// Flashing red: at least one sensor missing.
void led_status_fail(uint8_t flag);
