#ifndef IMU_H
#define IMU_H

#include "esp_err.h"

#define ICM20948_ADDR 0x69

// Adds the ICM-20948 as a device on the shared I2C bus and verifies
// WHO_AM_I. Call after i2c_bus_init().
esp_err_t imu_start(void);

#endif
