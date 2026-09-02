#pragma once

#include <stdint.h>

#define POKEDEX_COUNT 151
#define POKEDEX_TAB_COUNT 5
#define POKEDEX_HOME_COUNT 2
#define POKEDEX_MOVE_MAX 8
#define POKEDEX_EVO_MAX 8

typedef enum {
    POKEDEX_SCREEN_HOME = 0,
    POKEDEX_SCREEN_ENTRY,
} pokedex_screen_t;

typedef enum {
    POKEDEX_HOME_BROWSE = 0,
    POKEDEX_HOME_RANDOM = 1,
} pokedex_home_t;

typedef enum {
    POKEDEX_TAB_COVER = 0,
    POKEDEX_TAB_BIO,
    POKEDEX_TAB_STATS,
    POKEDEX_TAB_MOVES,
    POKEDEX_TAB_EVO,
} pokedex_tab_t;

typedef enum {
    POKEDEX_ACT_NONE = 0,
    POKEDEX_ACT_PLAY_CRY,
} pokedex_act_t;

typedef struct {
    pokedex_screen_t screen;
    int home_sel;
    int id;
    pokedex_tab_t tab;
    int last_random_id;
} pokedex_state_t;

typedef struct {
    const char *zh;
    const char *en;
    const char *category;
    const char *type_a;
    const char *type_b;
    const char *intro;
    uint8_t hp;
    uint8_t atk;
    uint8_t def_;
    uint8_t spa;
    uint8_t spd;
    uint8_t spe;
    uint8_t move_n;
    uint8_t move_lv[POKEDEX_MOVE_MAX];
    const char *move_zh[POKEDEX_MOVE_MAX];
    uint8_t evo_n;
    const char *evo_zh[POKEDEX_EVO_MAX];
    const char *evo_cond[POKEDEX_EVO_MAX];
} pokedex_entry_t;

void pokedex_init(pokedex_state_t *s);
int pokedex_is_home(const pokedex_state_t *s);
const pokedex_entry_t *pokedex_entry(int id);

void pokedex_home_move(pokedex_state_t *s, int delta);
void pokedex_enter_from_home(pokedex_state_t *s, uint32_t rng);

void pokedex_step_id(pokedex_state_t *s, int delta);
void pokedex_next_tab(pokedex_state_t *s);
pokedex_act_t pokedex_ok_long(pokedex_state_t *s);

int pokedex_wrap_id(int id, int count);
int pokedex_wrap_index(int index, int count);
int pokedex_pick_random(int count, int avoid, uint32_t rng);
