// demo_pokedex.c —— 手持图鉴外壳 + 封面立绘 + 叫声。按键语义仍走 pokedex.c。
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
static lv_image_dsc_t s_sprite_dsc;
static int s_cry_for_id;
static lv_obj_t *s_matchup_scroll;

static const char *s_home_titles[POKEDEX_HOME_COUNT] = {
    "图鉴浏览",
    "随机遇见",
    "宝可梦小知识",
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
    return pokedex_is_home(&s_state);
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

static void wipe(void)
{
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
        for (int i = 0; i < POKEDEX_HOME_COUNT; i++) {
            s_home_cards[i] = NULL;
        }
        s_matchup_scroll = NULL;
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

static void show_home(void)
{
    lv_obj_t *lcd = make_shell();
    lv_obj_t *title = zh_at(lcd, "宝可梦图鉴", DEX_INK, 0, 10);
    lv_obj_set_width(title, 208);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *sub = zh_at(lcd, "第一世代151只", DEX_INK_DIM, 0, 32);
    lv_obj_set_width(sub, 208);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);

    for (int i = 0; i < POKEDEX_HOME_COUNT; i++) {
        s_home_cards[i] = box(lcd, 12, 58 + i * 44, 184, 38, 0xA5D6A7, 6);
        lv_obj_set_style_border_width(s_home_cards[i], 1, 0);
        lv_obj_set_style_border_color(s_home_cards[i], lv_color_hex(DEX_INK), 0);
        lv_obj_t *name = zh_at(s_home_cards[i], s_home_titles[i], DEX_INK, 0, 9);
        lv_obj_set_width(name, 184);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
    }
    paint_home_sel();
    lv_screen_load(s_scr);
}

static void show_fact(void)
{
    lv_obj_t *lcd = make_shell();
    lv_obj_t *title = zh_at(lcd, "宝可梦小知识", DEX_INK, 0, 10);
    lv_obj_set_width(title, 208);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *panel = box(lcd, 8, 40, 192, 140, 0xA5D6A7, 6);
    lv_obj_t *body = zh_at(panel, pokedex_fact_at(s_state.fact_index), DEX_INK, 10, 12);
    lv_obj_set_width(body, 172);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
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
    if (s_cry_for_id == id) {
        return;
    }
    s_cry_for_id = id;
    pokedex_media_play_cry(id);
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
    } else {
        show_entry();
    }
}

void demo_pokedex_enter(void)
{
    pokedex_media_init();
    pokedex_init(&s_state);
    s_cry_for_id = 0;
    render();
}

void demo_pokedex_exit(void)
{
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
            pokedex_step_fact(&s_state, -1);
            render();
        } else if (btn == BSP_BTN_DOWN || btn == BSP_BTN_OK) {
            pokedex_step_fact(&s_state, 1);
            render();
        }
        return;
    }
    if (s_state.tab == POKEDEX_TAB_MATCHUP &&
        (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) &&
        matchup_scroll(btn == BSP_BTN_UP ? -1 : 1)) {
        return;
    }
    if (btn == BSP_BTN_UP) {
        pokedex_media_stop_cry();
        s_cry_for_id = 0;
        pokedex_step_id(&s_state, -1);
        render();
    } else if (btn == BSP_BTN_DOWN) {
        pokedex_media_stop_cry();
        s_cry_for_id = 0;
        pokedex_step_id(&s_state, 1);
        render();
    } else if (btn == BSP_BTN_OK) {
        pokedex_next_tab(&s_state);
        render();
    }
}
