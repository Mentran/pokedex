#pragma once

#include <stddef.h>
#include <stdint.h>

#define POKEDEX_COUNT 151
#define POKEDEX_TAB_COUNT 6
#define POKEDEX_HOME_COUNT 4
#define POKEDEX_MOVE_MAX 24
#define POKEDEX_EVO_MAX 8
#define POKEDEX_MATCH_MAX 18
#define POKEDEX_ABILITY_MAX 2
#define POKEDEX_SPEAKER_COUNT 5
#define POKEDEX_BOOT_SCENE_COUNT 1
#define POKEDEX_CORNER_COUNT 2

typedef enum {
    POKEDEX_SCREEN_HOME = 0,
    POKEDEX_SCREEN_ENTRY,
    POKEDEX_SCREEN_FACT,
} pokedex_screen_t;

typedef enum {
    POKEDEX_HOME_BROWSE = 0,
    POKEDEX_HOME_RANDOM = 1,
    POKEDEX_HOME_GUESS = 2,
    POKEDEX_HOME_FACTS = 3,
} pokedex_home_t;

typedef enum {
    POKEDEX_TAB_COVER = 0,
    POKEDEX_TAB_BIO,
    POKEDEX_TAB_STATS,
    POKEDEX_TAB_MOVES,
    POKEDEX_TAB_MATCHUP,
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
    int random_walk;
    int guess_mode;
    int guess_revealed;
    int fact_index;
    int last_fact_index;
    int speaker;
    int last_speaker;
    int portrait_corner;
    int last_portrait_corner;
    int zoomed;
} pokedex_state_t;

typedef struct {
    const char *zh;
    const char *en;
    const char *category;
    const char *type_a;
    const char *type_b;
    const char *intro;
    const char *trivia;
    uint8_t height_dm;
    uint16_t weight_hg;
    uint8_t catch_rate;
    int8_t gender_rate;
    uint8_t ability_n;
    const char *ability_zh[POKEDEX_ABILITY_MAX];
    const char *ability_intro[POKEDEX_ABILITY_MAX];
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
int pokedex_is_fact(const pokedex_state_t *s);
int pokedex_is_guess(const pokedex_state_t *s);
int pokedex_guess_revealed(const pokedex_state_t *s);
void pokedex_reveal_guess(pokedex_state_t *s);
const pokedex_entry_t *pokedex_entry(int id);
int pokedex_has_trivia(const pokedex_entry_t *e);
const char *pokedex_trivia_text(const pokedex_entry_t *e);

void pokedex_home_move(pokedex_state_t *s, int delta);
void pokedex_enter_from_home(pokedex_state_t *s, uint32_t rng);

void pokedex_step_id(pokedex_state_t *s, int delta, uint32_t rng);
void pokedex_step_fact(pokedex_state_t *s, int delta, uint32_t rng);
int pokedex_pick_slot(int count, int avoid, uint32_t rng);
const char *pokedex_speaker_name(int speaker);
void pokedex_next_tab(pokedex_state_t *s);
void pokedex_toggle_zoom(pokedex_state_t *s);
int pokedex_is_zoomed(const pokedex_state_t *s);
pokedex_act_t pokedex_ok_long(pokedex_state_t *s);

int pokedex_wrap_id(int id, int count);
int pokedex_wrap_index(int index, int count);
int pokedex_pick_random(int count, int avoid, uint32_t rng);
int pokedex_stat_total(const pokedex_entry_t *e);

typedef struct {
    const char *weak[POKEDEX_MATCH_MAX];
    uint8_t weak_x[POKEDEX_MATCH_MAX];
    int weak_n;
    const char *resist[POKEDEX_MATCH_MAX];
    uint8_t resist_x[POKEDEX_MATCH_MAX];
    int resist_n;
    const char *immune[POKEDEX_MATCH_MAX];
    int immune_n;
    const char *hit2[POKEDEX_MATCH_MAX];
    int hit2_n;
    const char *hit_half[POKEDEX_MATCH_MAX];
    int hit_half_n;
    const char *hit0[POKEDEX_MATCH_MAX];
    int hit0_n;
} pokedex_matchup_t;

void pokedex_matchup(const char *type_a, const char *type_b, pokedex_matchup_t *out);
int pokedex_format_size(const pokedex_entry_t *e, char *dst, size_t n);
int pokedex_format_meta(const pokedex_entry_t *e, char *dst, size_t n);
int pokedex_fact_count(void);
const char *pokedex_fact_at(int index);
const char *pokedex_fact(uint32_t rng);
