#include "led_status.h"
#include "led_strip_encoder.h"

#include <stdbool.h>

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LED_STATUS_GPIO       21
#define LED_RMT_RESOLUTION_HZ 10000000   // 10MHz, 1 tick = 0.1us (WS2812 needs this precision)
#define LED_BLINK_PERIOD_MS   400

static const char *TAG = "led";
static rmt_channel_handle_t s_chan = NULL;
static rmt_encoder_handle_t s_encoder = NULL;
static TaskHandle_t s_blink_task = NULL;

// WS2812 pixel byte order is GRB, not RGB.
static void send_pixel(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t pixel[3] = { g, r, b };
    rmt_transmit_config_t tx_config = { .loop_count = 0 };
    rmt_transmit(s_chan, s_encoder, pixel, sizeof(pixel), &tx_config);
    rmt_tx_wait_all_done(s_chan, portMAX_DELAY);
}

static void blink_task(void *arg) {
    bool on = false;
    while (1) {
        on = !on;
        send_pixel(on ? 32 : 0, 0, 0);   // dim red, flashing
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_PERIOD_MS));
    }
}

esp_err_t led_status_init(void) {
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .gpio_num          = LED_STATUS_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz     = LED_RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &s_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_tx_channel failed: %s", esp_err_to_name(ret));
        return ret;
    }

    led_strip_encoder_config_t encoder_config = { .resolution = LED_RMT_RESOLUTION_HZ };
    ret = rmt_new_led_strip_encoder(&encoder_config, &s_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_led_strip_encoder failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = rmt_enable(s_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "rmt_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    send_pixel(0, 0, 0);   // start off
    return ESP_OK;
}

void led_status_ok(void) {
    if (s_blink_task) {
        vTaskDelete(s_blink_task);
        s_blink_task = NULL;
    }
    send_pixel(0, 32, 0);   // dim green, solid
}

void led_status_fail(void) {
    if (s_blink_task) return;   // already flashing
    xTaskCreate(blink_task, "led_blink", 2048, NULL, 3, &s_blink_task);
}
