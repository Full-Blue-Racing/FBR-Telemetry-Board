#ifndef IMU_H
#define IMU_H

#include <stdbool.h>
#include "esp_err.h"

#define ICM20948_ADDR 0x69

// Adds the ICM-20948 as a device on the shared I2C bus, starts its read task
// regardless of probe outcome (self-recovers if the chip wasn't ready yet),
// and returns whether the initial presence probe ACK'd. Call after i2c_bus_init().
bool imu_start(void);

#endif
