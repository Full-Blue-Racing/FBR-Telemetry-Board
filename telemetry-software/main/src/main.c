#include "main.h"
#include "GPS.h"
#include "sd_log.h"
#include "log_queue.h"
#include "i2c.h"
#include "imu.h"


const char new_line[17] = "HELLOOOO testing";

void app_main(void) {


    log_queue_init();
    sd_init();
    i2c_bus_init();
    
    sd_log_line("test line 1");
    sd_log_line("test line 2");
    sd_log_line("test line 3");
    vTaskDelay(pdMS_TO_TICKS(6000));
    sd_log_line("test line 4");

    sd_dump_file(MOUNT_POINT "/gpslog_000.txt");   // TEMP: verify flash logging - remove after checking
    sd_dump_file(MOUNT_POINT "/gpslog_001.txt");
    sd_dump_file(MOUNT_POINT "/gpslog_002.txt");
    // sd_logger_start();
    // gps_start();
    // imu_start();
}