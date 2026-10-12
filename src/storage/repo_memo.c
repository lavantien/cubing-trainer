#include "storage/db.h"

#include <string.h>

#include "storage/repo_cards.h"
#include "storage/repo_settings.h"

static const char CT_SQL_MEMO_GET[] =
    "SELECT phoneme, image FROM memo_pair WHERE orbit = ? AND pair = ?";
static const char CT_SQL_MEMO_LIST[] =
    "SELECT pair, phoneme, image FROM memo_pair WHERE orbit = ? ORDER BY pair";
static const char CT_SQL_MEMO_SET[] =
    "INSERT INTO memo_pair(orbit, pair, phoneme, image) VALUES(?, ?, ?, ?)"
    " ON CONFLICT(orbit, pair) DO UPDATE SET"
    " phoneme = excluded.phoneme, image = excluded.image, updated_at = datetime('now')";

static int ct_memo_orbit_ok(int orbit) {
    return orbit == CT_ORBIT_CORNER || orbit == CT_ORBIT_EDGE;
}

static int ct_memo_pair_ok(const char *pair) {
    if (pair == NULL)
        return 0;
    if (pair[0] < CT_LETTER_FIRST || pair[0] > CT_LETTER_LAST)
        return 0;
    if (pair[1] < CT_LETTER_FIRST || pair[1] > CT_LETTER_LAST)
        return 0;
    return pair[2] == '\0';
}

static void ct_memo_read_text(sqlite3_stmt *st, int col, char *dst, size_t cap) {
    const unsigned char *s = sqlite3_column_text(st, col);
    if (s == NULL) {
        dst[0] = '\0';
        return;
    }
    size_t n = strlen((const char *)s);
    if (n >= cap)
        n = cap - 1;
    memcpy(dst, s, n);
    dst[n] = '\0';
}

int ct_memo_get(sqlite3 *db, int orbit, const char *pair, ct_memo_row *out) {
    if (db == NULL || out == NULL)
        return CT_REPO_ERR_ARG;
    if (!ct_memo_orbit_ok(orbit) || !ct_memo_pair_ok(pair))
        return CT_REPO_ERR_ARG;
    memset(out, 0, sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_MEMO_GET, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int(st, 1, orbit);
    sqlite3_bind_text(st, 2, pair, -1, SQLITE_STATIC);
    int step = sqlite3_step(st);
    int ret = CT_REPO_ERR_DB;
    if (step == SQLITE_ROW) {
        ct_memo_read_text(st, 0, out->phoneme, sizeof out->phoneme);
        ct_memo_read_text(st, 1, out->image, sizeof out->image);
        ret = CT_REPO_OK;
    } else if (step == SQLITE_DONE) {
        ret = CT_REPO_MISSING;
    }
    sqlite3_finalize(st);
    return ret;
}

int ct_memo_list(sqlite3 *db, int orbit, ct_memo_list_row *rows, int cap) {
    if (db == NULL || cap < 0 || (rows == NULL && cap > 0)) {
        return CT_REPO_ERR_ARG;
    }
    if (!ct_memo_orbit_ok(orbit)) {
        return CT_REPO_ERR_ARG;
    }
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_MEMO_LIST, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int(st, 1, orbit);
    int count = 0;
    while (count < cap && sqlite3_step(st) == SQLITE_ROW) {
        ct_memo_read_text(st, 0, rows[count].pair, sizeof rows[count].pair);
        ct_memo_read_text(st, 1, rows[count].phoneme, sizeof rows[count].phoneme);
        ct_memo_read_text(st, 2, rows[count].image, sizeof rows[count].image);
        count++;
    }
    sqlite3_finalize(st);
    return count;
}

int ct_memo_set(sqlite3 *db, int orbit, const char *pair, const char *phoneme, const char *image) {
    if (db == NULL)
        return CT_REPO_ERR_ARG;
    if (!ct_memo_orbit_ok(orbit) || !ct_memo_pair_ok(pair))
        return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(phoneme, CT_MEMO_TEXT_MAX))
        return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(image, CT_MEMO_TEXT_MAX))
        return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_MEMO_SET, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int(st, 1, orbit);
    sqlite3_bind_text(st, 2, pair, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 3, phoneme, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 4, image, -1, SQLITE_STATIC);
    int step = sqlite3_step(st);
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_REPO_OK : CT_REPO_ERR_DB;
}
