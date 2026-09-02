// pokedex_ima.c —— IMA-ADPCM 解码,与 tools/pokedex/build_media.py 的 IMAD 打包对应。
#include "pokedex_ima.h"

#include <string.h>

static const int s_step[] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767,
};

static const int s_index[] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static int clamp16(int v)
{
    if (v < -32768) {
        return -32768;
    }
    if (v > 32767) {
        return 32767;
    }
    return v;
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int pokedex_ima_begin(pokedex_ima_st_t *st, const uint8_t *src, size_t src_len)
{
    if (!st || !src || src_len < 16 || memcmp(src, "IMAD", 4) != 0) {
        return -1;
    }
    st->pred = 0;
    st->index = 0;
    st->i = 0;
    st->total = le32(src + 12);
    return (int)st->total;
}

int pokedex_ima_next(pokedex_ima_st_t *st, const uint8_t *src, size_t src_len,
                     int16_t *dst, int max_samples)
{
    if (!st || !src || !dst || max_samples <= 0 || src_len < 16) {
        return -1;
    }
    const uint8_t *packed = src + 16;
    size_t packed_len = src_len - 16;
    int n = 0;
    while (n < max_samples && st->i < st->total) {
        size_t byte_i = st->i / 2;
        if (byte_i >= packed_len) {
            break;
        }
        int nibble = (st->i & 1) ? (packed[byte_i] >> 4) : (packed[byte_i] & 0x0F);
        int step = s_step[st->index];
        int delta = step >> 3;
        if (nibble & 4) {
            delta += step;
        }
        if (nibble & 2) {
            delta += step >> 1;
        }
        if (nibble & 1) {
            delta += step >> 2;
        }
        st->pred = (nibble & 8) ? st->pred - delta : st->pred + delta;
        st->pred = clamp16(st->pred);
        st->index += s_index[nibble & 7];
        if (st->index < 0) {
            st->index = 0;
        }
        if (st->index > 88) {
            st->index = 88;
        }
        dst[n++] = (int16_t)st->pred;
        st->i++;
    }
    return n;
}

int pokedex_ima_decode(const uint8_t *src, size_t src_len,
                       int16_t *dst, size_t dst_samples)
{
    pokedex_ima_st_t st;
    if (pokedex_ima_begin(&st, src, src_len) < 0) {
        return -1;
    }
    if (st.total > dst_samples) {
        st.total = (uint32_t)dst_samples;
    }
    return pokedex_ima_next(&st, src, src_len, dst, (int)st.total);
}
