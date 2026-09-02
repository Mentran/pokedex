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

void pokedex_init(pokedex_state_t *s)
{
    s->screen = POKEDEX_SCREEN_HOME;
    s->home_sel = POKEDEX_HOME_BROWSE;
    s->id = 1;
    s->tab = POKEDEX_TAB_COVER;
    s->last_random_id = 0;
    s->fact_index = 0;
    s->last_fact_index = -1;
    s->zoomed = 0;
}

void pokedex_home_move(pokedex_state_t *s, int delta)
{
    if (s->screen != POKEDEX_SCREEN_HOME) {
        return;
    }
    s->home_sel = wrap(s->home_sel + delta, POKEDEX_HOME_COUNT);
}

void pokedex_enter_from_home(pokedex_state_t *s, uint32_t rng)
{
    if (s->screen != POKEDEX_SCREEN_HOME) {
        return;
    }
    s->tab = POKEDEX_TAB_COVER;
    s->zoomed = 0;
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
        s->screen = POKEDEX_SCREEN_FACT;
        return;
    }
    if (s->home_sel == POKEDEX_HOME_RANDOM) {
        s->id = pokedex_pick_random(POKEDEX_COUNT, s->last_random_id, rng);
        s->last_random_id = s->id;
    } else {
        s->id = 1;
    }
    s->screen = POKEDEX_SCREEN_ENTRY;
}

void pokedex_step_id(pokedex_state_t *s, int delta)
{
    if (s->screen != POKEDEX_SCREEN_ENTRY) {
        return;
    }
    s->id = pokedex_wrap_id(s->id + delta, POKEDEX_COUNT);
    s->tab = POKEDEX_TAB_COVER;
}

void pokedex_step_fact(pokedex_state_t *s, int delta)
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
}

void pokedex_next_tab(pokedex_state_t *s)
{
    if (s->screen != POKEDEX_SCREEN_ENTRY) {
        return;
    }
    s->zoomed = 0;
    s->tab = (pokedex_tab_t)wrap((int)s->tab + 1, POKEDEX_TAB_COUNT);
}

pokedex_act_t pokedex_ok_long(pokedex_state_t *s)
{
    if (s->screen == POKEDEX_SCREEN_HOME) {
        return POKEDEX_ACT_NONE;
    }
    pokedex_act_t act = POKEDEX_ACT_NONE;
    if (s->screen == POKEDEX_SCREEN_ENTRY && s->tab == POKEDEX_TAB_COVER) {
        act = POKEDEX_ACT_PLAY_CRY;
    }
    s->screen = POKEDEX_SCREEN_HOME;
    s->tab = POKEDEX_TAB_COVER;
    s->zoomed = 0;
    return act;
}

void pokedex_toggle_zoom(pokedex_state_t *s)
{
    if (!s || s->screen != POKEDEX_SCREEN_ENTRY || s->tab != POKEDEX_TAB_COVER) {
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
