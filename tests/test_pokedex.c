#include <assert.h>
#include <stdint.h>
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
    assert(s.home_sel == POKEDEX_HOME_GUESS);
    pokedex_home_move(&s, 1);
    assert(s.home_sel == POKEDEX_HOME_FACTS);
    pokedex_home_move(&s, 1);
    assert(s.home_sel == POKEDEX_HOME_BROWSE);

    pokedex_ok_long(&s);
    assert(pokedex_is_settings(&s));
    assert(pokedex_settings_sel(&s) == POKEDEX_SET_VOLUME);
    pokedex_settings_toggle(&s);
    assert(pokedex_settings_sel(&s) == POKEDEX_SET_BACKLIGHT);
    pokedex_ok_long(&s);
    assert(pokedex_is_home(&s));
    assert(!pokedex_is_settings(&s));

    pokedex_enter_from_home(&s, 0);
    assert(!pokedex_is_home(&s));
    assert(s.id == 1);
    assert(s.tab == POKEDEX_TAB_COVER);

    pokedex_step_id(&s, 1, 0);
    assert(s.id == 2);
    pokedex_step_id(&s, -2, 0);
    assert(s.id == 151);

    pokedex_next_tab(&s);
    assert(s.tab == POKEDEX_TAB_BIO);
    for (int i = 0; i < POKEDEX_TAB_COUNT - 1; i++) {
        pokedex_next_tab(&s);
    }
    assert(s.tab == POKEDEX_TAB_COVER);

    assert(!pokedex_is_zoomed(&s));
    pokedex_toggle_zoom(&s);
    assert(pokedex_is_zoomed(&s));
    pokedex_toggle_zoom(&s);
    assert(!pokedex_is_zoomed(&s));
    pokedex_toggle_zoom(&s);
    pokedex_step_id(&s, 1, 0);
    assert(s.id == 1);
    assert(pokedex_is_zoomed(&s));
    pokedex_next_tab(&s);
    assert(!pokedex_is_zoomed(&s));
    assert(s.tab == POKEDEX_TAB_BIO);
    pokedex_toggle_zoom(&s);
    assert(!pokedex_is_zoomed(&s));
    pokedex_ok_long(&s);
    assert(pokedex_is_home(&s));
    assert(!pokedex_is_zoomed(&s));

    pokedex_home_move(&s, 1);
    pokedex_enter_from_home(&s, 24);
    assert(s.id == 25);
    pokedex_act_t act = pokedex_ok_long(&s);
    assert(act == POKEDEX_ACT_PLAY_CRY);
    assert(pokedex_is_home(&s));

    pokedex_enter_from_home(&s, 24);
    assert(s.id != 25);

    pokedex_init(&s);
    pokedex_home_move(&s, 1);
    pokedex_enter_from_home(&s, 24);
    assert(s.id == 25);
    assert(s.random_walk);
    pokedex_step_id(&s, 1, 0);
    assert(s.id == 1);
    pokedex_step_id(&s, -1, 1);
    assert(s.id == pokedex_pick_random(POKEDEX_COUNT, 1, 1));

    pokedex_init(&s);
    pokedex_home_move(&s, 1);
    pokedex_home_move(&s, 1);
    pokedex_home_move(&s, 1);
    assert(s.home_sel == POKEDEX_HOME_FACTS);
    pokedex_enter_from_home(&s, 0);
    assert(pokedex_is_fact(&s));
    assert(!pokedex_is_home(&s));
    int first_fact = s.fact_index;
    assert(first_fact >= 0 && first_fact < pokedex_fact_count());
    pokedex_step_fact(&s, 1, 1);
    assert(s.fact_index == pokedex_wrap_index(first_fact + 1, pokedex_fact_count()));
    int first_speaker = s.speaker;
    assert(first_speaker >= 0 && first_speaker < POKEDEX_SPEAKER_COUNT);
    assert(pokedex_speaker_name(first_speaker)[0] != '\0');
    pokedex_step_fact(&s, -1, 0);
    assert(s.fact_index == first_fact);
    assert(s.speaker != first_speaker);
    assert(s.portrait_corner >= 0 && s.portrait_corner < POKEDEX_CORNER_COUNT);
    assert(strcmp(pokedex_speaker_name(0), "大木博士") == 0);
    assert(strcmp(pokedex_speaker_name(4), "小茂") == 0);
    assert(pokedex_pick_slot(5, 0, 0) == 1);
    assert(pokedex_pick_slot(5, 2, 2) != 2);
    pokedex_act_t fact_act = pokedex_ok_long(&s);
    assert(fact_act == POKEDEX_ACT_NONE);
    assert(pokedex_is_home(&s));
    assert(!pokedex_is_fact(&s));

    pokedex_init(&s);
    pokedex_home_move(&s, 1);
    pokedex_home_move(&s, 1);
    assert(s.home_sel == POKEDEX_HOME_GUESS);
    pokedex_enter_from_home(&s, 24);
    assert(pokedex_is_guess(&s));
    assert(!pokedex_guess_revealed(&s));
    assert(s.id == 25);
    assert(pokedex_guess_clue(&s) == POKEDEX_GUESS_SILHOUETTE);
    pokedex_next_tab(&s);
    assert(pokedex_guess_revealed(&s));
    assert(s.tab == POKEDEX_TAB_COVER);
    pokedex_next_tab(&s);
    assert(s.tab == POKEDEX_TAB_BIO);
    pokedex_step_id(&s, 1, 1u << 11);
    assert(s.id == pokedex_pick_random(POKEDEX_COUNT, 25, 1u << 11));
    assert(pokedex_is_guess(&s));
    assert(!pokedex_guess_revealed(&s));
    assert(pokedex_guess_clue(&s) == POKEDEX_GUESS_BIO);
    assert(s.tab == POKEDEX_TAB_COVER);
    pokedex_toggle_zoom(&s);
    assert(!pokedex_is_zoomed(&s));
    pokedex_act_t guess_act = pokedex_ok_long(&s);
    assert(guess_act == POKEDEX_ACT_NONE);
    assert(pokedex_is_home(&s));
    assert(!pokedex_is_guess(&s));

    pokedex_init(&s);
    pokedex_home_move(&s, 1);
    pokedex_home_move(&s, 1);
    pokedex_enter_from_home(&s, 1u << 11);
    assert(pokedex_is_guess(&s));
    assert(pokedex_guess_clue(&s) == POKEDEX_GUESS_BIO);

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
    assert(one->trivia && one->trivia[0]);
    assert(strcmp(one->trivia, one->intro) != 0);
    assert(pokedex_has_trivia(one));
    assert(strcmp(pokedex_trivia_text(one), one->trivia) == 0);
    assert(!pokedex_has_trivia(pokedex_entry(151)));
    assert(pokedex_trivia_text(pokedex_entry(151))[0] != '\0');
    assert(strcmp(pokedex_guess_bio_text(one), one->intro) == 0);
    assert(strcmp(pokedex_guess_bio_text(pika), pika->trivia) == 0);
    assert(strstr(pokedex_guess_bio_text(pokedex_entry(94)), "耿鬼") == NULL);
    assert(one->ability_n >= 1);
    assert(one->ability_zh[0] && one->ability_zh[0][0]);
    assert(one->ability_intro[0] && one->ability_intro[0][0]);
    assert(one->height_dm == 7);
    assert(one->weight_hg == 69);
    assert(one->catch_rate == 45);
    assert(one->gender_rate == 1);
    assert(pika->gender_rate == 4);
    assert(pokedex_entry(29)->gender_rate == 8);
    assert(pokedex_entry(32)->gender_rate == 0);
    assert(pokedex_entry(151)->gender_rate == -1);

    char size[32];
    char meta[48];
    assert(pokedex_format_size(one, size, sizeof(size)) > 0);
    assert(strcmp(size, "0.7m  6.9kg") == 0);
    assert(pokedex_format_meta(one, meta, sizeof(meta)) > 0);
    assert(strcmp(meta, "雄87% 雌13%") == 0);
    assert(strstr(meta, "捕获") == NULL);
    assert(pokedex_format_meta(pokedex_entry(151), meta, sizeof(meta)) > 0);
    assert(strcmp(meta, "无性别") == 0);
    assert(pokedex_fact_count() >= 220);
    const char *fact0 = pokedex_fact(0);
    assert(fact0 && fact0[0]);
    assert(strcmp(pokedex_fact((uint32_t)pokedex_fact_count()), fact0) == 0);
    assert(strcmp(pokedex_fact_at(0), fact0) == 0);
    assert(strcmp(pokedex_fact_at(pokedex_fact_count()), fact0) == 0);

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
