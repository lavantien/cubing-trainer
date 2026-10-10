#include <string.h>

#include <sqlite3.h>

#include "../src/config/config.h"
#include "../src/packs/packs.h"
#include "../src/storage/db.h"
#include "../src/trainer/selector.h"
#include "framework.h"

enum { SP_PACKS = 7, SP_METHODS = 2, SP_LISTS = 6, SP_KEY_MAX = 64 };

struct sp_pack {
    const char *path;
    ct_event event;
    ct_method method;
    ct_stage stage;
    int rows;
    const char *case_key;
    const char *alg;
};

static const struct sp_pack sp_packs[SP_PACKS] = {
    {"db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, 169,
     "cs.169", "/ 0,1 / 0,-2 / 0,2 / 0,-2 / 0,1 / 0,3 /"},
    {"db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO, 7, "co.05",
     "(1,0) / (0,3) / (0,3) / (-1,0)"},
    {"db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO, 7, "eo.04",
     "(1,0) / (3,0) / (3,0) / (-1,-1) / (-2,1) / (-4,-1) / (1,0)"},
    {"db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP, 16, "cpp.07",
     "/ (3,3) / (-1,0) / (2,0) / (-4,0) / (4,0) / (2,0) / (-5,0) / (-3,-3) /"},
    {"db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP, 99, "ep.50",
     "(D) / (-3,-3) / (-3,0) / (-1,-1) / (4,1) / (3,3)"},
    {"db/seeds/packs/clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT, 14,
     "clock.06",
     "Push down the bottom two pins, push up the top two. Turn a top corner until the centre"
     " clock matches the bottom middle."},
    {"db/seeds/packs/bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY, 5,
     "parity.wing5", "Rw U2 x Rw U2 Rw U2 Rw' U2 Lw U2 3Rw' U2 Rw U2 Rw' U2 Rw'"},
};

CT_SUITE(selector_pack_reach) {
    for (int p = 0; p < SP_PACKS; p++) {
        const struct sp_pack *sp = &sp_packs[p];
        sqlite3 *db = NULL;
        ct_db_open(&db, ":memory:");
        ct_db_migrate(db, "db/migrations");
        CT_ASSERT_EQ(ct_pack_seed_file(db, sp->path, sp->event, sp->method, sp->stage), sp->rows);
        ct_method methods[SP_METHODS];
        int method_count = ct_selector_methods(sp->event, methods, SP_METHODS);
        CT_ASSERT(method_count >= 1 && method_count <= SP_METHODS);
        int paired = 0;
        for (int i = 0; i < method_count; i++) {
            if (methods[i] == sp->method) {
                paired = 1;
            }
        }
        CT_ASSERT(paired);
        ct_selector_list lists[SP_LISTS];
        int list_count = ct_selector_lists(sp->event, sp->method, lists, SP_LISTS);
        CT_ASSERT(list_count >= 1 && list_count <= SP_LISTS);
        int list_id = -1;
        for (int i = 0; i < list_count; i++) {
            if (lists[i].stage == sp->stage) {
                list_id = i;
            }
        }
        CT_ASSERT(list_id >= 0);
        CT_ASSERT_EQ(lists[list_id].source, CT_SELECTOR_PACK);
        CT_ASSERT_EQ(ct_selector_case_count(db, NULL, sp->event, sp->method, list_id), sp->rows);
        ct_pack_entry got;
        CT_ASSERT_EQ(ct_pack_get(db, sp->event, sp->method, sp->stage, sp->case_key, &got),
                     CT_PACK_OK);
        CT_ASSERT_STR_EQ(got.alg, sp->alg);
        CT_ASSERT_STR_EQ(got.source_at, "2026-10-10");
        char key[SP_KEY_MAX];
        for (int r = 0; r < sp->rows; r++) {
            CT_ASSERT_EQ(
                ct_selector_case_key(db, NULL, sp->event, sp->method, list_id, r, key, sizeof key),
                0);
            CT_ASSERT_EQ(strlen(key) > 0, true);
        }
        CT_ASSERT(ct_selector_case_key(db, NULL, sp->event, sp->method, list_id, sp->rows, key,
                                       sizeof key) < 0);
        ct_pack_entry sel;
        CT_ASSERT_EQ(ct_selector_pack_case(db, sp->event, sp->method, list_id, 0, &sel), 0);
        CT_ASSERT_EQ(
            ct_selector_case_key(db, NULL, sp->event, sp->method, list_id, 0, key, sizeof key), 0);
        ct_pack_entry first;
        CT_ASSERT_EQ(ct_pack_get(db, sp->event, sp->method, sp->stage, key, &first), CT_PACK_OK);
        CT_ASSERT_STR_EQ(sel.case_key, first.case_key);
        CT_ASSERT_STR_EQ(sel.alg, first.alg);
        CT_ASSERT_STR_EQ(sel.front_json, first.front_json);
        ct_db_close(db);
    }
}

CT_SUITE(selector_pack_groups) {
    sqlite3 *db = NULL;
    ct_db_open(&db, ":memory:");
    ct_db_migrate(db, "db/migrations");
    CT_ASSERT_EQ(ct_pack_seed_file(db, "db/seeds/packs/bld_3style_specials.tsv", CT_EVENT_3BLD,
                                   CT_METHOD_3STYLE, CT_STAGE_PARITY),
                 85);
    ct_selector_list lists[SP_LISTS];
    int list_count = ct_selector_lists(CT_EVENT_3BLD, CT_METHOD_3STYLE, lists, SP_LISTS);
    CT_ASSERT_EQ(list_count, 3);
    int list_id = -1;
    for (int i = 0; i < list_count; i++) {
        if (lists[i].stage == CT_STAGE_PARITY) {
            list_id = i;
        }
    }
    CT_ASSERT_EQ(list_id, 2);
    static const char *const prefixes[] = {"flip.", "twist.", "parity.", "ltct."};
    int counts[CT_SELECTOR_GROUPS_MAX] = {0};
    char key[SP_KEY_MAX];
    for (int r = 0; r < 85; r++) {
        int group = ct_selector_case_group(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, list_id, r);
        CT_ASSERT(group >= 0 && group < CT_SELECTOR_GROUPS_MAX);
        CT_ASSERT_EQ(ct_selector_case_key(db, NULL, CT_EVENT_3BLD, CT_METHOD_3STYLE, list_id, r,
                                          key, sizeof key),
                     0);
        CT_ASSERT(strncmp(key, prefixes[group], strlen(prefixes[group])) == 0);
        counts[group]++;
    }
    CT_ASSERT_EQ(counts[0], 25);
    CT_ASSERT_EQ(counts[1], 23);
    CT_ASSERT_EQ(counts[2], 26);
    CT_ASSERT_EQ(counts[3], 11);
    CT_ASSERT_STR_EQ(ct_selector_group_label(0), "flip");
    CT_ASSERT_STR_EQ(ct_selector_group_label(1), "twist");
    CT_ASSERT_STR_EQ(ct_selector_group_label(2), "parity");
    CT_ASSERT_STR_EQ(ct_selector_group_label(3), "ltct");
    CT_ASSERT(ct_selector_group_label(CT_SELECTOR_GROUPS_MAX) == NULL);
    CT_ASSERT(ct_selector_group_label(-1) == NULL);

    CT_ASSERT_EQ(ct_pack_seed_file(db, "db/seeds/packs/clock_algs.tsv", CT_EVENT_CLOCK,
                                   CT_METHOD_LBL, CT_STAGE_CONCEPT),
                 14);
    CT_ASSERT_EQ(ct_selector_case_group(db, CT_EVENT_CLOCK, CT_METHOD_LBL, 0, 0), 0);
    CT_ASSERT_EQ(ct_selector_case_group(db, CT_EVENT_CLOCK, CT_METHOD_LBL, 0, 13), 0);
    CT_ASSERT_EQ(ct_selector_case_group(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0),
                 CT_SELECTOR_ERR);
    CT_ASSERT_EQ(ct_selector_case_group(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2, 85),
                 CT_SELECTOR_ERR);
    CT_ASSERT_EQ(ct_selector_case_group(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 9, 0),
                 CT_SELECTOR_ERR);
    CT_ASSERT_EQ(ct_selector_case_group(NULL, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2, 0),
                 CT_SELECTOR_ERR);
    ct_db_close(db);
}
