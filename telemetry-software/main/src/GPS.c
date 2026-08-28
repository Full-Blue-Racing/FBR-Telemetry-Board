#include "GPS.h"
#include "main.h"
#include "log_queue.h"
#include "i2c.h"

static const char *TAG = "gps";
static i2c_master_dev_handle_t s_gps;
i2c_master_bus_handle_t bus;
static char acc[ACC_SIZE];
static size_t acc_len = 0;

/**
 * NMEA HELPERS 
 */
bool valid_checksum(const char *s) {
    const char *star = strchr(s, '*');
    if (!star) return false;
    uint8_t calc = 0;
    for (const char *p = s + 1; p < star; p++) calc ^= (uint8_t)*p;
    unsigned int given;
    if (sscanf(star + 1, "%2x", &given) != 1) return false;
    return calc == (uint8_t)given;
}


// Split on commas IN PLACE, preserving empty fields. Returns field count.
int split_fields(char *s, char *fields[], int max_fields) {
    int n = 0;
    fields[n++] = s;
    for (char *p = s; *p && n < max_fields; p++) {
        if (*p == ',') { *p = '\0'; fields[n++] = p + 1; }
    }
    return n;
}


// ddmm.mmmm -> decimal degrees. Anchors on the decimal point:
// the two digits before it are the minutes' integer part.
double nmea_to_deg(const char *raw, char hemi) {
    if (!raw || !*raw) return NAN;
    const char *dot = strchr(raw, '.');
    if (!dot || (dot - raw) < 3) return NAN;
    int deg_len = (int)(dot - raw) - 2;
    char degbuf[8];
    if (deg_len <= 0 || deg_len >= (int)sizeof(degbuf)) return NAN;
    memcpy(degbuf, raw, deg_len);
    degbuf[deg_len] = '\0';
    double val = atof(degbuf) + atof(raw + deg_len) / 60.0;
    if (hemi == 'S' || hemi == 'W') val = -val;
    return val;
}

void parse_gga(char *f[], int n) {
    // 0:$..GGA 1:time 2:lat 3:N/S 4:lon 5:E/W 6:qual 7:sats 8:hdop 9:alt 10:M
    if (n < 10) return;
    if (f[6][0] == '\0' || f[6][0] == '0') { ESP_LOGI(TAG, "no fix"); return; }
    double lat = nmea_to_deg(f[2], f[3][0]);
    double lon = nmea_to_deg(f[4], f[5][0]);
    double alt = f[9][0] ? atof(f[9]) : 0.0;
    int   sats = f[7][0] ? atoi(f[7]) : 0;
    ESP_LOGI(TAG, "lat=%.6f lon=%.6f alt=%.1fm sats=%d", lat, lon, alt, sats);
}



/** **************************************************** */

void process_line(char *line) {
    if (line[0] != '$') return;
    if (!valid_checksum(line)) return;
   
    // char *star = strchr(line, '*');
    // if (star) *star = '\0';  

    // char *fields[24];
    // int nf = split_fields(line, fields, 24);
    // if (nf < 1) return;
    // size_t l0 = strlen(fields[0]);
    // if (l0 < 3) return;
    if (strncmp(line + 3, "RMC", 3) == 0 || strncmp(line + 3, "GGA", 3) == 0){
        ESP_LOGD(TAG, "%s", line);
        log_queue_push(LOG_SRC_GPS, line);
    }

}

// Feed a raw I2C chunk into the accumulator, emitting complete lines.
void feed(const uint8_t *chunk, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint8_t b = chunk[i];
        if (b == '\n') {                  // line end AND bare padding both land here
            acc[acc_len] = '\0';
            process_line(acc);            // ignores anything not starting with '$'
            acc_len = 0;
        } else if (b == '\r') {
            // skip
        } else if (acc_len < ACC_SIZE - 1) {
            acc[acc_len++] = (char)b;
        } else {
            acc_len = 0;                  // overflow guard: drop the runaway line
        }
    }
}


void gps_task(void *arg) {
    uint8_t chunk[32];
    while (1) {
        if (i2c_master_receive(s_gps, chunk, sizeof(chunk),
                               pdMS_TO_TICKS(100)) == ESP_OK) {
            feed(chunk, sizeof(chunk));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}



bool gps_start(void){
    bus = i2c_bus_get_handle();
        // Scan equivalent: probe returns ESP_OK if 0x10 ACKs.
    bool found = (i2c_master_probe(bus, PA1010D_ADDR, pdMS_TO_TICKS(100)) == ESP_OK);
    if (found)
        ESP_LOGI(TAG, "PA1010D found at 0x10");
    else
        ESP_LOGE(TAG, "no ACK at 0x10 - check wiring");


    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PA1010D_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &s_gps);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "add_device failed: %s", esp_err_to_name(ret));
        return false;
    }

    xTaskCreate(gps_task, "gps", 4096, NULL, 5, NULL);
    return found;
}