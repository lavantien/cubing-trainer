#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/packs/packs.h"
#include "../src/packs/provenance.h"
#include "../src/storage/db.h"
#include "framework.h"

enum { CT_SEED_LIST_CAP = 64, CT_SEED_LONG_FIELD = 4096 };

static const char CT_SEED_OK_PATH[] = "build/ct_seed_ok.tsv";
static const char CT_SEED_NOHDR_PATH[] = "build/ct_seed_nohdr.tsv";
static const char CT_SEED_CRLF_PATH[] = "build/ct_seed_crlf.tsv";
static const char CT_SEED_EMPTY_PATH[] = "build/ct_seed_empty.tsv";
static const char CT_SEED_BAD_PATH[] = "build/ct_seed_bad.tsv";
static const char CT_SEED_LONG_PATH[] = "build/ct_seed_long.tsv";
static const char CT_SEED_MISSING_PATH[] = "build/ct_seed_missing.tsv";

static sqlite3 *open_db(void) {
    sqlite3 *db = NULL;
    ct_db_open(&db, ":memory:");
    ct_db_migrate(db, "db/migrations");
    return db;
}

static void write_file(const char *path, const char *body) {
    FILE *f = fopen(path, "wb");
    CT_ASSERT(f != NULL);
    size_t n = strlen(body);
    CT_ASSERT(fwrite(body, 1, n, f) == n);
    CT_ASSERT(ferror(f) == 0);
    fclose(f);
}

static void write_long_file(const char *path) {
    char body[CT_SEED_LONG_FIELD + CT_B8];
    memset(body, 'x', sizeof body - 1);
    body[0] = 'k';
    body[sizeof body - 2] = '\n';
    body[sizeof body - 1] = '\0';
    write_file(path, body);
}

static void stored_hash(sqlite3 *db, ct_event ev, ct_method m, ct_stage st, const char *case_key,
                        char *out, size_t cap) {
    sqlite3_stmt *stmt = NULL;
    out[0] = '\0';
    if (sqlite3_prepare_v2(db,
                           "SELECT content_hash FROM event_pack WHERE event = ? AND method = ?"
                           " AND stage = ? AND case_key = ?",
                           -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, (int)ev);
        sqlite3_bind_int(stmt, 2, (int)m);
        sqlite3_bind_int(stmt, 3, (int)st);
        sqlite3_bind_text(stmt, 4, case_key, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *s = sqlite3_column_text(stmt, 0);
            if (s != NULL)
                snprintf(out, cap, "%s", (const char *)s);
        }
    }
    sqlite3_finalize(stmt);
}

CT_SUITE(pack_seed_loader) {
    sqlite3 *db = open_db();
    write_file(CT_SEED_OK_PATH,
               "case_key\talg\tfront_json\tsource_url\tsource_at\n"
               "k1\tR U R'\t{\"case\":\"k1\"}\thttps://ex.example/a\t2026-10-10\n"
               "\n"
               "k2\tR U2 R'\t{\"case\":\"k2\"}\thttps://ex.example/a\t2026-10-10\n"
               "k3\tL' U' L\t{\"case\":\"k3\"}\thttps://ex.example/a\t2026-10-10\n");
    CT_ASSERT_EQ(ct_pack_seed_file(db, CT_SEED_OK_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L),
                 3);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 3);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L, "k2", &got),
                 CT_PACK_OK);
    CT_ASSERT_STR_EQ(got.alg, "R U2 R'");
    CT_ASSERT_STR_EQ(got.front_json, "{\"case\":\"k2\"}");
    CT_ASSERT_STR_EQ(got.source_url, "https://ex.example/a");
    CT_ASSERT_STR_EQ(got.source_at, "2026-10-10");
    char hex[CT_PACK_HASH_MAX];
    char stored[CT_PACK_HASH_MAX];
    CT_ASSERT_EQ(ct_pack_content_hash(&got, hex), CT_PACK_OK);
    stored_hash(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L, "k2", stored, sizeof stored);
    CT_ASSERT_STR_EQ(stored, hex);
    CT_ASSERT_EQ(ct_pack_seed_file(db, CT_SEED_OK_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L),
                 3);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 3);
    ct_db_close(db);
}

CT_SUITE(pack_seed_loader_header) {
    sqlite3 *db = open_db();
    write_file(CT_SEED_NOHDR_PATH, "n1\tF R' F' R\t{\"case\":\"n1\"}\thttps://ex.example/b\t"
                                   "2026-10-10\n");
    CT_ASSERT_EQ(
        ct_pack_seed_file(db, CT_SEED_NOHDR_PATH, CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L), 1);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L), 1);
    write_file(CT_SEED_EMPTY_PATH, "case_key\talg\tfront_json\tsource_url\tsource_at\n");
    CT_ASSERT_EQ(
        ct_pack_seed_file(db, CT_SEED_EMPTY_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    ct_db_close(db);
}

CT_SUITE(pack_seed_loader_crlf) {
    sqlite3 *db = open_db();
    write_file(CT_SEED_CRLF_PATH, "case_key\talg\tfront_json\tsource_url\tsource_at\r\n"
                                  "c1\tR U R' U'\t{\"case\":\"c1\"}\thttps://ex.example/c\t"
                                  "2026-10-10\r\n");
    CT_ASSERT_EQ(
        ct_pack_seed_file(db, CT_SEED_CRLF_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 1);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L, "c1", &got),
                 CT_PACK_OK);
    CT_ASSERT_STR_EQ(got.alg, "R U R' U'");
    CT_ASSERT_STR_EQ(got.source_at, "2026-10-10");
    ct_db_close(db);
}

CT_SUITE(pack_seed_loader_rejects) {
    sqlite3 *db = open_db();
    const char *cases[] = {
        "g3\tR U R'\t{\"case\":\"g3\"}\thttps://ex.example/d\n",
        "g4\t\t{\"case\":\"g4\"}\thttps://ex.example/d\t2026-10-10\n",
        "g5\tR U R'\t{\"case\":\"g5\"}\t\t2026-10-10\n",
        "g6\tR U R'\t{\"case\":\"g6\"}\thttps://ex.example/d\t2026-10-10\textra\n",
        "\tR U R'\t{\"case\":\"g7\"}\thttps://ex.example/d\t2026-10-10\n",
        "g8\tR U R'\t\thttps://ex.example/d\t2026-10-10\n",
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        char body[CT_B256 * 8];
        snprintf(body, sizeof body, "%s%s",
                 "g1\tR U R'\t{\"case\":\"g1\"}\thttps://ex.example/d\t"
                 "2026-10-10\n",
                 cases[i]);
        write_file(CT_SEED_BAD_PATH, body);
        int rc =
            ct_pack_seed_file(db, CT_SEED_BAD_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L);
        CT_ASSERT(rc < 0);
        CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    }
    write_long_file(CT_SEED_LONG_PATH);
    CT_ASSERT_EQ(
        ct_pack_seed_file(db, CT_SEED_LONG_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L),
        CT_PACK_ERR_FORMAT);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    CT_ASSERT_EQ(
        ct_pack_seed_file(db, CT_SEED_MISSING_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L),
        CT_PACK_ERR_FILE);
    CT_ASSERT(ct_pack_seed_file(NULL, CT_SEED_OK_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L) <
              0);
    CT_ASSERT(ct_pack_seed_file(db, NULL, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L) < 0);
    CT_ASSERT(ct_pack_seed_file(db, "", CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L) < 0);
    ct_db_close(db);
}
