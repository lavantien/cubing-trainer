#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/config/config.h"
#include "../src/gen/targets.h"
#include "../src/packs/packs.h"
#include "../src/packs/seed_registry.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_algs.h"
#include "../src/trainer/selector.h"
#include "../src/ui/ui.h"
#include "framework.h"

enum {
    TSS_KEY_MAX = 64,
    TSS_CORNER_CASES = 378,
    TSS_EDGE_CASES = 440,
    TSS_CUBESHAPE_ROWS = 169,
    TSS_SPECIALS_ROWS = 85,
    TSS_PLL_CASES = 21,
    TSS_SAMPLES = 3,
    TSS_CELL = 1
};

static sqlite3 *tss_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_pack_seed_registered(db, "db/seeds/packs"), 0);
    return db;
}

CT_SUITE(trainer_form_open) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(NULL, db), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_open(&f, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT(f.db == db);
    CT_ASSERT_EQ(f.sel.event, CT_EVENT_3X3);
    CT_ASSERT_EQ(f.sel.method, CT_METHOD_CFOP);
    CT_ASSERT_EQ(f.sel.list_id, 0);
    CT_ASSERT_EQ(f.list_count, 3);
    CT_ASSERT_EQ(f.case_count, TSS_PLL_CASES);
    CT_ASSERT_EQ(f.cell, CT_TRAINER_CELL_NONE);
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        CT_ASSERT_EQ(f.scheme.map.corner[i], 'A' + i);
    }
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    ct_db_close(db);
}

CT_SUITE(trainer_form_navigation) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_event(NULL, CT_EVENT_3X3), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_method(NULL, CT_METHOD_CFOP), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_list(NULL, 0), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_case(NULL, 0), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(NULL), 0);
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(NULL, 0), -1);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_SQUARE1), CT_REPO_OK);
    CT_ASSERT_EQ(f.sel.method, CT_METHOD_LBL);
    CT_ASSERT_EQ(f.list_count, 5);
    CT_ASSERT_EQ(f.sel.list_id, 0);
    CT_ASSERT_EQ(f.case_count, TSS_CUBESHAPE_ROWS);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_FMC), CT_REPO_OK);
    CT_ASSERT_EQ(f.sel.method, CT_METHOD_CONCEPT);
    CT_ASSERT_EQ(f.list_count, 1);
    CT_ASSERT_EQ(f.case_count, 16);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, (ct_event)-1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, (ct_event)99), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(f.sel.event, CT_EVENT_FMC);
    CT_ASSERT_EQ(ct_trainer_form_select_method(&f, CT_METHOD_CFOP), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_SQUARE1), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_method(&f, CT_METHOD_LBL), CT_REPO_OK);
    CT_ASSERT_EQ(f.sel.list_id, 0);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.lists[0].stage, CT_STAGE_CUBESHAPE);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 4), CT_REPO_OK);
    CT_ASSERT_EQ(f.lists[4].stage, CT_STAGE_EP);
    CT_ASSERT_EQ(f.case_count, 99);
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(&f, 0), 0);
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(&f, 98), 98);
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(&f, 99), -1);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 5), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, -1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(f.sel.list_id, 4);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(f.sel.method, CT_METHOD_3STYLE);
    CT_ASSERT_EQ(f.list_count, 3);
    CT_ASSERT_EQ(f.case_count, TSS_CORNER_CASES);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 1), CT_REPO_OK);
    CT_ASSERT_EQ(f.case_count, TSS_EDGE_CASES);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 2), CT_REPO_OK);
    CT_ASSERT_EQ(f.lists[2].stage, CT_STAGE_PARITY);
    CT_ASSERT_EQ(f.lists[2].source, CT_SELECTOR_PACK);
    CT_ASSERT_EQ(f.case_count, TSS_SPECIALS_ROWS);
    CT_ASSERT_EQ(f.group_count, 4);
    CT_ASSERT_EQ(f.group_counts[0], 25);
    CT_ASSERT_EQ(f.group_counts[1], 23);
    CT_ASSERT_EQ(f.group_counts[2], 26);
    CT_ASSERT_EQ(f.group_counts[3], 11);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.group_count, 0);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_SQUARE1), CT_REPO_OK);
    CT_ASSERT_EQ(f.group_count, 1);
    CT_ASSERT_EQ(f.group_counts[0], TSS_CUBESHAPE_ROWS);
    ct_db_close(db);
}

CT_SUITE(trainer_form_case_detail) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_SQUARE1), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, -1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, TSS_CUBESHAPE_ROWS), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.sel.case_index, 0);
    CT_ASSERT_EQ(f.pack_status, CT_PACK_OK);
    char key[TSS_KEY_MAX];
    CT_ASSERT_EQ(
        ct_selector_case_key(db, &f.scheme, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, 0, key, sizeof key),
        0);
    ct_pack_entry want;
    CT_ASSERT_EQ(ct_pack_get(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, key, &want),
                 CT_PACK_OK);
    CT_ASSERT_STR_EQ(f.front, want.front_json);
    CT_ASSERT_STR_EQ(f.alg_text, want.alg);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3X3), CT_REPO_OK);
    CT_ASSERT_EQ(f.sel.case_index, -1);
    CT_ASSERT_STR_EQ(f.front, "");
    CT_ASSERT_STR_EQ(f.alg_text, "");
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.pack_status, CT_TRAINER_NO_DETAIL);
    CT_ASSERT_STR_EQ(f.alg_text, "");
    CT_ASSERT(strlen(f.front) > 0);
    ct_db_close(db);
}

CT_SUITE(trainer_form_generated_alg) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_STR_EQ(f.front, "AB");
    CT_ASSERT_STR_EQ(f.alg_text, "");
    CT_ASSERT(f.alg_status != CT_REPO_OK);
    ct_config cfg = ct_config_defaults();
    char hash[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, f.scheme_id, CT_BUFFER_FIXED, f.scheme.buffer_corner, f.scheme.buffer_edge,
                   &f.scheme.map, hash);
    ct_generated_alg a;
    memset(&a, 0, sizeof a);
    a.orbit = CT_ORBIT_CORNER;
    snprintf(a.target_pair, sizeof a.target_pair, "%s", "AB");
    a.buffer_mode = CT_BUFFER_FIXED;
    a.category = 0;
    snprintf(a.sequence, sizeof a.sequence, "%s", "[R U : [D2, R U R']]");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, hash), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.alg_status, CT_REPO_OK);
    CT_ASSERT_STR_EQ(f.alg_text, "[R U : [D2, R U R']]");
    ct_db_close(db);
}

CT_SUITE(trainer_form_select_refreshes_inputs) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(f.sel.method, CT_METHOD_3STYLE);

    ct_config cfg = ct_config_defaults();
    char hash_old[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, f.scheme_id, CT_BUFFER_FIXED, f.scheme.buffer_corner, f.scheme.buffer_edge,
                   &f.scheme.map, hash_old);
    ct_generated_alg old_alg;
    memset(&old_alg, 0, sizeof old_alg);
    old_alg.orbit = CT_ORBIT_CORNER;
    old_alg.buffer_mode = CT_BUFFER_FIXED;
    old_alg.category = 0;
    old_alg.classification = CT_CLS_PURE;
    old_alg.qtm = 9;
    old_alg.score = 1.0;
    snprintf(old_alg.target_pair, sizeof old_alg.target_pair, "%s", "AB");
    snprintf(old_alg.sequence, sizeof old_alg.sequence, "%s", "OLD_BUFFER_UFR");
    snprintf(old_alg.score_parts_json, sizeof old_alg.score_parts_json, "%s", "{\"len\":9}");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &old_alg, hash_old), CT_REPO_OK);

    ct_settings_form sf;
    ct_settings_form_open(&sf, db);
    CT_ASSERT_EQ(sf.status, CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&sf, CT_ORBIT_CORNER, 5), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_save(&sf), CT_REPO_OK);

    ct_selector_scheme fresh;
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &fresh), 0);
    CT_ASSERT_EQ(fresh.buffer_corner, 5);
    char hash_new[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, f.scheme_id, CT_BUFFER_FIXED, fresh.buffer_corner, fresh.buffer_edge,
                   &fresh.map, hash_new);
    CT_ASSERT(strcmp(hash_old, hash_new) != 0);
    ct_generated_alg new_alg = old_alg;
    snprintf(new_alg.sequence, sizeof new_alg.sequence, "%s", "NEW_BUFFER_5");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &new_alg, hash_new), CT_REPO_OK);

    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.scheme.buffer_corner, 5);
    CT_ASSERT_EQ(f.alg_status, CT_REPO_OK);
    CT_ASSERT_STR_EQ(f.alg_text, "NEW_BUFFER_5");

    ct_settings_form sf2;
    ct_settings_form_open(&sf2, db);
    CT_ASSERT_EQ(sf2.status, CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&sf2, CT_ORBIT_CORNER, 0, 'B'), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&sf2, CT_MOVE_BIT_L), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_save(&sf2), CT_REPO_OK);

    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.scheme.map.corner[0], 'B');
    CT_ASSERT_EQ(f.cfg.gen.preferred_moves & CT_MOVE_BIT_L, CT_MOVE_BIT_L);
    char hash_fresh[CT_ALG_HASH_HEX_MAX];
    ct_selector_scheme after;
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &after), 0);
    ct_params_hash(&f.cfg, f.scheme_id, CT_BUFFER_FIXED, after.buffer_corner, after.buffer_edge,
                   &after.map, hash_fresh);
    CT_ASSERT(strcmp(hash_new, hash_fresh) != 0);
    CT_ASSERT_STR_EQ(f.alg_text, "");
    CT_ASSERT(f.alg_status != CT_REPO_OK);
    ct_db_close(db);
}

CT_SUITE(trainer_form_chart_cells) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_cell(&f, TSS_CELL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(f.lists[0].source, CT_SELECTOR_CHART);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f), TSS_CORNER_CASES);
    CT_ASSERT_EQ(ct_trainer_form_select_cell(&f, -1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_cell(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_cell(&f, CT_LETTER_STICKERS), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_trainer_form_select_cell(&f, TSS_CELL), CT_REPO_OK);
    CT_ASSERT_EQ(f.cell, TSS_CELL);
    static ct_target_pair pairs[TSS_CORNER_CASES];
    CT_ASSERT_EQ(ct_targets_corner(CT_3STYLE_CORNER_BUFFER, pairs, TSS_CORNER_CASES),
                 TSS_CORNER_CASES);
    int expect = 0;
    for (int i = 0; i < TSS_CORNER_CASES; i++) {
        if (pairs[i].t1 == TSS_CELL || pairs[i].t2 == TSS_CELL) {
            expect++;
        }
    }
    CT_ASSERT(expect > 0);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f), expect);
    int first = -1;
    for (int i = 0; i < TSS_CORNER_CASES; i++) {
        if (pairs[i].t1 == TSS_CELL || pairs[i].t2 == TSS_CELL) {
            first = i;
            break;
        }
    }
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(&f, 0), first);
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(&f, expect), -1);
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(&f, -1), -1);
    int nth = ct_trainer_form_nth_visible(&f, 0);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, nth), CT_REPO_OK);
    char key[TSS_KEY_MAX];
    int t1 = -1;
    int t2 = -1;
    CT_ASSERT_EQ(
        ct_selector_case_pair(&f.scheme, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, nth, &t1, &t2), 0);
    CT_ASSERT(t1 == TSS_CELL || t2 == TSS_CELL);
    snprintf(key, sizeof key, "%c%c", f.scheme.map.corner[t1], f.scheme.map.corner[t2]);
    CT_ASSERT_STR_EQ(f.front, key);
    CT_ASSERT_EQ(ct_trainer_form_clear_cell(&f), CT_REPO_OK);
    CT_ASSERT_EQ(f.cell, CT_TRAINER_CELL_NONE);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f), TSS_CORNER_CASES);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 1), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_cell(&f, TSS_CELL), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f) > 0, true);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f) < TSS_EDGE_CASES, true);
    ct_db_close(db);
}

CT_SUITE(trainer_form_letter_scheme_routed) {
    sqlite3 *db = tss_db();
    int scheme = ct_letter_scheme_default_id(db);
    if (scheme <= 0) {
        CT_ASSERT_EQ(ct_letter_seed_speffz(db), 0);
        scheme = ct_letter_scheme_default_id(db);
    }
    CT_ASSERT(scheme > 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, 0, 1, 'C'), 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, 0, 2, 'B'), 0);
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_STR_EQ(f.front, "AC");
    ct_db_close(db);
}
