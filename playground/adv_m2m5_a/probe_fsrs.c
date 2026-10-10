#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "srs/fsrs.h"
#include "storage/repo_cards.h"
#include "storage/repo_reviews.h"
#include "trainer/session.h"

static const char SCHEMA[] =
    "CREATE TABLE flashcard(id INTEGER PRIMARY KEY, deck TEXT NOT NULL, case_key TEXT NOT NULL,"
    " direction INTEGER NOT NULL DEFAULT 0, front_json TEXT NOT NULL DEFAULT '{}', alg_3x3 TEXT,"
    " alg_big TEXT, alg_oh TEXT, notes TEXT, created_at TEXT NOT NULL, updated_at TEXT NOT NULL,"
    " UNIQUE(deck, case_key, direction));"
    "CREATE TABLE srs_state(card_id INTEGER PRIMARY KEY, stability REAL NOT NULL,"
    " difficulty REAL NOT NULL, due TEXT NOT NULL, last_review TEXT, reps INTEGER NOT NULL"
    " DEFAULT 0, lapses INTEGER NOT NULL DEFAULT 0, state INTEGER NOT NULL DEFAULT 0);"
    "CREATE TABLE review_log(id INTEGER PRIMARY KEY, card_id INTEGER NOT NULL, rating INTEGER"
    " NOT NULL, reviewed_at TEXT NOT NULL, stability REAL NOT NULL, difficulty REAL NOT NULL,"
    " elapsed_days REAL NOT NULL, scheduled_days REAL NOT NULL);"
    "CREATE TABLE session_log(id INTEGER PRIMARY KEY, deck TEXT, started_at TEXT, ended_at TEXT,"
    " n_cards INTEGER, n_again INTEGER, n_hard INTEGER, n_good INTEGER, n_easy INTEGER);";

static int seed(sqlite3 *db, const char *difficulty_sql) {
    char sql[512];
    snprintf(sql, sizeof sql,
             "INSERT INTO flashcard(id, deck, case_key, created_at, updated_at)"
             " VALUES(1, 'd', 'k', '2026-10-10T00:00:00Z', '2026-10-10T00:00:00Z');"
             "INSERT INTO srs_state(card_id, stability, difficulty, due, last_review, reps,"
             " lapses, state) VALUES(1, 10.0, %s, '2026-10-09', '2026-10-09', 3, 0, 1);",
             difficulty_sql);
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (err != NULL) {
        printf("seed err: %s\n", err);
        sqlite3_free(err);
    }
    return rc;
}

static int run_grade(sqlite3 *db, ct_fsrs *f, const char *label) {
    ct_session s;
    memset(&s, 0, sizeof s);
    ct_config cfg = ct_config_defaults();
    ct_fsrs_init(f, &cfg);
    int n = ct_session_begin(&s, db, f, "d", "2026-10-10T12:00:00Z", 20636, 10);
    printf("%s begin=%d\n", label, n);
    if (n <= 0) {
        return -1;
    }
    int rc = ct_session_grade(&s, CT_GOOD, "2026-10-10T12:00:05Z");
    printf("%s grade rc=%d\n", label, rc);
    ct_session_end(&s, "2026-10-10T12:01:00Z");
    ct_session_free(&s);
    return rc;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    sqlite3 *db = NULL;
    remove("build/adv_a_fsrs.db");
    if (sqlite3_open("build/adv_a_fsrs.db", &db) != SQLITE_OK) {
        return 1;
    }
    sqlite3_exec(db, SCHEMA, NULL, NULL, NULL);

    seed(db, "5.0");
    ct_fsrs f;
    run_grade(db, &f, "clean");

    sqlite3_exec(db, "DELETE FROM review_log; DELETE FROM srs_state; DELETE FROM flashcard;"
                     " DELETE FROM session_log;", NULL, NULL, NULL);
    seed(db, "9e999");
    ct_srs_row row;
    int grc = ct_repo_srs_get(db, 1, &row);
    printf("inf read rc=%d difficulty=%f isinf=%d\n", grc, row.difficulty,
           row.difficulty > 1e308);
    run_grade(db, &f, "inf-difficulty");

    sqlite3_exec(db, "DELETE FROM review_log; DELETE FROM srs_state; DELETE FROM flashcard;"
                     " DELETE FROM session_log;", NULL, NULL, NULL);
    seed(db, "1e308");
    run_grade(db, &f, "huge-difficulty");

    sqlite3_exec(db, "DELETE FROM review_log; DELETE FROM srs_state; DELETE FROM flashcard;"
                     " DELETE FROM session_log;", NULL, NULL, NULL);
    seed(db, "5.0");
    sqlite3_exec(db, "UPDATE srs_state SET stability = 9e999 WHERE card_id = 1", NULL, NULL,
                 NULL);
    run_grade(db, &f, "inf-stability");

    sqlite3_exec(db, "DELETE FROM review_log; DELETE FROM srs_state; DELETE FROM flashcard;"
                     " DELETE FROM session_log;", NULL, NULL, NULL);
    seed(db, "5.0");
    sqlite3_exec(db, "UPDATE srs_state SET last_review = '9999-12-31' WHERE card_id = 1",
                 NULL, NULL, NULL);
    run_grade(db, &f, "far-last-review");

    ct_fsrs_card direct = {0};
    direct.stability = 10.0;
    direct.difficulty = 5.0;
    direct.state = 1;
    direct.last_review_day = 20000;
    ct_fsrs_card out = ct_fsrs_review(&f, &direct, CT_GOOD, 630.0, 20636);
    printf("direct elapsed=630 due=%ld stability=%f\n", out.due_day, out.stability);

    sqlite3_close(db);
    return 0;
}
