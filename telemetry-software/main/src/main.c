#include "main.h"
#include "GPS.h"
#include "sd_log.h"
#include "log_queue.h"
#include "i2c.h"
#include "imu.h"
#include "console.h"
#include "led_status.h"

void app_main(void) {
    led_status_init();

    log_queue_init();
    sd_init();
    i2c_bus_init();

    sd_logger_start();
    bool gps_ok = gps_start();
    bool imu_ok = (imu_start() == ESP_OK);
    console_start();

    if (gps_ok && imu_ok) {
        led_status_ok();
    } else {
        led_status_fail();
    }
}
