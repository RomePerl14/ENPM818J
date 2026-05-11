idf_component_register(SRCS "the_main_thingy.c"
                       PRIV_REQUIRES esp_driver_pcnt esp_driver_gpio esp_driver_ledc
                       INCLUDE_DIRS ".")
