#include "repo_cards.h"

#include <time.h>

static const char CT_SQL_CARD_INSERT[] =
    "INSERT INTO flashcard(deck, case_key, direction, front_json, alg_3x3, alg_big, alg_oh, notes, created_at, updated_at)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
static const char CT_SQL_CARD_UPDATE[] =
    "UPDATE flashcard SET deck = ?, case_key = ?, direction = ?, front_json = ?, alg_3x3 = ?, alg_big = ?, alg_oh = ?,"
    " notes = ?, updated_at = ? WHERE id = ?";
static const char CT_SQL_CARD_GET[] =
    "SELECT id, deck, case_key, direction, front_json, alg_3x3, alg_big, alg_oh, notes, created_at, updated_at"
    " FROM flashcard WHERE id = ?";
static const char CT_SQL_CARD_GET_BY_KEY[] =
    "SELECT id, deck, case_key, direction, front_json, alg_3x3, alg_big, alg_oh, notes, created_at, updated_at"
    " FROM flashcard WHERE deck = ? AND case_key = ? AND direction = ?";
static const char CT_SQL_CARD_DELETE[] =
    "DELETE FROM flashcard WHERE id = ?";
static const char CT_SQL_CARD_LIST[] =
    "SELECT id, deck, case_key, direction, front_json, alg_3x3, alg_big, alg_oh, notes, created_at, updated_at"
    " FROM flashcard WHERE deck = ? ORDER BY id";
static const char CT_SQL_SRS_GET[] =
    "SELECT stability, difficulty, due, last_review, reps, lapses, state FROM srs_state WHERE card_id = ?";
static const char CT_SQL_SRS_PUT[] =
    "INSERT INTO srs_state(card_id, stability, difficulty, due, last_review, reps, lapses, state)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
    " ON CONFLICT(card_id) DO UPDATE SET"
    " stability = excluded.stability, difficulty = excluded.difficulty, due = excluded.due,"
    " last_review = excluded.last_review, reps = excluded.reps, lapses = excluded.lapses, state = excluded.state";

typedef struct ct_bind {
    sqlite3_stmt *st;
    int next;
} ct_bind;

typedef struct ct_read {
    sqlite3_stmt *st;
    int col;
} ct_read;

static void ct_bind_open(ct_bind *b, sqlite3_stmt *st) {
    b->st = st;
    b->next = 0;
}

static void ct_bind_text(ct_bind *b, const char *s) {
    sqlite3_bind_text(b->st, ++b->next, s, -1, SQLITE_STATIC);
}

static void ct_bind_int(ct_bind *b, int v) {
    sqlite3_bind_int(b->st, ++b->next, v);
}

static void ct_bind_int64(ct_bind *b, sqlite3_int64 v) {
    sqlite3_bind_int64(b->st, ++b->next, v);
}

static void ct_bind_double(ct_bind *b, double v) {
    sqlite3_bind_double(b->st, ++b->next, v);
}

static void ct_read_open(ct_read *r, sqlite3_stmt *st) {
    r->st = st;
    r->col = 0;
}

static long ct_read_long(ct_read *r) {
    return (long)sqlite3_column_int64(r->st, r->col++);
}

static int ct_read_int(ct_read *r) {
    return sqlite3_column_int(r->st, r->col++);
}

static double ct_read_double(ct_read *r) {
    return sqlite3_column_double(r->st, r->col++);
}

static void ct_read_text(ct_read *r, char *dst, size_t cap) {
    const unsigned char *s = sqlite3_column_text(r->st, r->col++);
    if (s == NULL) {
        dst[0] = '\0';
        return;
    }
    size_t n = strlen((const char *)s);
    if (n >= cap) n = cap - 1;
    memcpy(dst, s, n);
    dst[n] = '\0';
}

void ct_now_utc_iso(char *out, unsigned cap) {
    if (out == NULL) return;
    out[0] = '\0';
    if (cap < sizeof "0000-00-00T00:00:00Z") return;
    time_t now = time(NULL);
    const struct tm *utc = gmtime(&now);
    if (utc == NULL) return;
    strftime(out, (size_t)cap, "%Y-%m-%dT%H:%M:%SZ", utc);
}

static int ct_card_valid(const ct_card *c) {
    if (c == NULL) return CT_REPO_ERR_ARG;
    if (c->deck[0] == '\0' || c->case_key[0] == '\0') return CT_REPO_ERR_ARG;
    return CT_REPO_OK;
}

static void ct_card_from_row(sqlite3_stmt *st, ct_card *c) {
    ct_read r;
    ct_read_open(&r, st);
    c->id = ct_read_long(&r);
    ct_read_text(&r, c->deck, sizeof c->deck);
    ct_read_text(&r, c->case_key, sizeof c->case_key);
    c->direction = ct_read_int(&r);
    ct_read_text(&r, c->front_json, sizeof c->front_json);
    ct_read_text(&r, c->alg_3x3, sizeof c->alg_3x3);
    ct_read_text(&r, c->alg_big, sizeof c->alg_big);
    ct_read_text(&r, c->alg_oh, sizeof c->alg_oh);
    ct_read_text(&r, c->notes, sizeof c->notes);
    ct_read_text(&r, c->created_at, sizeof c->created_at);
    ct_read_text(&r, c->updated_at, sizeof c->updated_at);
}

int ct_repo_card_insert(sqlite3 *db, ct_card *c) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    int rc = ct_card_valid(c);
    if (rc != CT_REPO_OK) return rc;
    char now[CT_CARD_STAMP_MAX];
    ct_now_utc_iso(now, (unsigned)sizeof now);
    if (now[0] == '\0') return CT_REPO_ERR_DB;
    memcpy(c->created_at, now, sizeof now);
    memcpy(c->updated_at, now, sizeof now);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CARD_INSERT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    ct_bind b;
    ct_bind_open(&b, st);
    ct_bind_text(&b, c->deck);
    ct_bind_text(&b, c->case_key);
    ct_bind_int(&b, c->direction);
    ct_bind_text(&b, c->front_json);
    ct_bind_text(&b, c->alg_3x3);
    ct_bind_text(&b, c->alg_big);
    ct_bind_text(&b, c->alg_oh);
    ct_bind_text(&b, c->notes);
    ct_bind_text(&b, c->created_at);
    ct_bind_text(&b, c->updated_at);
    int step = sqlite3_step(st);
    int ext = step == SQLITE_DONE ? SQLITE_OK : sqlite3_extended_errcode(db);
    sqlite3_finalize(st);
    if (step == SQLITE_DONE) {
        c->id = (long)sqlite3_last_insert_rowid(db);
        return CT_REPO_OK;
    }
    if (ext == SQLITE_CONSTRAINT_UNIQUE) return CT_REPO_CONFLICT;
    return CT_REPO_ERR_DB;
}

int ct_repo_card_update(sqlite3 *db, const ct_card *c) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    int rc = ct_card_valid(c);
    if (rc != CT_REPO_OK) return rc;
    if (c->id <= 0) return CT_REPO_ERR_ARG;
    char now[CT_CARD_STAMP_MAX];
    ct_now_utc_iso(now, (unsigned)sizeof now);
    if (now[0] == '\0') return CT_REPO_ERR_DB;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CARD_UPDATE, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    ct_bind b;
    ct_bind_open(&b, st);
    ct_bind_text(&b, c->deck);
    ct_bind_text(&b, c->case_key);
    ct_bind_int(&b, c->direction);
    ct_bind_text(&b, c->front_json);
    ct_bind_text(&b, c->alg_3x3);
    ct_bind_text(&b, c->alg_big);
    ct_bind_text(&b, c->alg_oh);
    ct_bind_text(&b, c->notes);
    ct_bind_text(&b, now);
    ct_bind_int64(&b, c->id);
    int step = sqlite3_step(st);
    int ext = step == SQLITE_DONE ? SQLITE_OK : sqlite3_extended_errcode(db);
    int changed = sqlite3_changes(db);
    sqlite3_finalize(st);
    if (step == SQLITE_DONE) {
        return changed == 0 ? CT_REPO_MISSING : CT_REPO_OK;
    }
    if (ext == SQLITE_CONSTRAINT_UNIQUE) return CT_REPO_CONFLICT;
    return CT_REPO_ERR_DB;
}

int ct_repo_card_get(sqlite3 *db, long id, ct_card *out) {
    if (db == NULL || out == NULL || id <= 0) return CT_REPO_ERR_ARG;
    memset(out, 0, sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CARD_GET, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int64(st, 1, id);
    int step = sqlite3_step(st);
    if (step == SQLITE_ROW) {
        ct_card_from_row(st, out);
        sqlite3_finalize(st);
        return CT_REPO_OK;
    }
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_REPO_MISSING : CT_REPO_ERR_DB;
}

int ct_repo_card_get_by_key(sqlite3 *db, const char *deck, const char *case_key, int direction, ct_card *out) {
    if (db == NULL || out == NULL) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(deck, CT_CARD_DECK_MAX)) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(case_key, CT_CARD_CASE_KEY_MAX)) return CT_REPO_ERR_ARG;
    memset(out, 0, sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CARD_GET_BY_KEY, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    ct_bind b;
    ct_bind_open(&b, st);
    ct_bind_text(&b, deck);
    ct_bind_text(&b, case_key);
    ct_bind_int(&b, direction);
    int step = sqlite3_step(st);
    if (step == SQLITE_ROW) {
        ct_card_from_row(st, out);
        sqlite3_finalize(st);
        return CT_REPO_OK;
    }
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_REPO_MISSING : CT_REPO_ERR_DB;
}

int ct_repo_card_delete(sqlite3 *db, long id) {
    if (db == NULL || id <= 0) return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CARD_DELETE, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int64(st, 1, id);
    int step = sqlite3_step(st);
    int changed = sqlite3_changes(db);
    sqlite3_finalize(st);
    if (step == SQLITE_DONE) {
        return changed == 0 ? CT_REPO_MISSING : CT_REPO_OK;
    }
    return CT_REPO_ERR_DB;
}

int ct_repo_card_list(sqlite3 *db, const char *deck, ct_card *out, int cap) {
    if (db == NULL) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(deck, CT_CARD_DECK_MAX)) return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL)) return CT_REPO_ERR_ARG;
    if (cap == 0) return CT_REPO_OK;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CARD_LIST, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_text(st, 1, deck, -1, SQLITE_STATIC);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        ct_card_from_row(st, &out[n]);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return CT_REPO_ERR_DB;
    return n;
}

int ct_repo_srs_get(sqlite3 *db, long card_id, ct_srs_row *out) {
    if (db == NULL || out == NULL || card_id <= 0) return CT_REPO_ERR_ARG;
    memset(out, 0, sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_SRS_GET, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_int64(st, 1, card_id);
    int step = sqlite3_step(st);
    if (step == SQLITE_ROW) {
        ct_read r;
        ct_read_open(&r, st);
        out->stability = ct_read_double(&r);
        out->difficulty = ct_read_double(&r);
        ct_read_text(&r, out->due, sizeof out->due);
        ct_read_text(&r, out->last_review, sizeof out->last_review);
        out->reps = ct_read_int(&r);
        out->lapses = ct_read_int(&r);
        out->state = ct_read_int(&r);
        sqlite3_finalize(st);
        return CT_REPO_OK;
    }
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_REPO_MISSING : CT_REPO_ERR_DB;
}

int ct_repo_srs_put(sqlite3 *db, long card_id, const ct_srs_row *row) {
    if (db == NULL || row == NULL || card_id <= 0) return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(row->due, CT_CARD_STAMP_MAX)) return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_SRS_PUT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    ct_bind b;
    ct_bind_open(&b, st);
    ct_bind_int64(&b, card_id);
    ct_bind_double(&b, row->stability);
    ct_bind_double(&b, row->difficulty);
    ct_bind_text(&b, row->due);
    ct_bind_text(&b, row->last_review);
    ct_bind_int(&b, row->reps);
    ct_bind_int(&b, row->lapses);
    ct_bind_int(&b, row->state);
    int step = sqlite3_step(st);
    int ext = step == SQLITE_DONE ? SQLITE_OK : sqlite3_extended_errcode(db);
    sqlite3_finalize(st);
    if (step == SQLITE_DONE) return CT_REPO_OK;
    if (ext == SQLITE_CONSTRAINT_UNIQUE) return CT_REPO_CONFLICT;
    if (ext == SQLITE_CONSTRAINT_FOREIGNKEY) return CT_REPO_MISSING;
    return CT_REPO_ERR_DB;
}
