#include "sd_log.h"



#include "log_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sd";
static FILE *s_logf = NULL; //flash buffer 
static sdmmc_card_t *s_card = NULL;
static int64_t s_last_flush = 0;  //for timing, flash every 5 seconds
static int64_t s_file_started = 0; //for timing, start a new file every 1 hr or upon start up



static const char *src_tag(log_source_t s) {
    switch (s) {
        case LOG_SRC_GPS: return "GPS";
        case LOG_SRC_IMU: return "IMU";
        default:          return "???";
    }
}



// Find the lowest gpslog_NNN.txt that doesn't exist yet, build its path.
static bool next_free_path(char *out, size_t out_sz) {
    for (int i = 0; i < MAX_FILE_IDX; i++) {
        snprintf(out, out_sz, MOUNT_POINT "/gpslog_%03d.txt", i);
        struct stat st;
        if (stat(out, &st) != 0) {          // stat fails => file doesn't exist
            return true;                    // ...so this name is free
        }
    }
    ESP_LOGE(TAG, "no free file index (all %d used)", MAX_FILE_IDX);
    return false;
}

// Open a fresh log file at the next free index. Closes any current one first.
static esp_err_t open_new_file(void) {
    if (s_logf) {
        fflush(s_logf);
        fsync(fileno(s_logf));
        fclose(s_logf);
        s_logf = NULL;
    }
    char path[64];
    if (!next_free_path(path, sizeof(path))) return ESP_FAIL;

    s_logf = fopen(path, "w");              // "w" is safe: name is guaranteed unused
    if (!s_logf) {
        ESP_LOGE(TAG, "fopen(%s) failed", path);
        return ESP_FAIL;
    }
    int64_t now = esp_timer_get_time();
    s_file_started = now;
    s_last_flush   = now;
    ESP_LOGI(TAG, "logging to %s", path);
    return ESP_OK;
}

esp_err_t sd_init(void) {
    esp_err_t ret;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = SD_SPI_MOSI,
        .miso_io_num     = SD_SPI_MISO,
        .sclk_io_num     = SD_SPI_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
    };

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_SPI_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 3,
        .allocation_unit_size   = 16 * 1024,
    };

    // Bus init and mount are retried together: a failed mount still leaves
    // the SPI bus (and CS pin) claimed, so it must be freed before the next
    // spi_bus_initialize() or the retry conflicts on GPIO[SD_SPI_CS].
    while (1) {
        ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config,
                                      &mount_config, &s_card);
        if (ret == ESP_OK) break;

        if (ret == ESP_FAIL)
            ESP_LOGE(TAG, "mount failed - card not FAT32 formatted?");
        else
            ESP_LOGE(TAG, "card init failed: %s - check wiring/power",
                     esp_err_to_name(ret));

        spi_bus_free(host.slot);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    sdmmc_card_print_info(stdout, s_card);

    return open_new_file();                 // requirement: new file on power-on
}

void sd_log_line(const char *line) {
    if (!s_logf) return;

    int64_t now = esp_timer_get_time();

    // Roll to a new file every hour of runtime.
    if (now - s_file_started >= ROLL_US) {
        ESP_LOGI(TAG, "hourly roll");
        if (open_new_file() != ESP_OK) return;
    }

    fprintf(s_logf, "%s\n", line);

    // Flush every 5 s.
    if (now - s_last_flush >= FLUSH_US) {
        fflush(s_logf);
        fsync(fileno(s_logf));
        s_last_flush = now;
    }
}

// static void logger_task(void *arg) {
//     log_item_t item; //sensor source tag + data 
//     while (1) {
//         if (xQueueReceive(g_log_queue, &item, portMAX_DELAY) == pdTRUE) {
//             char out[LOG_LINE_MAX + 8];
//             snprintf(out, sizeof(out), "%s,%s", src_tag(item.source), item.text);
//             sd_log_line(out);       
//         }

//     }
// }


static void logger_task(void *arg) {
    log_item_t item;
    while (1) {
        if (xQueueReceive(g_log_queue, &item, portMAX_DELAY) == pdTRUE) {
            // vTaskDelay(pdMS_TO_TICKS(1000));   // <-- artificially slow the consumer, for debugging
            UBaseType_t waiting = uxQueueMessagesWaiting(g_log_queue);
            ESP_LOGI(TAG, "logged (%u still queued)", (unsigned)waiting);
            
            char out[LOG_LINE_MAX + 8];
            snprintf(out, sizeof(out), "%s,%s", src_tag(item.source), item.text);
            sd_log_line(out);
        }
    }
}

void sd_logger_start(void) {
    xTaskCreate(logger_task, "logger", 4096, NULL, 4, NULL);
}
