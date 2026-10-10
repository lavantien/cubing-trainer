#include "storage/repo_algs.h"

#include <stdarg.h>
#include <stdio.h>

#include "packs/provenance.h"

static const char CT_SQL_ALG_PUT[] =
    "INSERT INTO generated_alg(orbit, target_pair, buffer_mode, category, sequence, classification,"
    " setup, interchange, insertion, qtm, score, score_parts_json, params_hash, created_at)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    " ON CONFLICT(orbit, category, target_pair, buffer_mode, params_hash) DO UPDATE SET"
    " sequence = excluded.sequence, classification = excluded.classification, setup = "
    "excluded.setup,"
    " interchange = excluded.interchange, insertion = excluded.insertion, qtm = excluded.qtm,"
    " score = excluded.score, score_parts_json = excluded.score_parts_json";
static const char CT_SQL_ALG_GET[] =
    "SELECT id, orbit, target_pair, buffer_mode, category, sequence, classification, setup,"
    " interchange, insertion, qtm, score, score_parts_json, created_at"
    " FROM generated_alg WHERE orbit = ? AND target_pair = ? AND buffer_mode = ? AND category = ?"
    " AND params_hash = ?";
static const char CT_SQL_ALG_COUNT[] = "SELECT COUNT(*) FROM generated_alg WHERE params_hash = ?";
static const char CT_SQL_ALG_LIST[] =
    "SELECT id, orbit, target_pair, buffer_mode, category, sequence, classification, setup,"
    " interchange, insertion, qtm, score, score_parts_json, created_at"
    " FROM generated_alg WHERE params_hash = ? AND orbit = ? ORDER BY target_pair";
static const char CT_SQL_ALG_STALE[] =
    "SELECT id FROM generated_alg WHERE params_hash <> ? ORDER BY id";
static const char CT_SQL_ALG_DROP[] = "DELETE FROM generated_alg WHERE params_hash = ?";
static const char CT_SQL_ALG_DROP_ORBIT[] =
    "DELETE FROM generated_alg WHERE params_hash = ? AND orbit = ?";

typedef struct ct_alg_bind {
    sqlite3_stmt *st;
    int next;
} ct_alg_bind;

typedef struct ct_alg_read {
    sqlite3_stmt *st;
    int col;
} ct_alg_read;

static void ct_alg_bind_open(ct_alg_bind *b, sqlite3_stmt *st) {
    b->st = st;
    b->next = 0;
}

static void ct_alg_bind_text(ct_alg_bind *b, const char *s) {
    sqlite3_bind_text(b->st, ++b->next, s, -1, SQLITE_STATIC);
}

static void ct_alg_bind_int(ct_alg_bind *b, int v) {
    sqlite3_bind_int(b->st, ++b->next, v);
}

static void ct_alg_bind_double(ct_alg_bind *b, double v) {
    sqlite3_bind_double(b->st, ++b->next, v);
}

static void ct_alg_read_open(ct_alg_read *r, sqlite3_stmt *st) {
    r->st = st;
    r->col = 0;
}

static long ct_alg_read_long(ct_alg_read *r) {
    return (long)sqlite3_column_int64(r->st, r->col++);
}

static int ct_alg_read_int(ct_alg_read *r) {
    return sqlite3_column_int(r->st, r->col++);
}

static double ct_alg_read_double(ct_alg_read *r) {
    return sqlite3_column_double(r->st, r->col++);
}

static void ct_alg_read_text(ct_alg_read *r, char *dst, size_t cap) {
    const unsigned char *s = sqlite3_column_text(r->st, r->col++);
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

static void ct_alg_from_row(sqlite3_stmt *st, ct_generated_alg *a) {
    ct_alg_read r;
    ct_alg_read_open(&r, st);
    a->id = ct_alg_read_long(&r);
    a->orbit = ct_alg_read_int(&r);
    ct_alg_read_text(&r, a->target_pair, sizeof a->target_pair);
    a->buffer_mode = ct_alg_read_int(&r);
    a->category = ct_alg_read_int(&r);
    ct_alg_read_text(&r, a->sequence, sizeof a->sequence);
    a->classification = ct_alg_read_int(&r);
    ct_alg_read_text(&r, a->setup, sizeof a->setup);
    ct_alg_read_text(&r, a->interchange, sizeof a->interchange);
    ct_alg_read_text(&r, a->insertion, sizeof a->insertion);
    a->qtm = ct_alg_read_int(&r);
    a->score = ct_alg_read_double(&r);
    ct_alg_read_text(&r, a->score_parts_json, sizeof a->score_parts_json);
    ct_alg_read_text(&r, a->created_at, sizeof a->created_at);
}

static int ct_alg_emit(char *out, size_t cap, size_t *used, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out + *used, cap - *used, fmt, ap);
    va_end(ap);
    if (n < 0) {
        return CT_REPO_ERR_ARG;
    }
    *used += (size_t)n;
    if (*used >= cap) {
        return CT_REPO_ERR_ARG;
    }
    return CT_REPO_OK;
}

static int ct_alg_map_segment(const char *letters, int count, char *out, size_t cap, size_t *used) {
    for (int i = 0; i < count; i++) {
        char c = letters[i];
        if (c < CT_LETTER_FIRST || c > CT_LETTER_LAST)
            c = '.';
        if (ct_alg_emit(out, cap, used, "%c", c) != CT_REPO_OK) {
            return CT_REPO_ERR_ARG;
        }
    }
    return CT_REPO_OK;
}

static void ct_alg_canonical(const ct_config *cfg, int scheme_id, ct_buffer_mode mode,
                             int buffer_corner, int buffer_edge, const ct_letter_map *map,
                             char *out, size_t cap) {
    size_t used = 0;
    int ok = ct_alg_emit(out, cap, &used, "%d|%d|%d|%d|%x|", scheme_id, (int)mode, buffer_corner,
                         buffer_edge, cfg->gen.preferred_moves);
    for (int i = 0; ok == CT_REPO_OK && i < CT_FACE_COUNT; i++) {
        ok = ct_alg_emit(out, cap, &used, i == 0 ? "%.6f" : ",%.6f", cfg->gen.penalty[i]);
    }
    if (ok == CT_REPO_OK) {
        ok = ct_alg_emit(out, cap, &used, "|");
    }
    int finger_total = CT_FINGER_FACE_COUNT * CT_FINGER_DIR_COUNT;
    for (int k = 0; ok == CT_REPO_OK && k < finger_total; k++) {
        int face = k / CT_FINGER_DIR_COUNT;
        int dir = k % CT_FINGER_DIR_COUNT;
        ok = ct_alg_emit(out, cap, &used, k == 0 ? "%.6f" : ",%.6f",
                         cfg->gen.finger_cost[face][dir]);
    }
    if (ok == CT_REPO_OK) {
        ok = ct_alg_emit(out, cap, &used, "|%d|%d|%d|%.6f|%.6f|%.6f|%.6f", cfg->gen.setup_bfs_depth,
                         cfg->gen.fallback_depth, cfg->gen.candidate_max, cfg->gen.weight_len,
                         cfg->gen.weight_subset, cfg->gen.weight_regrip, cfg->gen.weight_finger);
    }
    if (ok == CT_REPO_OK) {
        ok = ct_alg_emit(out, cap, &used, "|");
    }
    if (ok == CT_REPO_OK) {
        ok = ct_alg_map_segment(map->corner, CT_LETTER_STICKERS, out, cap, &used);
    }
    if (ok == CT_REPO_OK) {
        ok = ct_alg_emit(out, cap, &used, "|");
    }
    if (ok == CT_REPO_OK) {
        ok = ct_alg_map_segment(map->edge, CT_LETTER_STICKERS, out, cap, &used);
    }
    if (ok != CT_REPO_OK) {
        out[0] = '\0';
    }
}

static int ct_alg_field_ok(const char *s, unsigned max) {
    return memchr(s, '\0', max) != NULL;
}

static int ct_alg_row_valid(const ct_generated_alg *a) {
    if (!ct_repo_text_ok(a->target_pair, CT_ALG_PAIR_MAX))
        return 0;
    if (!ct_repo_text_ok(a->sequence, CT_ALG_SEQ_MAX))
        return 0;
    if (!ct_alg_field_ok(a->setup, CT_ALG_SETUP_MAX))
        return 0;
    if (!ct_alg_field_ok(a->interchange, CT_ALG_INTER_MAX))
        return 0;
    if (!ct_alg_field_ok(a->insertion, CT_ALG_INSERT_MAX))
        return 0;
    if (!ct_alg_field_ok(a->score_parts_json, CT_ALG_PARTS_MAX))
        return 0;
    return 1;
}

int ct_repo_alg_put(sqlite3 *db, const ct_generated_alg *a, const char *params_hash) {
    if (db == NULL)
        return CT_REPO_ERR_ARG;
    if (a == NULL || !ct_alg_row_valid(a))
        return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(params_hash, CT_ALG_HASH_HEX_MAX))
        return CT_REPO_ERR_ARG;
    char now[CT_ALG_STAMP_MAX];
    ct_now_utc_iso(now, (unsigned)sizeof now);
    if (now[0] == '\0')
        return CT_REPO_ERR_DB;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_ALG_PUT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    ct_alg_bind b;
    ct_alg_bind_open(&b, st);
    ct_alg_bind_int(&b, a->orbit);
    ct_alg_bind_text(&b, a->target_pair);
    ct_alg_bind_int(&b, a->buffer_mode);
    ct_alg_bind_int(&b, a->category);
    ct_alg_bind_text(&b, a->sequence);
    ct_alg_bind_int(&b, a->classification);
    ct_alg_bind_text(&b, a->setup);
    ct_alg_bind_text(&b, a->interchange);
    ct_alg_bind_text(&b, a->insertion);
    ct_alg_bind_int(&b, a->qtm);
    ct_alg_bind_double(&b, a->score);
    ct_alg_bind_text(&b, a->score_parts_json);
    ct_alg_bind_text(&b, params_hash);
    ct_alg_bind_text(&b, now);
    int step = sqlite3_step(st);
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_REPO_OK : CT_REPO_ERR_DB;
}

int ct_repo_alg_get(sqlite3 *db, int orbit, const char *target_pair, int buffer_mode, int category,
                    const char *params_hash, ct_generated_alg *out) {
    if (db == NULL || out == NULL)
        return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(target_pair, CT_ALG_PAIR_MAX))
        return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(params_hash, CT_ALG_HASH_HEX_MAX))
        return CT_REPO_ERR_ARG;
    memset(out, 0, sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_ALG_GET, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    ct_alg_bind b;
    ct_alg_bind_open(&b, st);
    ct_alg_bind_int(&b, orbit);
    ct_alg_bind_text(&b, target_pair);
    ct_alg_bind_int(&b, buffer_mode);
    ct_alg_bind_int(&b, category);
    ct_alg_bind_text(&b, params_hash);
    int step = sqlite3_step(st);
    if (step == SQLITE_ROW) {
        ct_alg_from_row(st, out);
        sqlite3_finalize(st);
        return CT_REPO_OK;
    }
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_REPO_MISSING : CT_REPO_ERR_DB;
}

int ct_repo_alg_count(sqlite3 *db, const char *params_hash) {
    if (db == NULL || !ct_repo_text_ok(params_hash, CT_ALG_HASH_HEX_MAX))
        return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_ALG_COUNT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_text(st, 1, params_hash, -1, SQLITE_STATIC);
    int step = sqlite3_step(st);
    int n = step == SQLITE_ROW ? sqlite3_column_int(st, 0) : CT_REPO_ERR_DB;
    sqlite3_finalize(st);
    return n;
}

int ct_repo_alg_list(sqlite3 *db, const char *params_hash, int orbit, ct_generated_alg *out,
                     int cap) {
    if (db == NULL || !ct_repo_text_ok(params_hash, CT_ALG_HASH_HEX_MAX))
        return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL))
        return CT_REPO_ERR_ARG;
    if (cap == 0)
        return CT_REPO_OK;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_ALG_LIST, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    ct_alg_bind b;
    ct_alg_bind_open(&b, st);
    ct_alg_bind_text(&b, params_hash);
    ct_alg_bind_int(&b, orbit);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        ct_alg_from_row(st, &out[n]);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE)
        return CT_REPO_ERR_DB;
    return n;
}

int ct_repo_alg_stale(sqlite3 *db, const char *params_hash, int *out_ids, int cap) {
    if (db == NULL || !ct_repo_text_ok(params_hash, CT_ALG_HASH_HEX_MAX))
        return CT_REPO_ERR_ARG;
    if (cap < 0 || (cap > 0 && out_ids == NULL))
        return CT_REPO_ERR_ARG;
    if (cap == 0)
        return CT_REPO_OK;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_ALG_STALE, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_text(st, 1, params_hash, -1, SQLITE_STATIC);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        out_ids[n] = sqlite3_column_int(st, 0);
        n++;
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE)
        return CT_REPO_ERR_DB;
    return n;
}

int ct_repo_alg_drop(sqlite3 *db, const char *params_hash) {
    if (db == NULL || !ct_repo_text_ok(params_hash, CT_ALG_HASH_HEX_MAX))
        return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_ALG_DROP, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_text(st, 1, params_hash, -1, SQLITE_STATIC);
    int step = sqlite3_step(st);
    int dropped = step == SQLITE_DONE ? sqlite3_changes(db) : CT_REPO_ERR_DB;
    sqlite3_finalize(st);
    return dropped;
}

int ct_repo_alg_drop_orbit(sqlite3 *db, const char *params_hash, int orbit) {
    if (db == NULL || !ct_repo_text_ok(params_hash, CT_ALG_HASH_HEX_MAX))
        return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_ALG_DROP_ORBIT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    sqlite3_bind_text(st, 1, params_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, 2, orbit);
    int step = sqlite3_step(st);
    int dropped = step == SQLITE_DONE ? sqlite3_changes(db) : CT_REPO_ERR_DB;
    sqlite3_finalize(st);
    return dropped;
}

void ct_params_hash(const ct_config *cfg, int scheme_id, ct_buffer_mode mode, int buffer_corner,
                    int buffer_edge, const ct_letter_map *map, char hex_out[CT_ALG_HASH_HEX_MAX]) {
    if (hex_out == NULL)
        return;
    hex_out[0] = '\0';
    if (cfg == NULL || map == NULL)
        return;
    char canon[CT_ALG_CANON_MAX];
    ct_alg_canonical(cfg, scheme_id, mode, buffer_corner, buffer_edge, map, canon, sizeof canon);
    if (canon[0] == '\0') {
        return;
    }
    ct_sha256(canon, (unsigned)strlen(canon), hex_out);
}
