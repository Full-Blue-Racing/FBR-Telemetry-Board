#pragma once
#include "esp_err.h"
esp_err_t sd_init(void);
void sd_log_line(const char *line);
void sd_logger_start(void);

#include <stdio.h>
#include <string.h>
#include <unistd.h>              // fsync
#include <sys/stat.h>           // stat (file-exists check)
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_timer.h"
#include "esp_log.h"

#define SD_SPI_CS 9
#define SD_SPI_MOSI 8
#define SD_SPI_MISO 18
#define SD_SPI_SCK 17

#define MOUNT_POINT   "/sdcard"
#define FLUSH_US      (5   * 1000000LL)     // 5 s   in microseconds
#define ROLL_US       (3600 * 1000000LL)    // 1 hr  in microseconds
#define MAX_FILE_IDX  1000                  // gpslog_000 .. gpslog_999