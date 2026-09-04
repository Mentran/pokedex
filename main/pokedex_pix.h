#pragma once

#include <stdint.h>

#define POKEDEX_PIX_W 16
#define POKEDEX_PIX_H 16
#define POKEDEX_PIX_BYTES (POKEDEX_PIX_W * POKEDEX_PIX_H * 2)

enum {
    POKEDEX_PIX_GHOST = 0,
    POKEDEX_PIX_RHINO,
    POKEDEX_PIX_LIZARD,
    POKEDEX_PIX_TURTLE,
    POKEDEX_PIX_OAK,
    POKEDEX_PIX_ASH,
    POKEDEX_PIX_BROCK,
    POKEDEX_PIX_MISTY,
    POKEDEX_PIX_GARY,
};

void pokedex_pix_rgb565(int id, uint8_t *dst, uint32_t bg_rgb);
int pokedex_pix_speaker_id(int speaker);
