#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>

#define LOG_LINE_MAX 128
#define LOG_QUEUE_DEPTH 32     // how many lines can back up before drops

typedef enum {
    LOG_SRC_GPS = 0,
    LOG_SRC_IMU = 1,        // your future sensor
    // add more here
} log_source_t;

typedef struct {
    uint32_t time;
    log_source_t source;
    char text[LOG_LINE_MAX];
} log_item_t;

extern QueueHandle_t g_log_queue;

void log_queue_init(void);
// Non-blocking enqueue; returns false if the queue is full (line dropped).
bool log_queue_push(log_source_t src, const char *line);