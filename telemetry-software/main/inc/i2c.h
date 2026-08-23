#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#define I2C_SDA       35
#define I2C_SCL       2
#define I2C_FREQ_HZ   400000

esp_err_t i2c_bus_init(void);
i2c_master_bus_handle_t i2c_bus_get_handle(void);
