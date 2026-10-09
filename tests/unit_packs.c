#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/packs/packs.h"
#include "../src/packs/provenance.h"
#include "../src/storage/db.h"
#include "framework.h"

static const char CT_SHA_EMPTY[] =
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
static const char CT_SHA_ABC[] = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
static const char CT_SHA_MSG448[] =
    "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";
static const char CT_SHA_A64[] = "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb";
static const char CT_SHA_TEST4[] =
    "594847328451bdfa85056225462cc1d867d877fb388df0ce35f25ab5562bfbb5";
static const char CT_SHA_PAT1000[] =
    "a9425c416f534025a4e2422bd14adba4ec3d4a68d10c3329be8df612964d2b6e";
static const char CT_SHA_CANON[] =
    "44fddd6c229984df998db23200a21f0c41c8d036940f55bb42e072e708233df3";
static const char CT_CANON_ENTRY[] = "16|10|14|opp_parity|/ (3,3) / (0,-1) /|{\"shape\":\"muffin\"}"
                                     "|https://www.cubestation.net/sq1/parity|2026-10-09T12:34:56Z";

static sqlite3 *open_db(void) {
    sqlite3 *db = NULL;
    ct_db_open(&db, ":memory:");
    ct_db_migrate(db, "db/migrations");
    return db;
}

static void fill_unterminated(char *dst, size_t n, char c) {
    memset(dst, (int)c, n);
}

static void fill_fit(char *dst, size_t n, char c) {
    memset(dst, (int)c, n);
    dst[n] = '\0';
}

static void query_text(sqlite3 *db, const char *sql, char *out, size_t cap) {
    sqlite3_stmt *st = NULL;
    out[0] = '\0';
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) {
            const unsigned char *s = sqlite3_column_text(st, 0);
            if (s != NULL)
                snprintf(out, cap, "%s", (const char *)s);
        }
    }
    sqlite3_finalize(st);
}

static ct_pack_entry make_entry(void) {
    ct_pack_entry e;
    memset(&e, 0, sizeof e);
    e.event = CT_EVENT_SQUARE1;
    e.method = CT_METHOD_YAU;
    e.stage = CT_STAGE_PARITY;
    strcpy(e.case_key, "opp_parity");
    strcpy(e.alg, "/ (3,3) / (0,-1) /");
    strcpy(e.front_json, "{\"shape\":\"muffin\"}");
    strcpy(e.source_url, "https://www.cubestation.net/sq1/parity");
    strcpy(e.source_at, "2026-10-09T12:34:56Z");
    return e;
}

CT_SUITE(packs_sha256) {
    char hex[CT_SHA_HEX_MAX];
    unsigned char a64[64];
    unsigned char pat640[640];
    unsigned char pat1000[1000];
    ct_sha256("", 0, hex);
    CT_ASSERT_STR_EQ(hex, CT_SHA_EMPTY);
    ct_sha256("abc", 3, hex);
    CT_ASSERT_STR_EQ(hex, CT_SHA_ABC);
    ct_sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, hex);
    CT_ASSERT_STR_EQ(hex, CT_SHA_MSG448);
    memset(a64, 'a', sizeof a64);
    ct_sha256(a64, sizeof a64, hex);
    CT_ASSERT_STR_EQ(hex, CT_SHA_A64);
    for (unsigned i = 0; i < sizeof pat640; i++)
        pat640[i] = (unsigned char)("01234567"[i % 8]);
    ct_sha256(pat640, sizeof pat640, hex);
    CT_ASSERT_STR_EQ(hex, CT_SHA_TEST4);
    for (unsigned i = 0; i < sizeof pat1000; i++)
        pat1000[i] = (unsigned char)((i * 7 + 3) % 251);
    ct_sha256(pat1000, sizeof pat1000, hex);
    CT_ASSERT_STR_EQ(hex, CT_SHA_PAT1000);
}

CT_SUITE(packs_source_record) {
    char out[CT_B128];
    ct_pack_source_record(out, (unsigned)sizeof out, "https://ex.example/a", "2026-01-02T03:04:05Z",
                          "R U R' U'");
    CT_ASSERT_STR_EQ(out, "https://ex.example/a|2026-01-02T03:04:05Z|R U R' U'");
    char tiny[CT_B8];
    ct_pack_source_record(tiny, (unsigned)sizeof tiny, "https://ex.example/a",
                          "2026-01-02T03:04:05Z", "R U R' U'");
    CT_ASSERT_EQ(strlen(tiny), (size_t)(sizeof tiny - 1));
    CT_ASSERT(strncmp(tiny, "https:/", sizeof tiny - 1) == 0);
    char one[2];
    ct_pack_source_record(one, (unsigned)sizeof one, "u", "r", "a");
    CT_ASSERT_EQ(one[0], 'u');
    CT_ASSERT_EQ(one[1], '\0');
}

CT_SUITE(packs_content_hash) {
    ct_pack_entry e = make_entry();
    char hex[CT_SHA_HEX_MAX];
    char direct[CT_SHA_HEX_MAX];
    CT_ASSERT_EQ(ct_pack_content_hash(&e, hex), CT_PACK_OK);
    CT_ASSERT_STR_EQ(hex, CT_SHA_CANON);
    ct_sha256(CT_CANON_ENTRY, (unsigned)strlen(CT_CANON_ENTRY), direct);
    CT_ASSERT_STR_EQ(hex, direct);
    strcpy(e.alg, "R U R' U' R' F R2 U' R' U' R U R' F'");
    char other[CT_SHA_HEX_MAX];
    CT_ASSERT_EQ(ct_pack_content_hash(&e, other), CT_PACK_OK);
    CT_ASSERT(strcmp(hex, other) != 0);
}

CT_SUITE(packs_upsert) {
    sqlite3 *db = open_db();
    ct_pack_entry e = make_entry();
    char hex[CT_SHA_HEX_MAX];
    char stored[CT_SHA_HEX_MAX];
    char sql[CT_B256];
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    CT_ASSERT_EQ(ct_pack_count(db, e.event, e.method, e.stage), 1);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, e.event, e.method, e.stage, e.case_key, &got), CT_PACK_OK);
    CT_ASSERT_EQ(got.event, e.event);
    CT_ASSERT_EQ(got.method, e.method);
    CT_ASSERT_EQ(got.stage, e.stage);
    CT_ASSERT_STR_EQ(got.case_key, e.case_key);
    CT_ASSERT_STR_EQ(got.alg, e.alg);
    CT_ASSERT_STR_EQ(got.front_json, e.front_json);
    CT_ASSERT_STR_EQ(got.source_url, e.source_url);
    CT_ASSERT_STR_EQ(got.source_at, e.source_at);
    CT_ASSERT_EQ(ct_pack_content_hash(&e, hex), CT_PACK_OK);
    snprintf(sql, sizeof sql,
             "SELECT content_hash FROM event_pack WHERE event = %d AND method = %d AND stage = %d "
             "AND case_key = 'opp_parity'",
             (int)e.event, (int)e.method, (int)e.stage);
    query_text(db, sql, stored, sizeof stored);
    CT_ASSERT_STR_EQ(stored, hex);
    ct_db_close(db);
}

CT_SUITE(packs_upsert_update) {
    sqlite3 *db = open_db();
    ct_pack_entry e = make_entry();
    char before[CT_SHA_HEX_MAX];
    char after[CT_SHA_HEX_MAX];
    char stored[CT_SHA_HEX_MAX];
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    CT_ASSERT_EQ(ct_pack_content_hash(&e, before), CT_PACK_OK);
    strcpy(e.alg, "/ (-3,0) / (1,2) / (-3,0) /");
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    CT_ASSERT_EQ(ct_pack_count(db, e.event, e.method, e.stage), 1);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, e.event, e.method, e.stage, e.case_key, &got), CT_PACK_OK);
    CT_ASSERT_STR_EQ(got.alg, e.alg);
    CT_ASSERT_EQ(ct_pack_content_hash(&e, after), CT_PACK_OK);
    CT_ASSERT(strcmp(before, after) != 0);
    query_text(db, "SELECT content_hash FROM event_pack", stored, sizeof stored);
    CT_ASSERT_STR_EQ(stored, after);
    ct_db_close(db);
}

CT_SUITE(packs_list) {
    sqlite3 *db = open_db();
    ct_pack_entry e = make_entry();
    ct_pack_entry out[CT_B8];
    strcpy(e.case_key, "c_case");
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    strcpy(e.case_key, "a_case");
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    strcpy(e.case_key, "b_case");
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    CT_ASSERT_EQ(ct_pack_count(db, e.event, e.method, e.stage), 3);
    int n = ct_pack_list(db, e.event, e.method, e.stage, out, (int)(sizeof out / sizeof out[0]));
    CT_ASSERT_EQ(n, 3);
    CT_ASSERT_STR_EQ(out[0].case_key, "a_case");
    CT_ASSERT_STR_EQ(out[1].case_key, "b_case");
    CT_ASSERT_STR_EQ(out[2].case_key, "c_case");
    n = ct_pack_list(db, e.event, e.method, e.stage, out, 2);
    CT_ASSERT_EQ(n, 2);
    CT_ASSERT_STR_EQ(out[0].case_key, "a_case");
    CT_ASSERT_STR_EQ(out[1].case_key, "b_case");
    n = ct_pack_list(db, e.event, e.method, e.stage, out, 0);
    CT_ASSERT_EQ(n, 0);
    ct_db_close(db);
}

CT_SUITE(packs_isolation) {
    sqlite3 *db = open_db();
    ct_pack_entry e = make_entry();
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    e.event = CT_EVENT_2X2;
    e.method = CT_METHOD_CLL;
    e.stage = CT_STAGE_CLL;
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_SQUARE1, CT_METHOD_YAU, CT_STAGE_PARITY), 1);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL), 1);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL, e.case_key, &got),
                 CT_PACK_OK);
    CT_ASSERT_EQ(got.event, CT_EVENT_2X2);
    CT_ASSERT_EQ(ct_pack_get(db, CT_EVENT_3X3, CT_METHOD_CLL, CT_STAGE_CLL, e.case_key, &got),
                 CT_PACK_MISSING);
    ct_db_close(db);
}

CT_SUITE(packs_bounds) {
    sqlite3 *db = open_db();
    ct_pack_entry e = make_entry();
    ct_pack_entry got;
    fill_fit(e.case_key, CT_PACK_CASE_KEY_MAX - 1, 'k');
    fill_fit(e.alg, CT_PACK_ALG_MAX - 1, 'a');
    fill_fit(e.front_json, CT_PACK_FRONT_JSON_MAX - 1, 'j');
    fill_fit(e.source_url, CT_PACK_URL_MAX - 1, 'u');
    fill_fit(e.source_at, CT_PACK_AT_MAX - 1, 't');
    CT_ASSERT_EQ(ct_pack_upsert(db, &e), CT_PACK_OK);
    CT_ASSERT_EQ(ct_pack_get(db, e.event, e.method, e.stage, e.case_key, &got), CT_PACK_OK);
    CT_ASSERT_EQ(strlen(got.case_key), (size_t)(CT_PACK_CASE_KEY_MAX - 1));
    CT_ASSERT_EQ(strlen(got.alg), (size_t)(CT_PACK_ALG_MAX - 1));
    CT_ASSERT_EQ(strlen(got.front_json), (size_t)(CT_PACK_FRONT_JSON_MAX - 1));
    CT_ASSERT_EQ(strlen(got.source_url), (size_t)(CT_PACK_URL_MAX - 1));
    CT_ASSERT_EQ(strlen(got.source_at), (size_t)(CT_PACK_AT_MAX - 1));
    ct_db_close(db);
}

CT_SUITE(packs_bad_args) {
    sqlite3 *db = open_db();
    ct_pack_entry e = make_entry();
    ct_pack_entry got;
    char hex[CT_SHA_HEX_MAX];
    CT_ASSERT(ct_pack_upsert(NULL, &e) < 0);
    CT_ASSERT(ct_pack_upsert(db, NULL) < 0);
    ct_pack_entry bad = make_entry();
    bad.case_key[0] = '\0';
    CT_ASSERT(ct_pack_upsert(db, &bad) < 0);
    bad = make_entry();
    fill_unterminated(bad.case_key, CT_PACK_CASE_KEY_MAX, 'k');
    CT_ASSERT(ct_pack_upsert(db, &bad) < 0);
    bad = make_entry();
    fill_unterminated(bad.alg, CT_PACK_ALG_MAX, 'a');
    CT_ASSERT(ct_pack_upsert(db, &bad) < 0);
    bad = make_entry();
    fill_unterminated(bad.front_json, CT_PACK_FRONT_JSON_MAX, 'j');
    CT_ASSERT(ct_pack_upsert(db, &bad) < 0);
    bad = make_entry();
    fill_unterminated(bad.source_url, CT_PACK_URL_MAX, 'u');
    CT_ASSERT(ct_pack_upsert(db, &bad) < 0);
    bad = make_entry();
    fill_unterminated(bad.source_at, CT_PACK_AT_MAX, 't');
    CT_ASSERT(ct_pack_upsert(db, &bad) < 0);
    CT_ASSERT(ct_pack_get(NULL, e.event, e.method, e.stage, e.case_key, &got) < 0);
    CT_ASSERT(ct_pack_get(db, e.event, e.method, e.stage, e.case_key, NULL) < 0);
    CT_ASSERT(ct_pack_get(db, e.event, e.method, e.stage, "", &got) < 0);
    CT_ASSERT(ct_pack_get(db, e.event, e.method, e.stage, NULL, &got) < 0);
    CT_ASSERT(ct_pack_count(NULL, e.event, e.method, e.stage) < 0);
    CT_ASSERT(ct_pack_list(NULL, e.event, e.method, e.stage, &got, 1) < 0);
    CT_ASSERT(ct_pack_list(db, e.event, e.method, e.stage, NULL, 1) < 0);
    CT_ASSERT(ct_pack_list(db, e.event, e.method, e.stage, &got, -1) < 0);
    CT_ASSERT(ct_pack_content_hash(NULL, hex) < 0);
    CT_ASSERT(ct_pack_content_hash(&e, NULL) < 0);
    ct_db_close(db);
}
