#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/packs/packs.h"
#include "../src/storage/db.h"
#include "framework.h"

enum { CT_SEED2_LIST_CAP = 192 };

static sqlite3 *open_db(void) {
    sqlite3 *db = NULL;
    ct_db_open(&db, ":memory:");
    ct_db_migrate(db, "db/migrations");
    return db;
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

static void verify_pack(const char *path, ct_event ev, ct_method m, ct_stage st, int expected) {
    sqlite3 *db = open_db();
    CT_ASSERT_EQ(ct_pack_seed_file(db, path, ev, m, st), expected);
    CT_ASSERT_EQ(ct_pack_count(db, ev, m, st), expected);
    CT_ASSERT_EQ(tsv_data_lines(path), expected);
    ct_pack_entry got[CT_SEED2_LIST_CAP];
    CT_ASSERT(expected <= CT_SEED2_LIST_CAP);
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

CT_SUITE(pack_seed_sq1_cubeshape) {
    verify_pack("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                CT_STAGE_CUBESHAPE, 169);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_CUBESHAPE, "cs.001", 169);
    CT_ASSERT_STR_EQ(e.alg, "/");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Kite / Kite\"}");
    e = fetch_one("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                  CT_STAGE_CUBESHAPE, "cs.011", 169);
    CT_ASSERT_STR_EQ(e.alg, "/ 1,2 / -3,-3 /");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Scallop / Scallop\"}");
    e = fetch_one("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                  CT_STAGE_CUBESHAPE, "cs.012", 169);
    CT_ASSERT_STR_EQ(e.alg, "/ 2,1 / 0,3 /");
}

CT_SUITE(pack_seed_sq1_co) {
    verify_pack("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO, 7);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_CO, "co.01", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0)/(3,0)/(-1,0)");
    e = fetch_one("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO,
                  "co.03", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0)/(-1,0)");
    e = fetch_one("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO,
                  "co.07", 7);
    CT_ASSERT_STR_EQ(e.alg, "/ (6,6) / (-1,1)");
}

CT_SUITE(pack_seed_sq1_eo) {
    verify_pack("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO, 7);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_EO, "eo.01", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0) / (3,0) / (3,0) / (-1,-1) / (-2,1) / (-3,0) /");
    e = fetch_one("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO,
                  "eo.03", 7);
    CT_ASSERT_STR_EQ(e.alg, "(0,-1) / (0,-3) / (0,-3) / (1,1) / (-1,2) / (1,4) / (-1,0)");
    e = fetch_one("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO,
                  "eo.07", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0) / (-1,-1) / (3,3) / (1,1) / (-1,0)");
}

CT_SUITE(pack_seed_sq1_cp) {
    verify_pack("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP, 16);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_CP, "cp.01", 16);
    CT_ASSERT_STR_EQ(e.alg, "/ (3,-3) / (-3,0) / (0,3) / (0,-3) / (0,3) /");
    e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP,
                  "cp.08", 16);
    CT_ASSERT_STR_EQ(e.alg, "/ (3,-3) / (-3,3) /");
    e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP,
                  "cpp.01", 16);
    CT_ASSERT_STR_EQ(e.alg,
                     "/ (3,3) / (-1,0) / (2,0) / (-4,0) / (4,0) / (2,0) / (1,0) / (-3,-3) /");
    e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP,
                  "cpp.08", 16);
    CT_ASSERT_STR_EQ(e.alg, "/ (3,3) / (1,0) / (4,-2) / (2,-4) / (0,-4) / (3,3) / (3,0) / (3,3) /");
}

CT_SUITE(pack_seed_sq1_ep) {
    verify_pack("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP, 99);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_EP, "ep.01", 99);
    CT_ASSERT_STR_EQ(e.alg, "/ (-3,0) / (0,3) / (0,-3) / (0,3) / (2,0) / (0,2) / (-2,0) / (4,0)"
                            " / (0,-2) / (0,2) / (-1,4) / (0,-3) / (0,3) *");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Adj on U\",\"parity\":true}");
    e = fetch_one("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP,
                  "ep.10", 99);
    CT_ASSERT_STR_EQ(e.alg, "/ (0,3) / (-3,0) / (3,0) / (-3,0) / (0,-2) / (-2,0) / (0,2) / (0,-4)"
                            " / (2,0) / (-2,0) / (-4,1) / (3,0) / (-3,0) *");
    e = fetch_one("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP,
                  "ep.99", 99);
    CT_ASSERT_STR_EQ(e.alg,
                     "(U) (1,0) / (5,-1) / (3,3) / (3,0) / (1,1) / (-3,0) / (3,-3) / (5,0) *");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"W-W\"}");
}
