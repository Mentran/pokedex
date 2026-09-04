// demo_pokedex.c —— 开机官方立绘短动画 + 手持图鉴外壳 + 封面立绘 + 叫声。按键语义仍走 pokedex.c。
#include "demo.h"
#include "pokedex.h"
#include "pokedex_ima.h"
#include "pokedex_media.h"
#include "font_pokedex_16.h"
#include "bsp_battery.h"
#include "esp_random.h"
#include "lvgl.h"

#include <stdio.h>
#include <string.h>

#define FONT_ZH (&font_pokedex_16)

#define DEX_RED       0xD32F2F
#define DEX_RED_DARK  0x8B0000
#define DEX_RED_DEEP  0x4A0000
#define DEX_NAVY      0x1A237E
#define DEX_LCD       0xC8E6C9
#define DEX_INK       0x1B5E20
#define DEX_INK_DIM   0x33691E
#define DEX_YELLOW    0xFBC02D
#define DEX_BLUE      0x1565C0
#define DEX_WHITE     0xF5F5F0
#define DEX_BLACK     0x212121
#define DEX_CHIP_INK  0xFFFFFF

static pokedex_state_t s_state;
static lv_obj_t *s_scr;
static lv_obj_t *s_home_cards[POKEDEX_HOME_COUNT];
static uint8_t s_sprite_rgb[POKEDEX_SPRITE_BYTES];
static uint8_t s_sprite_rgb2[POKEDEX_SPRITE_BYTES];
static lv_image_dsc_t s_sprite_dsc;
static lv_image_dsc_t s_sprite_dsc2;
static int s_cry_for_id;
static lv_obj_t *s_matchup_scroll;
static int s_boot_on;
static int s_boot_frame;
static lv_timer_t *s_boot_timer;
static lv_obj_t *s_boot_left;
static lv_obj_t *s_boot_right;
static lv_obj_t *s_boot_shadow_l;
static lv_obj_t *s_boot_shadow_r;
static lv_obj_t *s_boot_flash;

/* 红蓝开场节奏，站位跟封面立绘朝向：耿鬼在左往右扑，尼多力诺在右往左冲。 */
#define BOOT_BG        0xEDEBE4
#define BOOT_FLOOR     0xD8D4CA
#define BOOT_FRAMES    24
#define BOOT_MS        170
#define BOOT_GX        40
#define BOOT_GY        124
#define BOOT_NX        120
#define BOOT_NY        132
#define BOOT_SHADOW_GY 210
#define BOOT_SHADOW_NY 210
#define BOOT_SHADOW_GX 14
#define BOOT_SHADOW_NX 12

typedef struct {
    int16_t gx;
    int16_t gy;
    int16_t nx;
    int16_t ny;
    uint16_t gscale;
    uint16_t nscale;
    uint8_t flash;
} boot_pose_t;

static const boot_pose_t s_boot_pose[BOOT_FRAMES] = {
    /* 0-4 入场：按立绘重心落到地面，不要贴边 */
    { -80, BOOT_GY, 240, BOOT_NY, 256, 256, 0 },
    { -16, BOOT_GY, 196, BOOT_NY, 256, 256, 0 },
    {  16, BOOT_GY, 160, BOOT_NY, 256, 256, 0 },
    {  32, BOOT_GY, 136, BOOT_NY, 256, 256, 0 },
    { BOOT_GX, BOOT_GY, BOOT_NX, BOOT_NY, 256, 256, 0 },
    /* 5-9 hip / hop，尼多力诺跳得更高 */
    { BOOT_GX, BOOT_GY - 4, BOOT_NX, BOOT_NY - 14, 256, 256, 0 },
    { BOOT_GX, BOOT_GY,     BOOT_NX, BOOT_NY,      256, 256, 0 },
    { BOOT_GX, BOOT_GY - 5, BOOT_NX, BOOT_NY - 16, 256, 256, 0 },
    { BOOT_GX, BOOT_GY,     BOOT_NX, BOOT_NY,      256, 256, 0 },
    { BOOT_GX, BOOT_GY - 2, BOOT_NX, BOOT_NY - 6,  256, 256, 0 },
    /* 10-13 耿鬼后撤蓄力 */
    { BOOT_GX - 4, BOOT_GY - 2, BOOT_NX, BOOT_NY,     268, 256, 0 },
    { BOOT_GX - 8, BOOT_GY - 4, BOOT_NX, BOOT_NY + 2, 276, 256, 0 },
    { BOOT_GX - 8, BOOT_GY - 4, BOOT_NX, BOOT_NY,     276, 256, 0 },
    { BOOT_GX - 8, BOOT_GY - 4, BOOT_NX, BOOT_NY,     276, 256, 0 },
    /* 14-16 闪白，耿鬼右冲，尼多力诺向左上跳开 */
    { BOOT_GX + 14, BOOT_GY + 8,  BOOT_NX,      BOOT_NY,      256, 256, 255 },
    { BOOT_GX + 26, BOOT_GY + 16, BOOT_NX - 12, BOOT_NY - 20, 248, 248, 200 },
    { BOOT_GX + 30, BOOT_GY + 18, BOOT_NX - 20, BOOT_NY - 32, 248, 240,  48 },
    /* 17-19 落地再轻轻一跳 */
    { BOOT_GX + 14, BOOT_GY + 8,  BOOT_NX - 8, BOOT_NY - 10, 256, 256, 0 },
    { BOOT_GX + 4,  BOOT_GY - 4,  BOOT_NX,     BOOT_NY,      256, 256, 0 },
    { BOOT_GX,      BOOT_GY,      BOOT_NX,     BOOT_NY,      256, 256, 0 },
    /* 20-23 淡出到白 */
    { BOOT_GX, BOOT_GY, BOOT_NX, BOOT_NY, 256, 256,  70 },
    { BOOT_GX, BOOT_GY, BOOT_NX, BOOT_NY, 256, 256, 140 },
    { BOOT_GX, BOOT_GY, BOOT_NX, BOOT_NY, 256, 256, 210 },
    { BOOT_GX, BOOT_GY, BOOT_NX, BOOT_NY, 256, 256, 255 },
};

static const char *s_home_titles[POKEDEX_HOME_COUNT] = {
    "图鉴浏览",
    "随机遇见",
    "猜猜我是谁",
    "大木讲堂",
};

static const char *s_stat_name[] = {
    "HP", "ATK", "DEF", "SPA", "SPD", "SPE",
};

typedef struct {
    const char *name;
    uint32_t color;
} dex_type_t;

static const dex_type_t s_types[] = {
    { "一般", 0xA8A878 }, { "火", 0xF08030 }, { "水", 0x6890F0 },
    { "电", 0xF8D030 }, { "草", 0x78C850 }, { "冰", 0x98D8D8 },
    { "格斗", 0xC03028 }, { "毒", 0xA040A0 }, { "地面", 0xE0C068 },
    { "飞行", 0xA890F0 }, { "超能力", 0xF85888 }, { "虫", 0xA8B820 },
    { "岩石", 0xB8A038 }, { "幽灵", 0x705898 }, { "龙", 0x7038F8 },
    { "恶", 0x705848 }, { "钢", 0xB8B8D0 }, { "妖精", 0xEE99AC },
};

bool demo_pokedex_is_home(void)
{
    return !s_boot_on && pokedex_is_home(&s_state);
}

static uint32_t type_color(const char *name)
{
    if (!name || !name[0]) {
        return 0x888888;
    }
    for (size_t i = 0; i < sizeof(s_types) / sizeof(s_types[0]); i++) {
        if (strcmp(s_types[i].name, name) == 0) {
            return s_types[i].color;
        }
    }
    return 0x607D8B;
}

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h,
                     uint32_t color, int radius)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    return obj;
}

static lv_obj_t *label_at(lv_obj_t *parent, const char *text, const lv_font_t *font,
                          uint32_t color, int x, int y)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_label_set_text(obj, text ? text : "");
    lv_obj_set_pos(obj, x, y);
    return obj;
}

static lv_obj_t *zh_at(lv_obj_t *parent, const char *text, uint32_t color, int x, int y)
{
    return label_at(parent, text, FONT_ZH, color, x, y);
}

static uint16_t hex565(uint32_t hex)
{
    uint32_t r = (hex >> 16) & 0xFF;
    uint32_t g = (hex >> 8) & 0xFF;
    uint32_t b = hex & 0xFF;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static int is_lcd_fill(uint16_t p)
{
    uint32_t r = (p >> 11) & 31;
    uint32_t g = (p >> 5) & 63;
    uint32_t b = p & 31;
    return r >= 22 && r <= 27 && g >= 52 && g <= 60 && b >= 22 && b <= 27;
}

static void rekey_lcd_fill(uint8_t *buf, uint32_t to_hex)
{
    uint16_t to = hex565(to_hex);
    uint16_t *p = (uint16_t *)buf;
    int n = POKEDEX_SPRITE_W * POKEDEX_SPRITE_H;
    for (int i = 0; i < n; i++) {
        if (is_lcd_fill(p[i])) {
            p[i] = to;
        }
    }
}

static lv_obj_t *dex_img(lv_obj_t *parent, lv_image_dsc_t *dsc, int x, int y)
{
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, dsc);
    lv_image_set_antialias(img, false);
    lv_obj_set_pos(img, x, y);
    return img;
}

static lv_obj_t *boot_poke(lv_obj_t *parent, uint8_t *buf, lv_image_dsc_t *dsc,
                           int id, int x, int y, uint32_t key)
{
    if (!pokedex_media_load_sprite(id, buf, dsc)) {
        return NULL;
    }
    rekey_lcd_fill(buf, key);
    return dex_img(parent, dsc, x, y);
}

static uint8_t s_sil_mark[(POKEDEX_SPRITE_W * POKEDEX_SPRITE_H + 7) / 8];

static int sil_get(int i)
{
    return (s_sil_mark[i >> 3] >> (i & 7)) & 1;
}

static void sil_set(int i)
{
    s_sil_mark[i >> 3] |= (uint8_t)(1u << (i & 7));
}

static void sprite_silhouette(uint8_t *buf)
{
    uint16_t *p = (uint16_t *)buf;
    const int w = POKEDEX_SPRITE_W;
    const int h = POKEDEX_SPRITE_H;
    const int n = w * h;
    uint16_t ink = 0;
    int x, y, i, changed;

    memset(s_sil_mark, 0, sizeof(s_sil_mark));
    for (x = 0; x < w; x++) {
        if (is_lcd_fill(p[x])) {
            sil_set(x);
        }
        if (is_lcd_fill(p[(h - 1) * w + x])) {
            sil_set((h - 1) * w + x);
        }
    }
    for (y = 0; y < h; y++) {
        if (is_lcd_fill(p[y * w])) {
            sil_set(y * w);
        }
        if (is_lcd_fill(p[y * w + w - 1])) {
            sil_set(y * w + w - 1);
        }
    }

    changed = 1;
    while (changed) {
        changed = 0;
        for (i = 0; i < n; i++) {
            int nx, ny;
            if (sil_get(i) || !is_lcd_fill(p[i])) {
                continue;
            }
            x = i % w;
            y = i / w;
            nx = x > 0 && sil_get(i - 1);
            ny = x + 1 < w && sil_get(i + 1);
            if (nx || ny || (y > 0 && sil_get(i - w)) || (y + 1 < h && sil_get(i + w))) {
                sil_set(i);
                changed = 1;
            }
        }
    }

    for (i = 0; i < n; i++) {
        if (!sil_get(i)) {
            p[i] = ink;
        }
    }
}

static void wipe(void)
{
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
        for (int i = 0; i < POKEDEX_HOME_COUNT; i++) {
            s_home_cards[i] = NULL;
        }
        s_matchup_scroll = NULL;
        s_boot_left = NULL;
        s_boot_right = NULL;
        s_boot_shadow_l = NULL;
        s_boot_shadow_r = NULL;
        s_boot_flash = NULL;
    }
}

static void add_led(lv_obj_t *parent, int x, int y, uint32_t color)
{
    lv_obj_t *led = box(parent, x, y, 14, 14, color, LV_RADIUS_CIRCLE);
    lv_obj_set_style_border_width(led, 2, 0);
    lv_obj_set_style_border_color(led, lv_color_hex(DEX_RED_DEEP), 0);
}

static lv_obj_t *make_shell(void)
{
    wipe();
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DEX_RED), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    box(s_scr, 0, 0, 240, 40, DEX_RED_DARK, 0);
    add_led(s_scr, 10, 12, 0xFF1744);
    add_led(s_scr, 28, 12, 0xFFD600);
    add_led(s_scr, 46, 12, 0x00E676);
    label_at(s_scr, "POKeDEX", &lv_font_montserrat_20, DEX_WHITE, 70, 8);

    int soc = bsp_battery_soc();
    if (soc >= 0) {
        lv_obj_t *bat = label_at(s_scr, "", &lv_font_montserrat_14, DEX_WHITE, 188, 10);
        lv_label_set_text_fmt(bat, "%d%%", soc);
    }

    box(s_scr, 0, 40, 240, 10, DEX_RED_DEEP, 0);
    box(s_scr, 52, 38, 28, 14, DEX_BLACK, LV_RADIUS_CIRCLE);
    box(s_scr, 160, 38, 28, 14, DEX_BLACK, LV_RADIUS_CIRCLE);

    box(s_scr, 8, 52, 224, 214, DEX_NAVY, 8);
    lv_obj_t *lcd = box(s_scr, 16, 60, 208, 198, DEX_LCD, 4);

    box(s_scr, 0, 268, 240, 52, DEX_RED_DARK, 0);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            box(s_scr, 16 + j * 14, 280 + i * 12, 8, 8, DEX_BLACK, LV_RADIUS_CIRCLE);
        }
    }
    box(s_scr, 186, 276, 36, 36, DEX_BLUE, LV_RADIUS_CIRCLE);
    box(s_scr, 196, 286, 16, 16, 0x42A5F5, LV_RADIUS_CIRCLE);
    return lcd;
}

static void add_tabs(lv_obj_t *lcd, int active)
{
    int n = POKEDEX_TAB_COUNT;
    int total = n * 10 + (n - 1) * 6;
    int x = (208 - total) / 2;
    for (int i = 0; i < n; i++) {
        uint32_t color = (i == active) ? DEX_YELLOW : 0x81C784;
        int size = (i == active) ? 10 : 8;
        int oy = (i == active) ? 184 : 185;
        box(lcd, x, oy, size, size, color, LV_RADIUS_CIRCLE);
        x += 16;
    }
}

static void add_type_chip_at(lv_obj_t *parent, const char *name, int x, int y, int w, int h)
{
    if (!name || !name[0]) {
        return;
    }
    lv_obj_t *chip = box(parent, x, y, w, h, type_color(name), 4);
    lv_obj_t *text = zh_at(chip, name, DEX_CHIP_INK, 0, (h > 18) ? 1 : 0);
    lv_obj_set_width(text, w);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
}

static void add_type_chip(lv_obj_t *parent, const char *name, int x, int y)
{
    add_type_chip_at(parent, name, x, y, 56, 20);
}

static void paint_home_sel(void)
{
    for (int i = 0; i < POKEDEX_HOME_COUNT; i++) {
        if (!s_home_cards[i]) {
            continue;
        }
        bool on = s_state.home_sel == i;
        lv_obj_set_style_border_width(s_home_cards[i], on ? 3 : 1, 0);
        lv_obj_set_style_border_color(
            s_home_cards[i],
            lv_color_hex(on ? DEX_YELLOW : DEX_INK), 0);
        lv_obj_set_style_bg_color(
            s_home_cards[i],
            lv_color_hex(on ? 0xFFF59D : 0xA5D6A7), 0);
    }
}

static void show_home(void);
static void show_fact(void);
static void show_entry(void);
static void render(void);
static void boot_finish(void);

static lv_obj_t *boot_screen(void)
{
    wipe();
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(BOOT_BG), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);
    return s_scr;
}

static lv_obj_t *boot_shadow(lv_obj_t *parent, int x, int y, int w, int h, lv_opa_t opa)
{
    lv_obj_t *obj = box(parent, x, y, w, h, 0x6B6148, LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    return obj;
}

static void boot_place(lv_obj_t *img, int x, int y, uint16_t scale)
{
    if (!img) {
        return;
    }
    lv_obj_set_pos(img, x, y);
    lv_image_set_scale(img, scale);
}

static void apply_boot_pose(int frame)
{
    const boot_pose_t *p = &s_boot_pose[frame];
    int air_l = BOOT_GY - p->gy;
    int air_r = BOOT_NY - p->ny;
    if (air_l < 0) {
        air_l = 0;
    }
    if (air_r < 0) {
        air_r = 0;
    }

    boot_place(s_boot_left, p->gx, p->gy, p->gscale);
    boot_place(s_boot_right, p->nx, p->ny, p->nscale);

    if (s_boot_shadow_l) {
        int w = 56 - air_l / 2;
        int opa = 48 - air_l;
        if (w < 28) {
            w = 28;
        }
        if (opa < 16) {
            opa = 16;
        }
        lv_obj_set_pos(s_boot_shadow_l, p->gx + BOOT_SHADOW_GX, BOOT_SHADOW_GY);
        lv_obj_set_width(s_boot_shadow_l, w);
        lv_obj_set_style_bg_opa(s_boot_shadow_l,
                                (p->gx + POKEDEX_SPRITE_W < 8) ? LV_OPA_TRANSP : (lv_opa_t)opa, 0);
    }
    if (s_boot_shadow_r) {
        int w = 64 - air_r / 2;
        int opa = 90 - air_r;
        if (w < 32) {
            w = 32;
        }
        if (opa < 24) {
            opa = 24;
        }
        lv_obj_set_pos(s_boot_shadow_r, p->nx + BOOT_SHADOW_NX, BOOT_SHADOW_NY);
        lv_obj_set_width(s_boot_shadow_r, w);
        lv_obj_set_style_bg_opa(s_boot_shadow_r,
                                (p->nx > 220) ? LV_OPA_TRANSP : (lv_opa_t)opa, 0);
    }
    if (s_boot_flash) {
        lv_obj_set_style_bg_opa(s_boot_flash, p->flash, 0);
    }
}

static void boot_paint_field(lv_obj_t *scr)
{
    box(scr, 20, 204, 200, 18, BOOT_FLOOR, LV_RADIUS_CIRCLE);
}

static void show_boot(void)
{
    lv_obj_t *scr = boot_screen();
    boot_paint_field(scr);
    s_boot_shadow_l = boot_shadow(scr, BOOT_GX + BOOT_SHADOW_GX, BOOT_SHADOW_GY, 56, 12, 48);
    s_boot_shadow_r = boot_shadow(scr, BOOT_NX + BOOT_SHADOW_NX, BOOT_SHADOW_NY, 64, 14, 90);
    s_boot_left = boot_poke(scr, s_sprite_rgb, &s_sprite_dsc, 94,
                            BOOT_GX, BOOT_GY, BOOT_BG);
    s_boot_right = boot_poke(scr, s_sprite_rgb2, &s_sprite_dsc2, 33,
                             BOOT_NX, BOOT_NY, BOOT_BG);
    if (s_boot_left) {
        lv_image_set_pivot(s_boot_left, POKEDEX_SPRITE_W / 2, POKEDEX_SPRITE_H);
        lv_obj_add_flag(s_boot_left, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    }
    if (s_boot_right) {
        lv_image_set_pivot(s_boot_right, POKEDEX_SPRITE_W / 2, POKEDEX_SPRITE_H);
        lv_obj_add_flag(s_boot_right, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    }
    s_boot_flash = box(scr, 0, 0, 240, 320, 0xFFFFFF, 0);
    lv_obj_set_style_bg_opa(s_boot_flash, LV_OPA_TRANSP, 0);
    apply_boot_pose(s_boot_frame);
    lv_screen_load(s_scr);
}

static void boot_tick(lv_timer_t *timer)
{
    (void)timer;
    if (!s_boot_on) {
        return;
    }
    s_boot_frame++;
    if (s_boot_frame >= BOOT_FRAMES) {
        boot_finish();
        return;
    }
    apply_boot_pose(s_boot_frame);
}

static void boot_stop_timer(void)
{
    if (s_boot_timer) {
        lv_timer_delete(s_boot_timer);
        s_boot_timer = NULL;
    }
}

static void boot_finish(void)
{
    if (!s_boot_on) {
        return;
    }
    s_boot_on = 0;
    boot_stop_timer();
    pokedex_media_start_bgm();
    render();
}

static void boot_start(uint32_t rng)
{
    (void)rng;
    s_boot_on = 1;
    s_boot_frame = 0;
    boot_stop_timer();
    show_boot();
    s_boot_timer = lv_timer_create(boot_tick, BOOT_MS, NULL);
}

static void show_home(void)
{
    lv_obj_t *lcd = make_shell();
    lv_obj_t *title = zh_at(lcd, "宝可梦图鉴", DEX_INK, 0, 4);
    lv_obj_set_width(title, 208);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *sub = zh_at(lcd, "第一世代151只", DEX_INK_DIM, 0, 24);
    lv_obj_set_width(sub, 208);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);

    for (int i = 0; i < POKEDEX_HOME_COUNT; i++) {
        s_home_cards[i] = box(lcd, 12, 46 + i * 36, 184, 32, 0xA5D6A7, 6);
        lv_obj_set_style_border_width(s_home_cards[i], 1, 0);
        lv_obj_set_style_border_color(s_home_cards[i], lv_color_hex(DEX_INK), 0);
        lv_obj_t *name = zh_at(s_home_cards[i], s_home_titles[i], DEX_INK, 0, 6);
        lv_obj_set_width(name, 184);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
    }
    paint_home_sel();
    lv_screen_load(s_scr);
}

static void show_fact(void)
{
    const int paper = 0xF1F8E9;
    const int panel_w = 192;
    const int panel_h = 164;
    const int portrait = 68;
    const int inset = 2;
    const int scale = 218;

    lv_obj_t *lcd = make_shell();
    lv_obj_t *title = zh_at(lcd, "大木讲堂", DEX_INK, 0, 4);
    lv_obj_set_width(title, 208);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *panel = box(lcd, 8, 26, panel_w, panel_h, paper, 6);
    lv_obj_t *body = zh_at(panel, pokedex_fact_at(s_state.fact_index), DEX_INK, 8, 8);
    lv_obj_set_width(body, panel_w - 16);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);

    if (pokedex_media_load_trainer(s_state.speaker, s_sprite_rgb, &s_sprite_dsc)) {
        rekey_lcd_fill(s_sprite_rgb, paper);
        int right = s_state.portrait_corner & 1;
        int x = right ? (panel_w - inset - portrait) : inset;
        int y = panel_h - inset - portrait;
        lv_obj_t *img = dex_img(panel, &s_sprite_dsc, x, y);
        lv_image_set_pivot(img, 0, 0);
        lv_image_set_scale(img, scale);
        lv_obj_add_flag(img, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_move_foreground(img);
    }
    lv_screen_load(s_scr);
}

static int utf8_chars(const char *text)
{
    int n = 0;
    if (!text) {
        return 0;
    }
    while (*text) {
        unsigned char c = (unsigned char)*text;
        if ((c & 0xC0) != 0x80) {
            n++;
        }
        text++;
    }
    return n;
}

static int match_chip_w(const char *name)
{
    int n = utf8_chars(name);
    if (n <= 1) {
        return 28;
    }
    if (n == 2) {
        return 44;
    }
    return 52;
}

static int add_type_list(lv_obj_t *lcd, int y, const char *title,
                         const char **names, const uint8_t *mul, int n, int want)
{
    int listed = 0;
    for (int i = 0; i < n; i++) {
        if (mul && want && mul[i] != want) {
            continue;
        }
        listed++;
    }
    if (!listed) {
        return y;
    }

    const int chip_h = 18;
    const int gap = 2;
    const int label_w = 28;
    int x = label_w;
    int row_y = y;
    label_at(lcd, title, &lv_font_montserrat_14, DEX_INK_DIM, 2, y + 1);
    for (int i = 0; i < n; i++) {
        if (mul && want && mul[i] != want) {
            continue;
        }
        int w = match_chip_w(names[i]);
        if (x + w > 206) {
            x = label_w;
            row_y += chip_h + gap;
        }
        add_type_chip_at(lcd, names[i], x, row_y, w, chip_h);
        x += w + gap;
    }
    return row_y + chip_h + 3;
}

static int add_matchup_page(lv_obj_t *lcd, const pokedex_entry_t *e)
{
    pokedex_matchup_t m;
    pokedex_matchup(e->type_a, e->type_b, &m);
    int y = 2;
    zh_at(lcd, "被打", DEX_INK, 6, y);
    y += 18;
    y = add_type_list(lcd, y, "4x", m.weak, m.weak_x, m.weak_n, 40);
    y = add_type_list(lcd, y, "2x", m.weak, m.weak_x, m.weak_n, 20);
    y = add_type_list(lcd, y, "0", m.immune, 0, m.immune_n, 0);
    y = add_type_list(lcd, y, "1/2", m.resist, m.resist_x, m.resist_n, 5);
    y = add_type_list(lcd, y, "1/4", m.resist, m.resist_x, m.resist_n, 2);
    y += 6;
    zh_at(lcd, "打出", DEX_INK, 6, y);
    y += 18;
    y = add_type_list(lcd, y, "2x", m.hit2, 0, m.hit2_n, 0);
    y = add_type_list(lcd, y, "0", m.hit0, 0, m.hit0_n, 0);
    y = add_type_list(lcd, y, "1/2", m.hit_half, 0, m.hit_half_n, 0);
    box(lcd, 0, y, 1, 1, DEX_LCD, 0);
    return y;
}

static void add_stat_row(lv_obj_t *parent, int y, const char *name, int value)
{
    label_at(parent, name, &lv_font_montserrat_14, DEX_INK, 6, y);
    int fill = value * 118 / 255;
    if (fill < 1 && value > 0) {
        fill = 1;
    }
    lv_obj_t *track = box(parent, 48, y + 4, 118, 8, 0xA5D6A7, 0);
    lv_obj_set_style_border_width(track, 1, 0);
    lv_obj_set_style_border_color(track, lv_color_hex(DEX_INK), 0);
    if (fill > 0) {
        box(track, 0, 0, fill, 8, DEX_YELLOW, 0);
    }
    lv_obj_t *num = label_at(parent, "", &lv_font_montserrat_14, DEX_INK, 170, y);
    lv_label_set_text_fmt(num, "%d", value);
}

static void maybe_play_cover_cry(int id)
{
    if (s_state.tab != POKEDEX_TAB_COVER) {
        return;
    }
    if (pokedex_is_guess(&s_state) && !pokedex_guess_revealed(&s_state)) {
        return;
    }
    if (s_cry_for_id == id) {
        return;
    }
    s_cry_for_id = id;
    pokedex_media_play_cry(id);
}

static void show_zoom(void)
{
    const pokedex_entry_t *e = pokedex_entry(s_state.id);
    wipe();
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_scr, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(DEX_BLACK), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    if (!e) {
        zh_at(s_scr, "?", DEX_WHITE, 112, 152);
        lv_screen_load(s_scr);
        return;
    }

    lv_obj_t *num = label_at(s_scr, "", &lv_font_montserrat_20, DEX_WHITE, 0, 8);
    lv_obj_set_width(num, 240);
    lv_obj_set_style_text_align(num, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(num, "No.%03d", s_state.id);

    bool got = pokedex_media_load_sprite(s_state.id, s_sprite_rgb, &s_sprite_dsc);
    if (got) {
        lv_obj_t *img = lv_image_create(s_scr);
        lv_image_set_src(img, &s_sprite_dsc);
        lv_image_set_antialias(img, false);
        lv_image_set_pivot(img, POKEDEX_SPRITE_W / 2, POKEDEX_SPRITE_H / 2);
        lv_image_set_scale(img, 256 * 3);
        lv_obj_add_flag(img, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_pos(img, 120 - POKEDEX_SPRITE_W / 2, 160 - POKEDEX_SPRITE_H / 2);
    } else {
        lv_obj_t *miss = box(s_scr, 40, 80, 160, 160, DEX_YELLOW, 8);
        lv_obj_t *soon = zh_at(miss, "暂无图片", DEX_INK, 0, 72);
        lv_obj_set_width(soon, 160);
        lv_obj_set_style_text_align(soon, LV_TEXT_ALIGN_CENTER, 0);
    }

    lv_obj_t *name = zh_at(s_scr, e->zh, DEX_WHITE, 0, 284);
    lv_obj_set_width(name, 240);
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *hint = zh_at(s_scr, "还原", DEX_YELLOW, 0, 302);
    lv_obj_set_width(hint, 240);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    maybe_play_cover_cry(s_state.id);
    lv_screen_load(s_scr);
}

static void show_entry(void)
{
    const pokedex_entry_t *e = pokedex_entry(s_state.id);
    lv_obj_t *lcd = make_shell();

    if (!e) {
        zh_at(lcd, "?", DEX_INK, 96, 88);
        lv_screen_load(s_scr);
        return;
    }

    if (s_state.tab == POKEDEX_TAB_COVER) {
        if (pokedex_is_guess(&s_state) && !pokedex_guess_revealed(&s_state)) {
            lv_obj_t *title = zh_at(lcd, "猜猜我是谁", DEX_INK, 0, 8);
            lv_obj_set_width(title, 208);
            lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
            bool got = pokedex_media_load_sprite(s_state.id, s_sprite_rgb, &s_sprite_dsc);
            if (got) {
                sprite_silhouette(s_sprite_rgb);
                lv_obj_t *img = lv_image_create(lcd);
                lv_image_set_src(img, &s_sprite_dsc);
                lv_image_set_antialias(img, false);
                lv_obj_set_pos(img, 64, 40);
            } else {
                box(lcd, 64, 40, 80, 80, DEX_BLACK, 4);
            }
            lv_obj_t *hint = zh_at(lcd, "按确定查看", DEX_INK_DIM, 0, 132);
            lv_obj_set_width(hint, 208);
            lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
            lv_screen_load(s_scr);
            return;
        }
        bool got = pokedex_media_load_sprite(s_state.id, s_sprite_rgb, &s_sprite_dsc);
        if (got) {
            lv_obj_t *img = lv_image_create(lcd);
            lv_image_set_src(img, &s_sprite_dsc);
            lv_obj_set_pos(img, 8, 8);
        } else {
            lv_obj_t *miss = box(lcd, 8, 8, 80, 80, DEX_YELLOW, 4);
            lv_obj_t *soon = zh_at(miss, "暂无图片", DEX_INK, 0, 32);
            lv_obj_set_width(soon, 80);
            lv_obj_set_style_text_align(soon, LV_TEXT_ALIGN_CENTER, 0);
        }
        lv_obj_t *num = label_at(lcd, "", &lv_font_montserrat_20, DEX_INK, 96, 10);
        lv_label_set_text_fmt(num, "No.%03d", s_state.id);
        zh_at(lcd, e->zh, DEX_INK, 96, 36);
        label_at(lcd, e->en, &lv_font_montserrat_14, DEX_INK_DIM, 96, 58);
        zh_at(lcd, e->category, DEX_INK_DIM, 8, 96);
        add_type_chip(lcd, e->type_a, 8, 118);
        if (e->type_b[0]) {
            add_type_chip(lcd, e->type_b, 72, 118);
        }
        char size[32];
        char meta[48];
        pokedex_format_size(e, size, sizeof(size));
        pokedex_format_meta(e, meta, sizeof(meta));
        zh_at(lcd, size, DEX_INK, 8, 142);
        lv_obj_t *meta_line = zh_at(lcd, meta, DEX_INK_DIM, 8, 160);
        lv_obj_set_width(meta_line, 192);
        maybe_play_cover_cry(s_state.id);
    } else if (s_state.tab == POKEDEX_TAB_BIO) {
        lv_obj_t *body = zh_at(lcd, e->intro, DEX_INK, 8, 6);
        lv_obj_set_width(body, 192);
        lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
        lv_obj_update_layout(body);
        int y = 6 + (int)lv_obj_get_height(body) + 8;
        if (e->trivia && e->trivia[0] && strcmp(e->trivia, e->intro) != 0 && y < 160) {
            lv_obj_t *extra = zh_at(lcd, e->trivia, DEX_INK_DIM, 8, y);
            lv_obj_set_width(extra, 192);
            lv_label_set_long_mode(extra, LV_LABEL_LONG_WRAP);
        }
    } else if (s_state.tab == POKEDEX_TAB_STATS) {
        const uint8_t values[] = {
            e->hp, e->atk, e->def_, e->spa, e->spd, e->spe,
        };
        for (int i = 0; i < 6; i++) {
            add_stat_row(lcd, 2 + i * 16, s_stat_name[i], values[i]);
        }
        lv_obj_t *sum = zh_at(lcd, "", DEX_INK, 6, 98);
        lv_label_set_text_fmt(sum, "合计 %d", pokedex_stat_total(e));
        int y = 116;
        for (int i = 0; i < e->ability_n && i < POKEDEX_ABILITY_MAX && y < 170; i++) {
            lv_obj_t *name = zh_at(lcd, "", DEX_INK, 6, y);
            lv_label_set_text_fmt(name, "特性：%s", e->ability_zh[i] ? e->ability_zh[i] : "");
            y += 16;
            if (e->ability_intro[i] && e->ability_intro[i][0]) {
                lv_obj_t *intro = zh_at(lcd, e->ability_intro[i], DEX_INK_DIM, 6, y);
                lv_obj_set_width(intro, 196);
                lv_label_set_long_mode(intro, LV_LABEL_LONG_WRAP);
                lv_obj_update_layout(intro);
                y += (int)lv_obj_get_height(intro) + 4;
            }
        }
    } else if (s_state.tab == POKEDEX_TAB_MOVES) {
        if (e->move_n <= 0) {
            zh_at(lcd, "-", DEX_INK, 8, 8);
        } else {
            int cols = (e->move_n > 10) ? 2 : 1;
            int rows = (e->move_n + cols - 1) / cols;
            for (int i = 0; i < e->move_n && i < POKEDEX_MOVE_MAX; i++) {
                int col = i / rows;
                int row = i % rows;
                int x = (cols == 1) ? 8 : (6 + col * 100);
                lv_obj_t *line = zh_at(lcd, "", DEX_INK, x, 4 + row * 16);
                lv_obj_set_width(line, cols == 1 ? 192 : 96);
                lv_label_set_text_fmt(line, "%d %s", e->move_lv[i], e->move_zh[i]);
            }
        }
    } else if (s_state.tab == POKEDEX_TAB_MATCHUP) {
        s_matchup_scroll = box(lcd, 0, 0, 208, 176, DEX_LCD, 0);
        lv_obj_add_flag(s_matchup_scroll, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(s_matchup_scroll, LV_OBJ_FLAG_SCROLL_CHAIN);
        lv_obj_set_scroll_dir(s_matchup_scroll, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(s_matchup_scroll, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_pad_bottom(s_matchup_scroll, 8, 0);
        add_matchup_page(s_matchup_scroll, e);
    } else if (e->evo_n <= 1) {
        lv_obj_t *none = zh_at(lcd, "不会进化", DEX_INK, 0, 80);
        lv_obj_set_width(none, 208);
        lv_obj_set_style_text_align(none, LV_TEXT_ALIGN_CENTER, 0);
    } else {
        for (int i = 0; i < e->evo_n && i < POKEDEX_EVO_MAX; i++) {
            lv_obj_t *row = zh_at(lcd, "", DEX_INK, 8, 8 + i * 24);
            if (e->evo_cond[i][0]) {
                lv_label_set_text_fmt(row, "%s  %s", e->evo_zh[i], e->evo_cond[i]);
            } else {
                lv_label_set_text(row, e->evo_zh[i]);
            }
        }
    }

    add_tabs(lcd, (int)s_state.tab);
    lv_screen_load(s_scr);
}

static void render(void)
{
    if (pokedex_is_home(&s_state)) {
        show_home();
    } else if (pokedex_is_fact(&s_state)) {
        show_fact();
    } else if (pokedex_is_zoomed(&s_state)) {
        show_zoom();
    } else {
        show_entry();
    }
}

void demo_pokedex_enter(void)
{
    pokedex_media_init();
    pokedex_init(&s_state);
    s_cry_for_id = 0;
    boot_start(esp_random());
}

void demo_pokedex_exit(void)
{
    s_boot_on = 0;
    boot_stop_timer();
    pokedex_media_stop_bgm();
    pokedex_media_stop_cry();
    pokedex_media_deinit();
    wipe();
}

static int matchup_scroll(int dir)
{
    if (!s_matchup_scroll || s_state.tab != POKEDEX_TAB_MATCHUP) {
        return 0;
    }
    lv_obj_update_layout(s_matchup_scroll);
    if (dir < 0) {
        if (lv_obj_get_scroll_top(s_matchup_scroll) <= 0) {
            return 0;
        }
        lv_obj_scroll_by(s_matchup_scroll, 0, 32, LV_ANIM_OFF);
        return 1;
    }
    if (lv_obj_get_scroll_bottom(s_matchup_scroll) <= 0) {
        return 0;
    }
    lv_obj_scroll_by(s_matchup_scroll, 0, -32, LV_ANIM_OFF);
    return 1;
}

void demo_pokedex_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (s_boot_on) {
        boot_finish();
        return;
    }
    if (ev == BSP_BTN_LONG && btn == BSP_BTN_OK) {
        int id = s_state.id;
        pokedex_act_t act = pokedex_ok_long(&s_state);
        if (act == POKEDEX_ACT_PLAY_CRY) {
            s_cry_for_id = 0;
            pokedex_media_play_cry(id);
        } else {
            pokedex_media_stop_cry();
        }
        s_cry_for_id = 0;
        render();
        return;
    }
    if (ev == BSP_BTN_DOUBLE && btn == BSP_BTN_OK &&
        !pokedex_is_home(&s_state) && !pokedex_is_fact(&s_state) &&
        s_state.tab == POKEDEX_TAB_COVER &&
        !(pokedex_is_guess(&s_state) && !pokedex_guess_revealed(&s_state))) {
        pokedex_toggle_zoom(&s_state);
        render();
        return;
    }
    if (ev != BSP_BTN_CLICK) {
        return;
    }
    if (pokedex_is_home(&s_state)) {
        if (btn == BSP_BTN_UP) {
            pokedex_home_move(&s_state, -1);
            paint_home_sel();
        } else if (btn == BSP_BTN_DOWN) {
            pokedex_home_move(&s_state, 1);
            paint_home_sel();
        } else if (btn == BSP_BTN_OK) {
            pokedex_enter_from_home(&s_state, esp_random());
            s_cry_for_id = 0;
            render();
        }
        return;
    }
    if (pokedex_is_fact(&s_state)) {
        if (btn == BSP_BTN_UP) {
            pokedex_step_fact(&s_state, -1, esp_random());
            render();
        } else if (btn == BSP_BTN_DOWN || btn == BSP_BTN_OK) {
            pokedex_step_fact(&s_state, 1, esp_random());
            render();
        }
        return;
    }
    if (!pokedex_is_guess(&s_state) &&
        s_state.tab == POKEDEX_TAB_MATCHUP &&
        (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) &&
        matchup_scroll(btn == BSP_BTN_UP ? -1 : 1)) {
        return;
    }
    if (btn == BSP_BTN_UP) {
        pokedex_media_stop_cry();
        s_cry_for_id = 0;
        pokedex_step_id(&s_state, -1, esp_random());
        render();
    } else if (btn == BSP_BTN_DOWN) {
        pokedex_media_stop_cry();
        s_cry_for_id = 0;
        pokedex_step_id(&s_state, 1, esp_random());
        render();
    } else if (btn == BSP_BTN_OK) {
        if (pokedex_is_zoomed(&s_state)) {
            pokedex_toggle_zoom(&s_state);
        } else {
            pokedex_next_tab(&s_state);
        }
        render();
    }
}
