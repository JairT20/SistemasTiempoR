idf_component_register(SRCS "oneshot_read_main.c" "led.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_ledc esp_adc)