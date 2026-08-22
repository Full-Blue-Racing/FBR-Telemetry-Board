idf_component_register(SRCS "src/main.c"
                       PRIV_REQUIRES spi_flash esp_driver_i2c
                       INCLUDE_DIRS "inc")