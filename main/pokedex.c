// pokedex.c —— 图鉴翻页状态机,与 LVGL / ESP-IDF 解耦,供主机测试和固件共用。
#include "pokedex.h"

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

void pokedex_next_tab(pokedex_state_t *s)
{
    if (s->screen != POKEDEX_SCREEN_ENTRY) {
        return;
    }
    s->tab = (pokedex_tab_t)wrap((int)s->tab + 1, POKEDEX_TAB_COUNT);
}

pokedex_act_t pokedex_ok_long(pokedex_state_t *s)
{
    if (s->screen != POKEDEX_SCREEN_ENTRY) {
        return POKEDEX_ACT_NONE;
    }
    pokedex_act_t act = (s->tab == POKEDEX_TAB_COVER)
        ? POKEDEX_ACT_PLAY_CRY
        : POKEDEX_ACT_NONE;
    s->screen = POKEDEX_SCREEN_HOME;
    s->tab = POKEDEX_TAB_COVER;
    return act;
}

int pokedex_is_home(const pokedex_state_t *s)
{
    return s->screen == POKEDEX_SCREEN_HOME;
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
