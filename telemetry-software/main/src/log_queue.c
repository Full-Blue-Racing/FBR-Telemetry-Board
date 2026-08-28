// log_queue.c
#include "log_queue.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"


QueueHandle_t g_log_queue = NULL;
static const char *TAG = "logq";

void log_queue_init(void) {
    g_log_queue = xQueueCreate(LOG_QUEUE_DEPTH, sizeof(log_item_t));
    if (!g_log_queue){
         ESP_LOGE(TAG, "queue create failed");
    }else {
    ESP_LOGI(TAG, "queue creation successful");
    }
}

bool log_queue_push(log_source_t src, const char *line) {
    if (!g_log_queue) return false;
    log_item_t item;
    item.source = src;
    item.time = (uint32_t)(esp_timer_get_time() / 1000);   // ms since boot
    strncpy(item.text, line, LOG_LINE_MAX - 1);
    item.text[LOG_LINE_MAX - 1] = '\0';    // guarantee termination
    // 0 timeout: never block a sensor task waiting for queue space.
    return xQueueSend(g_log_queue, &item, 0) == pdTRUE;
}