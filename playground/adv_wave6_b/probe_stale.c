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

static void put_alg(sqlite3 *db, const char *hash, const char *pair, const char *seq) {
    ct_generated_alg a;
    memset(&a, 0, sizeof a);
    a.orbit = CT_ORBIT_CORNER;
    a.buffer_mode = CT_BUFFER_FIXED;
    a.category = 0;
    a.classification = CT_CLS_PURE;
    a.qtm = 9;
    a.score = 1.0;
    snprintf(a.target_pair, sizeof a.target_pair, "%s", pair);
    snprintf(a.sequence, sizeof a.sequence, "%s", seq);
    snprintf(a.score_parts_json, sizeof a.score_parts_json, "%s", "{\"len\":9}");
    if (ct_repo_alg_put(db, &a, hash) != CT_REPO_OK)
        printf("put failed for %s\n", seq);
}

int main(void) {
    sqlite3 *db = open_db();
    if (db == NULL) {
        printf("db open failed\n");
        return 1;
    }
    ct_trainer_form tf;
    if (ct_trainer_form_open(&tf, db) != CT_REPO_OK) {
        printf("trainer open failed\n");
        return 1;
    }
    if (ct_trainer_form_select_event(&tf, CT_EVENT_3BLD) != CT_REPO_OK) {
        printf("event select failed\n");
        return 1;
    }
    if (ct_trainer_form_select_method(&tf, CT_METHOD_3STYLE) != CT_REPO_OK) {
        printf("method select failed\n");
        return 1;
    }
    printf("lists %d corner_cases %d\n", tf.list_count, tf.case_count);

    char hash_old[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&tf.cfg, tf.scheme_id, CT_BUFFER_FIXED, tf.scheme.buffer_corner,
                   tf.scheme.buffer_edge, &tf.scheme.map, hash_old);
    put_alg(db, hash_old, "AB", "OLD_BUFFER_UFR");

    ct_settings_form sf;
    ct_settings_form_open(&sf, db);
    if (sf.status != CT_REPO_OK) {
        printf("settings open failed %d\n", (int)sf.status);
        return 1;
    }
    printf("settings_default_corner %d\n", sf.buffer_corner);
    if (ct_settings_form_stage_buffer(&sf, CT_ORBIT_CORNER, 5) != CT_REPO_OK) {
        printf("buffer stage failed\n");
        return 1;
    }
    if (ct_settings_form_save(&sf) != CT_REPO_OK) {
        printf("settings save failed\n");
        return 1;
    }

    ct_selector_scheme fresh;
    if (ct_selector_scheme_load(db, &fresh) != 0) {
        printf("fresh scheme load failed\n");
        return 1;
    }
    printf("fresh_corner %d stale_form_corner %d\n", fresh.buffer_corner,
           tf.scheme.buffer_corner);

    char hash_new[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&tf.cfg, tf.scheme_id, CT_BUFFER_FIXED, fresh.buffer_corner,
                   fresh.buffer_edge, &fresh.map, hash_new);
    printf("hash_changed_after_save %s\n", strcmp(hash_old, hash_new) == 0 ? "no" : "yes");
    put_alg(db, hash_new, "AB", "NEW_BUFFER_5");

    if (ct_trainer_form_select_case(&tf, 0) != CT_REPO_OK) {
        printf("case select failed\n");
        return 1;
    }
    printf("stale_form_serves %s\n", tf.alg_text);

    ct_trainer_form tf2;
    if (ct_trainer_form_open(&tf2, db) != CT_REPO_OK) {
        printf("reopen failed\n");
        return 1;
    }
    ct_trainer_form_select_event(&tf2, CT_EVENT_3BLD);
    ct_trainer_form_select_method(&tf2, CT_METHOD_3STYLE);
    ct_trainer_form_select_case(&tf2, 0);
    printf("reopened_form_serves %s\n", tf2.alg_text);

    ct_settings_form sf2;
    ct_settings_form_open(&sf2, db);
    if (ct_settings_form_swap_letter(&sf2, CT_ORBIT_CORNER, 0, 'B') != CT_REPO_OK) {
        printf("letter swap failed\n");
        return 1;
    }
    if (ct_settings_form_stage_move(&sf2, CT_MOVE_BIT_L) != CT_REPO_OK) {
        printf("move toggle failed\n");
        return 1;
    }
    if (ct_settings_form_save(&sf2) != CT_REPO_OK) {
        printf("second save failed\n");
        return 1;
    }
    ct_selector_scheme fresh2;
    ct_selector_scheme_load(db, &fresh2);
    char hash_after[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&tf2.cfg, tf2.scheme_id, CT_BUFFER_FIXED, fresh2.buffer_corner,
                   fresh2.buffer_edge, &fresh2.map, hash_after);
    printf("hash_changed_after_map_and_moves %s\n",
           strcmp(hash_after, hash_new) == 0 ? "no" : "yes");
    printf("fresh_corner0_letter %c stale_corner0_letter %c\n", fresh2.map.corner[0],
           tf2.scheme.map.corner[0]);

    sqlite3_close(db);
    return 0;
}
