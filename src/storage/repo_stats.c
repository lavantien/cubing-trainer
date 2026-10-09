#include "repo_stats.h"

#include <stdio.h>

#include "config/config.h"

static const char CT_SQL_DECK_STATS[] =
    "SELECT f.deck, COUNT(*), COALESCE(SUM(s.due <= ?), 0), COALESCE(SUM(s.state = 1), 0),"
    " COALESCE(SUM(s.state >= 2), 0), COALESCE(AVG(s.stability), 0), COALESCE(AVG(s.difficulty), 0)"
    " FROM flashcard AS f LEFT JOIN srs_state AS s ON s.card_id = f.id"
    " GROUP BY f.deck ORDER BY f.deck";
static const char CT_SQL_DUE_FORECAST[] =
    "WITH RECURSIVE seq(d, n) AS ("
    "SELECT date(?), 0 UNION ALL SELECT date(d, '+1 day'), n + 1 FROM seq WHERE n + 1 < ?),"
    " per_day AS (SELECT substr(due, 1, 10) AS day, COUNT(*) AS c FROM srs_state"
    " GROUP BY substr(due, 1, 10))"
    " SELECT seq.d, COALESCE(per_day.c, 0) FROM seq LEFT JOIN per_day ON per_day.day = seq.d";
static const char CT_SQL_CASE_ACCURACY[] =
    "SELECT case_key, direction, reviews, correct FROM ("
    "SELECT f.case_key AS case_key, f.direction AS direction, COUNT(*) AS reviews,"
    " COALESCE(SUM(r.rating > ?), 0) AS correct"
    " FROM review_log AS r JOIN flashcard AS f ON f.id = r.card_id"
    " WHERE f.deck = ? GROUP BY f.case_key, f.direction)"
    " ORDER BY correct * 1.0 / reviews, case_key, direction";
static const char CT_SQL_SESSION_INSERT[] =
    "INSERT INTO session_log(deck, started_at, ended_at, n_cards, n_again, n_hard, n_good, n_easy)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
static const char CT_SQL_SESSIONS[] =
    "SELECT id, deck, started_at, ended_at, n_cards, n_again, n_hard, n_good, n_easy"
    " FROM session_log ORDER BY started_at DESC, id DESC";

static void ct_stats_text(char *dst, size_t cap, const unsigned char *src) {
    snprintf(dst, cap, "%s", src ? (const char *)src : "");
}

int ct_repo_deck_stats(sqlite3 *db, const char *now_iso, ct_deck_stats *out, int cap) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(now_iso, CT_STATS_STAMP_MAX)) return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL)) return CT_REPO_ERR_ARG;
    if (cap == 0) return CT_REPO_OK;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_DECK_STATS, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_text(st, 1, now_iso, -1, SQLITE_STATIC);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        int col = 0;
        ct_stats_text(out[n].deck, sizeof out[n].deck, sqlite3_column_text(st, col++));
        out[n].total_cards = (long)sqlite3_column_int64(st, col++);
        out[n].due_now = (long)sqlite3_column_int64(st, col++);
        out[n].learning = (long)sqlite3_column_int64(st, col++);
        out[n].review_state = (long)sqlite3_column_int64(st, col++);
        out[n].avg_stability = sqlite3_column_double(st, col++);
        out[n].avg_difficulty = sqlite3_column_double(st, col++);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return CT_REPO_ERR_DB;
    return n;
}

int ct_repo_due_forecast(sqlite3 *db, const char *from_day, int days, ct_due_forecast_row *out,
                         int cap) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(from_day, CT_STATS_DAY_MAX)) return CT_REPO_ERR_ARG;
    if (days < 0) return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL)) return CT_REPO_ERR_ARG;
    if (days == 0) return 0;
    if (cap == 0) return days;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_DUE_FORECAST, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_text(st, 1, from_day, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, 2, days);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        ct_stats_text(out[n].day, sizeof out[n].day, sqlite3_column_text(st, 0));
        out[n].due_count = (long)sqlite3_column_int64(st, 1);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return CT_REPO_ERR_DB;
    return n;
}

int ct_repo_case_accuracy(sqlite3 *db, const char *deck, ct_case_accuracy_row *out, int cap) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(deck, CT_STATS_DECK_MAX)) return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL)) return CT_REPO_ERR_ARG;
    if (cap == 0) return CT_REPO_OK;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CASE_ACCURACY, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int(st, 1, CT_AGAIN);
    sqlite3_bind_text(st, 2, deck, -1, SQLITE_STATIC);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        int col = 0;
        ct_stats_text(out[n].case_key, sizeof out[n].case_key, sqlite3_column_text(st, col++));
        out[n].direction = sqlite3_column_int(st, col++);
        out[n].reviews = (long)sqlite3_column_int64(st, col++);
        out[n].correct = (long)sqlite3_column_int64(st, col++);
        out[n].accuracy = (double)out[n].correct / (double)out[n].reviews;
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return CT_REPO_ERR_DB;
    return n;
}

int ct_repo_session_log(sqlite3 *db, const ct_session_row *row) {
    if (db == NULL || row == NULL) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(row->deck, CT_STATS_DECK_MAX)) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(row->started_at, CT_STATS_STAMP_MAX)) return CT_REPO_ERR_ARG;
    if (strlen(row->ended_at) >= CT_STATS_STAMP_MAX) return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_SESSION_INSERT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    int ix = 0;
    sqlite3_bind_text(st, ++ix, row->deck, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, ++ix, row->started_at, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, ++ix, row->ended_at, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, ++ix, row->n_cards);
    sqlite3_bind_int(st, ++ix, row->n_again);
    sqlite3_bind_int(st, ++ix, row->n_hard);
    sqlite3_bind_int(st, ++ix, row->n_good);
    sqlite3_bind_int(st, ++ix, row->n_easy);
    int step = sqlite3_step(st);
    sqlite3_finalize(st);
    if (step == SQLITE_DONE) return CT_REPO_OK;
    return CT_REPO_ERR_DB;
}

int ct_repo_sessions(sqlite3 *db, ct_session_row *out, int cap) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL)) return CT_REPO_ERR_ARG;
    if (cap == 0) return CT_REPO_OK;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_SESSIONS, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        int col = 0;
        out[n].id = (long)sqlite3_column_int64(st, col++);
        ct_stats_text(out[n].deck, sizeof out[n].deck, sqlite3_column_text(st, col++));
        ct_stats_text(out[n].started_at, sizeof out[n].started_at, sqlite3_column_text(st, col++));
        ct_stats_text(out[n].ended_at, sizeof out[n].ended_at, sqlite3_column_text(st, col++));
        out[n].n_cards = sqlite3_column_int(st, col++);
        out[n].n_again = sqlite3_column_int(st, col++);
        out[n].n_hard = sqlite3_column_int(st, col++);
        out[n].n_good = sqlite3_column_int(st, col++);
        out[n].n_easy = sqlite3_column_int(st, col++);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return CT_REPO_ERR_DB;
    return n;
}
