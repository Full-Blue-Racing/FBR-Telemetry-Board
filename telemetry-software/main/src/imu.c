#include "imu.h"
#include "i2c.h"
#include "main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"

static const char *TAG = "imu";
static i2c_master_dev_handle_t s_imu;
static i2c_master_bus_handle_t bus;

esp_err_t imu_start(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = ICM20948_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    bus = i2c_bus_get_handle();
    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &s_imu);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "add_device failed: %s", esp_err_to_name(ret));
        return ret;
    }

        // vTaskDelay(pdMS_TO_TICKS(250));   // let the IMU boot
    if (i2c_master_probe(bus, 0x68, pdMS_TO_TICKS(150)) == ESP_OK){
        ESP_LOGI(TAG, "ICM20948 found at 0x68");
        return ESP_OK;}
    else if (i2c_master_probe(bus, 0x69, pdMS_TO_TICKS(150)) == ESP_OK){
        ESP_LOGI(TAG, "ICM20948 found at 0x69");
        return ESP_OK;}
    else{
        ESP_LOGW(TAG, "ICM20948 not found at 0x68 or 0x69");
        return ESP_LOG_ERROR;
    }
    
}


