#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "srs/fsrs.h"
#include "storage/db.h"
#include "storage/repo_cards.h"
#include "trainer/session.h"

static int count_rows(sqlite3 *db, const char *sql) {
    sqlite3_stmt *st = NULL;
    sqlite3_prepare_v2(db, sql, -1, &st, NULL);
    int n = sqlite3_step(st) == SQLITE_ROW ? sqlite3_column_int(st, 0) : -1;
    sqlite3_finalize(st);
    return n;
}

int main(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK || ct_db_migrate(db, "db/migrations") != 0) {
        printf("open/migrate failed\n");
        return 1;
    }
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", "d");
    snprintf(c.case_key, sizeof c.case_key, "%s", "k");
    ct_repo_card_insert(db, &c);
    ct_srs_row seed_row;
    memset(&seed_row, 0, sizeof seed_row);
    seed_row.stability = 5.0;
    seed_row.difficulty = 5.0;
    snprintf(seed_row.due, sizeof seed_row.due, "%s", "2026-01-01");
    ct_repo_srs_put(db, c.id, &seed_row);

    sqlite3_exec(db, "DROP TABLE review_log", NULL, NULL, NULL);
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    ct_session s;
    memset(&s, 0, sizeof s);
    int begin = ct_session_begin(&s, db, &f, "d", "2026-02-02T10:00:00Z", 20000, 0);
    int grade = ct_session_grade(&s, CT_GOOD, "2026-02-02T10:00:05Z");
    int srs_rows = count_rows(db, "SELECT count(*) FROM srs_state");
    sqlite3_stmt *st = NULL;
    sqlite3_prepare_v2(db, "SELECT stability FROM srs_state WHERE card_id = ?", -1, &st, NULL);
    sqlite3_bind_int64(st, 1, c.id);
    double stability = sqlite3_step(st) == SQLITE_ROW ? sqlite3_column_double(st, 0) : -1.0;
    sqlite3_finalize(st);
    printf("begin=%d grade=%d srs_rows=%d stability_unchanged=%d stability=%.3f txn_left_open=%d\n",
           begin, grade, srs_rows, stability == 5.0, stability,
           sqlite3_get_autocommit(db) == 0);

    sqlite3_exec(db,
                 "CREATE TABLE review_log(id INTEGER PRIMARY KEY, card_id INTEGER, rating INTEGER,"
                 " reviewed_at TEXT, stability REAL, difficulty REAL, elapsed_days REAL,"
                 " scheduled_days REAL)",
                 NULL, NULL, NULL);
    int grade2 = ct_session_grade(&s, CT_GOOD, "2026-02-02T10:00:05Z");
    int srs_rows2 = count_rows(db, "SELECT count(*) FROM srs_state");
    printf("grade2=%d srs_rows2=%d\n", grade2, srs_rows2);
    ct_session_free(&s);
    ct_db_close(db);
    return 0;
}
