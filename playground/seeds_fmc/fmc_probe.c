#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "config/config.h"
#include "packs/packs.h"

enum {
    PROBE_CAP = 64,
    PROBE_LINE = 512,
    PROBE_URLS = 5,
    PROBE_OK = 0,
    PROBE_FAIL = 1
};

static const char *const probe_urls[PROBE_URLS] = {
    "https://www.speedsolving.com/wiki/index.php/Fewest_Moves_techniques",
    "https://www.speedsolving.com/wiki/index.php/Premoves",
    "https://www.speedsolving.com/wiki/index.php/Insertions",
    "https://www.speedsolving.com/wiki/index.php/Skeleton",
    "https://www.speedsolving.com/wiki/index.php/Blockbuilding",
};

static const char PROBE_MIGRATION[] = "db/migrations/001_init.sql";
static const char PROBE_TSV[] = "db/seeds/packs/fmc_concepts.tsv";

static int probe_fail = PROBE_OK;

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
    int rc = sqlite3_prepare_v2(db,
                                "SELECT content_hash FROM event_pack WHERE event=? AND method=?"
                                " AND stage=? AND case_key=?",
                                -1, &st, NULL);
    if (rc != SQLITE_OK) {
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

int main(void) {
    sqlite3 *db = NULL;
    probe_check(sqlite3_open(":memory:", &db) == SQLITE_OK, "open db");
    probe_check(probe_schema(db), "schema");
    int rows = ct_pack_seed_file(db, PROBE_TSV, CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT);
    probe_check(rows == 16, "seed rows");
    probe_check(ct_pack_count(db, CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT) == 16,
                "pack count");
    probe_check(ct_pack_seed_file(db, PROBE_TSV, CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT) ==
                    16,
                "seed idempotent");
    ct_pack_entry entries[PROBE_CAP];
    int n = ct_pack_list(db, CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, entries, PROBE_CAP);
    probe_check(n == 16, "list count");
    int concepts = 0;
    for (int k = 0; k < n; k++) {
        const ct_pack_entry *e = &entries[k];
        char where[PROBE_LINE];
        snprintf(where, sizeof where, "row %s", e->case_key);
        probe_check(e->alg[0] != '\0', where);
        probe_check(e->front_json[0] == '{' && probe_json_balanced(e->front_json), where);
        probe_check(strstr(e->front_json, "\"concept\"") != NULL, where);
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
        if (strstr(e->front_json, "\"concept\"") != NULL)
            concepts++;
    }
    probe_check(concepts == n, "concept tag count");
    sqlite3_close(db);
    if (probe_fail == PROBE_OK)
        printf("fmc_concepts probe ok, 16 rows, 5 sources\n");
    return probe_fail;
}
