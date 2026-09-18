// pokedex.c —— 图鉴翻页状态机,与 LVGL / ESP-IDF 解耦,供主机测试和固件共用。
#include "pokedex.h"

#include <stdio.h>
#include <string.h>

static int wrap(int value, int count)
{
    if (count <= 0) {
        return 0;
    }
    int n = value % count;
    return n < 0 ? n + count : n;
}

int pokedex_wrap_index(int index, int count)
{
    return wrap(index, count);
}

int pokedex_wrap_id(int id, int count)
{
    if (count <= 0) {
        return 1;
    }
    return wrap(id - 1, count) + 1;
}

int pokedex_pick_random(int count, int avoid, uint32_t rng)
{
    if (count <= 1) {
        return 1;
    }
    // 把 rng 映射到 1..count;若撞上 avoid,改走下一个,避免随机模式连抽同一只。
    int pick = (int)(rng % (uint32_t)count) + 1;
    if (avoid >= 1 && avoid <= count && pick == avoid) {
        pick = pokedex_wrap_id(pick + 1, count);
    }
    return pick;
}

int pokedex_pick_slot(int count, int avoid, uint32_t rng)
{
    if (count <= 1) {
        return 0;
    }
    int pick = (int)(rng % (uint32_t)count);
    if (avoid >= 0 && avoid < count && pick == avoid) {
        pick = wrap(pick + 1, count);
    }
    return pick;
}

static const char *const s_speakers[POKEDEX_SPEAKER_COUNT] = {
    "大木博士",
    "小智",
    "小刚",
    "小霞",
    "小茂",
};

const char *pokedex_speaker_name(int speaker)
{
    if (speaker < 0 || speaker >= POKEDEX_SPEAKER_COUNT) {
        return s_speakers[0];
    }
    return s_speakers[speaker];
}

static void roll_speaker(pokedex_state_t *s, uint32_t rng)
{
    s->speaker = pokedex_pick_slot(POKEDEX_SPEAKER_COUNT, s->last_speaker, rng);
    s->last_speaker = s->speaker;
    s->portrait_corner = pokedex_pick_slot(POKEDEX_CORNER_COUNT, s->last_portrait_corner, rng >> 8);
    s->last_portrait_corner = s->portrait_corner;
}

void pokedex_init(pokedex_state_t *s)
{
    s->screen = POKEDEX_SCREEN_HOME;
    s->home_sel = POKEDEX_HOME_BROWSE;
    s->id = 1;
    s->tab = POKEDEX_TAB_COVER;
    s->last_random_id = 0;
    s->random_walk = 0;
    s->guess_mode = 0;
    s->guess_revealed = 0;
    s->guess_clue = POKEDEX_GUESS_SILHOUETTE;
    s->fact_index = 0;
    s->last_fact_index = -1;
    s->speaker = 0;
    s->last_speaker = -1;
    s->portrait_corner = 0;
    s->last_portrait_corner = -1;
    s->zoomed = 0;
    s->settings_sel = POKEDEX_SET_VOLUME;
}

void pokedex_home_move(pokedex_state_t *s, int delta)
{
    if (s->screen != POKEDEX_SCREEN_HOME) {
        return;
    }
    s->home_sel = wrap(s->home_sel + delta, POKEDEX_HOME_COUNT);
}

static void deal_guess(pokedex_state_t *s, uint32_t rng)
{
    s->id = pokedex_pick_random(POKEDEX_COUNT, s->last_random_id, rng);
    s->last_random_id = s->id;
    s->guess_mode = 1;
    s->guess_revealed = 0;
    s->guess_clue = pokedex_pick_slot(POKEDEX_GUESS_CLUE_COUNT, -1, rng >> 11);
    s->random_walk = 0;
    s->zoomed = 0;
    s->tab = POKEDEX_TAB_COVER;
    s->screen = POKEDEX_SCREEN_ENTRY;
}

void pokedex_enter_from_home(pokedex_state_t *s, uint32_t rng)
{
    if (s->screen != POKEDEX_SCREEN_HOME) {
        return;
    }
    s->tab = POKEDEX_TAB_COVER;
    s->zoomed = 0;
    s->guess_mode = 0;
    s->guess_revealed = 0;
    s->guess_clue = POKEDEX_GUESS_SILHOUETTE;
    if (s->home_sel == POKEDEX_HOME_FACTS) {
        int n = pokedex_fact_count();
        if (n <= 0) {
            s->fact_index = 0;
        } else {
            int pick = (int)(rng % (uint32_t)n);
            if (s->last_fact_index >= 0 && s->last_fact_index < n && pick == s->last_fact_index) {
                pick = wrap(pick + 1, n);
            }
            s->fact_index = pick;
            s->last_fact_index = pick;
        }
        roll_speaker(s, rng >> 11);
        s->screen = POKEDEX_SCREEN_FACT;
        return;
    }
    if (s->home_sel == POKEDEX_HOME_GUESS) {
        deal_guess(s, rng);
        return;
    }
    if (s->home_sel == POKEDEX_HOME_RANDOM) {
        s->id = pokedex_pick_random(POKEDEX_COUNT, s->last_random_id, rng);
        s->last_random_id = s->id;
        s->random_walk = 1;
    } else {
        s->id = 1;
        s->random_walk = 0;
    }
    s->screen = POKEDEX_SCREEN_ENTRY;
}

void pokedex_step_id(pokedex_state_t *s, int delta, uint32_t rng)
{
    if (s->screen != POKEDEX_SCREEN_ENTRY) {
        return;
    }
    if (s->random_walk) {
        s->id = pokedex_pick_random(POKEDEX_COUNT, s->id, rng);
        s->last_random_id = s->id;
    } else if (s->guess_mode) {
        deal_guess(s, rng);
        return;
    } else {
        s->id = pokedex_wrap_id(s->id + delta, POKEDEX_COUNT);
    }
    s->tab = POKEDEX_TAB_COVER;
}

void pokedex_step_fact(pokedex_state_t *s, int delta, uint32_t rng)
{
    if (s->screen != POKEDEX_SCREEN_FACT) {
        return;
    }
    int n = pokedex_fact_count();
    if (n <= 0) {
        s->fact_index = 0;
        return;
    }
    s->fact_index = wrap(s->fact_index + delta, n);
    s->last_fact_index = s->fact_index;
    roll_speaker(s, rng);
}

void pokedex_next_tab(pokedex_state_t *s)
{
    if (s->screen != POKEDEX_SCREEN_ENTRY) {
        return;
    }
    if (s->guess_mode && !s->guess_revealed) {
        pokedex_reveal_guess(s);
        return;
    }
    s->zoomed = 0;
    s->tab = (pokedex_tab_t)wrap((int)s->tab + 1, POKEDEX_TAB_COUNT);
}

pokedex_act_t pokedex_ok_long(pokedex_state_t *s)
{
    if (s->screen == POKEDEX_SCREEN_HOME) {
        s->screen = POKEDEX_SCREEN_SETTINGS;
        s->settings_sel = POKEDEX_SET_VOLUME;
        s->zoomed = 0;
        return POKEDEX_ACT_NONE;
    }
    if (s->screen == POKEDEX_SCREEN_SETTINGS) {
        s->screen = POKEDEX_SCREEN_HOME;
        return POKEDEX_ACT_NONE;
    }
    pokedex_act_t act = POKEDEX_ACT_NONE;
    if (s->screen == POKEDEX_SCREEN_ENTRY && s->tab == POKEDEX_TAB_COVER &&
        !(s->guess_mode && !s->guess_revealed)) {
        act = POKEDEX_ACT_PLAY_CRY;
    }
    s->screen = POKEDEX_SCREEN_HOME;
    s->tab = POKEDEX_TAB_COVER;
    s->zoomed = 0;
    s->guess_mode = 0;
    s->guess_revealed = 0;
    s->guess_clue = POKEDEX_GUESS_SILHOUETTE;
    return act;
}

void pokedex_reveal_guess(pokedex_state_t *s)
{
    if (!s || !s->guess_mode || s->screen != POKEDEX_SCREEN_ENTRY) {
        return;
    }
    s->guess_revealed = 1;
    s->tab = POKEDEX_TAB_COVER;
    s->zoomed = 0;
}

int pokedex_is_guess(const pokedex_state_t *s)
{
    return s && s->guess_mode && s->screen == POKEDEX_SCREEN_ENTRY;
}

int pokedex_guess_revealed(const pokedex_state_t *s)
{
    return pokedex_is_guess(s) && s->guess_revealed;
}

pokedex_guess_clue_t pokedex_guess_clue(const pokedex_state_t *s)
{
    if (!pokedex_is_guess(s) || s->guess_revealed) {
        return POKEDEX_GUESS_SILHOUETTE;
    }
    if (s->guess_clue == POKEDEX_GUESS_BIO) {
        return POKEDEX_GUESS_BIO;
    }
    return POKEDEX_GUESS_SILHOUETTE;
}

void pokedex_toggle_zoom(pokedex_state_t *s)
{
    if (!s || s->screen != POKEDEX_SCREEN_ENTRY || s->tab != POKEDEX_TAB_COVER) {
        return;
    }
    if (s->guess_mode && !s->guess_revealed) {
        return;
    }
    s->zoomed = !s->zoomed;
}

int pokedex_is_zoomed(const pokedex_state_t *s)
{
    return s && s->screen == POKEDEX_SCREEN_ENTRY &&
           s->tab == POKEDEX_TAB_COVER && s->zoomed;
}

int pokedex_is_home(const pokedex_state_t *s)
{
    return s->screen == POKEDEX_SCREEN_HOME;
}

int pokedex_is_fact(const pokedex_state_t *s)
{
    return s->screen == POKEDEX_SCREEN_FACT;
}

int pokedex_is_settings(const pokedex_state_t *s)
{
    return s && s->screen == POKEDEX_SCREEN_SETTINGS;
}

void pokedex_settings_toggle(pokedex_state_t *s)
{
    if (!pokedex_is_settings(s)) {
        return;
    }
    s->settings_sel = wrap(s->settings_sel + 1, POKEDEX_SETTINGS_COUNT);
}

int pokedex_settings_sel(const pokedex_state_t *s)
{
    if (!pokedex_is_settings(s)) {
        return POKEDEX_SET_VOLUME;
    }
    return s->settings_sel;
}

static const pokedex_entry_t s_entries[POKEDEX_COUNT] = {
#include "pokedex_catalog.inc"
};

const pokedex_entry_t *pokedex_entry(int id)
{
    if (id < 1 || id > POKEDEX_COUNT) {
        return 0;
    }
    return &s_entries[id - 1];
}

int pokedex_has_trivia(const pokedex_entry_t *e)
{
    return e && e->trivia && e->trivia[0] && strcmp(e->trivia, e->intro) != 0;
}

const char *pokedex_trivia_text(const pokedex_entry_t *e)
{
    if (pokedex_has_trivia(e)) {
        return e->trivia;
    }
    return "目前没有更多记录。";
}

static int guess_text_names(const char *text, const char *zh)
{
    return text && text[0] && zh && zh[0] && strstr(text, zh) != NULL;
}

const char *pokedex_guess_bio_text(const pokedex_entry_t *e)
{
    static char buf[320];
    static const char *repl = "这种宝可梦";
    const char *src;
    const char *zh;
    size_t zlen;
    size_t rlen;
    size_t o;
    const char *p;

    if (!e || !e->intro || !e->intro[0]) {
        return "";
    }
    zh = e->zh ? e->zh : "";
    if (!guess_text_names(e->intro, zh)) {
        return e->intro;
    }
    if (pokedex_has_trivia(e) && !guess_text_names(e->trivia, zh)) {
        return e->trivia;
    }
    src = e->intro;
    zlen = strlen(zh);
    rlen = strlen(repl);
    o = 0;
    p = src;
    while (*p && o + 1 < sizeof(buf)) {
        if (zlen && strncmp(p, zh, zlen) == 0) {
            if (o + rlen >= sizeof(buf)) {
                break;
            }
            memcpy(buf + o, repl, rlen);
            o += rlen;
            p += zlen;
        } else {
            buf[o++] = *p++;
        }
    }
    buf[o] = 0;
    return buf;
}

int pokedex_stat_total(const pokedex_entry_t *e)
{
    if (!e) {
        return 0;
    }
    return (int)e->hp + e->atk + e->def_ + e->spa + e->spd + e->spe;
}

enum {
    T_NOR, T_FIR, T_WAT, T_ELE, T_GRA, T_ICE, T_FIG, T_POI, T_GRO,
    T_FLY, T_PSY, T_BUG, T_ROC, T_GHO, T_DRA, T_DAR, T_STE, T_FAI, T_COUNT
};

#define N_ 10
#define H_ 5
#define S_ 20
#define Z_ 0

static const char *s_type_zh[T_COUNT] = {
    "一般", "火", "水", "电", "草", "冰", "格斗", "毒", "地面",
    "飞行", "超能力", "虫", "岩石", "幽灵", "龙", "恶", "钢", "妖精",
};

/* chart[atk][def]: 0 / 0.5 / 1 / 2 in tenths. */
static const uint8_t s_chart[T_COUNT][T_COUNT] = {
    /*         NOR FIR WAT ELE GRA ICE FIG POI GRO FLY PSY BUG ROC GHO DRA DAR STE FAI */
    /* NOR */ { N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, H_, Z_, N_, N_, H_, N_ },
    /* FIR */ { N_, H_, H_, N_, S_, S_, N_, N_, N_, N_, N_, S_, H_, N_, H_, N_, S_, N_ },
    /* WAT */ { N_, S_, H_, N_, H_, N_, N_, N_, S_, N_, N_, N_, S_, N_, H_, N_, N_, N_ },
    /* ELE */ { N_, N_, S_, H_, H_, N_, N_, N_, Z_, S_, N_, N_, N_, N_, H_, N_, N_, N_ },
    /* GRA */ { N_, H_, S_, N_, H_, N_, N_, H_, S_, H_, N_, H_, S_, N_, H_, N_, H_, N_ },
    /* ICE */ { N_, H_, H_, N_, S_, H_, N_, N_, S_, S_, N_, N_, N_, N_, S_, N_, H_, N_ },
    /* FIG */ { S_, N_, N_, N_, N_, S_, N_, H_, N_, H_, H_, H_, S_, Z_, N_, S_, S_, H_ },
    /* POI */ { N_, N_, N_, N_, S_, N_, N_, H_, H_, N_, N_, N_, H_, H_, N_, N_, Z_, S_ },
    /* GRO */ { N_, S_, N_, S_, H_, N_, N_, S_, N_, Z_, N_, H_, S_, N_, N_, N_, S_, N_ },
    /* FLY */ { N_, N_, N_, H_, S_, N_, S_, N_, N_, N_, N_, S_, H_, N_, N_, N_, H_, N_ },
    /* PSY */ { N_, N_, N_, N_, N_, N_, S_, S_, N_, N_, H_, N_, N_, N_, N_, Z_, H_, N_ },
    /* BUG */ { N_, H_, N_, N_, S_, N_, H_, H_, N_, H_, S_, N_, N_, H_, N_, S_, H_, H_ },
    /* ROC */ { N_, S_, N_, N_, N_, S_, H_, N_, H_, S_, N_, S_, N_, N_, N_, N_, H_, N_ },
    /* GHO */ { Z_, N_, N_, N_, N_, N_, N_, N_, N_, N_, S_, N_, N_, S_, N_, H_, N_, N_ },
    /* DRA */ { N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, N_, S_, N_, H_, Z_ },
    /* DAR */ { N_, N_, N_, N_, N_, N_, H_, N_, N_, N_, S_, N_, N_, S_, N_, H_, N_, H_ },
    /* STE */ { N_, H_, H_, H_, N_, S_, N_, N_, N_, N_, N_, N_, S_, N_, N_, N_, H_, S_ },
    /* FAI */ { N_, H_, N_, N_, N_, N_, S_, H_, N_, N_, N_, N_, N_, N_, S_, S_, H_, N_ },
};

static int type_index(const char *name)
{
    if (!name || !name[0]) {
        return -1;
    }
    for (int i = 0; i < T_COUNT; i++) {
        if (strcmp(s_type_zh[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static int def_mul(int atk, int def_a, int def_b)
{
    int m = s_chart[atk][def_a];
    if (def_b >= 0) {
        m = m * s_chart[atk][def_b] / 10;
    }
    return m;
}

static int off_mul(int atk_a, int atk_b, int def)
{
    int m = s_chart[atk_a][def];
    if (atk_b >= 0) {
        int other = s_chart[atk_b][def];
        if (other > m) {
            m = other;
        }
    }
    return m;
}

static void push_name(const char **dst, uint8_t *mul, int *n, const char *name, uint8_t x)
{
    if (*n >= POKEDEX_MATCH_MAX) {
        return;
    }
    dst[*n] = name;
    if (mul) {
        mul[*n] = x;
    }
    *n += 1;
}

void pokedex_matchup(const char *type_a, const char *type_b, pokedex_matchup_t *out)
{
    memset(out, 0, sizeof(*out));
    int a = type_index(type_a);
    int b = type_index(type_b);
    if (a < 0) {
        return;
    }
    for (int atk = 0; atk < T_COUNT; atk++) {
        int m = def_mul(atk, a, b);
        if (m >= 20) {
            push_name(out->weak, out->weak_x, &out->weak_n, s_type_zh[atk], (uint8_t)m);
        } else if (m == 0) {
            push_name(out->immune, 0, &out->immune_n, s_type_zh[atk], 0);
        } else if (m < 10) {
            push_name(out->resist, out->resist_x, &out->resist_n, s_type_zh[atk], (uint8_t)m);
        }
    }
    for (int def = 0; def < T_COUNT; def++) {
        int m = off_mul(a, b, def);
        if (m >= 20) {
            push_name(out->hit2, 0, &out->hit2_n, s_type_zh[def], 0);
        } else if (m == 0) {
            push_name(out->hit0, 0, &out->hit0_n, s_type_zh[def], 0);
        } else if (m < 10) {
            push_name(out->hit_half, 0, &out->hit_half_n, s_type_zh[def], 0);
        }
    }
}

#include "pokedex_facts.inc"

int pokedex_format_size(const pokedex_entry_t *e, char *dst, size_t n)
{
    if (!e || !dst || n == 0) {
        return 0;
    }
    return snprintf(
        dst,
        n,
        "%u.%um  %u.%ukg",
        (unsigned)(e->height_dm / 10),
        (unsigned)(e->height_dm % 10),
        (unsigned)(e->weight_hg / 10),
        (unsigned)(e->weight_hg % 10));
}

int pokedex_format_meta(const pokedex_entry_t *e, char *dst, size_t n)
{
    if (!e || !dst || n == 0) {
        return 0;
    }
    if (e->gender_rate < 0) {
        return snprintf(dst, n, "无性别");
    }
    if (e->gender_rate == 0) {
        return snprintf(dst, n, "雄100%%");
    }
    if (e->gender_rate >= 8) {
        return snprintf(dst, n, "雌100%%");
    }
    unsigned female = ((unsigned)e->gender_rate * 100u + 4u) / 8u;
    unsigned male = 100u - female;
    return snprintf(dst, n, "雄%u%% 雌%u%%", male, female);
}

int pokedex_fact_count(void)
{
    return (int)(sizeof(s_facts) / sizeof(s_facts[0]));
}

const char *pokedex_fact_at(int index)
{
    int n = pokedex_fact_count();
    if (n <= 0) {
        return "";
    }
    return s_facts[wrap(index, n)];
}

const char *pokedex_fact(uint32_t rng)
{
    int n = pokedex_fact_count();
    if (n <= 0) {
        return "";
    }
    return pokedex_fact_at((int)(rng % (uint32_t)n));
}
