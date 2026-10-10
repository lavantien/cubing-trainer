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

static int tsv_data_lines(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    int lines = 0;
    int c;
    int last = '\n';
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n')
            lines++;
        last = c;
    }
    if (last != '\n')
        lines++;
    fclose(f);
    return lines - 1;
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
        ct_pack_seed_file(db, CT_SEED_NOHDR_PATH, CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L),
        CT_PACK_ERR_FORMAT);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    write_file(CT_SEED_EMPTY_PATH, "case_key\talg\tfront_json\tsource_url\tsource_at\n");
    CT_ASSERT_EQ(
        ct_pack_seed_file(db, CT_SEED_EMPTY_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    ct_db_close(db);
}

CT_SUITE(pack_seed_loader_columns) {
    sqlite3 *db = open_db();
    write_file(CT_SEED_BAD_PATH,
               "source_at\tcase_key\talg\tsource_url\tfront_json\n"
               "2026-10-11\tk1\tR U R'\thttps://ex.example/e\t{\"case\":\"k1\"}\n"
               "2026-10-11\tk2\tR U2 R'\thttps://ex.example/e\t{\"case\":\"k2\"}\n");
    CT_ASSERT_EQ(
        ct_pack_seed_file(db, CT_SEED_BAD_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 2);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L, "k1", &got),
                 CT_PACK_OK);
    CT_ASSERT_STR_EQ(got.alg, "R U R'");
    CT_ASSERT_STR_EQ(got.front_json, "{\"case\":\"k1\"}");
    CT_ASSERT_STR_EQ(got.source_url, "https://ex.example/e");
    CT_ASSERT_STR_EQ(got.source_at, "2026-10-11");

    const char *bad_headers[] = {
        "case_key\talg\tfront_json\tsource_url\n",
        "case_key\talg\tfront_json\tsource_url\tsource_at\textra\n",
        "case_key\talg\tfront_json\tsource_url\tbogus\n",
        "case_key\talg\talg\tsource_url\tsource_at\n",
        "case_key\talg\tfront_json\tsource_url\tsource_at\tsource_at\n",
        "case_key\talg\tfront_json\tsource_url\tsource_at\n"
        "k1\tR U R'\t{\"case\":\"k1\"}\thttps://ex.example/f\t2026-10-12\n"
        "case_key\talg\tfront_json\tsource_url\tsource_at\n",
        "case_key\talg\tfront_json\tsource_url\tsource_at\n"
        "k1\tR U R'\t{\"case\":\"k1\"}\thttps://ex.example/f\t2026-10-12\n"
        "k2\tR U'\t{\"case\":\"k2\"}\n",
        "case_key\talg\tfront_json\tsource_url\tsource_at\n"
        "k1\tR U R'\t{\"case\":\"k1\"}\thttps://ex.example/f\t"
        "12345678901234567890123456789012\n",
    };
    for (unsigned i = 0; i < sizeof bad_headers / sizeof bad_headers[0]; i++) {
        write_file(CT_SEED_BAD_PATH, bad_headers[i]);
        CT_ASSERT_EQ(
            ct_pack_seed_file(db, CT_SEED_BAD_PATH, CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L),
            CT_PACK_ERR_FORMAT);
    }
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L), 0);
    ct_db_close(db);
}

CT_SUITE(pack_seed_commit_rollback) {
    sqlite3 *db = open_db();
    write_file(CT_SEED_OK_PATH,
               "case_key\talg\tfront_json\tsource_url\tsource_at\n"
               "k1\tR U R'\t{\"case\":\"k1\"}\thttps://ex.example/g\t2026-10-10\n");
    CT_ASSERT_EQ(sqlite3_exec(db, "DROP TABLE event_pack", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT_EQ(
        sqlite3_exec(db, "CREATE TABLE seed_fk_guard(id INTEGER PRIMARY KEY)", NULL, NULL, NULL),
        SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_exec(db,
                              "CREATE TABLE event_pack("
                              "id INTEGER PRIMARY KEY,"
                              "event INTEGER NOT NULL REFERENCES seed_fk_guard(id),"
                              "method INTEGER NOT NULL,"
                              "stage INTEGER NOT NULL,"
                              "case_key TEXT NOT NULL,"
                              "alg TEXT,"
                              "front_json TEXT NOT NULL DEFAULT '{}',"
                              "source_url TEXT,"
                              "source_at TEXT,"
                              "content_hash TEXT,"
                              "UNIQUE(event, method, stage, case_key))",
                              NULL, NULL, NULL),
                 SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_exec(db, "PRAGMA defer_foreign_keys=ON", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT_EQ(ct_pack_seed_file(db, CT_SEED_OK_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L),
                 CT_PACK_ERR_DB);
    CT_ASSERT_EQ(sqlite3_get_autocommit(db), 1);
    sqlite3_stmt *st = NULL;
    CT_ASSERT_EQ(sqlite3_prepare_v2(db, "SELECT count(*) FROM event_pack", -1, &st, NULL),
                 SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_step(st), SQLITE_ROW);
    CT_ASSERT_EQ(sqlite3_column_int(st, 0), 0);
    sqlite3_finalize(st);
    ct_db_close(db);

    sqlite3 *plain = open_db();
    CT_ASSERT_EQ(
        ct_pack_seed_file(plain, CT_SEED_OK_PATH, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 1);
    CT_ASSERT_EQ(ct_pack_count(plain, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L), 1);
    ct_db_close(plain);
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
    CT_ASSERT(ct_pack_seed_file(NULL, CT_SEED_OK_PATH, CT_EVENT_3X3, CT_METHOD_CFOP,
                                CT_STAGE_F2L) == CT_PACK_ERR_ARG);
    CT_ASSERT(ct_pack_seed_file(db, NULL, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L) ==
              CT_PACK_ERR_ARG);
    CT_ASSERT(ct_pack_seed_file(db, "", CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L) ==
              CT_PACK_ERR_ARG);
    ct_db_close(db);
}

static void verify_pack(const char *path, ct_event ev, ct_method m, ct_stage st, int expected) {
    sqlite3 *db = open_db();
    CT_ASSERT_EQ(ct_pack_seed_file(db, path, ev, m, st), expected);
    CT_ASSERT_EQ(ct_pack_count(db, ev, m, st), expected);
    CT_ASSERT_EQ(tsv_data_lines(path), expected);
    ct_pack_entry got[CT_SEED_LIST_CAP];
    CT_ASSERT(expected <= CT_SEED_LIST_CAP);
    int n = ct_pack_list(db, ev, m, st, got, expected);
    CT_ASSERT_EQ(n, expected);
    for (int i = 0; i < n; i++) {
        CT_ASSERT(got[i].case_key[0] != '\0');
        CT_ASSERT(got[i].alg[0] != '\0');
        CT_ASSERT(got[i].front_json[0] != '\0');
        CT_ASSERT(got[i].source_url[0] != '\0');
        CT_ASSERT(got[i].source_at[0] != '\0');
        CT_ASSERT_EQ(got[i].event, ev);
        CT_ASSERT_EQ(got[i].method, m);
        CT_ASSERT_EQ(got[i].stage, st);
        char hex[CT_PACK_HASH_MAX];
        char stored[CT_PACK_HASH_MAX];
        CT_ASSERT_EQ(ct_pack_content_hash(&got[i], hex), CT_PACK_OK);
        stored_hash(db, ev, m, st, got[i].case_key, stored, sizeof stored);
        CT_ASSERT_STR_EQ(stored, hex);
    }
    CT_ASSERT_EQ(ct_pack_seed_file(db, path, ev, m, st), expected);
    CT_ASSERT_EQ(ct_pack_count(db, ev, m, st), expected);
    ct_db_close(db);
}

static ct_pack_entry fetch_one(const char *path, ct_event ev, ct_method m, ct_stage st,
                               const char *case_key, int expected) {
    sqlite3 *db = open_db();
    CT_ASSERT_EQ(ct_pack_seed_file(db, path, ev, m, st), expected);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, ev, m, st, case_key, &got), CT_PACK_OK);
    ct_db_close(db);
    return got;
}

CT_SUITE(pack_seed_cll) {
    verify_pack("db/seeds/packs/twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL, 42);
    ct_pack_entry e = fetch_one("db/seeds/packs/twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL,
                                CT_STAGE_CLL, "cll.12", 42);
    CT_ASSERT_STR_EQ(e.alg, "F R U R' U' F'");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"U6\"}");
    e = fetch_one("db/seeds/packs/twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL,
                  "cll.17", 42);
    CT_ASSERT_STR_EQ(e.alg, "R U R' U' R' F R F'");
    e = fetch_one("db/seeds/packs/twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL,
                  "cll.25", 42);
    CT_ASSERT_STR_EQ(e.alg, "R2 U2 R' U2 R2");
    e = fetch_one("db/seeds/packs/twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL,
                  "cll.41", 42);
    CT_ASSERT_STR_EQ(e.alg, "R U R' U' R' F R2 U' R' U' R U R' F'");
}

CT_SUITE(pack_seed_sarah) {
    verify_pack("db/seeds/packs/skewb_sarah.tsv", CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE,
                CT_STAGE_SARAH, 15);
    ct_pack_entry e = fetch_one("db/seeds/packs/skewb_sarah.tsv", CT_EVENT_SKEWB,
                                CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH, "sarah.03", 15);
    CT_ASSERT_STR_EQ(e.alg, "F' L F L'");
    e = fetch_one("db/seeds/packs/skewb_sarah.tsv", CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE,
                  CT_STAGE_SARAH, "sarah.05", 15);
    CT_ASSERT_STR_EQ(e.alg, "(y2) (F' L F L')2");
    e = fetch_one("db/seeds/packs/skewb_sarah.tsv", CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE,
                  CT_STAGE_SARAH, "sarah.13", 15);
    CT_ASSERT_STR_EQ(e.alg, "R' F R F' (y2) R' F R F'");
    e = fetch_one("db/seeds/packs/skewb_sarah.tsv", CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE,
                  CT_STAGE_SARAH, "sarah.15", 15);
    CT_ASSERT_STR_EQ(e.alg, "R' F R F' (y2) R' F R F' (z2 x') R' F R F' (y2) R' F R F'");
}

CT_SUITE(pack_seed_l4e) {
    verify_pack("db/seeds/packs/pyraminx_l4e.tsv", CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E,
                35);
    ct_pack_entry e = fetch_one("db/seeds/packs/pyraminx_l4e.tsv", CT_EVENT_PYRAMINX, CT_METHOD_L4E,
                                CT_STAGE_L4E, "l4e.01", 35);
    CT_ASSERT_STR_EQ(e.alg, "(R U' R') U' (R U' R')");
    e = fetch_one("db/seeds/packs/pyraminx_l4e.tsv", CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E,
                  "l4e.06", 35);
    CT_ASSERT_STR_EQ(e.alg, "(R' L R L') U (R' L R L') U (R' L R L')");
    e = fetch_one("db/seeds/packs/pyraminx_l4e.tsv", CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E,
                  "l4e.18", 35);
    CT_ASSERT_STR_EQ(e.alg, "R U' R'");
}

CT_SUITE(pack_seed_4lll) {
    verify_pack("db/seeds/packs/megaminx_4lll.tsv", CT_EVENT_MEGAMINX, CT_METHOD_4LLL,
                CT_STAGE_4LLL, 39);
    ct_pack_entry e = fetch_one("db/seeds/packs/megaminx_4lll.tsv", CT_EVENT_MEGAMINX,
                                CT_METHOD_4LLL, CT_STAGE_4LLL, "4lll.01", 39);
    CT_ASSERT_STR_EQ(e.alg, "F (R U R' U') F'");
    e = fetch_one("db/seeds/packs/megaminx_4lll.tsv", CT_EVENT_MEGAMINX, CT_METHOD_4LLL,
                  CT_STAGE_4LLL, "4lll.27", 39);
    CT_ASSERT_STR_EQ(e.alg, "BR' (R' U L U') (R' U L' U') R2 BR");
    e = fetch_one("db/seeds/packs/megaminx_4lll.tsv", CT_EVENT_MEGAMINX, CT_METHOD_4LLL,
                  CT_STAGE_4LLL, "4lll.33", 39);
    CT_ASSERT_STR_EQ(e.alg, "[(R U R' U) (R' U' R2 U') (R' U R' U R) U] *2");
}
