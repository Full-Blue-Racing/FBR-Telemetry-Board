#include "main.h"
#include "GPS.h"
#include "sd_log.h"
#include "log_queue.h"



void app_main(void) {
    log_queue_init();
    // sd_init();
    sd_logger_start();
    gps_i2c_start();
}