idf_component_register(SRCS "sprite.c" "pico-celeste.c" "classiceleste_bg.c" "classic.c" "pico8sim.c" "tilemap.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_lcd_st6201 libfixmath)
