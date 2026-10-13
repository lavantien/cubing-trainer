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
    TSS_CELL = 1,
    TSS_FIND_ROWS = 5,
    TSS_OVERFLOW = 2
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
    CT_ASSERT_EQ(f.memo_row_count, 0);
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
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_4X4), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_method(&f, CT_METHOD_YAU), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.lists[0].stage, CT_STAGE_PARITY);
    CT_ASSERT_EQ(f.case_count, 5);
    CT_ASSERT_EQ(f.group_count, 1);
    CT_ASSERT_EQ(f.group_counts[0], 5);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_5X5), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_method(&f, CT_METHOD_REDUCTION), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.case_count, 17);
    CT_ASSERT_EQ(f.group_count, 1);
    CT_ASSERT_EQ(f.group_counts[0], 17);
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

CT_SUITE(trainer_decomp_lines) {
    ct_trainer_form f;
    memset(&f, 0, sizeof f);
    char lines[CT_TRAINER_DECOMP_LINES][CT_TRAINER_DECOMP_MAX];
    CT_ASSERT_EQ(ct_trainer_decomp(NULL, lines, CT_TRAINER_DECOMP_LINES), 0);
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, 0), 0);
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 0);
    f.sel.case_index = 0;
    f.alg_status = CT_REPO_MISSING;
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 0);
    f.alg_status = CT_REPO_OK;
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 0);
    snprintf(f.alg.sequence, sizeof f.alg.sequence, "%s", "S");
    snprintf(f.alg.setup, sizeof f.alg.setup, "%s", "x y");
    snprintf(f.alg.interchange, sizeof f.alg.interchange, "%s", "R2 D2");
    snprintf(f.alg.insertion, sizeof f.alg.insertion, "%s", "R' U' R");
    f.alg.classification = CT_CLS_PER_SPECIAL;
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 4);
    CT_ASSERT_STR_EQ(lines[0], "setup x y");
    CT_ASSERT_STR_EQ(lines[1], "interchange R2 D2");
    CT_ASSERT_STR_EQ(lines[2], "insertion R' U' R");
    CT_ASSERT_STR_EQ(lines[3], "class per special");
    f.sel.case_index = CT_TRAINER_CELL_NONE;
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 0);
    f.sel.case_index = 0;
    memset(lines, 0, sizeof lines);
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, 0), 0);
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, 1), 1);
    CT_ASSERT_STR_EQ(lines[0], "setup x y");
    CT_ASSERT_EQ(lines[1][0], '\0');
    f.alg.interchange[0] = '\0';
    f.alg.classification = CT_CLS_SP7;
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 3);
    CT_ASSERT_STR_EQ(lines[1], "insertion R' U' R");
    CT_ASSERT_STR_EQ(lines[2], "class SP7");
    memset(lines, 0, sizeof lines);
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, 2), 2);
    CT_ASSERT_STR_EQ(lines[0], "setup x y");
    CT_ASSERT_STR_EQ(lines[1], "insertion R' U' R");
    CT_ASSERT_STR_EQ(lines[2], "");
    snprintf(f.alg.setup, sizeof f.alg.setup, "%s", "x");
    snprintf(f.alg.insertion, sizeof f.alg.insertion, "%s", "R");
    memset(lines, 0, sizeof lines);
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 3);
    CT_ASSERT_STR_EQ(lines[0], "setup x");
    CT_ASSERT_STR_EQ(lines[1], "insertion R");
    CT_ASSERT_STR_EQ(lines[2], "class SP7");
    f.alg.setup[0] = '\0';
    memset(lines, 0, sizeof lines);
    CT_ASSERT_EQ(ct_trainer_decomp(&f, lines, CT_TRAINER_DECOMP_LINES), 2);
    CT_ASSERT_STR_EQ(lines[0], "insertion R");
    CT_ASSERT_STR_EQ(lines[1], "class SP7");
    CT_ASSERT_EQ(lines[2][0], '\0');
}

CT_SUITE(trainer_form_visible_count_edges) {
    CT_ASSERT_EQ(ct_trainer_form_visible_count(NULL), 0);
    ct_trainer_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f), 0);
    f.case_count = 1;
    f.cell = CT_TRAINER_CELL_NONE;
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f), 1);
    f.case_count = 0;
    f.cell = 2;
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f), 0);
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

CT_SUITE(trainer_form_memo_lifecycle) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(f.memo_row_count, 0);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(f.memo_row_count, CT_TRAINER_MEMO_ROWS_MAX);
    CT_ASSERT_STR_EQ(f.memo_rows[0].pair, "AA");
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", "ph", "img"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.memo_row_count, CT_TRAINER_MEMO_ROWS_MAX);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_STR_EQ(f.front, "AB");
    CT_ASSERT_STR_EQ(f.memo_phoneme, "ph");
    CT_ASSERT_STR_EQ(f.memo_image, "img");
    CT_ASSERT_STR_EQ(f.memo_saved_phoneme, "ph");
    CT_ASSERT_STR_EQ(f.memo_saved_image, "img");
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 1), CT_REPO_OK);
    CT_ASSERT_STR_EQ(f.memo_phoneme, "");
    CT_ASSERT_STR_EQ(f.memo_image, "");
    CT_ASSERT_STR_EQ(f.memo_saved_phoneme, "");
    CT_ASSERT_STR_EQ(f.memo_saved_image, "");
    CT_ASSERT_EQ(f.memo_row_count, CT_TRAINER_MEMO_ROWS_MAX);
    ct_db_close(db);
}

CT_SUITE(trainer_form_memo_find_rows) {
    sqlite3 *db = tss_db();
    static const char *const pairs[TSS_FIND_ROWS] = {"AB", "AC", "AD", "AE", "AF"};
    static const char *const images[TSS_FIND_ROWS] = {"imgB", "imgC", "imgD", "imgE", "imgF"};
    for (int i = 0; i < TSS_FIND_ROWS; i++) {
        CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, pairs[i], "ph", images[i]), CT_REPO_OK);
    }
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(f.memo_row_count, CT_TRAINER_MEMO_ROWS_MAX);
    for (int i = 0; i < f.memo_row_count; i++) {
        const ct_memo_list_row *row = ct_trainer_form_memo_find(&f, f.memo_rows[i].pair);
        CT_ASSERT(row != NULL);
        CT_ASSERT_STR_EQ(row->pair, f.memo_rows[i].pair);
    }
    for (int i = 0; i < TSS_FIND_ROWS; i++) {
        const ct_memo_list_row *row = ct_trainer_form_memo_find(&f, pairs[i]);
        CT_ASSERT(row != NULL);
        CT_ASSERT_STR_EQ(row->image, images[i]);
    }
    ct_db_close(db);
}

CT_SUITE(trainer_form_memo_save_guards) {
    sqlite3 *db = tss_db();
    CT_ASSERT_EQ(ct_trainer_form_memo_save(NULL), CT_REPO_ERR_ARG);
    CT_ASSERT(ct_trainer_form_memo_find(NULL, "AB") == NULL);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", "sentinel", "sentinel"), CT_REPO_OK);
    ct_trainer_form f;
    memset(&f, 0, sizeof f);
    f.db = db;
    snprintf(f.front, sizeof f.front, "%s", "AB");
    snprintf(f.memo_phoneme, sizeof f.memo_phoneme, "%s", "p");
    snprintf(f.memo_image, sizeof f.memo_image, "%s", "i");
    f.sel.case_index = 0;
    f.sel.list_id = -1;
    CT_ASSERT_EQ(ct_trainer_form_memo_save(&f), CT_REPO_ERR_ARG);
    f.sel.list_id = 1;
    f.list_count = 1;
    f.lists[1].orbit = CT_ORBIT_CORNER;
    f.lists[1].source = CT_SELECTOR_CHART;
    CT_ASSERT_EQ(ct_trainer_form_memo_save(&f), CT_REPO_ERR_ARG);
    f.sel.list_id = 2;
    f.lists[2].orbit = CT_ORBIT_CORNER;
    f.lists[2].source = CT_SELECTOR_CHART;
    CT_ASSERT_EQ(ct_trainer_form_memo_save(&f), CT_REPO_ERR_ARG);
    ct_memo_row row;
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "AB", &row), CT_REPO_OK);
    CT_ASSERT_STR_EQ(row.phoneme, "sentinel");
    CT_ASSERT_STR_EQ(row.image, "sentinel");
    ct_db_close(db);
}

CT_SUITE(trainer_form_all_lists_counts) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    for (ct_event ev = CT_EVENT_3X3; ev <= CT_EVENT_FTO; ev++) {
        ct_method methods[CT_TRAINER_LISTS_MAX];
        int method_count = ct_selector_methods(ev, methods, CT_TRAINER_LISTS_MAX);
        CT_ASSERT(method_count > 0);
        for (int mi = 0; mi < method_count; mi++) {
            CT_ASSERT_EQ(ct_trainer_form_select_event(&f, ev), CT_REPO_OK);
            CT_ASSERT_EQ(ct_trainer_form_select_method(&f, methods[mi]), CT_REPO_OK);
            int lists = ct_selector_lists(ev, methods[mi], NULL, 0);
            CT_ASSERT(lists > 0);
            for (int li = 0; li < lists; li++) {
                CT_ASSERT_EQ(ct_trainer_form_select_list(&f, li), CT_REPO_OK);
                CT_ASSERT_EQ(f.sel.list_id, li);
                CT_ASSERT_EQ(f.list_count, lists);
                CT_ASSERT_EQ(f.case_count,
                             ct_selector_case_count(db, &f.scheme, ev, methods[mi], li));
            }
        }
    }
    ct_db_close(db);
}

CT_SUITE(trainer_form_cell_pair_overflow) {
    sqlite3 *db = tss_db();
    ct_trainer_form f;
    memset(&f, 0, sizeof f);
    f.db = db;
    f.sel.event = CT_EVENT_3BLD;
    f.sel.method = CT_METHOD_3STYLE;
    f.sel.list_id = 0;
    f.list_count = 1;
    f.lists[0].orbit = CT_ORBIT_CORNER;
    f.lists[0].source = CT_SELECTOR_CHART;
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &f.scheme), 0);
    f.case_count = TSS_CORNER_CASES + TSS_OVERFLOW;
    f.cell = TSS_CELL;
    static ct_target_pair pairs[TSS_CORNER_CASES];
    CT_ASSERT_EQ(ct_targets_corner(f.scheme.buffer_corner, pairs, TSS_CORNER_CASES),
                 TSS_CORNER_CASES);
    int expect = 0;
    for (int i = 0; i < TSS_CORNER_CASES; i++) {
        if (pairs[i].t1 == TSS_CELL || pairs[i].t2 == TSS_CELL) {
            expect++;
        }
    }
    CT_ASSERT(expect > 0);
    CT_ASSERT_EQ(ct_trainer_form_visible_count(&f), expect);
    CT_ASSERT_EQ(ct_trainer_form_nth_visible(&f, expect), -1);
    ct_db_close(db);
}

static bool trainer_screen_bound = false;

static void trainer_screen_cleanup(void) {
    if (!trainer_screen_bound) {
        return;
    }
    ct_trainer_form_open(ct_screen_trainer_form(), NULL);
    trainer_screen_bound = false;
}

CT_SUITE(trainer_screen_update_reopen) {
    trainer_screen_cleanup();
    ct_screen_trainer_update(NULL);
    sqlite3 *db = tss_db();
    ct_app app;
    memset(&app, 0, sizeof app);
    app.db = db;
    app.screen = CT_SCREEN_TRAINER;
    ct_screen_trainer_update(&app);
    trainer_screen_bound = true;
    ct_trainer_form *f = ct_screen_trainer_form();
    CT_ASSERT(f->db == db);
    CT_ASSERT_EQ(f->sel.event, CT_EVENT_3X3);
    CT_ASSERT_EQ(ct_trainer_form_select_case(f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f->sel.case_index, 0);
    ct_screen_trainer_update(&app);
    CT_ASSERT_EQ(f->sel.case_index, 0);
    sqlite3 *db2 = tss_db();
    ct_app app2;
    memset(&app2, 0, sizeof app2);
    app2.db = db2;
    app2.screen = CT_SCREEN_TRAINER;
    ct_screen_trainer_update(&app2);
    CT_ASSERT(ct_screen_trainer_form()->db == db2);
    CT_ASSERT_EQ(ct_screen_trainer_form()->sel.case_index, -1);
    trainer_screen_cleanup();
    CT_ASSERT(ct_screen_trainer_form()->db == NULL);
    ct_db_close(db);
    ct_db_close(db2);
}
