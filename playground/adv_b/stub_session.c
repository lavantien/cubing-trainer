#include "trainer/session.c"

#include <stdio.h>

int ct_repo_due_cards(sqlite3 *db, const char *deck, const char *now_iso, long *ids, int cap) {
    (void)db;
    (void)deck;
    (void)now_iso;
    (void)ids;
    (void)cap;
    return 0;
}

int ct_repo_card_get(sqlite3 *db, long id, ct_card *out) {
    (void)db;
    (void)id;
    (void)out;
    return CT_REPO_MISSING;
}

int ct_repo_srs_get(sqlite3 *db, long card_id, ct_srs_row *out) {
    (void)db;
    (void)card_id;
    (void)out;
    return CT_REPO_MISSING;
}

int ct_repo_srs_put(sqlite3 *db, long card_id, const ct_srs_row *row) {
    (void)db;
    (void)card_id;
    (void)row;
    return CT_REPO_OK;
}

int ct_repo_review_log(sqlite3 *db, long card_id, int rating, const char *reviewed_at,
                       double stability, double difficulty, double elapsed_days,
                       double scheduled_days) {
    (void)db;
    (void)card_id;
    (void)rating;
    (void)reviewed_at;
    (void)stability;
    (void)difficulty;
    (void)elapsed_days;
    (void)scheduled_days;
    return CT_REPO_OK;
}

int sqlite3_prepare_v2(sqlite3 *db, const char *sql, int n, sqlite3_stmt **out, const char **tail) {
    (void)db;
    (void)sql;
    (void)n;
    (void)out;
    (void)tail;
    return 1;
}

int sqlite3_bind_text(sqlite3_stmt *st, int i, const char *v, int n, void (*d)(void *)) {
    (void)st;
    (void)i;
    (void)v;
    (void)n;
    (void)d;
    return 1;
}

int sqlite3_bind_int(sqlite3_stmt *st, int i, int v) {
    (void)st;
    (void)i;
    (void)v;
    return 1;
}

int sqlite3_step(sqlite3_stmt *st) {
    (void)st;
    return 101;
}

int sqlite3_finalize(sqlite3_stmt *st) {
    (void)st;
    return 0;
}
