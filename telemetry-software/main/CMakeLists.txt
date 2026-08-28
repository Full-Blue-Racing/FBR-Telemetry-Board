idf_component_register(SRCS
    "src/main.c"
    "src/GPS.c"
    "src/sd_log.c"
    "src/log_queue.c"
    "src/i2c.c"
    "src/imu.c"
    "src/console.c"
    "src/led_strip_encoder.c"
    "src/led_status.c"
    PRIV_REQUIRES spi_flash esp_driver_i2c fatfs esp_timer wear_levelling console esp_driver_gpio esp_driver_rmt
    INCLUDE_DIRS "inc"
)