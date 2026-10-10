#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/srs/fsrs.h"
#include "../src/storage/repo_cards.h"
#include "../src/storage/repo_settings.h"
#include "../src/ui/ui.h"
#include "framework.h"

static const char merge_day1[] = "2026-10-09T00:00:00Z";
static const char merge_day2[] = "2026-10-10T00:00:00Z";
static const long merge_today1 = 20735;
static const long merge_today2 = 20736;

static sqlite3 *merge_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static void assert_same_config(const ct_config *a, const ct_config *b) {
    CT_ASSERT(memcmp(a, b, sizeof(ct_config)) == 0);
}

CT_SUITE(settings_merge_empty) {
    sqlite3 *db = merge_db();
    ct_config def = ct_config_defaults();
    ct_config merged = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &merged), 0);
    assert_same_config(&merged, &def);
    ct_db_close(db);
}

CT_SUITE(settings_merge_round_trip) {
    static const struct {
        const char *key;
        const char *value;
    } rows[] = {
        {"srs.request_retention", "0.85"},
        {"srs.max_ivl_days", "40000"},
        {"srs.w0", "0.5"},
        {"srs.w9", "1.25"},
        {"srs.w18", "0.7"},
        {"gen.preferred_moves", "FU"},
        {"gen.weight_len", "2.5"},
        {"gen.weight_subset", "1.5"},
        {"gen.weight_regrip", "2.25"},
        {"gen.weight_finger", "0.75"},
        {"gen.penalty_U", "1.5"},
        {"gen.penalty_x", "3.25"},
        {"gen.finger_cost_U_cw", "0.3"},
        {"gen.finger_cost_B_ccw", "0.9"},
        {"gen.setup_bfs_depth", "6"},
        {"gen.fallback_depth", "7"},
        {"gen.straggler_depth", "8"},
        {"anim.move_seconds", "0.4"},
        {"window.width", "1600"},
        {"window.height", "900"},
        {"window.target_fps", "144"},
        {"db.filename", "other.db"},
        {"fuzz.iterations", "5000"},
    };
    sqlite3 *db = merge_db();
    ct_config ref = ct_config_defaults();
    ct_config merged = ct_config_defaults();
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        CT_ASSERT_EQ(ct_config_set(&ref, rows[i].key, rows[i].value), CT_CFG_OK);
        CT_ASSERT_EQ(ct_repo_settings_set(db, rows[i].key, rows[i].value), 0);
    }
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &merged), 0);
    assert_same_config(&merged, &ref);
    CT_ASSERT_DBL_EQ(merged.fsrs.request_retention, 0.85, 1e-12);
    CT_ASSERT_DBL_EQ(merged.fsrs.w[9], 1.25, 1e-12);
    CT_ASSERT_DBL_EQ(merged.anim.move_seconds, 0.4, 1e-12);
    CT_ASSERT_EQ(merged.gen.preferred_moves, (unsigned)(CT_MOVE_BIT_F | CT_MOVE_BIT_U));
    CT_ASSERT_DBL_EQ(merged.gen.penalty[9], 3.25, 1e-12);
    CT_ASSERT_DBL_EQ(merged.gen.finger_cost[5][CT_FINGER_DIR_CCW], 0.9, 1e-12);
    CT_ASSERT_EQ(merged.gen.setup_bfs_depth, 6);
    CT_ASSERT_EQ(merged.window.width, 1600);
    CT_ASSERT_EQ(merged.window.target_fps, 144);
    CT_ASSERT_STR_EQ(merged.db.filename, "other.db");
    CT_ASSERT_EQ(merged.fuzz.iterations, 5000);
    ct_db_close(db);
}

CT_SUITE(settings_merge_invalid_values) {
    static const struct {
        const char *key;
        const char *value;
    } rows[] = {
        {"srs.request_retention", "abc"},
        {"srs.request_retention", ""},
        {"srs.request_retention", "nan"},
        {"srs.request_retention", "1e999"},
        {"srs.max_ivl_days", "1.5x"},
        {"srs.w4", "junk"},
        {"anim.move_seconds", "xyz"},
        {"gen.preferred_moves", "Q!"},
        {"gen.weight_len", "12,5"},
        {"gen.setup_bfs_depth", "zzz"},
        {"window.width", "0"},
        {"window.height", "-5"},
        {"window.target_fps", "0.5"},
        {"db.filename", "a/b"},
        {"db.filename", ""},
        {"fuzz.iterations", "99999999999999999999"},
    };
    sqlite3 *db = merge_db();
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        CT_ASSERT_EQ(ct_repo_settings_set(db, rows[i].key, rows[i].value), 0);
        ct_config def = ct_config_defaults();
        ct_config merged = ct_config_defaults();
        CT_ASSERT_EQ(ct_repo_settings_merge(db, &merged), 0);
        assert_same_config(&merged, &def);
        CT_ASSERT_EQ(sqlite3_exec(db, "DELETE FROM settings", NULL, NULL, NULL), SQLITE_OK);
    }
    ct_db_close(db);
}

CT_SUITE(settings_merge_unknown_keys) {
    static const char *const keys[] = {
        "bld.buffer_corner", "bld.buffer_edge",      "ui.color_u", "ui.color_d", "nope", "srs.w19",
        "gen.penalty_Q",     "gen.finger_cost_M_cw", "",           "anim",       "srs.",
    };
    sqlite3 *db = merge_db();
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++) {
        CT_ASSERT_EQ(ct_repo_settings_set(db, keys[i], "1.5"), 0);
    }
    ct_config def = ct_config_defaults();
    ct_config merged = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &merged), 0);
    assert_same_config(&merged, &def);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "anim.move_seconds", "0.4"), 0);
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &merged), 0);
    CT_ASSERT_DBL_EQ(merged.anim.move_seconds, 0.4, 1e-12);
    ct_db_close(db);
}

CT_SUITE(settings_merge_guards) {
    sqlite3 *db = merge_db();
    ct_config def = ct_config_defaults();
    ct_config merged = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_merge(NULL, &merged), -1);
    assert_same_config(&merged, &def);
    CT_ASSERT_EQ(ct_repo_settings_merge(db, NULL), -1);
    CT_ASSERT_EQ(ct_repo_settings_merge(NULL, NULL), -1);
    CT_ASSERT_EQ(ct_repo_settings_merge_rejected(NULL, &merged, NULL), -1);
    CT_ASSERT_EQ(ct_repo_settings_merge_rejected(db, NULL, NULL), -1);
    int rejected = -1;
    CT_ASSERT_EQ(ct_repo_settings_merge_rejected(db, &merged, &rejected), 0);
    CT_ASSERT_EQ(rejected, 0);
    ct_db_close(db);
}

CT_SUITE(settings_merge_rejected_count) {
    sqlite3 *db = merge_db();
    CT_ASSERT_EQ(ct_repo_settings_set(db, "anim.move_seconds", "0.4"), 0);
    int rejected = -1;
    ct_config merged = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_merge_rejected(db, &merged, &rejected), 0);
    CT_ASSERT_EQ(rejected, 0);
    CT_ASSERT_DBL_EQ(merged.anim.move_seconds, 0.4, 1e-12);

    CT_ASSERT_EQ(ct_repo_settings_set(db, "nope", "1"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "anim.move_seconds", "junk"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "srs.w19", "1"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "gen.weight_len", "-2"), 0);
    CT_ASSERT_EQ(ct_repo_settings_merge_rejected(db, &merged, &rejected), 0);
    CT_ASSERT_EQ(rejected, 4);
    CT_ASSERT_DBL_EQ(merged.anim.move_seconds, 0.4, 1e-12);

    CT_ASSERT_EQ(
        sqlite3_exec(db, "INSERT INTO settings(key, value) VALUES('srs.request_retention', NULL)",
                     NULL, NULL, NULL),
        SQLITE_OK);
    CT_ASSERT_EQ(ct_repo_settings_merge_rejected(db, &merged, &rejected), 0);
    CT_ASSERT_EQ(rejected, 5);

    ct_config fresh = ct_config_defaults();
    ct_config def = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &fresh), 0);
    CT_ASSERT(memcmp(&fresh, &def, sizeof(ct_config)) == 0);
    ct_db_close(db);
}

CT_SUITE(settings_merge_null_value) {
    sqlite3 *db = merge_db();
    CT_ASSERT_EQ(sqlite3_exec(db,
                              "INSERT INTO settings(key, value) VALUES('anim.move_seconds', NULL)",
                              NULL, NULL, NULL),
                 SQLITE_OK);
    CT_ASSERT_EQ(
        sqlite3_exec(db, "INSERT INTO settings(key, value) VALUES('srs.request_retention', '')",
                     NULL, NULL, NULL),
        SQLITE_OK);
    ct_config def = ct_config_defaults();
    ct_config merged = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &merged), 0);
    assert_same_config(&merged, &def);
    ct_db_close(db);
}

CT_SUITE(settings_merge_after_form_save) {
    sqlite3 *db = merge_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, 0.85), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(&f, 0.5), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_F), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_save(&f), CT_REPO_OK);

    ct_config merged = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &merged), 0);
    CT_ASSERT_DBL_EQ(merged.fsrs.request_retention, 0.85, 1e-9);
    CT_ASSERT_DBL_EQ(merged.anim.move_seconds, 0.5, 1e-9);
    CT_ASSERT_EQ(merged.gen.preferred_moves,
                 (unsigned)(CT_MOVE_BIT_U | CT_MOVE_BIT_D | CT_MOVE_BIT_R | CT_MOVE_BIT_F));
    ct_db_close(db);
}

CT_SUITE(settings_merge_overwrites_live_cfg) {
    sqlite3 *db = merge_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, 0.85), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(&f, 0.5), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_save(&f), CT_REPO_OK);

    ct_config live = ct_config_defaults();
    CT_ASSERT_EQ(ct_config_set(&live, "srs.request_retention", "0.99"), CT_CFG_OK);
    CT_ASSERT_EQ(ct_config_set(&live, "anim.move_seconds", "0.05"), CT_CFG_OK);
    CT_ASSERT_EQ(ct_repo_settings_merge(db, &live), 0);
    CT_ASSERT_DBL_EQ(live.fsrs.request_retention, 0.85, 1e-9);
    CT_ASSERT_DBL_EQ(live.anim.move_seconds, 0.5, 1e-9);
    ct_db_close(db);
}

static long form_second_good_scheduled(const char *retention) {
    sqlite3 *db = merge_db();
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "rev");
    snprintf(c.case_key, sizeof c.case_key, "a");
    c.direction = CT_DIR_FR;
    snprintf(c.alg_3x3, sizeof c.alg_3x3, "R U R'");
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    snprintf(row.due, sizeof row.due, "2026-10-01T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, c.id, &row), CT_REPO_OK);
    if (retention != NULL)
        CT_ASSERT_EQ(ct_repo_settings_set(db, "srs.request_retention", retention), 0);

    ct_review_form f;
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", merge_day1, merge_today1, 0), 1);
    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_GOOD, merge_day1), CT_SESSION_OK);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", merge_day2, merge_today2, 0), 1);
    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_GOOD, merge_day2), CT_SESSION_OK);

    sqlite3_stmt *st = NULL;
    CT_ASSERT_EQ(
        sqlite3_prepare_v2(db, "SELECT scheduled_days FROM review_log ORDER BY id DESC LIMIT 1", -1,
                           &st, NULL),
        SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_step(st), SQLITE_ROW);
    long scheduled = (long)sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    ct_db_close(db);
    return scheduled;
}

static long api_second_good_scheduled(const char *retention) {
    ct_config cfg = ct_config_defaults();
    if (retention != NULL)
        CT_ASSERT_EQ(ct_config_set(&cfg, "srs.request_retention", retention), CT_CFG_OK);
    ct_fsrs fs;
    ct_fsrs_init(&fs, &cfg);
    ct_fsrs_card zero = {0};
    ct_fsrs_card first = ct_fsrs_review(&fs, &zero, CT_GOOD, 0, merge_today1);
    ct_fsrs_card second = ct_fsrs_review(&fs, &first, CT_GOOD, 1, merge_today2);
    return ct_fsrs_next_interval_days(&fs, second.stability);
}

CT_SUITE(settings_merge_reaches_review) {
    long sched_70 = form_second_good_scheduled("0.7");
    long sched_99 = form_second_good_scheduled("0.99");
    long sched_default = form_second_good_scheduled(NULL);
    CT_ASSERT(sched_70 > sched_99);
    CT_ASSERT(sched_default > sched_99);
    CT_ASSERT(sched_70 > sched_default);
    CT_ASSERT_EQ(sched_70, api_second_good_scheduled("0.7"));
    CT_ASSERT_EQ(sched_99, api_second_good_scheduled("0.99"));
    CT_ASSERT_EQ(sched_default, api_second_good_scheduled(NULL));
}
