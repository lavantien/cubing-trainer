#include "repo_reviews.h"

#include "config/config.h"

#include <stdio.h>

static const char CT_SQL_REVIEW_INSERT[] =
    "INSERT INTO review_log(card_id, rating, reviewed_at, stability, difficulty, elapsed_days, scheduled_days)"
    " VALUES (?, ?, ?, ?, ?, ?, ?)";
static const char CT_SQL_REVIEWS_FOR_CARD[] =
    "SELECT id, card_id, rating, reviewed_at, stability, difficulty, elapsed_days, scheduled_days"
    " FROM review_log WHERE card_id = ? ORDER BY id";
static const char CT_SQL_DUE_CARDS[] =
    "SELECT s.card_id FROM srs_state AS s JOIN flashcard AS f ON f.id = s.card_id"
    " WHERE f.deck = ? AND s.due <= ? ORDER BY s.due, s.card_id LIMIT ?";

int ct_repo_review_log(sqlite3 *db, long card_id, int rating, const char *reviewed_at,
                       double stability, double difficulty, double elapsed_days, double scheduled_days) {
    if (db == NULL || card_id <= 0) return CT_REPO_ERR_ARG;
    if (rating < CT_AGAIN || rating > CT_EASY) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(reviewed_at, CT_CARD_STAMP_MAX)) return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_REVIEW_INSERT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    int ix = 0;
    sqlite3_bind_int64(st, ++ix, card_id);
    sqlite3_bind_int(st, ++ix, rating);
    sqlite3_bind_text(st, ++ix, reviewed_at, -1, SQLITE_STATIC);
    sqlite3_bind_double(st, ++ix, stability);
    sqlite3_bind_double(st, ++ix, difficulty);
    sqlite3_bind_double(st, ++ix, elapsed_days);
    sqlite3_bind_double(st, ++ix, scheduled_days);
    int step = sqlite3_step(st);
    int ext = step == SQLITE_DONE ? SQLITE_OK : sqlite3_extended_errcode(db);
    sqlite3_finalize(st);
    if (step == SQLITE_DONE) return CT_REPO_OK;
    if (ext == SQLITE_CONSTRAINT_UNIQUE) return CT_REPO_CONFLICT;
    if (ext == SQLITE_CONSTRAINT_FOREIGNKEY) return CT_REPO_MISSING;
    return CT_REPO_ERR_DB;
}

int ct_repo_reviews_for_card(sqlite3 *db, long card_id, ct_review_row *out, int cap) {
    if (db == NULL || card_id <= 0) return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL)) return CT_REPO_ERR_ARG;
    if (cap == 0) return CT_REPO_OK;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_REVIEWS_FOR_CARD, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int64(st, 1, card_id);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        int col = 0;
        out[n].id = (long)sqlite3_column_int64(st, col++);
        out[n].card_id = (long)sqlite3_column_int64(st, col++);
        out[n].rating = sqlite3_column_int(st, col++);
        const unsigned char *at = sqlite3_column_text(st, col++);
        snprintf(out[n].reviewed_at, sizeof out[n].reviewed_at, "%s", at ? (const char *)at : "");
        out[n].stability = sqlite3_column_double(st, col++);
        out[n].difficulty = sqlite3_column_double(st, col++);
        out[n].elapsed_days = sqlite3_column_double(st, col++);
        out[n].scheduled_days = sqlite3_column_double(st, col++);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return CT_REPO_ERR_DB;
    return n;
}

int ct_repo_due_cards(sqlite3 *db, const char *deck, const char *now_iso, long *ids, int cap) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(deck, CT_CARD_DECK_MAX)) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(now_iso, CT_CARD_STAMP_MAX)) return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && ids == NULL)) return CT_REPO_ERR_ARG;
    if (cap == 0) return CT_REPO_OK;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_DUE_CARDS, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    int ix = 0;
    sqlite3_bind_text(st, ++ix, deck, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, ++ix, now_iso, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, ++ix, cap);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        ids[n] = (long)sqlite3_column_int64(st, 0);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return CT_REPO_ERR_DB;
    return n;
}
