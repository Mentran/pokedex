#include <assert.h>
#include <string.h>
#include "pokedex.h"

int main(void)
{
    assert(pokedex_wrap_id(0, 151) == 151);
    assert(pokedex_wrap_id(1, 151) == 1);
    assert(pokedex_wrap_id(151, 151) == 151);
    assert(pokedex_wrap_id(152, 151) == 1);
    assert(pokedex_wrap_id(-1, 151) == 150);

    assert(pokedex_pick_random(151, 0, 0) == 1);
    assert(pokedex_pick_random(151, 0, 24) == 25);
    assert(pokedex_pick_random(151, 25, 24) != 25);
    assert(pokedex_pick_random(1, 1, 99) == 1);

    pokedex_state_t s;
    pokedex_init(&s);
    assert(pokedex_is_home(&s));
    assert(s.home_sel == POKEDEX_HOME_BROWSE);

    pokedex_home_move(&s, 1);
    assert(s.home_sel == POKEDEX_HOME_RANDOM);
    pokedex_home_move(&s, 1);
    assert(s.home_sel == POKEDEX_HOME_BROWSE);

    pokedex_enter_from_home(&s, 0);
    assert(!pokedex_is_home(&s));
    assert(s.id == 1);
    assert(s.tab == POKEDEX_TAB_COVER);

    pokedex_step_id(&s, 1);
    assert(s.id == 2);
    pokedex_step_id(&s, -2);
    assert(s.id == 151);

    pokedex_next_tab(&s);
    assert(s.tab == POKEDEX_TAB_BIO);
    for (int i = 0; i < POKEDEX_TAB_COUNT - 1; i++) {
        pokedex_next_tab(&s);
    }
    assert(s.tab == POKEDEX_TAB_COVER);

    pokedex_ok_long(&s);
    assert(pokedex_is_home(&s));

    pokedex_home_move(&s, 1);
    pokedex_enter_from_home(&s, 24);
    assert(s.id == 25);
    pokedex_act_t act = pokedex_ok_long(&s);
    assert(act == POKEDEX_ACT_PLAY_CRY);
    assert(pokedex_is_home(&s));

    pokedex_enter_from_home(&s, 24);
    assert(s.id != 25);

    const pokedex_entry_t *one = pokedex_entry(1);
    const pokedex_entry_t *pika = pokedex_entry(25);
    const pokedex_entry_t *eevee = pokedex_entry(133);
    const pokedex_entry_t *helix = pokedex_entry(138);
    assert(one && strcmp(one->en, "bulbasaur") == 0);
    assert(pika && strcmp(pika->en, "pikachu") == 0);
    assert(eevee && strcmp(eevee->en, "eevee") == 0);
    assert(helix && helix->type_a[0] != '\0');
    assert(pokedex_stat_total(one) == 318);
    assert(pokedex_stat_total(pika) == 320);
    assert(pika->move_n > 8);

    pokedex_matchup_t m;
    pokedex_matchup(one->type_a, one->type_b, &m);
    int saw_fire = 0, saw_water = 0, saw_fairy = 0, saw_quarter = 0;
    for (int i = 0; i < m.weak_n; i++) {
        if (strcmp(m.weak[i], "火") == 0) {
            saw_fire = 1;
        }
    }
    for (int i = 0; i < m.resist_n; i++) {
        if (strcmp(m.resist[i], "草") == 0 && m.resist_x[i] == 2) {
            saw_quarter = 1;
        }
    }
    for (int i = 0; i < m.hit2_n; i++) {
        if (strcmp(m.hit2[i], "水") == 0) {
            saw_water = 1;
        }
        if (strcmp(m.hit2[i], "妖精") == 0) {
            saw_fairy = 1;
        }
    }
    assert(saw_fire && saw_water && saw_fairy && saw_quarter);
    assert(pokedex_entry(0) == 0);
    assert(pokedex_entry(152) == 0);
    return 0;
}
