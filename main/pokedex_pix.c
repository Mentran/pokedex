// pokedex_pix.c —— 开机动画和大木讲堂用的原创 16x16 像素。
#include "pokedex_pix.h"

#include <string.h>

static uint16_t hex565(uint32_t hex)
{
    unsigned r = (hex >> 16) & 0xffu;
    unsigned g = (hex >> 8) & 0xffu;
    unsigned b = hex & 0xffu;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static const char *const GHOST[] = {
    "..kkkkkkkkkk....",
    ".kppppddppppk...",
    "kppdppddppdppk..",
    "kppwwkddkwwppk..",
    "kppwwkddkwwppk..",
    "kppppddddppppk..",
    "kppkwwwwwwkppk..",
    "kppkwkwkwkpppk..",
    "kpppkkkkkkpppk..",
    ".kppppppppppk...",
    ".kppkppppkppk...",
    "..kk.kkkk.kk....",
    "................",
    "................",
    "................",
    "................",
};

static const char *const RHINO[] = {
    "....ee.ee.......",
    "...ekkekke......",
    "..ekwwkwwke.....",
    "..ekwkwkwke.h...",
    ".ekkkkkkkkehkk..",
    ".ekmmmmmmmke.k..",
    ".ekmmkmmmmke....",
    "..ekmmmmmke.....",
    "..ekmmkmmke.....",
    "...ekkkkke......",
    "...ek..ke.......",
    "....k..k........",
    "................",
    "................",
    "................",
    "................",
};

static const char *const LIZARD[] = {
    "....rr......t...",
    "...ryrr....trt..",
    "..ryykrr..trrt..",
    ".ryyrrrr.rrr....",
    ".rykrrrrrrr.....",
    "..rrrrrrrrr.....",
    "...rrkrrrr......",
    "...rrrrr.r......",
    "...r..r..r......",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
};

static const char *const TURTLE[] = {
    "......cc........",
    ".....cbcc.......",
    "....cbbbbc......",
    "...cbbkbbbc.....",
    "..ccbbbbbcc.....",
    ".cwwcbbbcwwc....",
    ".cwwcccccwwc....",
    "..cccccccc......",
    "...cc..cc.......",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
};

static const char *const OAK[] = {
    "....hhhhhh......",
    "...hssssssh.....",
    "..hsswwwwssh....",
    "..hswkwkwksh....",
    "..hsswwwwssh....",
    "...hssssssh.....",
    "....s.ss.s......",
    "...wwwwwwww.....",
    "..w.......w.....",
    "..w.wwwww.w.....",
    "..w.w...w.w.....",
    "...wwwwwwww.....",
    "....w....w......",
    "................",
    "................",
    "................",
};

static const char *const ASH[] = {
    "....kkkkkk......",
    "...krrrrrrk.....",
    "..krrwwwwrrk....",
    "..krwkwkwkrk....",
    "...kwwwwwk......",
    "....ssssss......",
    "...s.ssss.s.....",
    "..rrrrrrrrrr....",
    "..r.wwwwww.r....",
    "..r.w....w.r....",
    "...wwwwwwww.....",
    "....b....b......",
    "................",
    "................",
    "................",
    "................",
};

static const char *const BROCK[] = {
    "....eeeeee......",
    "...eeeeeeee.....",
    "..eewwwwwwee....",
    "..eewkwkwkee....",
    "...ewwwwwwe.....",
    "....ssssss......",
    "...oooooooo.....",
    "..o.oooooo.o....",
    "..o.o....o.o....",
    "...oooooooo.....",
    "....n....n......",
    "....n....n......",
    "................",
    "................",
    "................",
    "................",
};

static const char *const MISTY[] = {
    "....oooooo......",
    "...oooooooo.....",
    "..oowwwwwwoo....",
    "..oowkwkwoo.....",
    "...owwwwwwo.....",
    "....ssssss......",
    "...yyyyyyyy.....",
    "..y.yyyyyy.y....",
    "...yccccccy.....",
    "....cccccc......",
    "....c....c......",
    "................",
    "................",
    "................",
    "................",
    "................",
};

static const char *const GARY[] = {
    "....hhhhhh......",
    "...hwwwwwh......",
    "...hwkwkwh......",
    "....wwwwww......",
    "....ssssss......",
    "...pppppppp.....",
    "..p.pppppp.p....",
    "..p.p....p.p....",
    "...pppppppp.....",
    "....n....n......",
    "....n....n......",
    "................",
    "................",
    "................",
    "................",
    "................",
};

typedef struct {
    const char *const *rows;
    uint32_t pal[128];
} pix_def_t;

static pix_def_t def(int id)
{
    pix_def_t d;
    memset(&d, 0, sizeof(d));
    switch (id) {
    case POKEDEX_PIX_GHOST:
        d.rows = GHOST;
        d.pal['k'] = 0x2A1830;
        d.pal['p'] = 0x6B3A86;
        d.pal['d'] = 0x4A2560;
        d.pal['w'] = 0xF5F0E8;
        break;
    case POKEDEX_PIX_RHINO:
        d.rows = RHINO;
        d.pal['e'] = 0x5A3060;
        d.pal['k'] = 0x2A1830;
        d.pal['w'] = 0xF5F0E8;
        d.pal['m'] = 0xC989B0;
        d.pal['h'] = 0xD8D0C0;
        break;
    case POKEDEX_PIX_LIZARD:
        d.rows = LIZARD;
        d.pal['r'] = 0xD06030;
        d.pal['y'] = 0xF0C040;
        d.pal['t'] = 0xE07020;
        d.pal['k'] = 0x2A1830;
        break;
    case POKEDEX_PIX_TURTLE:
        d.rows = TURTLE;
        d.pal['c'] = 0x4AA0C8;
        d.pal['b'] = 0x3D7A4A;
        d.pal['k'] = 0x2A1830;
        d.pal['w'] = 0xF5F0E8;
        break;
    case POKEDEX_PIX_OAK:
        d.rows = OAK;
        d.pal['h'] = 0x6B4A2A;
        d.pal['s'] = 0xE8C8A0;
        d.pal['w'] = 0xF5F5F0;
        d.pal['k'] = 0x2A1830;
        break;
    case POKEDEX_PIX_ASH:
        d.rows = ASH;
        d.pal['k'] = 0x2A1830;
        d.pal['r'] = 0xC62828;
        d.pal['w'] = 0xF5E6C8;
        d.pal['s'] = 0xE8C8A0;
        d.pal['b'] = 0x1565C0;
        break;
    case POKEDEX_PIX_BROCK:
        d.rows = BROCK;
        d.pal['e'] = 0x5D4037;
        d.pal['w'] = 0xF5E6C8;
        d.pal['k'] = 0x2A1830;
        d.pal['s'] = 0xE8C8A0;
        d.pal['o'] = 0xEF6C00;
        d.pal['n'] = 0x5D4037;
        break;
    case POKEDEX_PIX_MISTY:
        d.rows = MISTY;
        d.pal['o'] = 0xEF6C00;
        d.pal['w'] = 0xF5E6C8;
        d.pal['k'] = 0x2A1830;
        d.pal['s'] = 0xE8C8A0;
        d.pal['y'] = 0xFFD54F;
        d.pal['c'] = 0x4FC3F7;
        break;
    default:
        d.rows = GARY;
        d.pal['h'] = 0x5D4037;
        d.pal['w'] = 0xF5E6C8;
        d.pal['k'] = 0x2A1830;
        d.pal['s'] = 0xE8C8A0;
        d.pal['p'] = 0x7E57C2;
        d.pal['n'] = 0x3949AB;
        break;
    }
    return d;
}

void pokedex_pix_rgb565(int id, uint8_t *dst, uint32_t bg_rgb)
{
    pix_def_t d = def(id);
    uint16_t bg = hex565(bg_rgb);
    for (int y = 0; y < POKEDEX_PIX_H; y++) {
        const char *row = d.rows[y];
        for (int x = 0; x < POKEDEX_PIX_W; x++) {
            char ch = row[x];
            uint16_t px = (ch != '.' && d.pal[(unsigned char)ch])
                ? hex565(d.pal[(unsigned char)ch])
                : bg;
            memcpy(dst + (y * POKEDEX_PIX_W + x) * 2, &px, 2);
        }
    }
}

int pokedex_pix_speaker_id(int speaker)
{
    switch (speaker) {
    case 1:
        return POKEDEX_PIX_ASH;
    case 2:
        return POKEDEX_PIX_BROCK;
    case 3:
        return POKEDEX_PIX_MISTY;
    case 4:
        return POKEDEX_PIX_GARY;
    default:
        return POKEDEX_PIX_OAK;
    }
}
