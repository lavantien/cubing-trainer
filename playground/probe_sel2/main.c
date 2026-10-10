#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "config/config.h"
#include "gen/targets.h"
#include "packs/packs.h"
#include "packs/seed_registry.h"
#include "storage/db.h"
#include "storage/repo_settings.h"
#include "trainer/selector.h"

static int fails;

#define CHECK(cond)                                                                              \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);                                \
            fails++;                                                                             \
        }                                                                                        \
    } while (0)

#define CHECK_EQ(a, b)                                                                           \
    do {                                                                                         \
        long long x_ = (long long)(a);                                                           \
        long long y_ = (long long)(b);                                                           \
        if (x_ != y_) {                                                                          \
            printf("FAIL %s:%d %s == %s: %lld vs %lld\n", __FILE__, __LINE__, #a, #b, x_, y_);   \
            fails++;                                                                             \
        }                                                                                        \
    } while (0)

#define CHECK_STR(a, b)                                                                          \
    do {                                                                                         \
        if (strcmp((a), (b)) != 0) {                                                             \
            printf("FAIL %s:%d %s == %s: \"%s\" vs \"%s\"\n", __FILE__, __LINE__, #a, #b, (a),  \
                   (b));                                                                         \
            fails++;                                                                             \
        }                                                                                        \
    } while (0)

int main(void) {
    sqlite3 *db = NULL;
    CHECK_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CHECK_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CHECK_EQ(ct_pack_seed_registered(db, "db/seeds/packs"), 0);

    ct_selector_scheme s;
    CHECK_EQ(ct_selector_scheme_load(db, &s), 0);
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        CHECK_EQ(s.map.corner[i], 'A' + i);
        CHECK_EQ(s.map.edge[i], 'A' + i);
    }
    CHECK_EQ(s.buffer_corner, CT_3STYLE_CORNER_BUFFER);
    CHECK_EQ(s.buffer_edge, CT_3STYLE_EDGE_BUFFER);

    ct_event events[24];
    CHECK_EQ(ct_selector_events(events, 24), 18);
    CHECK_EQ(events[7], CT_EVENT_OH);

    ct_method fmc_methods[4];
    CHECK_EQ(ct_selector_methods(CT_EVENT_FMC, fmc_methods, 4), 1);
    CHECK_EQ(fmc_methods[0], CT_METHOD_CONCEPT);

    ct_selector_list lists[8];
    CHECK_EQ(ct_selector_lists(CT_EVENT_SQUARE1, CT_METHOD_LBL, lists, 8), 5);
    CHECK_EQ(lists[0].stage, CT_STAGE_CUBESHAPE);
    CHECK_EQ(lists[0].source, CT_SELECTOR_PACK);
    CHECK_EQ(ct_selector_lists(CT_EVENT_3BLD, CT_METHOD_3STYLE, lists, 8), 2);
    CHECK_EQ(lists[0].source, CT_SELECTOR_CHART);
    CHECK_EQ(lists[1].orbit, CT_ORBIT_EDGE);

    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, 0), 21);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, 1), 57);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_3X3, CT_METHOD_CFOP, 2), 164);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0), 378);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1), 440);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_FMC, CT_METHOD_CONCEPT, 0), 16);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0), 169);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_2X2, CT_METHOD_CLL, 0), 42);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_4X4, CT_METHOD_YAU, 0), 5);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_5X5, CT_METHOD_REDUCTION, 0), 0);
    CHECK_EQ(ct_selector_case_count(db, &s, CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE, 0), 14);
    CHECK_EQ(ct_selector_case_count(NULL, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0), 378);
    CHECK_EQ(ct_selector_case_count(db, NULL, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0), -1);

    long long total = 0;
    for (int e = 0; e < 18; e++) {
        ct_method ms[4];
        int mc = ct_selector_methods((ct_event)e, ms, 4);
        for (int mi = 0; mi < mc; mi++) {
            ct_selector_list ls[8];
            int lc = ct_selector_lists((ct_event)e, ms[mi], ls, 8);
            for (int li = 0; li < lc; li++) {
                int n = ct_selector_case_count(db, &s, (ct_event)e, ms[mi], li);
                CHECK(n >= 0);
                total += n;
            }
        }
    }
    CHECK_EQ(total, 3512);

    char key[64];
    CHECK_EQ(ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, key, sizeof key), 0);
    CHECK_STR(key, "BC");
    CHECK_EQ(ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 0, key, sizeof key), 0);
    CHECK_STR(key, "AB");
    CHECK_EQ(ct_selector_case_key(db, &s, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, 0, key, sizeof key), 0);
    ct_pack_entry want;
    CHECK_EQ(ct_pack_get(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, key, &want),
             CT_PACK_OK);
    CHECK(want.alg[0] != '\0');
    for (int r = 0; r < 169; r++) {
        char k2[64];
        CHECK_EQ(ct_selector_case_key(db, &s, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, r, k2, sizeof k2),
                 0);
        if (r > 0 && strcmp(key, k2) >= 0) {
            printf("FAIL order at %d: %s then %s\n", r, key, k2);
            fails++;
        }
        snprintf(key, sizeof key, "%s", k2);
    }

    int scheme = ct_letter_scheme_default_id(db);
    CHECK(scheme > 0);
    CHECK_EQ(ct_letter_map_set(db, scheme, 0, 1, 'C'), 0);
    CHECK_EQ(ct_letter_map_set(db, scheme, 0, 2, 'B'), 0);
    ct_selector_scheme s2;
    CHECK_EQ(ct_selector_scheme_load(db, &s2), 0);
    CHECK_EQ(ct_selector_case_key(db, &s2, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, key, sizeof key), 0);
    CHECK_STR(key, "CB");
    CHECK_EQ(ct_selector_case_key(db, &s, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, key, sizeof key), 0);
    CHECK_STR(key, "BC");

    CHECK_EQ(ct_repo_settings_set(db, "bld.buffer_corner", "5"), 0);
    ct_selector_scheme s3;
    CHECK_EQ(ct_selector_scheme_load(db, &s3), 0);
    CHECK_EQ(s3.buffer_corner, 5);
    int t1 = -1;
    int t2 = -1;
    CHECK_EQ(ct_selector_case_pair(&s3, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0, &t1, &t2), 0);
    static ct_target_pair alt[378];
    CHECK_EQ(ct_targets_corner(5, alt, 378), 378);
    CHECK_EQ(t1, alt[0].t1);
    CHECK_EQ(t2, alt[0].t2);

    ct_pack_entry e;
    CHECK_EQ(ct_selector_pack_case(db, CT_EVENT_2X2, CT_METHOD_CLL, 0, 41, &e), 0);
    CHECK_EQ(ct_selector_pack_case(db, CT_EVENT_2X2, CT_METHOD_CLL, 0, 42, &e), -1);
    CHECK_EQ(ct_selector_pack_case(db, CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, &e), -1);

    ct_db_close(db);
    printf(fails == 0 ? "probe ok\n" : "probe fails: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
