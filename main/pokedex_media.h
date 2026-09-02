#pragma once

#include "lvgl.h"
#include <stdbool.h>

bool pokedex_media_init(void);
void pokedex_media_deinit(void);
bool pokedex_media_load_sprite(int id, uint8_t *rgb565, lv_image_dsc_t *dsc);
void pokedex_media_play_cry(int id);
void pokedex_media_stop_cry(void);
