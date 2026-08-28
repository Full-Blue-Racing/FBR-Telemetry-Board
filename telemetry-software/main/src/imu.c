#include "imu.h"
#include "i2c.h"
#include "main.h"
#include "log_queue.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include <stdint.h>

static const char *TAG = "imu";
static i2c_master_dev_handle_t s_imu;
static i2c_master_bus_handle_t bus;
#define REG_PWR_MGMT_1    0x06
#define REG_ACCEL_XOUT_H  0x2D
#define IMU_SAMPLE_LEN    12   // ax,ay,az,gx,gy,gz - 2 bytes each

static esp_err_t imu_wake(void) {
    uint8_t buf[2] = { REG_PWR_MGMT_1, 0x01 };  // clear SLEEP, auto clock source
    return i2c_master_transmit(s_imu, buf, sizeof(buf), pdMS_TO_TICKS(100));
}
static void imu_task(void *arg) {
    uint8_t reg = REG_ACCEL_XOUT_H;
    uint8_t raw[IMU_SAMPLE_LEN];
    char line[96];
    int zero_streak = 0;

    while (1) {
        if (i2c_master_transmit_receive(s_imu, &reg, 1, raw, sizeof(raw),
                                        pdMS_TO_TICKS(100)) == ESP_OK) {
            int16_t ax = (raw[0]  << 8) | raw[1];
            int16_t ay = (raw[2]  << 8) | raw[3];
            int16_t az = (raw[4]  << 8) | raw[5];
            int16_t gx = (raw[6]  << 8) | raw[7];
            int16_t gy = (raw[8]  << 8) | raw[9];
            int16_t gz = (raw[10] << 8) | raw[11];

             if (ax == 0 && ay == 0 && az == 0 && gx == 0 && gy == 0 && gz == 0) {
                if (++zero_streak >= 3) {
                    ESP_LOGW(TAG, "all-zero for %d reads - re-waking IMU", zero_streak);
                    imu_wake();
                    zero_streak = 0;
                }
            } else {
                zero_streak = 0;
            }

            snprintf(line, sizeof(line), "%d,%d,%d,%d,%d,%d", ax, ay, az, gx, gy, gz);
            ESP_LOGD(TAG, "%s", line);
            log_queue_push(LOG_SRC_IMU, line);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool imu_start(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = ICM20948_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    bus = i2c_bus_get_handle();
    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &s_imu);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "add_device failed: %s", esp_err_to_name(ret));
        return false;
    }

    bool found = false;
    uint8_t found_addr = 0;
    for (int attempt = 0; attempt < 5 && !found; attempt++) {
        if (i2c_master_probe(bus, 0x68, pdMS_TO_TICKS(150)) == ESP_OK) {
            found = true;
            found_addr = 0x68;
        } else if (i2c_master_probe(bus, 0x69, pdMS_TO_TICKS(150)) == ESP_OK) {
            found = true;
            found_addr = 0x69;
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    if (found) {
        ESP_LOGI(TAG, "ICM20948 found at 0x%02X", found_addr);
    } else {
        ESP_LOGW(TAG, "ICM20948 not found at 0x68 or 0x69 after retries - starting task anyway");
    }

    esp_err_t wake_ret = imu_wake();
    if (wake_ret != ESP_OK) {
        ESP_LOGW(TAG, "initial wake failed: %s - imu_task's zero-streak check will retry", esp_err_to_name(wake_ret));
    }

    // Always start the task, even if the chip wasn't ready yet at boot (e.g. a
    // cold-boot power-on race - see imu_task's zero-streak re-wake): it'll
    // recover on its own once the IMU comes up, same as gps_task already does.
    xTaskCreate(imu_task, "imu", 4096, NULL, 5, NULL);
    return found;
}

