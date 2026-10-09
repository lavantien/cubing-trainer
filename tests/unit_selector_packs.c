#include <string.h>

#include <sqlite3.h>

#include "../src/config/config.h"
#include "../src/packs/packs.h"
#include "../src/storage/db.h"
#include "../src/trainer/selector.h"
#include "framework.h"

enum { SP_PACKS = 7, SP_METHODS = 2, SP_LISTS = 6 };

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
        int ids[SP_LISTS];
        ct_stage stages[SP_LISTS];
        ct_orbit orbits[SP_LISTS];
        int list_count = ct_selector_lists(sp->event, sp->method, ids, stages, orbits, SP_LISTS);
        CT_ASSERT(list_count >= 1 && list_count <= SP_LISTS);
        int staged = 0;
        for (int i = 0; i < list_count; i++) {
            if (stages[i] == sp->stage) {
                staged = 1;
            }
        }
        CT_ASSERT(staged);
        sqlite3 *db = NULL;
        ct_db_open(&db, ":memory:");
        ct_db_migrate(db, "db/migrations");
        CT_ASSERT_EQ(ct_pack_seed_file(db, sp->path, sp->event, sp->method, sp->stage), sp->rows);
        CT_ASSERT_EQ(ct_pack_count(db, sp->event, sp->method, sp->stage), sp->rows);
        ct_pack_entry got;
        CT_ASSERT_EQ(ct_pack_get(db, sp->event, sp->method, sp->stage, sp->case_key, &got),
                     CT_PACK_OK);
        CT_ASSERT_STR_EQ(got.alg, sp->alg);
        CT_ASSERT_STR_EQ(got.source_at, "2026-10-10");
        ct_db_close(db);
    }
}
