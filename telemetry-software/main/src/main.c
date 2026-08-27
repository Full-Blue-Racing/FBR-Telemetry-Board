#include "main.h"
#include "GPS.h"
#include "sd_log.h"
#include "log_queue.h"
#include "i2c.h"
#include "imu.h"
#include "console.h"

void app_main(void) {
    log_queue_init();
    sd_init();
    i2c_bus_init();

    

    sd_logger_start();
    gps_start();
    imu_start();
    console_start();
}