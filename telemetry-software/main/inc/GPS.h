#ifndef GPS_H
#define GPS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define PA1010D_ADDR  0x10
#define ACC_SIZE      512

// Returns true if the PA1010D ACK'd its I2C probe (i.e. GPS confirmed present).
bool gps_start(void);

#endif
