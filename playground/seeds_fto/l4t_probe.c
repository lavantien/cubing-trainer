#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "packs/packs.h"

enum {
    PROBE_CAP = 64,
    PROBE_LINE = 512,
    PROBE_TOKEN = 16,
    PROBE_URLS = 2,
    PROBE_GAPS = 4,
    PROBE_OK = 0,
    PROBE_FAIL = 1
};

static const char *const probe_urls[PROBE_URLS] = {
    "https://docs.google.com/spreadsheets/d/1_IHL077w-9U-qtpF9hvC6CrUn7etua5QgPH8vEMqK58/edit",
    "https://docs.google.com/document/d/18CMTrFC9PYkEsheOHnywzvV0EdCb3SFGQXSK0DDb30o/edit",
};

static const char *const probe_gap_tokens[PROBE_GAPS] = {"BR", "BR'", "L2'", "R2'"};

static const char PROBE_MIGRATION[] = "db/migrations/001_init.sql";
static const char PROBE_TSV[] = "db/seeds/packs/fto_l4t_redux.tsv";

static int probe_fail = PROBE_OK;
static int probe_gap_hits[PROBE_GAPS];

static void probe_check(int cond, const char *what) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", what);
        probe_fail = PROBE_FAIL;
    }
}

static char *probe_slurp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

static int probe_url_known(const char *url) {
    for (int k = 0; k < PROBE_URLS; k++) {
        if (strcmp(probe_urls[k], url) == 0)
            return 1;
    }
    return 0;
}

static int probe_json_balanced(const char *s) {
    int braces = 0;
    int quotes = 0;
    for (const char *p = s; *p != '\0'; p++) {
        if (*p == '{')
            braces++;
        else if (*p == '}')
            braces--;
        else if (*p == '"')
            quotes++;
    }
    return braces == 0 && quotes % 2 == 0;
}

static int probe_schema(sqlite3 *db) {
    char *sql = probe_slurp(PROBE_MIGRATION);
    if (sql == NULL)
        return 0;
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "FAIL migration exec %s\n", err != NULL ? err : "?");
        sqlite3_free(err);
    }
    free(sql);
    return rc == SQLITE_OK;
}

static int probe_stored_hash(sqlite3 *db, const ct_pack_entry *e, char *out, size_t cap) {
    sqlite3_stmt *st = NULL;
    out[0] = '\0';
    if (sqlite3_prepare_v2(db,
                           "SELECT content_hash FROM event_pack WHERE event=? AND method=?"
                           " AND stage=? AND case_key=?",
                           -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return 0;
    }
    sqlite3_bind_int(st, 1, (int)e->event);
    sqlite3_bind_int(st, 2, (int)e->method);
    sqlite3_bind_int(st, 3, (int)e->stage);
    sqlite3_bind_text(st, 4, e->case_key, -1, SQLITE_STATIC);
    int ok = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char *h = sqlite3_column_text(st, 0);
        if (h != NULL) {
            snprintf(out, cap, "%s", (const char *)h);
            ok = 1;
        }
    }
    sqlite3_finalize(st);
    return ok;
}

static int probe_token_gap(const char *tok) {
    for (int k = 0; k < PROBE_GAPS; k++) {
        if (strcmp(probe_gap_tokens[k], tok) == 0) {
            probe_gap_hits[k]++;
            return 1;
        }
    }
    return 0;
}

static void probe_alg_tokens(const char *alg, const char *case_key) {
    char buf[CT_PACK_ALG_MAX];
    snprintf(buf, sizeof buf, "%s", alg);
    char *save = NULL;
    for (char *tok = strtok_r(buf, " ", &save); tok != NULL; tok = strtok_r(NULL, " ", &save)) {
        ct_move m;
        if (ct_notation_parse_one(tok, 3, &m) != 0) {
            char what[PROBE_LINE];
            snprintf(what, sizeof what, "token %s in %s", tok, case_key);
            probe_check(probe_token_gap(tok), what);
        }
    }
}

int main(void) {
    sqlite3 *db = NULL;
    probe_check(sqlite3_open(":memory:", &db) == SQLITE_OK, "open db");
    probe_check(probe_schema(db), "schema");
    int rows = ct_pack_seed_file(db, PROBE_TSV, CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T);
    probe_check(rows == 36, "seed rows");
    probe_check(ct_pack_count(db, CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T) == 36,
                "pack count");
    probe_check(ct_pack_seed_file(db, PROBE_TSV, CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T) ==
                    36,
                "seed idempotent");
    ct_pack_entry entries[PROBE_CAP];
    int n = ct_pack_list(db, CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T, entries, PROBE_CAP);
    probe_check(n == 36, "list count");
    int sheet_rows = 0;
    int guide_rows = 0;
    for (int k = 0; k < n; k++) {
        const ct_pack_entry *e = &entries[k];
        char where[PROBE_LINE];
        snprintf(where, sizeof where, "row %s", e->case_key);
        probe_check(e->alg[0] != '\0', where);
        probe_check(e->front_json[0] == '{' && probe_json_balanced(e->front_json), where);
        probe_check(strstr(e->front_json, "\"case\"") != NULL, where);
        probe_check(e->source_at[0] != '\0' && strcmp(e->source_at, "2026-10-10") == 0, where);
        probe_check(probe_url_known(e->source_url), where);
        char h1[CT_PACK_HASH_MAX];
        char h2[CT_PACK_HASH_MAX];
        char stored[CT_PACK_HASH_MAX];
        probe_check(ct_pack_content_hash(e, h1) == CT_PACK_OK, where);
        ct_pack_entry again = *e;
        probe_check(ct_pack_content_hash(&again, h2) == CT_PACK_OK, where);
        probe_check(strcmp(h1, h2) == 0, where);
        probe_check(probe_stored_hash(db, e, stored, sizeof stored) && strcmp(stored, h1) == 0,
                    where);
        probe_alg_tokens(e->alg, e->case_key);
        if (strcmp(probe_urls[0], e->source_url) == 0)
            sheet_rows++;
        else
            guide_rows++;
    }
    probe_check(sheet_rows == 35 && guide_rows == 1, "source split");
    sqlite3_close(db);
    if (probe_fail == PROBE_OK) {
        printf("fto_l4t_redux probe ok, 36 rows, notation gaps:");
        for (int k = 0; k < PROBE_GAPS; k++)
            printf(" %s=%d", probe_gap_tokens[k], probe_gap_hits[k]);
        printf("\n");
    }
    return probe_fail;
}
