#include "config/config.h"
#include "storage/db.h"
#include "storage/repo_algs.h"
#include "storage/repo_settings.h"
#include "trainer/selector.h"
#include "ui/ui.h"

#include <stdio.h>
#include <string.h>

static sqlite3 *open_db(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK)
        return NULL;
    if (ct_db_migrate(db, "db/migrations") != CT_REPO_OK) {
        sqlite3_close(db);
        return NULL;
    }
    if (ct_letter_seed_speffz(db) != 0) {
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

static void put_edge_alg(sqlite3 *db, const char *hash, const char *pair, const char *seq,
                         int category) {
    ct_generated_alg a;
    memset(&a, 0, sizeof a);
    a.orbit = CT_ORBIT_EDGE;
    a.buffer_mode = CT_BUFFER_FIXED;
    a.category = category;
    a.classification = CT_CLS_PURE;
    a.qtm = 9;
    a.score = 1.0;
    snprintf(a.target_pair, sizeof a.target_pair, "%s", pair);
    snprintf(a.sequence, sizeof a.sequence, "%s", seq);
    snprintf(a.score_parts_json, sizeof a.score_parts_json, "%s", "{\"len\":9}");
    if (ct_repo_alg_put(db, &a, hash) != CT_REPO_OK)
        printf("put failed %s cat %d\n", seq, category);
}

int main(void) {
    sqlite3 *db = open_db();
    if (db == NULL) {
        printf("db open failed\n");
        return 1;
    }
    ct_trainer_form f;
    if (ct_trainer_form_open(&f, db) != CT_REPO_OK) {
        printf("trainer open failed\n");
        return 1;
    }
    ct_trainer_form_select_event(&f, CT_EVENT_3BLD);
    ct_trainer_form_select_list(&f, 1);
    printf("edge_list cases %d orbit %d\n", f.case_count, (int)f.lists[1].orbit);

    char hash[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&f.cfg, f.scheme_id, CT_BUFFER_FIXED, f.scheme.buffer_corner,
                   f.scheme.buffer_edge, &f.scheme.map, hash);

    put_edge_alg(db, hash, "AB", "SCRIPT_CAT_1", 1);
    ct_trainer_form_select_case(&f, 0);
    printf("ui_reads_script_row cat1 status %d text %s\n", (int)f.alg_status, f.alg_text);

    put_edge_alg(db, hash, "AB", "UI_CAT_0", 0);
    ct_trainer_form_select_case(&f, 0);
    printf("ui_reads_cat0_row status %d text %s\n", (int)f.alg_status, f.alg_text);

    sqlite3_close(db);
    return 0;
}
