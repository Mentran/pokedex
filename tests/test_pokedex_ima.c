#include "pokedex_ima.h"

#include <stdio.h>
#include <string.h>

static uint8_t s_silence[] = {
    'I', 'M', 'A', 'D',
    0x40, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

int main(void)
{
    int16_t pcm[8];
    memset(pcm, 0x7F, sizeof(pcm));
    int n = pokedex_ima_decode(s_silence, sizeof(s_silence), pcm, 8);
    if (n != 8) {
        fprintf(stderr, "decode n=%d\n", n);
        return 1;
    }
    for (int i = 0; i < 8; i++) {
        if (pcm[i] != 0) {
            fprintf(stderr, "sample %d = %d\n", i, pcm[i]);
            return 1;
        }
    }
    pokedex_ima_st_t st;
    if (pokedex_ima_begin(&st, s_silence, sizeof(s_silence)) != 8) {
        fprintf(stderr, "begin failed\n");
        return 1;
    }
    int got = pokedex_ima_next(&st, s_silence, sizeof(s_silence), pcm, 3);
    if (got != 3 || st.i != 3) {
        fprintf(stderr, "stream got=%d i=%u\n", got, st.i);
        return 1;
    }
    return 0;
}
