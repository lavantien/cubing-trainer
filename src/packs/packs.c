#include "packs/packs.h"

#include <stdio.h>
#include <string.h>

static const char CT_SQL_PACK_UPSERT[] =
    "INSERT INTO event_pack(event, method, stage, case_key, alg, front_json, source_url, "
    "source_at, content_hash)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
    " ON CONFLICT(event, method, stage, case_key) DO UPDATE SET"
    " alg = excluded.alg, front_json = excluded.front_json, source_url = excluded.source_url,"
    " source_at = excluded.source_at, content_hash = excluded.content_hash";
static const char CT_SQL_PACK_GET[] =
    "SELECT event, method, stage, case_key, alg, front_json, source_url, source_at FROM event_pack"
    " WHERE event = ? AND method = ? AND stage = ? AND case_key = ?";
static const char CT_SQL_PACK_LIST[] =
    "SELECT event, method, stage, case_key, alg, front_json, source_url, source_at FROM event_pack"
    " WHERE event = ? AND method = ? AND stage = ? ORDER BY case_key";
static const char CT_SQL_PACK_COUNT[] =
    "SELECT count(*) FROM event_pack WHERE event = ? AND method = ? AND stage = ?";

typedef struct ct_pack_bind {
    sqlite3_stmt *st;
    int next;
} ct_pack_bind;

typedef struct ct_pack_read {
    sqlite3_stmt *st;
    int col;
} ct_pack_read;

static void ct_pack_bind_open(ct_pack_bind *b, sqlite3_stmt *st) {
    b->st = st;
    b->next = 0;
}

static void ct_pack_bind_int(ct_pack_bind *b, int v) {
    sqlite3_bind_int(b->st, ++b->next, v);
}

static void ct_pack_bind_text(ct_pack_bind *b, const char *s) {
    sqlite3_bind_text(b->st, ++b->next, s, -1, SQLITE_STATIC);
}

static void ct_pack_read_open(ct_pack_read *r, sqlite3_stmt *st) {
    r->st = st;
    r->col = 0;
}

static int ct_pack_read_int(ct_pack_read *r) {
    return sqlite3_column_int(r->st, r->col++);
}

static void ct_pack_read_text(ct_pack_read *r, char *dst, size_t cap) {
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

static void ct_pack_from_row(sqlite3_stmt *st, ct_pack_entry *e) {
    ct_pack_read r;
    ct_pack_read_open(&r, st);
    e->event = (ct_event)ct_pack_read_int(&r);
    e->method = (ct_method)ct_pack_read_int(&r);
    e->stage = (ct_stage)ct_pack_read_int(&r);
    ct_pack_read_text(&r, e->case_key, sizeof e->case_key);
    ct_pack_read_text(&r, e->alg, sizeof e->alg);
    ct_pack_read_text(&r, e->front_json, sizeof e->front_json);
    ct_pack_read_text(&r, e->source_url, sizeof e->source_url);
    ct_pack_read_text(&r, e->source_at, sizeof e->source_at);
}

static int ct_pack_field_ok(const char *s, unsigned max) {
    return memchr(s, '\0', max) != NULL;
}

static int ct_pack_entry_valid(const ct_pack_entry *e) {
    if (e == NULL)
        return CT_PACK_ERR_ARG;
    if (!ct_pack_field_ok(e->case_key, CT_PACK_CASE_KEY_MAX))
        return CT_PACK_ERR_ARG;
    if (!ct_pack_field_ok(e->alg, CT_PACK_ALG_MAX))
        return CT_PACK_ERR_ARG;
    if (!ct_pack_field_ok(e->front_json, CT_PACK_FRONT_JSON_MAX))
        return CT_PACK_ERR_ARG;
    if (!ct_pack_field_ok(e->source_url, CT_PACK_URL_MAX))
        return CT_PACK_ERR_ARG;
    if (!ct_pack_field_ok(e->source_at, CT_PACK_AT_MAX))
        return CT_PACK_ERR_ARG;
    if (e->case_key[0] == '\0')
        return CT_PACK_ERR_ARG;
    return CT_PACK_OK;
}

int ct_pack_content_hash(const ct_pack_entry *e, char hex_out[CT_PACK_HASH_MAX]) {
    if (e == NULL || hex_out == NULL)
        return CT_PACK_ERR_ARG;
    int rc = ct_pack_entry_valid(e);
    if (rc != CT_PACK_OK)
        return rc;
    char canon[CT_PACK_CANON_MAX];
    int n =
        snprintf(canon, sizeof canon, "%d|%d|%d|%s|%s|%s|%s|%s", (int)e->event, (int)e->method,
                 (int)e->stage, e->case_key, e->alg, e->front_json, e->source_url, e->source_at);
    if (n < 0 || (unsigned)n >= sizeof canon)
        return CT_PACK_ERR_ARG;
    ct_sha256(canon, (unsigned)n, hex_out);
    return CT_PACK_OK;
}

int ct_pack_upsert(sqlite3 *db, const ct_pack_entry *e) {
    if (db == NULL)
        return CT_PACK_ERR_ARG;
    int rc = ct_pack_entry_valid(e);
    if (rc != CT_PACK_OK)
        return rc;
    char hash[CT_PACK_HASH_MAX];
    rc = ct_pack_content_hash(e, hash);
    if (rc != CT_PACK_OK)
        return rc;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_PACK_UPSERT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_PACK_ERR_DB;
    }
    ct_pack_bind b;
    ct_pack_bind_open(&b, st);
    ct_pack_bind_int(&b, (int)e->event);
    ct_pack_bind_int(&b, (int)e->method);
    ct_pack_bind_int(&b, (int)e->stage);
    ct_pack_bind_text(&b, e->case_key);
    ct_pack_bind_text(&b, e->alg);
    ct_pack_bind_text(&b, e->front_json);
    ct_pack_bind_text(&b, e->source_url);
    ct_pack_bind_text(&b, e->source_at);
    ct_pack_bind_text(&b, hash);
    int step = sqlite3_step(st);
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_PACK_OK : CT_PACK_ERR_DB;
}

int ct_pack_get(sqlite3 *db, ct_event ev, ct_method m, ct_stage st, const char *case_key,
                ct_pack_entry *out) {
    if (db == NULL || out == NULL || case_key == NULL || case_key[0] == '\0')
        return CT_PACK_ERR_ARG;
    memset(out, 0, sizeof *out);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_PACK_GET, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return CT_PACK_ERR_DB;
    }
    ct_pack_bind b;
    ct_pack_bind_open(&b, stmt);
    ct_pack_bind_int(&b, (int)ev);
    ct_pack_bind_int(&b, (int)m);
    ct_pack_bind_int(&b, (int)st);
    ct_pack_bind_text(&b, case_key);
    int step = sqlite3_step(stmt);
    if (step == SQLITE_ROW) {
        ct_pack_from_row(stmt, out);
        sqlite3_finalize(stmt);
        return CT_PACK_OK;
    }
    sqlite3_finalize(stmt);
    return step == SQLITE_DONE ? CT_PACK_MISSING : CT_PACK_ERR_DB;
}

int ct_pack_count(sqlite3 *db, ct_event ev, ct_method m, ct_stage st) {
    if (db == NULL)
        return CT_PACK_ERR_ARG;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_PACK_COUNT, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return CT_PACK_ERR_DB;
    }
    ct_pack_bind b;
    ct_pack_bind_open(&b, stmt);
    ct_pack_bind_int(&b, (int)ev);
    ct_pack_bind_int(&b, (int)m);
    ct_pack_bind_int(&b, (int)st);
    int step = sqlite3_step(stmt);
    int n = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (step != SQLITE_ROW || n < 0)
        return CT_PACK_ERR_DB;
    return n;
}

int ct_pack_list(sqlite3 *db, ct_event ev, ct_method m, ct_stage st, ct_pack_entry *out, int cap) {
    if (db == NULL)
        return CT_PACK_ERR_ARG;
    if (cap < 0 || (cap > 0 && out == NULL))
        return CT_PACK_ERR_ARG;
    if (cap == 0)
        return CT_PACK_OK;
    memset(out, 0, (size_t)cap * sizeof *out);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_PACK_LIST, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return CT_PACK_ERR_DB;
    }
    ct_pack_bind b;
    ct_pack_bind_open(&b, stmt);
    ct_pack_bind_int(&b, (int)ev);
    ct_pack_bind_int(&b, (int)m);
    ct_pack_bind_int(&b, (int)st);
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(stmt)) == SQLITE_ROW) {
        ct_pack_from_row(stmt, &out[n]);
        n++;
    }
    sqlite3_finalize(stmt);
    if (step != SQLITE_ROW && step != SQLITE_DONE)
        return CT_PACK_ERR_DB;
    return n;
}

static const char CT_SQL_SEED_BEGIN[] = "BEGIN IMMEDIATE";
static const char CT_SQL_SEED_COMMIT[] = "COMMIT";
static const char CT_SQL_SEED_ROLLBACK[] = "ROLLBACK";
static const char CT_PACK_SEED_HEADER[] = "case_key\talg\tfront_json\tsource_url\tsource_at";

static int ct_pack_seed_split(char *line, char *fields[], int max_fields) {
    fields[0] = line;
    int count = 1;
    for (char *p = line; *p != '\0'; p++) {
        if (*p == '\t') {
            *p = '\0';
            if (count == max_fields)
                return CT_PACK_ERR_FORMAT;
            fields[count] = p + 1;
            count++;
        }
    }
    return count;
}

static int ct_pack_seed_field(char *dst, const char *src, size_t cap) {
    size_t n = strlen(src);
    if (n == 0 || n >= cap)
        return CT_PACK_ERR_FORMAT;
    memcpy(dst, src, n + 1);
    return CT_PACK_OK;
}

int ct_pack_seed_file(sqlite3 *db, const char *path, ct_event ev, ct_method m, ct_stage st) {
    if (db == NULL || path == NULL || path[0] == '\0')
        return CT_PACK_ERR_ARG;
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return CT_PACK_ERR_FILE;
    if (sqlite3_exec(db, CT_SQL_SEED_BEGIN, NULL, NULL, NULL) != SQLITE_OK) {
        fclose(f);
        return CT_PACK_ERR_DB;
    }
    char line[CT_PACK_SEED_LINE_MAX];
    char *fields[CT_PACK_SEED_COLUMNS];
    int rows = 0;
    int rc = CT_PACK_OK;
    int header_left = 1;
    while (fgets(line, sizeof line, f) != NULL) {
        size_t n = strlen(line);
        if (n + 1 == sizeof line && line[n - 1] != '\n' && !feof(f)) {
            rc = CT_PACK_ERR_FORMAT;
            break;
        }
        if (n > 0 && line[n - 1] == '\n') {
            line[n - 1] = '\0';
            n--;
        }
        if (n > 0 && line[n - 1] == '\r')
            line[n - 1] = '\0';
        if (line[0] == '\0')
            continue;
        if (header_left) {
            header_left = 0;
            if (strcmp(line, CT_PACK_SEED_HEADER) == 0)
                continue;
        }
        int count = ct_pack_seed_split(line, fields, CT_PACK_SEED_COLUMNS);
        if (count < 0) {
            rc = count;
            break;
        }
        if (count != CT_PACK_SEED_COLUMNS) {
            rc = CT_PACK_ERR_FORMAT;
            break;
        }
        ct_pack_entry e;
        memset(&e, 0, sizeof e);
        e.event = ev;
        e.method = m;
        e.stage = st;
        rc = ct_pack_seed_field(e.case_key, fields[0], sizeof e.case_key);
        if (rc == CT_PACK_OK)
            rc = ct_pack_seed_field(e.alg, fields[1], sizeof e.alg);
        if (rc == CT_PACK_OK)
            rc = ct_pack_seed_field(e.front_json, fields[2], sizeof e.front_json);
        if (rc == CT_PACK_OK)
            rc = ct_pack_seed_field(e.source_url, fields[3], sizeof e.source_url);
        if (rc == CT_PACK_OK)
            rc = ct_pack_seed_field(e.source_at, fields[4], sizeof e.source_at);
        if (rc != CT_PACK_OK)
            break;
        rc = ct_pack_upsert(db, &e);
        if (rc != CT_PACK_OK)
            break;
        rows++;
    }
    if (ferror(f))
        rc = CT_PACK_ERR_FILE;
    fclose(f);
    if (rc != CT_PACK_OK) {
        sqlite3_exec(db, CT_SQL_SEED_ROLLBACK, NULL, NULL, NULL);
        return rc;
    }
    if (sqlite3_exec(db, CT_SQL_SEED_COMMIT, NULL, NULL, NULL) != SQLITE_OK)
        return CT_PACK_ERR_DB;
    return rows;
}
