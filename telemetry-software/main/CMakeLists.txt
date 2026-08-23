idf_component_register(SRCS
    "src/main.c"
    "src/GPS.c"
    "src/sd_log.c"
    "src/log_queue.c"
    "src/i2c.c"
    "src/imu.c"
    PRIV_REQUIRES spi_flash esp_driver_i2c fatfs sdmmc esp_timer
    INCLUDE_DIRS "inc"
)