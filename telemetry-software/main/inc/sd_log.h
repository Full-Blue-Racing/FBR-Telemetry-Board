#pragma once
#include "esp_err.h"
esp_err_t sd_init(void);
void sd_log_line(const char *line);
void sd_logger_start(void);
void sd_dump_file(const char *path);

#include <stdio.h>
#include <string.h>
#include <unistd.h>              // fsync
#include <sys/stat.h>           // stat (file-exists check)
#include "esp_vfs_fat.h"
#include "esp_timer.h"
#include "esp_log.h"

#define MOUNT_POINT   "/sdcard"
#define PARTITION_LABEL "storage"      // must match the "storage" entry in partitions.csv
#define FLUSH_US      (5   * 1000000LL)     // 5 s   in microseconds
#define ROLL_US       (3600 * 1000000LL)    // 1 hr  in microseconds
#define MAX_FILE_IDX  1000                  // gpslog_000 .. gpslog_999