#include "../src/config/config.h"
#include "../src/gen/targets.h"
#include "../src/storage/repo_cards.h"
#include "../src/trainer/cfop.h"
#include "../src/trainer/selector.h"
#include "framework.h"

#include <string.h>

enum {
    SPEC_EVENT_COUNT = 18,
    SPEC_METHOD_MAX = 3,
    SPEC_CFOP_LISTS = 4,
    SPEC_3STYLE_LISTS = 5,
    SPEC_ONE_LIST = 1,
    SPEC_PLL_CASES = 21,
    SPEC_OLL_CASES = 57,
    SPEC_F2L_CASES = 164,
    SPEC_CORNER_CASES = 378,
    SPEC_EDGE_CASES = 440,
    SPEC_KEY_SAMPLE = 20,
    SPEC_UNIQUE_KEYS = 50,
    SPEC_KEY_CAP = 3,
    SPEC_LISTS_SMALL_CAP = 2,
    SPEC_SAMPLES = 3
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
} spec_event_methods[] = {
    {CT_EVENT_3X3, CT_METHOD_CFOP},
    {CT_EVENT_2X2, CT_METHOD_CLL},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE},
    {CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN},
    {CT_EVENT_3BLD, CT_METHOD_M2},
    {CT_EVENT_4BLD, CT_METHOD_3STYLE},
    {CT_EVENT_4BLD, CT_METHOD_U2},
    {CT_EVENT_4BLD, CT_METHOD_R2},
    {CT_EVENT_5BLD, CT_METHOD_3STYLE},
    {CT_EVENT_5BLD, CT_METHOD_U2},
    {CT_EVENT_5BLD, CT_METHOD_R2},
    {CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE},
    {CT_EVENT_OH, CT_METHOD_CFOP},
    {CT_EVENT_4X4, CT_METHOD_YAU},
    {CT_EVENT_5X5, CT_METHOD_REDUCTION},
    {CT_EVENT_6X6, CT_METHOD_REDUCTION},
    {CT_EVENT_7X7, CT_METHOD_REDUCTION},
    {CT_EVENT_MEGAMINX, CT_METHOD_4LLL},
    {CT_EVENT_PYRAMINX, CT_METHOD_L4E},
    {CT_EVENT_PYRAMINX, CT_METHOD_LBL},
    {CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE},
    {CT_EVENT_FTO, CT_METHOD_L4T_REDUX},
};

static const ct_stage spec_cfop_stages[SPEC_CFOP_LISTS] = {CT_STAGE_PLL, CT_STAGE_OLL, CT_STAGE_F2L,
                                                           CT_STAGE_CROSS};

static const int spec_cfop_counts[SPEC_CFOP_LISTS] = {SPEC_PLL_CASES, SPEC_OLL_CASES,
                                                      SPEC_F2L_CASES, 0};

static const ct_stage spec_3style_stages[SPEC_3STYLE_LISTS] = {
    CT_STAGE_CONCEPT, CT_STAGE_CONCEPT, CT_STAGE_PARITY, CT_STAGE_CONCEPT, CT_STAGE_CONCEPT};

static const ct_orbit spec_3style_orbits[SPEC_3STYLE_LISTS] = {
    CT_ORBIT_CORNER, CT_ORBIT_EDGE, CT_ORBIT_CORNER, CT_ORBIT_EDGE, CT_ORBIT_CORNER};

static int spec_method_count(ct_event ev) {
    int n = 0;
    for (unsigned long i = 0; i < sizeof spec_event_methods / sizeof spec_event_methods[0]; i++) {
        if (spec_event_methods[i].event == ev) {
            n++;
        }
    }
    return n;
}

static void spec_method_fill(ct_event ev, ct_method *out) {
    int n = 0;
    for (unsigned long i = 0; i < sizeof spec_event_methods / sizeof spec_event_methods[0]; i++) {
        if (spec_event_methods[i].event == ev) {
            out[n] = spec_event_methods[i].method;
            n++;
        }
    }
}

static int spec_keys_unique(const char (*keys)[SPEC_KEY_CAP], int n) {
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (strcmp(keys[i], keys[j]) == 0) {
                return 0;
            }
        }
    }
    return 1;
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
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        few[i] = CT_EVENT_FTO;
    }
    CT_ASSERT_EQ(ct_selector_events(few, 0), SPEC_EVENT_COUNT);
    CT_ASSERT_EQ(few[0], CT_EVENT_FTO);
}

CT_SUITE(selector_methods) {
    ct_method got[SPEC_METHOD_MAX];
    ct_method want[SPEC_METHOD_MAX];
    for (int i = 0; i < SPEC_EVENT_COUNT; i++) {
        ct_event ev = spec_events[i];
        int expect = spec_method_count(ev);
        CT_ASSERT_EQ(ct_selector_methods(ev, NULL, 0), expect);
        CT_ASSERT_EQ(ct_selector_methods(ev, NULL, SPEC_METHOD_MAX), expect);
        memset(got, 0, sizeof got);
        CT_ASSERT_EQ(ct_selector_methods(ev, got, SPEC_METHOD_MAX), expect);
        spec_method_fill(ev, want);
        for (int j = 0; j < expect; j++) {
            CT_ASSERT_EQ(got[j], want[j]);
        }
    }
    CT_ASSERT_EQ(ct_selector_methods(CT_EVENT_CLOCK, got, SPEC_METHOD_MAX), 0);
    CT_ASSERT_EQ(ct_selector_methods(CT_EVENT_FMC, got, SPEC_METHOD_MAX), 0);
    CT_ASSERT_EQ(ct_selector_methods(CT_EVENT_SQUARE1, got, SPEC_METHOD_MAX), 0);
    CT_ASSERT_EQ(ct_selector_methods((ct_event)-1, got, SPEC_METHOD_MAX), 0);
    CT_ASSERT_EQ(ct_selector_methods((ct_event)SPEC_EVENT_COUNT, got, SPEC_METHOD_MAX), 0);
    for (int i = 0; i < SPEC_METHOD_MAX; i++) {
        got[i] = CT_METHOD_CONCEPT;
    }
    CT_ASSERT_EQ(ct_selector_methods(CT_EVENT_3BLD, got, 1), SPEC_METHOD_MAX);
    CT_ASSERT_EQ(got[0], CT_METHOD_3STYLE);
    CT_ASSERT_EQ(got[1], CT_METHOD_CONCEPT);
    for (int i = 0; i < SPEC_METHOD_MAX; i++) {
        got[i] = CT_METHOD_CONCEPT;
    }
    CT_ASSERT_EQ(ct_selector_methods(CT_EVENT_3BLD, got, 0), SPEC_METHOD_MAX);
    CT_ASSERT_EQ(got[0], CT_METHOD_CONCEPT);
}

CT_SUITE(selector_cfop_lists) {
    int ids[SPEC_CFOP_LISTS];
    ct_stage stages[SPEC_CFOP_LISTS];
    ct_orbit orbits[SPEC_CFOP_LISTS];
    CT_ASSERT_EQ(ct_selector_lists(CT_EVENT_3X3, CT_METHOD_CFOP, NULL, NULL, NULL, 0),
                 SPEC_CFOP_LISTS);
    CT_ASSERT_EQ(
        ct_selector_lists(CT_EVENT_3X3, CT_METHOD_CFOP, ids, stages, orbits, SPEC_CFOP_LISTS),
        SPEC_CFOP_LISTS);
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        CT_ASSERT_EQ(ids[i], i);
        CT_ASSERT_EQ(stages[i], spec_cfop_stages[i]);
    }
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        CT_ASSERT_EQ(ct_selector_case_count(CT_EVENT_3X3, CT_METHOD_CFOP, i), spec_cfop_counts[i]);
    }
    char key[CT_CARD_CASE_KEY_MAX];
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        int count = spec_cfop_counts[i];
        if (count == 0) {
            CT_ASSERT(ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, i, 0, key, sizeof key) <
                      0);
            continue;
        }
        int samples[SPEC_SAMPLES] = {0, count / 2, count - 1};
        for (int s = 0; s < SPEC_SAMPLES; s++) {
            char want[CT_CARD_CASE_KEY_MAX];
            CT_ASSERT_EQ(ct_cfop_case_key(spec_cfop_stages[i], samples[s], want, sizeof want), 0);
            CT_ASSERT_EQ(
                ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, i, samples[s], key, sizeof key),
                0);
            CT_ASSERT_STR_EQ(key, want);
        }
        CT_ASSERT(ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, i, count, key, sizeof key) <
                  0);
        CT_ASSERT(ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, i, -1, key, sizeof key) < 0);
    }
    CT_ASSERT_EQ(
        ct_selector_lists(CT_EVENT_OH, CT_METHOD_CFOP, ids, stages, orbits, SPEC_CFOP_LISTS),
        SPEC_CFOP_LISTS);
    CT_ASSERT_EQ(ct_selector_case_count(CT_EVENT_OH, CT_METHOD_CFOP, 0), SPEC_PLL_CASES);
    CT_ASSERT_EQ(ct_selector_case_count(CT_EVENT_OH, CT_METHOD_CFOP, 2), SPEC_F2L_CASES);
    CT_ASSERT(ct_selector_case_count(CT_EVENT_3X3, CT_METHOD_CFOP, SPEC_CFOP_LISTS) < 0);
    CT_ASSERT(ct_selector_case_count(CT_EVENT_3X3, CT_METHOD_CFOP, -1) < 0);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, -1, 0, key, sizeof key) < 0);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, SPEC_CFOP_LISTS, 0, key,
                                   sizeof key) < 0);
    CT_ASSERT(
        ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, 0, SPEC_PLL_CASES, key, sizeof key) < 0);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, NULL, sizeof key) < 0);
    CT_ASSERT_EQ(
        ct_selector_lists(CT_EVENT_2X2, CT_METHOD_CFOP, ids, stages, orbits, SPEC_CFOP_LISTS), 0);
    CT_ASSERT(ct_selector_case_count(CT_EVENT_2X2, CT_METHOD_CFOP, 0) < 0);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_2X2, CT_METHOD_CFOP, 0, 0, key, sizeof key) < 0);
}

CT_SUITE(selector_3style_lists) {
    int ids[SPEC_3STYLE_LISTS];
    ct_stage stages[SPEC_3STYLE_LISTS];
    ct_orbit orbits[SPEC_3STYLE_LISTS];
    const ct_event evs[SPEC_METHOD_MAX + 1] = {CT_EVENT_3BLD, CT_EVENT_4BLD, CT_EVENT_5BLD,
                                               CT_EVENT_MULTI_BLD};
    for (int e = 0; e < SPEC_METHOD_MAX + 1; e++) {
        CT_ASSERT_EQ(ct_selector_lists(evs[e], CT_METHOD_3STYLE, NULL, NULL, NULL, 0),
                     SPEC_3STYLE_LISTS);
        CT_ASSERT_EQ(
            ct_selector_lists(evs[e], CT_METHOD_3STYLE, ids, stages, orbits, SPEC_3STYLE_LISTS),
            SPEC_3STYLE_LISTS);
        for (int i = 0; i < SPEC_3STYLE_LISTS; i++) {
            CT_ASSERT_EQ(ids[i], i);
            CT_ASSERT_EQ(stages[i], spec_3style_stages[i]);
            CT_ASSERT_EQ(orbits[i], spec_3style_orbits[i]);
        }
        CT_ASSERT_EQ(ct_selector_case_count(evs[e], CT_METHOD_3STYLE, 0), SPEC_CORNER_CASES);
        CT_ASSERT_EQ(ct_selector_case_count(evs[e], CT_METHOD_3STYLE, 1), SPEC_EDGE_CASES);
    }
    static ct_target_pair corner_pairs[SPEC_CORNER_CASES];
    static ct_target_pair edge_pairs[SPEC_EDGE_CASES];
    CT_ASSERT_EQ(ct_targets_corner(0, corner_pairs, SPEC_CORNER_CASES), SPEC_CORNER_CASES);
    CT_ASSERT_EQ(ct_targets_edge(2, edge_pairs, SPEC_EDGE_CASES), SPEC_EDGE_CASES);
    char key[SPEC_KEY_CAP];
    CT_ASSERT_EQ(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, key, sizeof key), 0);
    CT_ASSERT_STR_EQ(key, "BC");
    CT_ASSERT_EQ(key[0], 'A' + corner_pairs[0].t1);
    CT_ASSERT_EQ(key[1], 'A' + corner_pairs[0].t2);
    CT_ASSERT_EQ(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 0, key, sizeof key), 0);
    CT_ASSERT_STR_EQ(key, "AB");
    CT_ASSERT_EQ(key[0], 'A' + edge_pairs[0].t1);
    CT_ASSERT_EQ(key[1], 'A' + edge_pairs[0].t2);
    for (int s = 0; s < SPEC_KEY_SAMPLE; s++) {
        int ci = s * (SPEC_CORNER_CASES - 1) / (SPEC_KEY_SAMPLE - 1);
        int ei = s * (SPEC_EDGE_CASES - 1) / (SPEC_KEY_SAMPLE - 1);
        CT_ASSERT_EQ(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, ci, key, sizeof key),
                     0);
        CT_ASSERT_EQ(strlen(key), 2u);
        CT_ASSERT(key[0] >= 'A' && key[0] <= 'X');
        CT_ASSERT(key[1] >= 'A' && key[1] <= 'X');
        CT_ASSERT_EQ(key[0], 'A' + corner_pairs[ci].t1);
        CT_ASSERT_EQ(key[1], 'A' + corner_pairs[ci].t2);
        CT_ASSERT_EQ(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, ei, key, sizeof key),
                     0);
        CT_ASSERT_EQ(key[0], 'A' + edge_pairs[ei].t1);
        CT_ASSERT_EQ(key[1], 'A' + edge_pairs[ei].t2);
    }
    static char keys[SPEC_UNIQUE_KEYS][SPEC_KEY_CAP];
    for (int i = 0; i < SPEC_UNIQUE_KEYS; i++) {
        CT_ASSERT_EQ(
            ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, i, keys[i], sizeof keys[i]),
            0);
    }
    CT_ASSERT(spec_keys_unique(keys, SPEC_UNIQUE_KEYS));
    for (int i = 2; i < SPEC_3STYLE_LISTS; i++) {
        CT_ASSERT_EQ(ct_selector_case_count(CT_EVENT_3BLD, CT_METHOD_3STYLE, i), 0);
        CT_ASSERT(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, i, 0, key, sizeof key) < 0);
    }
    char tiny[SPEC_KEY_CAP - 1];
    memset(tiny, 'q', sizeof tiny);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, tiny, sizeof tiny) < 0);
    CT_ASSERT_EQ(tiny[0], 'q');
    CT_ASSERT_EQ(tiny[1], 'q');
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, NULL, sizeof key) < 0);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, SPEC_CORNER_CASES, key,
                                   sizeof key) < 0);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, -1, key, sizeof key) < 0);
    CT_ASSERT(ct_selector_case_count(CT_EVENT_3BLD, CT_METHOD_3STYLE, SPEC_3STYLE_LISTS) < 0);
}

CT_SUITE(selector_reserved_lists) {
    static const struct {
        ct_event event;
        ct_method method;
        ct_stage stage;
        ct_orbit orbit;
    } spec[] = {
        {CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL, CT_ORBIT_CORNER},
        {CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
        {CT_EVENT_5X5, CT_METHOD_REDUCTION, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
        {CT_EVENT_MEGAMINX, CT_METHOD_4LLL, CT_STAGE_4LLL, CT_ORBIT_CORNER},
        {CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E, CT_ORBIT_CORNER},
        {CT_EVENT_PYRAMINX, CT_METHOD_LBL, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
        {CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH, CT_ORBIT_CORNER},
        {CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T, CT_ORBIT_CORNER},
        {CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
        {CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT, CT_ORBIT_EDGE},
        {CT_EVENT_4BLD, CT_METHOD_U2, CT_STAGE_CONCEPT, CT_ORBIT_CENTER},
        {CT_EVENT_4BLD, CT_METHOD_R2, CT_STAGE_CONCEPT, CT_ORBIT_WING},
    };
    int ids[SPEC_ONE_LIST];
    ct_stage stages[SPEC_ONE_LIST];
    ct_orbit orbits[SPEC_ONE_LIST];
    char key[SPEC_KEY_CAP];
    for (unsigned long i = 0; i < sizeof spec / sizeof spec[0]; i++) {
        CT_ASSERT_EQ(
            ct_selector_lists(spec[i].event, spec[i].method, ids, stages, orbits, SPEC_ONE_LIST),
            SPEC_ONE_LIST);
        CT_ASSERT_EQ(ids[0], 0);
        CT_ASSERT_EQ(stages[0], spec[i].stage);
        CT_ASSERT_EQ(orbits[0], spec[i].orbit);
        CT_ASSERT_EQ(ct_selector_case_count(spec[i].event, spec[i].method, 0), 0);
        CT_ASSERT(ct_selector_case_key(spec[i].event, spec[i].method, 0, 0, key, sizeof key) < 0);
    }
}

CT_SUITE(selector_navigation) {
    int ids[SPEC_CFOP_LISTS];
    ct_stage stages[SPEC_CFOP_LISTS];
    ct_orbit orbits[SPEC_CFOP_LISTS];
    for (int i = 0; i < SPEC_CFOP_LISTS; i++) {
        ids[i] = -1;
        stages[i] = CT_STAGE_CONCEPT;
        orbits[i] = CT_ORBIT_CENTER;
    }
    CT_ASSERT_EQ(
        ct_selector_lists(CT_EVENT_3X3, CT_METHOD_CFOP, ids, stages, orbits, SPEC_LISTS_SMALL_CAP),
        SPEC_CFOP_LISTS);
    CT_ASSERT_EQ(ids[0], 0);
    CT_ASSERT_EQ(ids[1], 1);
    CT_ASSERT_EQ(ids[2], -1);
    CT_ASSERT_EQ(ids[3], -1);
    CT_ASSERT_EQ(stages[2], CT_STAGE_CONCEPT);
    CT_ASSERT_EQ(ct_selector_case_count(CT_EVENT_3X3, CT_METHOD_CFOP, 2), SPEC_F2L_CASES);
    int few_ids[SPEC_LISTS_SMALL_CAP];
    ct_stage few_stages[SPEC_LISTS_SMALL_CAP];
    ct_orbit few_orbits[SPEC_LISTS_SMALL_CAP];
    CT_ASSERT_EQ(ct_selector_lists(CT_EVENT_3X3, CT_METHOD_CFOP, few_ids, few_stages, few_orbits,
                                   SPEC_LISTS_SMALL_CAP),
                 SPEC_CFOP_LISTS);
    CT_ASSERT_EQ(few_ids[0], 0);
    CT_ASSERT_EQ(few_ids[1], 1);
    CT_ASSERT(ct_selector_lists(CT_EVENT_3X3, (ct_method)-1, few_ids, few_stages, few_orbits,
                                SPEC_LISTS_SMALL_CAP) == 0);
    CT_ASSERT(ct_selector_lists(CT_EVENT_3X3, (ct_method)99, few_ids, few_stages, few_orbits,
                                SPEC_LISTS_SMALL_CAP) == 0);
    CT_ASSERT(ct_selector_lists((ct_event)-1, CT_METHOD_CFOP, few_ids, few_stages, few_orbits,
                                SPEC_LISTS_SMALL_CAP) == 0);
    CT_ASSERT(ct_selector_lists((ct_event)SPEC_EVENT_COUNT, CT_METHOD_CFOP, few_ids, few_stages,
                                few_orbits, SPEC_LISTS_SMALL_CAP) == 0);
    CT_ASSERT(ct_selector_lists(CT_EVENT_3X3, CT_METHOD_REDUCTION, few_ids, few_stages, few_orbits,
                                SPEC_LISTS_SMALL_CAP) == 0);
    CT_ASSERT(ct_selector_case_count(CT_EVENT_3X3, (ct_method)-1, 0) < 0);
    CT_ASSERT(ct_selector_case_count(CT_EVENT_3X3, CT_METHOD_REDUCTION, 0) < 0);
    CT_ASSERT(ct_selector_case_count((ct_event)-1, CT_METHOD_CFOP, 0) < 0);
    char key[SPEC_KEY_CAP];
    CT_ASSERT(ct_selector_case_key((ct_event)-1, CT_METHOD_CFOP, 0, 0, key, sizeof key) < 0);
    CT_ASSERT(ct_selector_case_key(CT_EVENT_3X3, (ct_method)99, 0, 0, key, sizeof key) < 0);
}
