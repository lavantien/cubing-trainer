#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/config/config.h"
#include "../src/gen/targets.h"
#include "../src/packs/packs.h"
#include "../src/packs/seed_registry.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_settings.h"
#include "../src/trainer/cfop.h"
#include "../src/trainer/selector.h"
#include "framework.h"

enum {
    SPEC_EVENT_COUNT = 18,
    SPEC_PAIRS = 26,
    SPEC_CFOP_LISTS = 3,
    SPEC_3STYLE_CHART_LISTS = 2,
    SPEC_SQ1_LISTS = 5,
    SPEC_ONE_LIST = 1,
    SPEC_MAX_LISTS = SPEC_SQ1_LISTS,
    SPEC_METHODS_MAX = 3,
    SPEC_PLL_CASES = 21,
    SPEC_OLL_CASES = 57,
    SPEC_F2L_CASES = 164,
    SPEC_CORNER_CASES = 378,
    SPEC_EDGE_CASES = 440,
    SPEC_SEED_ROWS_TOTAL = 714,
    SPEC_MBLD_RECORD_ROWS = 14,
    SPEC_REDUCTION_ROWS = 17,
    SPEC_CFOP_CASES_TOTAL = 2 * (SPEC_PLL_CASES + SPEC_OLL_CASES + SPEC_F2L_CASES),
    SPEC_CHART_EVENTS = 4,
    SPEC_CHART_CASES_TOTAL = SPEC_CHART_EVENTS * (SPEC_CORNER_CASES + SPEC_EDGE_CASES),
    SPEC_KEY_SAMPLE = 20,
    SPEC_UNIQUE_KEYS = 50,
    SPEC_KEY_CAP = 3,
    SPEC_ALT_BUFFER = 5,
    SPEC_SEEDS_DIR_ROWS = 0,
    SPEC_PACK_SAMPLES = 3
};

static const ct_event spec_events[SPEC_EVENT_COUNT] = {
    CT_EVENT_3X3,       CT_EVENT_2X2,     CT_EVENT_3BLD,  CT_EVENT_4BLD,     CT_EVENT_5BLD,
    CT_EVENT_MULTI_BLD, CT_EVENT_FMC,     CT_EVENT_OH,    CT_EVENT_4X4,      CT_EVENT_5X5,
    CT_EVENT_6X6,       CT_EVENT_7X7,     CT_EVENT_CLOCK, CT_EVENT_MEGAMINX, CT_EVENT_PYRAMINX,
    CT_EVENT_SKEWB,     CT_EVENT_SQUARE1, CT_EVENT_FTO,
};

static const struct {
    ct_event event;
    ct_method method;
} spec_pairs[] = {
    {CT_EVENT_3X3, CT_METHOD_CFOP},          {CT_EVENT_2X2, CT_METHOD_CLL},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE},       {CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN},
    {CT_EVENT_3BLD, CT_METHOD_M2},           {CT_EVENT_4BLD, CT_METHOD_3STYLE},
    {CT_EVENT_4BLD, CT_METHOD_U2},           {CT_EVENT_4BLD, CT_METHOD_R2},
    {CT_EVENT_5BLD, CT_METHOD_3STYLE},       {CT_EVENT_5BLD, CT_METHOD_U2},
    {CT_EVENT_5BLD, CT_METHOD_R2},           {CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE},
    {CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT}, {CT_EVENT_FMC, CT_METHOD_CONCEPT},
    {CT_EVENT_OH, CT_METHOD_CFOP},           {CT_EVENT_4X4, CT_METHOD_YAU},
    {CT_EVENT_5X5, CT_METHOD_REDUCTION},     {CT_EVENT_6X6, CT_METHOD_REDUCTION},
    {CT_EVENT_7X7, CT_METHOD_REDUCTION},     {CT_EVENT_CLOCK, CT_METHOD_LBL},
    {CT_EVENT_MEGAMINX, CT_METHOD_4LLL},     {CT_EVENT_PYRAMINX, CT_METHOD_L4E},
    {CT_EVENT_PYRAMINX, CT_METHOD_LBL},      {CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL},       {CT_EVENT_FTO, CT_METHOD_L4T_REDUX},
};

static const struct {
    ct_event event;
    ct_method method;
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
} spec_lists[] = {
    {CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_OLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_PLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_OLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_4BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_4BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_5BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_5BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_PACK},
    {CT_EVENT_4BLD, CT_METHOD_U2, CT_STAGE_CONCEPT, CT_ORBIT_CENTER, CT_SELECTOR_PACK},
    {CT_EVENT_4BLD, CT_METHOD_R2, CT_STAGE_CONCEPT, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_5BLD, CT_METHOD_U2, CT_STAGE_CONCEPT, CT_ORBIT_CENTER, CT_SELECTOR_PACK},
    {CT_EVENT_5BLD, CT_METHOD_R2, CT_STAGE_CONCEPT, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_5X5, CT_METHOD_REDUCTION, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_6X6, CT_METHOD_REDUCTION, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_7X7, CT_METHOD_REDUCTION, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_PYRAMINX, CT_METHOD_LBL, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH, CT_ORBIT_CORNER,
     CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO, CT_ORBIT_EDGE, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP, CT_ORBIT_EDGE, CT_SELECTOR_PACK},
    {CT_EVENT_MEGAMINX, CT_METHOD_4LLL, CT_STAGE_4LLL, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
};

static sqlite3 *spec_seeded_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_pack_seed_registered(db, "db/seeds/packs"), SPEC_SEEDS_DIR_ROWS);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT),
                 SPEC_MBLD_RECORD_ROWS);
    return db;
}

static const struct {
    const char *file;
    ct_event event;
    ct_method method;
    ct_stage stage;
    int rows;
} spec_record_packs[] = {
    {"bigcube_l2e.tsv", CT_EVENT_5X5, CT_METHOD_REDUCTION, CT_STAGE_PARITY, SPEC_REDUCTION_ROWS},
    {"bigcube_l2e.tsv", CT_EVENT_6X6, CT_METHOD_REDUCTION, CT_STAGE_PARITY, SPEC_REDUCTION_ROWS},
    {"bigcube_l2e.tsv", CT_EVENT_7X7, CT_METHOD_REDUCTION, CT_STAGE_PARITY, SPEC_REDUCTION_ROWS},
    {"bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY, 5},
    {"twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL, 42},
    {"skewb_sarah.tsv", CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH, 15},
    {"pyraminx_l4e.tsv", CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E, 35},
    {"megaminx_4lll.tsv", CT_EVENT_MEGAMINX, CT_METHOD_4LLL, CT_STAGE_4LLL, 39},
    {"sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, 169},
    {"sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO, 7},
    {"sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO, 7},
    {"sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP, 16},
    {"sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP, 99},
    {"clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT, 14},
    {"bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY, 5},
    {"bld_3style_specials.tsv", CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY, 85},
    {"bld_m2.tsv", CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT, 24},
    {"bld_oldpoch.tsv", CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT, 40},
    {"fmc_concepts.tsv", CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, 16},
    {"fto_l4t_redux.tsv", CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T, 36},
    {"mbld_concepts.tsv", CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, 14},
};

static void spec_identity_scheme(ct_selector_scheme *s) {
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        s->map.corner[i] = (char)('A' + i);
        s->map.edge[i] = (char)('A' + i);
    }
    s->buffer_corner = CT_3STYLE_CORNER_BUFFER;
    s->buffer_edge = CT_3STYLE_EDGE_BUFFER;
}

static int spec_pair_lists(ct_event ev, ct_method m, ct_selector_list *out) {
    return ct_selector_lists(ev, m, out, SPEC_MAX_LISTS);
}

static int spec_count(sqlite3 *db, const ct_selector_scheme *s, ct_event ev, ct_method m,
                      int list_id) {
    return ct_selector_case_count(db, s, ev, m, list_id);
}

CT_SUITE(selector_scheme_load) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    ct_selector_scheme s;
    CT_ASSERT_EQ(ct_selector_scheme_load(NULL, &s), -1);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, NULL), -1);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        CT_ASSERT_EQ(s.map.corner[i], 'A' + i);
        CT_ASSERT_EQ(s.map.edge[i], 'A' + i);
    }
    CT_ASSERT_EQ(s.buffer_corner, CT_3STYLE_CORNER_BUFFER);
    CT_ASSERT_EQ(s.buffer_edge, CT_3STYLE_EDGE_BUFFER);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_corner", "5"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "7"), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    CT_ASSERT_EQ(s.buffer_corner, 5);
    CT_ASSERT_EQ(s.buffer_edge, 7);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "0"), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    CT_ASSERT_EQ(s.buffer_edge, 0);
    CT_ASSERT_EQ(s.buffer_corner, 5);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "24"), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    CT_ASSERT_EQ(s.buffer_edge, CT_3STYLE_EDGE_BUFFER);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "-1"), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    CT_ASSERT_EQ(s.buffer_edge, CT_3STYLE_EDGE_BUFFER);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "abc"), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    CT_ASSERT_EQ(s.buffer_edge, CT_3STYLE_EDGE_BUFFER);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "2x"), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    CT_ASSERT_EQ(s.buffer_edge, CT_3STYLE_EDGE_BUFFER);
    int scheme = ct_letter_scheme_default_id(db);
    CT_ASSERT(scheme > 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, 0, 1, 'C'), 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, 0, 2, 'B'), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s), 0);
    CT_ASSERT_EQ(s.map.corner[1], 'C');
    CT_ASSERT_EQ(s.map.corner[2], 'B');
    ct_db_close(db);
}

CT_SUITE(selector_events) {
    ct_event events[SPEC_EVENT_COUNT];
    CT_ASSERT_EQ(ct_selector_events(NULL, 0), SPEC_EVENT_COUNT);
    CT_ASSERT_EQ(ct_selector_events(NULL, 100), SPEC_EVENT_COUNT);
    CT_ASSERT_EQ(ct_selector_events(events, SPEC_EVENT_COUNT), SPEC_EVENT_COUNT);
    for (int i = 0; i < SPEC_EVENT_COUNT; i++) {
        CT_ASSERT_EQ(events[i], spec_events[i]);
    }
    ct_event few[SPEC_CFOP_LISTS];
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        few[i] = CT_EVENT_FTO;
    }
    CT_ASSERT_EQ(ct_selector_events(few, 1), SPEC_EVENT_COUNT);
    CT_ASSERT_EQ(few[0], CT_EVENT_3X3);
    CT_ASSERT_EQ(few[1], CT_EVENT_FTO);
    CT_ASSERT_EQ(ct_selector_events(few, 0), SPEC_EVENT_COUNT);
    CT_ASSERT_EQ(few[0], CT_EVENT_3X3);
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        few[i] = CT_EVENT_FTO;
    }
    CT_ASSERT_EQ(ct_selector_events(few, -1), SPEC_EVENT_COUNT);
    CT_ASSERT_EQ(few[0], CT_EVENT_FTO);
    CT_ASSERT_EQ(few[1], CT_EVENT_FTO);
}

CT_SUITE(selector_methods) {
    for (int i = 0; i < SPEC_PAIRS; i++) {
        for (int j = i + 1; j < SPEC_PAIRS; j++) {
            CT_ASSERT(spec_pairs[i].event != spec_pairs[j].event ||
                      spec_pairs[i].method != spec_pairs[j].method);
        }
    }
    ct_method got[SPEC_METHODS_MAX];
    for (int e = 0; e < SPEC_EVENT_COUNT; e++) {
        ct_event ev = spec_events[e];
        int expect = 0;
        for (int i = 0; i < SPEC_PAIRS; i++) {
            if (spec_pairs[i].event == ev) {
                expect++;
            }
        }
        CT_ASSERT(expect >= 1);
        memset(got, 0, sizeof got);
        CT_ASSERT_EQ(ct_selector_methods(ev, got, SPEC_METHODS_MAX), expect);
        int n = 0;
        for (int i = 0; i < SPEC_PAIRS; i++) {
            if (spec_pairs[i].event == ev) {
                CT_ASSERT_EQ(got[n], spec_pairs[i].method);
                n++;
            }
        }
        CT_ASSERT_EQ(ct_selector_methods(ev, NULL, 0), expect);
        for (int i = 0; i < SPEC_METHODS_MAX; i++) {
            got[i] = CT_METHOD_CONCEPT;
        }
        CT_ASSERT_EQ(ct_selector_methods(ev, got, 1), expect);
        if (expect > 1) {
            CT_ASSERT_EQ(got[1], CT_METHOD_CONCEPT);
        }
    }
    CT_ASSERT_EQ(ct_selector_methods((ct_event)-1, got, SPEC_METHODS_MAX), 0);
    CT_ASSERT_EQ(ct_selector_methods((ct_event)SPEC_EVENT_COUNT, got, SPEC_METHODS_MAX), 0);
}

CT_SUITE(selector_lists) {
    for (unsigned long i = 0; i < sizeof spec_lists / sizeof spec_lists[0]; i++) {
        for (unsigned long j = i + 1; j < sizeof spec_lists / sizeof spec_lists[0]; j++) {
            CT_ASSERT(spec_lists[i].event != spec_lists[j].event ||
                      spec_lists[i].method != spec_lists[j].method ||
                      spec_lists[i].stage != spec_lists[j].stage ||
                      spec_lists[i].orbit != spec_lists[j].orbit);
        }
    }
    ct_selector_list lists[SPEC_MAX_LISTS];
    for (int p = 0; p < SPEC_PAIRS; p++) {
        ct_event ev = spec_pairs[p].event;
        ct_method m = spec_pairs[p].method;
        int expect = 0;
        for (unsigned long i = 0; i < sizeof spec_lists / sizeof spec_lists[0]; i++) {
            if (spec_lists[i].event == ev && spec_lists[i].method == m) {
                expect++;
            }
        }
        CT_ASSERT(expect >= 1);
        memset(lists, 0, sizeof lists);
        CT_ASSERT_EQ(spec_pair_lists(ev, m, lists), expect);
        int n = 0;
        for (unsigned long i = 0; i < sizeof spec_lists / sizeof spec_lists[0]; i++) {
            if (spec_lists[i].event != ev || spec_lists[i].method != m) {
                continue;
            }
            CT_ASSERT_EQ(lists[n].stage, spec_lists[i].stage);
            CT_ASSERT_EQ(lists[n].orbit, spec_lists[i].orbit);
            CT_ASSERT_EQ(lists[n].source, spec_lists[i].source);
            n++;
        }
        CT_ASSERT_EQ(ct_selector_lists(ev, m, NULL, 0), expect);
        for (int i = 0; i < SPEC_MAX_LISTS; i++) {
            lists[i].stage = CT_STAGE_CONCEPT;
        }
        CT_ASSERT_EQ(ct_selector_lists(ev, m, lists, 1), expect);
        if (expect > 1) {
            CT_ASSERT_EQ(lists[1].stage, CT_STAGE_CONCEPT);
        }
    }
    CT_ASSERT_EQ(ct_selector_lists(CT_EVENT_3X3, CT_METHOD_LBL, lists, SPEC_MAX_LISTS), 0);
    CT_ASSERT_EQ(ct_selector_lists((ct_event)-1, CT_METHOD_CFOP, lists, SPEC_MAX_LISTS), 0);
    CT_ASSERT_EQ(ct_selector_lists(CT_EVENT_3X3, (ct_method)99, lists, SPEC_MAX_LISTS), 0);
    CT_ASSERT_EQ(ct_selector_lists(CT_EVENT_FMC, CT_METHOD_CONCEPT, lists, SPEC_MAX_LISTS), 1);
    CT_ASSERT_EQ(lists[0].stage, CT_STAGE_CONCEPT);
    CT_ASSERT_EQ(lists[0].source, CT_SELECTOR_PACK);
    CT_ASSERT_EQ(ct_selector_lists(CT_EVENT_SQUARE1, CT_METHOD_LBL, lists, SPEC_MAX_LISTS),
                 SPEC_SQ1_LISTS);
    CT_ASSERT_EQ(lists[0].stage, CT_STAGE_CUBESHAPE);
    CT_ASSERT_EQ(lists[4].stage, CT_STAGE_EP);
}

CT_SUITE(selector_case_counts) {
    sqlite3 *db = spec_seeded_db();
    ct_selector_scheme s;
    spec_identity_scheme(&s);
    static const int spec_cfop_counts[SPEC_CFOP_LISTS] = {SPEC_PLL_CASES, SPEC_OLL_CASES,
                                                          SPEC_F2L_CASES};
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, i), spec_cfop_counts[i]);
        CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_OH, CT_METHOD_CFOP, i), spec_cfop_counts[i]);
        CT_ASSERT_EQ(spec_count(NULL, &s, CT_EVENT_3X3, CT_METHOD_CFOP, i), spec_cfop_counts[i]);
    }
    const ct_event chart_events[SPEC_CHART_EVENTS] = {CT_EVENT_3BLD, CT_EVENT_4BLD, CT_EVENT_5BLD,
                                                      CT_EVENT_MULTI_BLD};
    for (int e = 0; e < SPEC_CHART_EVENTS; e++) {
        CT_ASSERT_EQ(spec_count(db, &s, chart_events[e], CT_METHOD_3STYLE, 0), SPEC_CORNER_CASES);
        CT_ASSERT_EQ(spec_count(db, &s, chart_events[e], CT_METHOD_3STYLE, 1), SPEC_EDGE_CASES);
        CT_ASSERT_EQ(spec_count(NULL, &s, chart_events[e], CT_METHOD_3STYLE, 0), SPEC_CORNER_CASES);
        CT_ASSERT_EQ(spec_count(db, NULL, chart_events[e], CT_METHOD_3STYLE, 0), -1);
    }
    CT_ASSERT_EQ(spec_count(NULL, NULL, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0), -1);
    CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, 0),
                 SPEC_MBLD_RECORD_ROWS);
    for (unsigned long i = 0; i < sizeof spec_record_packs / sizeof spec_record_packs[0]; i++) {
        ct_selector_list lists[SPEC_MAX_LISTS];
        int list_count = ct_selector_lists(spec_record_packs[i].event, spec_record_packs[i].method,
                                           lists, SPEC_MAX_LISTS);
        int hit = 0;
        for (int l = 0; l < list_count; l++) {
            if (lists[l].stage != spec_record_packs[i].stage) {
                continue;
            }
            hit = 1;
            CT_ASSERT_EQ(
                spec_count(db, &s, spec_record_packs[i].event, spec_record_packs[i].method, l),
                spec_record_packs[i].rows);
            CT_ASSERT_EQ(
                spec_count(NULL, &s, spec_record_packs[i].event, spec_record_packs[i].method, l),
                -1);
        }
        CT_ASSERT(hit);
    }
    for (int i = 0; i < ct_pack_seed_registry_count(); i++) {
        const ct_pack_seed_reg *reg = &ct_pack_seed_registry[i];
        ct_selector_list lists[SPEC_MAX_LISTS];
        int list_count = ct_selector_lists(reg->event, reg->method, lists, SPEC_MAX_LISTS);
        int pinned = 0;
        for (int l = 0; l < list_count; l++) {
            if (lists[l].stage != reg->stage) {
                continue;
            }
            CT_ASSERT_EQ(lists[l].source, CT_SELECTOR_PACK);
            CT_ASSERT_EQ(spec_count(db, &s, reg->event, reg->method, l),
                         ct_pack_count(db, reg->event, reg->method, reg->stage));
            pinned = 1;
        }
        CT_ASSERT(pinned);
    }
    int total = 0;
    for (int p = 0; p < SPEC_PAIRS; p++) {
        ct_selector_list lists[SPEC_MAX_LISTS];
        int list_count = spec_pair_lists(spec_pairs[p].event, spec_pairs[p].method, lists);
        for (int l = 0; l < list_count; l++) {
            int n = spec_count(db, &s, spec_pairs[p].event, spec_pairs[p].method, l);
            CT_ASSERT(n >= 0);
            total += n;
        }
    }
    CT_ASSERT_EQ(total, SPEC_SEED_ROWS_TOTAL + SPEC_CFOP_CASES_TOTAL + SPEC_CHART_CASES_TOTAL);
    CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_5X5, CT_METHOD_REDUCTION, 0), SPEC_REDUCTION_ROWS);
    CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_6X6, CT_METHOD_REDUCTION, 0), SPEC_REDUCTION_ROWS);
    CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_7X7, CT_METHOD_REDUCTION, 0), SPEC_REDUCTION_ROWS);
    CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_4BLD, CT_METHOD_U2, 0), 0);
    CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_4BLD, CT_METHOD_R2, 0), 0);
    CT_ASSERT_EQ(spec_count(db, &s, CT_EVENT_PYRAMINX, CT_METHOD_LBL, 0), 0);
    CT_ASSERT(spec_count(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, SPEC_CFOP_LISTS) < 0);
    CT_ASSERT(spec_count(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, -1) < 0);
    CT_ASSERT(spec_count(db, &s, CT_EVENT_3X3, CT_METHOD_LBL, 0) < 0);
    CT_ASSERT(spec_count(db, &s, (ct_event)-1, CT_METHOD_CFOP, 0) < 0);
    CT_ASSERT(spec_count(db, &s, CT_EVENT_3X3, (ct_method)99, 0) < 0);
    ct_db_close(db);
}

CT_SUITE(selector_case_keys) {
    sqlite3 *db = spec_seeded_db();
    ct_selector_scheme s;
    spec_identity_scheme(&s);
    static const ct_stage cfop_stages[SPEC_CFOP_LISTS] = {CT_STAGE_PLL, CT_STAGE_OLL, CT_STAGE_F2L};
    static const int cfop_counts[SPEC_CFOP_LISTS] = {SPEC_PLL_CASES, SPEC_OLL_CASES,
                                                     SPEC_F2L_CASES};
    char key[CT_PACK_CASE_KEY_MAX];
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        int samples[SPEC_PACK_SAMPLES] = {0, cfop_counts[i] / 2, cfop_counts[i] - 1};
        for (int k = 0; k < SPEC_PACK_SAMPLES; k++) {
            char want[CT_PACK_CASE_KEY_MAX];
            CT_ASSERT_EQ(ct_cfop_case_key(cfop_stages[i], samples[k], want, sizeof want), 0);
            CT_ASSERT_EQ(ct_selector_case_key(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, i, samples[k],
                                              key, sizeof key),
                         0);
            CT_ASSERT_STR_EQ(key, want);
        }
        CT_ASSERT(ct_selector_case_key(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, i, cfop_counts[i], key,
                                       sizeof key) < 0);
    }
    static ct_target_pair corner_pairs[SPEC_CORNER_CASES];
    static ct_target_pair edge_pairs[SPEC_EDGE_CASES];
    CT_ASSERT_EQ(ct_targets_corner(CT_3STYLE_CORNER_BUFFER, corner_pairs, SPEC_CORNER_CASES),
                 SPEC_CORNER_CASES);
    CT_ASSERT_EQ(ct_targets_edge(CT_3STYLE_EDGE_BUFFER, edge_pairs, SPEC_EDGE_CASES),
                 SPEC_EDGE_CASES);
    char pair[SPEC_KEY_CAP];
    CT_ASSERT_EQ(
        ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, pair, sizeof pair), 0);
    CT_ASSERT_STR_EQ(pair, "BC");
    CT_ASSERT_EQ(
        ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 0, pair, sizeof pair), 0);
    CT_ASSERT_STR_EQ(pair, "AB");
    for (int k = 0; k < SPEC_KEY_SAMPLE; k++) {
        int ci = k * (SPEC_CORNER_CASES - 1) / (SPEC_KEY_SAMPLE - 1);
        int ei = k * (SPEC_EDGE_CASES - 1) / (SPEC_KEY_SAMPLE - 1);
        CT_ASSERT_EQ(
            ct_selector_case_key(db, &s, CT_EVENT_5BLD, CT_METHOD_3STYLE, 0, ci, pair, sizeof pair),
            0);
        CT_ASSERT_EQ(pair[0], 'A' + corner_pairs[ci].t1);
        CT_ASSERT_EQ(pair[1], 'A' + corner_pairs[ci].t2);
        CT_ASSERT_EQ(
            ct_selector_case_key(db, &s, CT_EVENT_5BLD, CT_METHOD_3STYLE, 1, ei, pair, sizeof pair),
            0);
        CT_ASSERT_EQ(pair[0], 'A' + edge_pairs[ei].t1);
        CT_ASSERT_EQ(pair[1], 'A' + edge_pairs[ei].t2);
    }
    static char keys[SPEC_UNIQUE_KEYS][SPEC_KEY_CAP];
    for (int i = 0; i < SPEC_UNIQUE_KEYS; i++) {
        CT_ASSERT_EQ(ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, i, keys[i],
                                          sizeof keys[i]),
                     0);
    }
    for (int i = 0; i < SPEC_UNIQUE_KEYS; i++) {
        for (int j = i + 1; j < SPEC_UNIQUE_KEYS; j++) {
            CT_ASSERT(strcmp(keys[i], keys[j]) != 0);
        }
    }
    char tiny[SPEC_KEY_CAP - 1];
    memset(tiny, 'q', sizeof tiny);
    CT_ASSERT(
        ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, tiny, sizeof tiny) < 0);
    CT_ASSERT_EQ(tiny[0], 'q');
    CT_ASSERT(
        ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, NULL, sizeof pair) < 0);
    CT_ASSERT(ct_selector_case_key(db, NULL, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, pair,
                                   sizeof pair) < 0);
    ct_selector_scheme s_edge;
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s_edge), 0);
    int edge_scheme = ct_letter_scheme_default_id(db);
    CT_ASSERT(edge_scheme > 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, edge_scheme, 1, 0, 'B'), 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, edge_scheme, 1, 1, 'A'), 0);
    CT_ASSERT_EQ(ct_selector_scheme_load(db, &s_edge), 0);
    CT_ASSERT_EQ(s_edge.map.edge[0], 'B');
    CT_ASSERT_EQ(s_edge.map.edge[1], 'A');
    CT_ASSERT_EQ(
        ct_selector_case_key(db, &s_edge, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 0, pair, sizeof pair),
        0);
    CT_ASSERT_STR_EQ(pair, "BA");
    char prev[CT_PACK_CASE_KEY_MAX];
    for (unsigned long i = 0; i < sizeof spec_record_packs / sizeof spec_record_packs[0]; i++) {
        ct_selector_list lists[SPEC_MAX_LISTS];
        int list_count = ct_selector_lists(spec_record_packs[i].event, spec_record_packs[i].method,
                                           lists, SPEC_MAX_LISTS);
        prev[0] = '\0';
        for (int l = 0; l < list_count; l++) {
            if (lists[l].stage != spec_record_packs[i].stage) {
                continue;
            }
            int rows = spec_record_packs[i].rows;
            for (int r = 0; r < rows; r++) {
                CT_ASSERT_EQ(ct_selector_case_key(db, &s, spec_record_packs[i].event,
                                                  spec_record_packs[i].method, l, r, key,
                                                  sizeof key),
                             0);
                if (r > 0) {
                    CT_ASSERT(strcmp(prev, key) < 0);
                }
                snprintf(prev, sizeof prev, "%s", key);
            }
            CT_ASSERT(ct_selector_case_key(db, &s, spec_record_packs[i].event,
                                           spec_record_packs[i].method, l, rows, key,
                                           sizeof key) < 0);
            CT_ASSERT_EQ(ct_selector_case_key(db, &s, spec_record_packs[i].event,
                                              spec_record_packs[i].method, l, 0, key, sizeof key),
                         0);
            CT_ASSERT(ct_selector_case_key(db, &s, spec_record_packs[i].event,
                                           spec_record_packs[i].method, l, 0, key,
                                           (unsigned)strlen(key)) < 0);
            CT_ASSERT_EQ(ct_selector_case_key(db, &s, spec_record_packs[i].event,
                                              spec_record_packs[i].method, l, 0, key,
                                              (unsigned)strlen(key) + 1),
                         0);
            break;
        }
    }
    ct_db_close(db);
}

CT_SUITE(selector_case_pairs) {
    ct_selector_scheme s;
    spec_identity_scheme(&s);
    int t1 = -1;
    int t2 = -1;
    CT_ASSERT_EQ(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, &t1, &t2), 0);
    CT_ASSERT_EQ(t1, 1);
    CT_ASSERT_EQ(t2, 2);
    CT_ASSERT_EQ(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 0, &t1, &t2), 0);
    CT_ASSERT_EQ(t1, 0);
    CT_ASSERT_EQ(t2, 1);
    static ct_target_pair alt[SPEC_CORNER_CASES];
    CT_ASSERT_EQ(ct_targets_corner(SPEC_ALT_BUFFER, alt, SPEC_CORNER_CASES), SPEC_CORNER_CASES);
    s.buffer_corner = SPEC_ALT_BUFFER;
    CT_ASSERT_EQ(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, &t1, &t2), 0);
    CT_ASSERT_EQ(t1, alt[0].t1);
    CT_ASSERT_EQ(t2, alt[0].t2);
    s.buffer_corner = CT_3STYLE_CORNER_BUFFER;
    s.buffer_edge = SPEC_ALT_BUFFER;
    static ct_target_pair alt_edge[SPEC_EDGE_CASES];
    CT_ASSERT_EQ(ct_targets_edge(SPEC_ALT_BUFFER, alt_edge, SPEC_EDGE_CASES), SPEC_EDGE_CASES);
    CT_ASSERT_EQ(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 0, &t1, &t2), 0);
    CT_ASSERT_EQ(t1, alt_edge[0].t1);
    CT_ASSERT_EQ(t2, alt_edge[0].t2);
    CT_ASSERT(ct_selector_case_pair(NULL, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, &t1, &t2) < 0);
    CT_ASSERT(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, SPEC_CORNER_CASES, &t1,
                                    &t2) < 0);
    CT_ASSERT(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, -1, 0, &t1, &t2) < 0);
    CT_ASSERT(ct_selector_case_pair(&s, CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, &t1, &t2) < 0);
    CT_ASSERT(ct_selector_case_pair(&s, CT_EVENT_2X2, CT_METHOD_CLL, 0, 0, &t1, &t2) < 0);
    CT_ASSERT(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, NULL, &t2) < 0);
    CT_ASSERT(ct_selector_case_pair(&s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, &t1, NULL) < 0);
}

CT_SUITE(selector_pack_cases) {
    sqlite3 *db = spec_seeded_db();
    ct_pack_entry e;
    CT_ASSERT_EQ(ct_selector_pack_case(NULL, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, 0, &e), -1);
    CT_ASSERT_EQ(ct_selector_pack_case(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, -1, 0, &e), -1);
    CT_ASSERT_EQ(ct_selector_pack_case(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, -1, &e), -1);
    CT_ASSERT_EQ(ct_selector_pack_case(
                     db, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0,
                     ct_pack_count(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE), &e),
                 -1);
    CT_ASSERT_EQ(ct_selector_pack_case(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, 0, NULL), -1);
    CT_ASSERT_EQ(ct_selector_pack_case(db, CT_EVENT_2X2, CT_METHOD_CLL, 0, 41, &e), 0);
    CT_ASSERT_EQ(ct_selector_pack_case(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, 0, &e), 0);
    CT_ASSERT_EQ(e.event, CT_EVENT_SQUARE1);
    CT_ASSERT_EQ(e.method, CT_METHOD_LBL);
    CT_ASSERT_EQ(e.stage, CT_STAGE_CUBESHAPE);
    ct_pack_entry want;
    char key[CT_PACK_CASE_KEY_MAX];
    CT_ASSERT_EQ(
        ct_selector_case_key(db, NULL, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, 0, key, sizeof key), 0);
    CT_ASSERT_EQ(ct_pack_get(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, key, &want),
                 CT_PACK_OK);
    CT_ASSERT_STR_EQ(e.alg, want.alg);
    CT_ASSERT_STR_EQ(e.front_json, want.front_json);
    CT_ASSERT_STR_EQ(e.case_key, want.case_key);
    ct_db_close(db);
}
