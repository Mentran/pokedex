#pragma once

#include <stddef.h>
#include <stdint.h>

#define POKEDEX_SPRITE_W 80
#define POKEDEX_SPRITE_H 80
#define POKEDEX_SPRITE_BYTES (POKEDEX_SPRITE_W * POKEDEX_SPRITE_H * 2)
#define POKEDEX_SPRITE_PACK1 80
#define POKEDEX_CRY_RATE 8000

typedef struct {
    int pred;
    int index;
    uint32_t i;
    uint32_t total;
} pokedex_ima_st_t;

int pokedex_ima_begin(pokedex_ima_st_t *st, const uint8_t *src, size_t src_len);
int pokedex_ima_next(pokedex_ima_st_t *st, const uint8_t *src, size_t src_len,
                     int16_t *dst, int max_samples);
int pokedex_ima_decode(const uint8_t *src, size_t src_len,
                       int16_t *dst, size_t dst_samples);
