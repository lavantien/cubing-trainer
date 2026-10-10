#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "config/config.h"
#include "gen/targets.h"
#include "packs/packs.h"
#include "packs/seed_registry.h"
#include "storage/db.h"
#include "storage/repo_algs.h"
#include "trainer/selector.h"
#include "ui/ui.h"

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

    ct_trainer_form f;
    CHECK_EQ(ct_trainer_form_open(NULL, db), CT_REPO_ERR_ARG);
    CHECK_EQ(ct_trainer_form_open(&f, NULL), CT_REPO_ERR_ARG);
    CHECK_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CHECK_EQ(f.sel.event, CT_EVENT_3X3);
    CHECK_EQ(f.sel.method, CT_METHOD_CFOP);
    CHECK_EQ(f.sel.list_id, 0);
    CHECK_EQ(f.list_count, 3);
    CHECK_EQ(f.case_count, 21);
    CHECK_EQ(f.cell, CT_TRAINER_CELL_NONE);

    CHECK_EQ(ct_trainer_form_select_event(&f, CT_EVENT_SQUARE1), CT_REPO_OK);
    CHECK_EQ(f.sel.method, CT_METHOD_LBL);
    CHECK_EQ(f.list_count, 5);
    CHECK_EQ(f.case_count, 169);
    CHECK_EQ(ct_trainer_form_select_event(&f, CT_EVENT_FMC), CT_REPO_OK);
    CHECK_EQ(f.sel.method, CT_METHOD_CONCEPT);
    CHECK_EQ(f.case_count, 16);
    CHECK_EQ(ct_trainer_form_select_event(&f, (ct_event)-1), CT_REPO_ERR_ARG);
    CHECK_EQ(ct_trainer_form_select_event(&f, CT_EVENT_SQUARE1), CT_REPO_OK);
    CHECK_EQ(ct_trainer_form_select_list(&f, 4), CT_REPO_OK);
    CHECK_EQ(f.lists[4].stage, CT_STAGE_EP);
    CHECK_EQ(f.case_count, 99);
    CHECK_EQ(ct_trainer_form_select_list(&f, 5), CT_REPO_ERR_ARG);
    CHECK_EQ(ct_trainer_form_select_method(&f, CT_METHOD_CFOP), CT_REPO_ERR_ARG);

    CHECK_EQ(ct_trainer_form_select_event(&f, CT_EVENT_SQUARE1), CT_REPO_OK);
    CHECK_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CHECK_EQ(f.pack_status, CT_PACK_OK);
    char key[64];
    CHECK_EQ(ct_selector_case_key(db, &f.scheme, CT_EVENT_SQUARE1, CT_METHOD_LBL, 0, 0, key,
                                  sizeof key),
             0);
    ct_pack_entry want;
    CHECK_EQ(ct_pack_get(db, CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, key, &want),
             CT_PACK_OK);
    CHECK_STR(f.front, want.front_json);
    CHECK_STR(f.alg_text, want.alg);
    CHECK_EQ(ct_trainer_form_select_case(&f, 169), CT_REPO_ERR_ARG);
    CHECK_EQ(ct_trainer_form_select_case(&f, -1), CT_REPO_ERR_ARG);

    CHECK_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CHECK_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CHECK_STR(f.front, "BC");
    CHECK_STR(f.alg_text, "");
    CHECK(f.alg_status != CT_REPO_OK);
    ct_config cfg = ct_config_defaults();
    char hash[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, f.scheme_id, CT_BUFFER_FIXED, hash);
    ct_generated_alg a;
    memset(&a, 0, sizeof a);
    a.orbit = CT_ORBIT_CORNER;
    snprintf(a.target_pair, sizeof a.target_pair, "%s", "BC");
    a.buffer_mode = CT_BUFFER_FIXED;
    a.category = 0;
    snprintf(a.sequence, sizeof a.sequence, "%s", "[R U : [D2, R U R']]");
    CHECK_EQ(ct_repo_alg_put(db, &a, hash), CT_REPO_OK);
    CHECK_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CHECK_EQ(f.alg_status, CT_REPO_OK);
    CHECK_STR(f.alg_text, "[R U : [D2, R U R']]");

    CHECK_EQ(ct_trainer_form_visible_count(&f), 378);
    CHECK_EQ(ct_trainer_form_select_cell(&f, 1), CT_REPO_OK);
    static ct_target_pair pairs[378];
    CHECK_EQ(ct_targets_corner(CT_3STYLE_CORNER_BUFFER, pairs, 378), 378);
    int expect = 0;
    int first = -1;
    for (int i = 0; i < 378; i++) {
        if (pairs[i].t1 == 1 || pairs[i].t2 == 1) {
            if (first < 0) {
                first = i;
            }
            expect++;
        }
    }
    CHECK(expect > 0);
    CHECK_EQ(ct_trainer_form_visible_count(&f), expect);
    CHECK_EQ(ct_trainer_form_nth_visible(&f, 0), first);
    CHECK_EQ(ct_trainer_form_nth_visible(&f, expect), -1);
    CHECK_EQ(ct_trainer_form_select_case(&f, first), CT_REPO_OK);
    int t1 = -1;
    int t2 = -1;
    CHECK_EQ(ct_selector_case_pair(&f.scheme, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, first, &t1, &t2), 0);
    CHECK(t1 == 1 || t2 == 1);
    CHECK_EQ(ct_trainer_form_clear_cell(&f), CT_REPO_OK);
    CHECK_EQ(ct_trainer_form_visible_count(&f), 378);
    CHECK_EQ(ct_trainer_form_select_cell(&f, -1), CT_REPO_ERR_ARG);
    CHECK_EQ(ct_trainer_form_select_cell(&f, 24), CT_REPO_ERR_ARG);
    CHECK_EQ(ct_trainer_form_select_list(&f, 1), CT_REPO_OK);
    CHECK_EQ(f.case_count, 440);

    printf(fails == 0 ? "probe screen ok\n" : "probe screen fails: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
