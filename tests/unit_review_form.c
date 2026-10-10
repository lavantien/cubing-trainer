#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../src/app/app.h"
#include "../src/engine/notation.h"
#include "../src/engine/transform.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/ui/ui.h"
#include "framework.h"

static const char now_stamp[] = "2026-10-09T00:00:00Z";
static const long today_num = 20735;

static sqlite3 *review_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static long seed_card(sqlite3 *db, const char *deck, const char *key, const char *alg_3x3,
                      const char *alg_big, const char *alg_oh) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", key);
    c.direction = CT_DIR_FR;
    snprintf(c.alg_3x3, sizeof c.alg_3x3, "%s", alg_3x3);
    snprintf(c.alg_big, sizeof c.alg_big, "%s", alg_big);
    snprintf(c.alg_oh, sizeof c.alg_oh, "%s", alg_oh);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    return c.id;
}

static void seed_due(sqlite3 *db, long id) {
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    snprintf(row.due, sizeof row.due, "%s", "2026-10-01T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);
}

static int parse_alg(const char *alg, ct_move *out) {
    return ct_notation_parse(alg, 3, out, CT_ANIM_QUEUE_CAP);
}

static void scramble_expected(ct_cube *exp, const ct_move *moves, int count) {
    ct_cube_init(exp, 3);
    ct_move inv[CT_ANIM_QUEUE_CAP];
    int n = ct_transform_inverse(moves, count, inv);
    for (int i = 0; i < n; i++) {
        ct_apply(exp, inv[i]);
    }
}

static bool read_last_session(sqlite3 *db, char *deck, char *started, char *ended, long *counts) {
    static const char sql[] =
        "SELECT deck, started_at, ended_at, n_cards, n_again, n_hard, n_good, n_easy"
        " FROM session_log ORDER BY id DESC LIMIT 1";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return false;
    }
    if (sqlite3_step(st) != SQLITE_ROW) {
        sqlite3_finalize(st);
        return false;
    }
    snprintf(deck, CT_SESSION_DECK_MAX, "%s", (const char *)sqlite3_column_text(st, 0));
    snprintf(started, CT_SESSION_STAMP_MAX, "%s", (const char *)sqlite3_column_text(st, 1));
    snprintf(ended, CT_SESSION_STAMP_MAX, "%s", (const char *)sqlite3_column_text(st, 2));
    for (int i = 0; i < 5; i++) {
        counts[i] = (long)sqlite3_column_int64(st, 3 + i);
    }
    sqlite3_finalize(st);
    return true;
}

CT_SUITE(review_form_begin) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    long b = seed_card(db, "rev", "b", "U R U'", "", "");
    seed_due(db, a);
    seed_due(db, b);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 2);
    CT_ASSERT_EQ(f.session.queue_count, 2);
    CT_ASSERT_EQ(f.status, CT_SESSION_OK);
    CT_ASSERT_EQ(f.card.id, a);
    CT_ASSERT_STR_EQ(f.card.case_key, "a");
    CT_ASSERT_STR_EQ(f.alg_text, "R U R'");
    CT_ASSERT_EQ(f.move_count, 3);
    CT_ASSERT_EQ(f.anim.count, 3);
    CT_ASSERT(!f.revealed);
    CT_ASSERT(!f.playing);
    CT_ASSERT(!f.anim.playing);
    CT_ASSERT_DBL_EQ(f.speed, 1.0, 1e-9);
    CT_ASSERT_DBL_EQ(f.anim.speed, 1.0, 1e-9);
    CT_ASSERT_EQ(f.scrub_index, 0);
    CT_ASSERT_EQ(f.cube.n, 3);

    ct_move moves[CT_ANIM_QUEUE_CAP];
    CT_ASSERT_EQ(parse_alg("R U R'", moves), 3);
    ct_cube exp;
    scramble_expected(&exp, moves, 3);
    CT_ASSERT(ct_cube_equal(&f.cube, &exp));
    CT_ASSERT(!ct_cube_solved(&f.cube));

    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 1), 1);
    CT_ASSERT_EQ(f.session.queue_count, 1);
    CT_ASSERT_EQ(f.card.id, a);
    CT_ASSERT_EQ(f.move_count, 3);

    ct_db_close(db);
}

CT_SUITE(review_form_begin_garbage) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    seed_due(db, a);

    ct_review_form f;
    memset(&f, 0xA5, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 1);
    CT_ASSERT_EQ(f.status, CT_SESSION_OK);
    CT_ASSERT_EQ(f.session.queue_count, 1);
    CT_ASSERT_EQ(f.card.id, a);

    memset(&f, 0x5A, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 1);
    CT_ASSERT_EQ(f.session.queue_count, 1);
    ct_review_form_end(&f, now_stamp);
    ct_session_free(&f.session);
    ct_db_close(db);
}

CT_SUITE(review_form_empty_deck) {
    sqlite3 *db = review_db();
    seed_card(db, "other", "x", "R U", "", "");

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 0);
    CT_ASSERT_EQ(f.session.queue_count, 0);
    CT_ASSERT(!ct_session_has_next(&f.session));
    CT_ASSERT_EQ(f.card.id, 0);
    CT_ASSERT_EQ(f.move_count, 0);
    CT_ASSERT_EQ(f.anim.count, 0);
    CT_ASSERT(ct_cube_solved(&f.cube));
    CT_ASSERT_EQ(f.alg_text[0], '\0');
    CT_ASSERT(ct_review_form_grade(&f, CT_GOOD, now_stamp) < 0);

    ct_db_close(db);
}

CT_SUITE(review_form_reveal_grade) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    long b = seed_card(db, "rev", "b", "U R U'", "", "");
    seed_due(db, a);
    seed_due(db, b);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 2);

    ct_move moves[CT_ANIM_QUEUE_CAP];
    CT_ASSERT_EQ(parse_alg("R U R'", moves), 3);
    ct_cube scrambled;
    scramble_expected(&scrambled, moves, 3);

    ct_review_form_reveal(&f);
    CT_ASSERT(f.revealed);
    CT_ASSERT(f.playing);
    CT_ASSERT(f.anim.playing);
    CT_ASSERT_EQ(f.anim.count, 3);
    CT_ASSERT_EQ(f.scrub_index, 0);
    CT_ASSERT_DBL_EQ(f.anim.elapsed, 0.0, 1e-9);
    CT_ASSERT(ct_cube_equal(&f.cube, &scrambled));

    ct_review_form_step(&f);
    ct_review_form_step(&f);
    ct_review_form_reveal(&f);
    CT_ASSERT(ct_cube_equal(&f.cube, &scrambled));
    CT_ASSERT_EQ(f.anim.count, 3);
    CT_ASSERT_EQ(f.scrub_index, 0);

    ct_review_form_play_pause(&f);
    CT_ASSERT(!f.playing);
    CT_ASSERT(!f.anim.playing);
    ct_review_form_play_pause(&f);
    CT_ASSERT(f.playing);
    CT_ASSERT(f.anim.playing);

    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_GOOD, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(f.status, CT_SESSION_OK);
    CT_ASSERT_EQ(f.session.n_graded, 1);
    CT_ASSERT_EQ(f.session.n_good, 1);
    CT_ASSERT_EQ(f.card.id, b);
    CT_ASSERT_STR_EQ(f.alg_text, "U R U'");
    CT_ASSERT(!f.revealed);
    CT_ASSERT(!f.playing);
    CT_ASSERT_EQ(f.move_count, 3);

    ct_srs_row row;
    memset(&row, 0, sizeof row);
    CT_ASSERT_EQ(ct_repo_srs_get(db, a, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 3.173, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 5.282434422319005, 1e-9);
    CT_ASSERT_STR_EQ(row.due, now_stamp);
    CT_ASSERT_STR_EQ(row.last_review, now_stamp);
    CT_ASSERT_EQ(row.reps, 1);
    CT_ASSERT_EQ(row.lapses, 0);
    CT_ASSERT_EQ(row.state, 1);

    CT_ASSERT(ct_review_form_grade(&f, (ct_grade)0, now_stamp) < 0);
    CT_ASSERT(ct_review_form_grade(&f, (ct_grade)5, now_stamp) < 0);

    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_EASY, now_stamp), CT_SESSION_OK);
    CT_ASSERT(!ct_session_has_next(&f.session));
    CT_ASSERT_EQ(f.card.id, 0);
    CT_ASSERT(ct_cube_solved(&f.cube));
    CT_ASSERT_EQ(f.move_count, 0);
    CT_ASSERT_EQ(f.anim.count, 0);
    CT_ASSERT(ct_review_form_grade(&f, CT_GOOD, now_stamp) < 0);
    CT_ASSERT_EQ(f.session.n_graded, 2);

    ct_db_close(db);
}

CT_SUITE(review_form_playback) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    seed_due(db, a);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 1);

    ct_move moves[CT_ANIM_QUEUE_CAP];
    CT_ASSERT_EQ(parse_alg("R U R'", moves), 3);
    ct_cube after_first;
    scramble_expected(&after_first, moves, 3);
    ct_apply(&after_first, moves[0]);

    ct_review_form_step(&f);
    CT_ASSERT_EQ(f.anim.count, 2);
    CT_ASSERT_EQ(f.scrub_index, 1);
    CT_ASSERT(ct_cube_equal(&f.cube, &after_first));
    CT_ASSERT(!ct_cube_solved(&f.cube));

    ct_review_form_step(&f);
    ct_review_form_step(&f);
    CT_ASSERT_EQ(f.anim.count, 0);
    CT_ASSERT_EQ(f.scrub_index, 3);
    CT_ASSERT(ct_cube_solved(&f.cube));

    ct_review_form_step(&f);
    CT_ASSERT_EQ(f.anim.count, 0);
    CT_ASSERT(ct_cube_solved(&f.cube));
    CT_ASSERT_EQ(f.scrub_index, 3);

    ct_db_close(db);
}

CT_SUITE(review_form_scrub) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    seed_due(db, a);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 1);

    ct_move moves[CT_ANIM_QUEUE_CAP];
    CT_ASSERT_EQ(parse_alg("R U R'", moves), 3);
    ct_cube scrambled;
    scramble_expected(&scrambled, moves, 3);
    ct_cube after_first = scrambled;
    ct_apply(&after_first, moves[0]);
    ct_cube after_two = after_first;
    ct_apply(&after_two, moves[1]);

    ct_review_form_step(&f);
    ct_review_form_step(&f);
    CT_ASSERT(ct_cube_equal(&f.cube, &after_two));

    CT_ASSERT_EQ(ct_review_form_scrub(&f, 1), CT_SESSION_OK);
    CT_ASSERT_EQ(f.scrub_index, 1);
    CT_ASSERT_EQ(f.anim.count, 2);
    CT_ASSERT(ct_cube_equal(&f.cube, &after_first));
    CT_ASSERT(!f.playing);
    CT_ASSERT(!f.anim.playing);
    CT_ASSERT_DBL_EQ(f.anim.elapsed, 0.0, 1e-9);

    CT_ASSERT_EQ(ct_review_form_scrub(&f, 99), CT_SESSION_OK);
    CT_ASSERT_EQ(f.scrub_index, 3);
    CT_ASSERT_EQ(f.anim.count, 0);
    CT_ASSERT(ct_cube_solved(&f.cube));

    CT_ASSERT_EQ(ct_review_form_scrub(&f, -5), CT_SESSION_OK);
    CT_ASSERT_EQ(f.scrub_index, 0);
    CT_ASSERT_EQ(f.anim.count, 3);
    CT_ASSERT(ct_cube_equal(&f.cube, &scrambled));

    ct_review_form_set_speed(&f, 2.0);
    CT_ASSERT_EQ(ct_review_form_scrub(&f, 1), CT_SESSION_OK);
    CT_ASSERT_DBL_EQ(f.speed, 2.0, 1e-9);
    CT_ASSERT_DBL_EQ(f.anim.speed, 2.0, 1e-9);

    long b = seed_card(db, "ghost", "b", "", "", "");
    seed_due(db, b);
    ct_review_form empty_alg;
    memset(&empty_alg, 0, sizeof empty_alg);
    CT_ASSERT_EQ(ct_review_form_begin(&empty_alg, db, "ghost", now_stamp, today_num, 0), 1);
    CT_ASSERT_EQ(empty_alg.move_count, 0);
    CT_ASSERT_EQ(ct_review_form_scrub(&empty_alg, 4), CT_SESSION_OK);
    CT_ASSERT_EQ(empty_alg.scrub_index, 0);
    CT_ASSERT(ct_cube_solved(&empty_alg.cube));

    ct_db_close(db);
}

CT_SUITE(review_form_speed) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    seed_due(db, a);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 1);

    ct_review_form_set_speed(&f, 1.5);
    CT_ASSERT_DBL_EQ(f.speed, 1.5, 1e-9);
    CT_ASSERT_DBL_EQ(f.anim.speed, 1.5, 1e-9);

    ct_review_form_set_speed(&f, 0.5);
    CT_ASSERT_DBL_EQ(f.speed, 0.5, 1e-9);
    CT_ASSERT_DBL_EQ(f.anim.speed, 0.5, 1e-9);

    ct_review_form_set_speed(&f, 0.0);
    CT_ASSERT(f.speed > 0.0);
    CT_ASSERT_DBL_EQ(f.speed, CT_REVIEW_SPEED_MIN, 1e-12);
    CT_ASSERT_DBL_EQ(f.anim.speed, CT_REVIEW_SPEED_MIN, 1e-12);

    ct_review_form_set_speed(&f, -2.0);
    CT_ASSERT_DBL_EQ(f.speed, CT_REVIEW_SPEED_MIN, 1e-12);
    CT_ASSERT_DBL_EQ(f.anim.speed, CT_REVIEW_SPEED_MIN, 1e-12);

    ct_review_form_grade(&f, CT_GOOD, now_stamp);
    long b = seed_card(db, "rev", "b", "U R", "", "");
    seed_due(db, b);
    ct_review_form fresh;
    memset(&fresh, 0, sizeof fresh);
    CT_ASSERT_EQ(ct_review_form_begin(&fresh, db, "rev", now_stamp, today_num, 0), 2);
    CT_ASSERT_DBL_EQ(fresh.speed, 1.0, 1e-9);
    CT_ASSERT_DBL_EQ(fresh.anim.speed, 1.0, 1e-9);

    ct_db_close(db);
}

CT_SUITE(review_form_fallback) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "", "Rw U Rw'", "");
    long b = seed_card(db, "rev", "b", "", "", "");
    seed_due(db, a);
    seed_due(db, b);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 2);
    CT_ASSERT_EQ(f.card.id, a);
    CT_ASSERT_STR_EQ(f.alg_text, "Rw U Rw'");
    CT_ASSERT_EQ(f.move_count, 3);
    CT_ASSERT_EQ(f.anim.count, 3);

    ct_move moves[CT_ANIM_QUEUE_CAP];
    CT_ASSERT_EQ(parse_alg("Rw U Rw'", moves), 3);
    ct_cube exp;
    scramble_expected(&exp, moves, 3);
    CT_ASSERT(ct_cube_equal(&f.cube, &exp));

    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_GOOD, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(f.card.id, b);
    CT_ASSERT_STR_EQ(f.alg_text, "");
    CT_ASSERT_EQ(f.move_count, 0);
    CT_ASSERT_EQ(f.anim.count, 0);
    CT_ASSERT(ct_cube_solved(&f.cube));
    ct_review_form_reveal(&f);
    CT_ASSERT(f.revealed);
    CT_ASSERT_EQ(f.anim.count, 0);
    CT_ASSERT(ct_cube_solved(&f.cube));

    ct_db_close(db);
}

CT_SUITE(review_form_end) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    long b = seed_card(db, "rev", "b", "U R U'", "", "");
    seed_due(db, a);
    seed_due(db, b);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 2);
    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_GOOD, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_EASY, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(f.session.n_graded, 2);
    CT_ASSERT_EQ(f.session.n_good, 1);
    CT_ASSERT_EQ(f.session.n_easy, 1);

    ct_review_form_end(&f, now_stamp);
    CT_ASSERT_EQ(f.status, CT_SESSION_OK);

    char deck[CT_SESSION_DECK_MAX];
    char started[CT_SESSION_STAMP_MAX];
    char ended[CT_SESSION_STAMP_MAX];
    long counts[5];
    CT_ASSERT(read_last_session(db, deck, started, ended, counts));
    CT_ASSERT_STR_EQ(deck, "rev");
    CT_ASSERT_STR_EQ(started, now_stamp);
    CT_ASSERT_STR_EQ(ended, now_stamp);
    CT_ASSERT_EQ(counts[0], 2);
    CT_ASSERT_EQ(counts[1], 0);
    CT_ASSERT_EQ(counts[2], 0);
    CT_ASSERT_EQ(counts[3], 1);
    CT_ASSERT_EQ(counts[4], 1);

    ct_review_form_end(&f, "2026");
    CT_ASSERT(f.status < 0);

    ct_db_close(db);
}

static int session_log_count(sqlite3 *db) {
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM session_log", -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return -1;
    }
    int n = sqlite3_step(st) == SQLITE_ROW ? sqlite3_column_int(st, 0) : -1;
    sqlite3_finalize(st);
    return n;
}

CT_SUITE(review_screen_session_log) {
    sqlite3 *db = review_db();
    seed_card(db, "other", "x", "R U", "", "");

    ct_app app;
    memset(&app, 0, sizeof app);
    app.cfg = ct_config_defaults();
    app.db = db;
    app.screen = CT_SCREEN_REVIEW;
    app.fixed_dt = 1.0 / (double)app.cfg.window.target_fps;

    ct_screen_review_update(&app);
    CT_ASSERT_EQ(session_log_count(db), 1);

    ct_screen_review_update(&app);
    ct_screen_review_update(&app);
    CT_ASSERT_EQ(session_log_count(db), 1);

    char deck[CT_SESSION_DECK_MAX];
    char started[CT_SESSION_STAMP_MAX];
    char ended[CT_SESSION_STAMP_MAX];
    long counts[5];
    CT_ASSERT(read_last_session(db, deck, started, ended, counts));
    CT_ASSERT_STR_EQ(deck, CT_UI_DEFAULT_DECK);
    CT_ASSERT_EQ(counts[0], 0);
    for (int i = 0; i < 5; i++) {
        CT_ASSERT_EQ(counts[i], 0);
    }

    ct_db_close(db);
}

CT_SUITE(review_form_guards) {
    sqlite3 *db = review_db();
    long a = seed_card(db, "rev", "a", "R U R'", "", "");
    seed_due(db, a);

    CT_ASSERT(ct_review_form_begin(NULL, db, "rev", now_stamp, today_num, 0) < 0);
    CT_ASSERT(ct_review_form_load_case(NULL) < 0);
    CT_ASSERT(ct_review_form_grade(NULL, CT_GOOD, now_stamp) < 0);
    CT_ASSERT(ct_review_form_scrub(NULL, 0) < 0);
    ct_review_form_reveal(NULL);
    ct_review_form_play_pause(NULL);
    ct_review_form_step(NULL);
    ct_review_form_set_speed(NULL, 1.0);
    ct_review_form_end(NULL, now_stamp);

    ct_review_form f;
    memset(&f, 0, sizeof f);
    CT_ASSERT(ct_review_form_begin(&f, NULL, "rev", now_stamp, today_num, 0) < 0);
    CT_ASSERT(ct_review_form_begin(&f, db, "", now_stamp, today_num, 0) < 0);
    CT_ASSERT(ct_review_form_begin(&f, db, "rev", NULL, today_num, 0) < 0);
    CT_ASSERT(ct_review_form_begin(&f, db, "rev", "2026", today_num, 0) < 0);

    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 1);
    CT_ASSERT(ct_review_form_grade(&f, CT_GOOD, NULL) < 0);
    CT_ASSERT_EQ(ct_review_form_grade(&f, CT_GOOD, now_stamp), CT_SESSION_OK);
    CT_ASSERT(ct_review_form_load_case(&f) < 0);
    CT_ASSERT_EQ(f.card.id, 0);
    CT_ASSERT(ct_cube_solved(&f.cube));

    ct_review_form_end(&f, now_stamp);
    CT_ASSERT_EQ(ct_review_form_begin(&f, db, "rev", now_stamp, today_num, 0), 1);

    ct_db_close(db);
}
