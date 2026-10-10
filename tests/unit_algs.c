#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/packs/provenance.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_algs.h"
#include "framework.h"

static sqlite3 *algs_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static ct_generated_alg base_alg(const char *pair) {
    ct_generated_alg a;
    memset(&a, 0, sizeof a);
    a.orbit = CT_ORBIT_CORNER;
    a.buffer_mode = CT_BUFFER_FIXED;
    a.category = 0;
    a.classification = CT_CLS_PURE;
    a.qtm = 11;
    a.score = 13.75;
    snprintf(a.target_pair, sizeof a.target_pair, "%s", pair);
    snprintf(a.sequence, sizeof a.sequence, "%s", "[R U : [D2, R U R']]");
    snprintf(a.setup, sizeof a.setup, "%s", "R U");
    snprintf(a.interchange, sizeof a.interchange, "%s", "D2");
    snprintf(a.insertion, sizeof a.insertion, "%s", "R U R'");
    snprintf(a.score_parts_json, sizeof a.score_parts_json, "%s",
             "{\"len\":11,\"subset\":0,\"regrip\":2,\"finger\":1.5}");
    return a;
}

static long put_pair(sqlite3 *db, const char *hash, int orbit, const char *pair) {
    ct_generated_alg a = base_alg(pair);
    a.orbit = orbit;
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, hash), CT_REPO_OK);
    ct_generated_alg back;
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_alg_get(db, orbit, pair, CT_BUFFER_FIXED, 0, hash, &back), CT_REPO_OK);
    return back.id;
}

CT_SUITE(alg_hash_deterministic) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT_STR_EQ(a, b);
    CT_ASSERT_EQ(strlen(a), 64u);
    for (unsigned i = 0; i < CT_SHA_HEX_MAX - 1; i++) {
        CT_ASSERT((a[i] >= '0' && a[i] <= '9') || (a[i] >= 'a' && a[i] <= 'f'));
    }
}

CT_SUITE(alg_hash_scheme_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 3, CT_BUFFER_FIXED, a);
    ct_params_hash(&cfg, 4, CT_BUFFER_FIXED, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_mode_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    ct_params_hash(&cfg, 1, CT_BUFFER_FLOATING, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_mask_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    cfg.gen.preferred_moves |= CT_MOVE_BIT_B;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_fsrs_inert) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    cfg.fsrs.w[3] += 0.5;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT_STR_EQ(a, b);
}

CT_SUITE(alg_hash_penalty_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    cfg.gen.penalty[4] += 1.0;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_finger_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    cfg.gen.finger_cost[2][CT_FINGER_DIR_CCW] += 0.25;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_bound_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    cfg.gen.setup_bfs_depth = CT_GEN_SETUP_BFS_DEPTH + 1;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_factor_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    cfg.gen.weight_finger = 0.75;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_candidate_differs) {
    ct_config cfg = ct_config_defaults();
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, a);
    cfg.gen.candidate_max = CT_GEN_CANDIDATE_MAX + 1;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_format_pinned) {
    ct_config cfg = ct_config_defaults();
    static const char canon[] =
        "7|0|b|"
        "0.000000,0.000000,2.000000,0.000000,2.000000,2.000000,2.000000,2.000000,2.000000,2.000000,"
        "2.000000,2.000000|"
        "0.200000,0.200000,0.200000,0.200000,0.400000,0.400000,0.400000,0.400000,0.800000,0.800000,"
        "0.800000,0.800000|"
        "4|5|12|1.000000|2.000000|1.500000|0.500000";
    char want[CT_SHA_HEX_MAX];
    ct_sha256(canon, (unsigned)(sizeof canon - 1), want);
    char got[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 7, CT_BUFFER_FIXED, got);
    CT_ASSERT_STR_EQ(got, want);
}

CT_SUITE(alg_put_get_roundtrip) {
    sqlite3 *db = algs_db();
    ct_generated_alg a = base_alg("UB");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, "hash-a"), CT_REPO_OK);

    ct_generated_alg b;
    memset(&b, 0xAA, sizeof b);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "hash-a", &b),
                 CT_REPO_OK);
    CT_ASSERT(b.id > 0);
    CT_ASSERT_EQ(b.orbit, CT_ORBIT_CORNER);
    CT_ASSERT_STR_EQ(b.target_pair, "UB");
    CT_ASSERT_EQ(b.buffer_mode, CT_BUFFER_FIXED);
    CT_ASSERT_EQ(b.category, 0);
    CT_ASSERT_STR_EQ(b.sequence, "[R U : [D2, R U R']]");
    CT_ASSERT_EQ(b.classification, CT_CLS_PURE);
    CT_ASSERT_STR_EQ(b.setup, "R U");
    CT_ASSERT_STR_EQ(b.interchange, "D2");
    CT_ASSERT_STR_EQ(b.insertion, "R U R'");
    CT_ASSERT_EQ(b.qtm, 11);
    CT_ASSERT_DBL_EQ(b.score, 13.75, 1e-9);
    CT_ASSERT_STR_EQ(b.score_parts_json, "{\"len\":11,\"subset\":0,\"regrip\":2,\"finger\":1.5}");
    CT_ASSERT_EQ(strlen(b.created_at), 20u);

    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "hash-b", &b),
                 CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_EDGE, "UB", CT_BUFFER_FIXED, 0, "hash-a", &b),
                 CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FLOATING, 0, "hash-a", &b),
                 CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 1, "hash-a", &b),
                 CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "AA", CT_BUFFER_FIXED, 0, "hash-a", &b),
                 CT_REPO_MISSING);
    ct_db_close(db);
}

CT_SUITE(alg_put_upsert) {
    sqlite3 *db = algs_db();
    ct_generated_alg a = base_alg("UB");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, "hash-a"), CT_REPO_OK);
    ct_generated_alg first;
    memset(&first, 0, sizeof first);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "hash-a", &first),
                 CT_REPO_OK);
    CT_ASSERT_EQ(strlen(first.created_at), 20u);

    ct_generated_alg upd = base_alg("UB");
    upd.classification = CT_CLS_A9;
    upd.qtm = 9;
    upd.score = 9.5;
    snprintf(upd.sequence, sizeof upd.sequence, "%s", "D2 R U R' D2");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &upd, "hash-a"), CT_REPO_OK);

    ct_generated_alg second;
    memset(&second, 0, sizeof second);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "hash-a", &second),
                 CT_REPO_OK);
    CT_ASSERT_EQ(second.id, first.id);
    CT_ASSERT_EQ(second.classification, CT_CLS_A9);
    CT_ASSERT_EQ(second.qtm, 9);
    CT_ASSERT_DBL_EQ(second.score, 9.5, 1e-9);
    CT_ASSERT_STR_EQ(second.sequence, "D2 R U R' D2");
    CT_ASSERT_STR_EQ(second.created_at, first.created_at);
    CT_ASSERT_EQ(ct_repo_alg_count(db, "hash-a"), 1);
    ct_db_close(db);
}

CT_SUITE(alg_count_list) {
    sqlite3 *db = algs_db();
    put_pair(db, "h1", CT_ORBIT_CORNER, "UB");
    put_pair(db, "h1", CT_ORBIT_CORNER, "MB");
    put_pair(db, "h1", CT_ORBIT_CORNER, "AB");
    put_pair(db, "h1", CT_ORBIT_EDGE, "UF");
    put_pair(db, "h2", CT_ORBIT_CORNER, "UB");
    put_pair(db, "h2", CT_ORBIT_CORNER, "AA");

    CT_ASSERT_EQ(ct_repo_alg_count(db, "h1"), 4);
    CT_ASSERT_EQ(ct_repo_alg_count(db, "h2"), 2);
    CT_ASSERT_EQ(ct_repo_alg_count(db, "h3"), 0);

    ct_generated_alg rows[8];
    memset(rows, 0, sizeof rows);
    CT_ASSERT_EQ(ct_repo_alg_list(db, "h1", CT_ORBIT_CORNER, rows, 8), 3);
    CT_ASSERT_STR_EQ(rows[0].target_pair, "AB");
    CT_ASSERT_STR_EQ(rows[1].target_pair, "MB");
    CT_ASSERT_STR_EQ(rows[2].target_pair, "UB");
    CT_ASSERT_EQ(rows[0].orbit, CT_ORBIT_CORNER);
    CT_ASSERT_STR_EQ(rows[0].sequence, "[R U : [D2, R U R']]");

    CT_ASSERT_EQ(ct_repo_alg_list(db, "h1", CT_ORBIT_EDGE, rows, 8), 1);
    CT_ASSERT_STR_EQ(rows[0].target_pair, "UF");

    CT_ASSERT_EQ(ct_repo_alg_list(db, "h2", CT_ORBIT_CORNER, rows, 8), 2);
    CT_ASSERT_STR_EQ(rows[0].target_pair, "AA");
    CT_ASSERT_STR_EQ(rows[1].target_pair, "UB");

    memset(rows, 0, sizeof rows);
    CT_ASSERT_EQ(ct_repo_alg_list(db, "h1", CT_ORBIT_CORNER, rows, 2), 2);
    CT_ASSERT_STR_EQ(rows[0].target_pair, "AB");
    CT_ASSERT_STR_EQ(rows[1].target_pair, "MB");
    CT_ASSERT_EQ(ct_repo_alg_list(db, "h1", CT_ORBIT_CORNER, rows, 0), 0);
    ct_db_close(db);
}

CT_SUITE(alg_stale) {
    sqlite3 *db = algs_db();
    long i1 = put_pair(db, "h1", CT_ORBIT_CORNER, "UB");
    long i2 = put_pair(db, "h1", CT_ORBIT_CORNER, "AB");
    long j1 = put_pair(db, "h2", CT_ORBIT_CORNER, "UB");
    long j2 = put_pair(db, "h2", CT_ORBIT_CORNER, "AB");
    long j3 = put_pair(db, "h2", CT_ORBIT_EDGE, "UF");

    int ids[8];
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h1", ids, 8), 3);
    CT_ASSERT_EQ(ids[0], j1);
    CT_ASSERT_EQ(ids[1], j2);
    CT_ASSERT_EQ(ids[2], j3);

    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h2", ids, 8), 2);
    CT_ASSERT_EQ(ids[0], i1);
    CT_ASSERT_EQ(ids[1], i2);

    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h3", ids, 8), 5);
    CT_ASSERT_EQ(ids[0], i1);
    CT_ASSERT_EQ(ids[1], i2);
    CT_ASSERT_EQ(ids[2], j1);
    CT_ASSERT_EQ(ids[3], j2);
    CT_ASSERT_EQ(ids[4], j3);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h1", ids, 2), 2);
    CT_ASSERT_EQ(ids[0], j1);
    CT_ASSERT_EQ(ids[1], j2);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h1", ids, 0), 0);
    ct_db_close(db);
}

CT_SUITE(alg_drop) {
    sqlite3 *db = algs_db();
    put_pair(db, "h1", CT_ORBIT_CORNER, "UB");
    put_pair(db, "h1", CT_ORBIT_EDGE, "UF");
    put_pair(db, "h2", CT_ORBIT_CORNER, "UB");
    put_pair(db, "h2", CT_ORBIT_EDGE, "UF");
    put_pair(db, "h2", CT_ORBIT_CORNER, "AB");

    CT_ASSERT_EQ(ct_repo_alg_drop(db, "h2"), 3);
    CT_ASSERT_EQ(ct_repo_alg_count(db, "h2"), 0);
    CT_ASSERT_EQ(ct_repo_alg_count(db, "h1"), 2);
    int ids[8];
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h1", ids, 8), 0);

    CT_ASSERT_EQ(ct_repo_alg_drop(db, "h2"), 0);
    CT_ASSERT_EQ(ct_repo_alg_drop(db, "h9"), 0);

    put_pair(db, "h3", CT_ORBIT_CORNER, "UB");
    put_pair(db, "h3", CT_ORBIT_EDGE, "UF");
    CT_ASSERT_EQ(ct_repo_alg_drop_orbit(db, "h3", CT_ORBIT_EDGE), 1);
    CT_ASSERT_EQ(ct_repo_alg_count(db, "h3"), 1);
    ct_generated_alg left;
    memset(&left, 0, sizeof left);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "h3", &left),
                 CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_EDGE, "UF", CT_BUFFER_FIXED, 0, "h3", &left),
                 CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_repo_alg_drop_orbit(db, "h3", CT_ORBIT_EDGE), 0);
    CT_ASSERT_EQ(ct_repo_alg_drop_orbit(db, "h4", CT_ORBIT_CORNER), 0);

    char big_hash[CT_ALG_HASH_HEX_MAX + 1];
    memset(big_hash, 'a', sizeof big_hash - 1);
    big_hash[sizeof big_hash - 1] = '\0';
    CT_ASSERT_EQ(ct_repo_alg_drop(NULL, "h1"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_drop(db, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_drop(db, ""), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_drop(db, big_hash), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_drop_orbit(NULL, "h1", CT_ORBIT_CORNER), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_drop_orbit(db, NULL, CT_ORBIT_CORNER), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_drop_orbit(db, big_hash, CT_ORBIT_CORNER), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_count(db, "h1"), 2);
    ct_db_close(db);
}

CT_SUITE(alg_text_bounds) {
    sqlite3 *db = algs_db();
    ct_generated_alg full = base_alg("UB");
    memset(full.sequence, 'R', CT_ALG_SEQ_MAX - 1);
    full.sequence[CT_ALG_SEQ_MAX - 1] = '\0';
    memset(full.setup, 'U', CT_ALG_SETUP_MAX - 1);
    full.setup[CT_ALG_SETUP_MAX - 1] = '\0';
    memset(full.interchange, 'D', CT_ALG_INTER_MAX - 1);
    full.interchange[CT_ALG_INTER_MAX - 1] = '\0';
    memset(full.insertion, 'F', CT_ALG_INSERT_MAX - 1);
    full.insertion[CT_ALG_INSERT_MAX - 1] = '\0';
    memset(full.score_parts_json, 'j', CT_ALG_PARTS_MAX - 1);
    full.score_parts_json[CT_ALG_PARTS_MAX - 1] = '\0';
    CT_ASSERT_EQ(ct_repo_alg_put(db, &full, "h1"), CT_REPO_OK);
    ct_generated_alg back;
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "h1", &back),
                 CT_REPO_OK);
    CT_ASSERT_EQ(strlen(back.sequence), (size_t)(CT_ALG_SEQ_MAX - 1));
    CT_ASSERT_EQ(strlen(back.setup), (size_t)(CT_ALG_SETUP_MAX - 1));
    CT_ASSERT_EQ(strlen(back.interchange), (size_t)(CT_ALG_INTER_MAX - 1));
    CT_ASSERT_EQ(strlen(back.insertion), (size_t)(CT_ALG_INSERT_MAX - 1));
    CT_ASSERT_EQ(strlen(back.score_parts_json), (size_t)(CT_ALG_PARTS_MAX - 1));
    CT_ASSERT_STR_EQ(back.sequence, full.sequence);

    ct_generated_alg unterm = base_alg("UF");
    memset(unterm.sequence, 'R', sizeof unterm.sequence);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &unterm, "h1"), CT_REPO_ERR_ARG);
    unterm = base_alg("UR");
    memset(unterm.setup, 'U', sizeof unterm.setup);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &unterm, "h1"), CT_REPO_ERR_ARG);
    unterm = base_alg("UL");
    memset(unterm.interchange, 'D', sizeof unterm.interchange);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &unterm, "h1"), CT_REPO_ERR_ARG);
    unterm = base_alg("UB2");
    memset(unterm.insertion, 'F', sizeof unterm.insertion);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &unterm, "h1"), CT_REPO_ERR_ARG);
    unterm = base_alg("UB3");
    memset(unterm.score_parts_json, 'j', sizeof unterm.score_parts_json);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &unterm, "h1"), CT_REPO_ERR_ARG);
    unterm = base_alg("UB4");
    memset(unterm.target_pair, 'c', sizeof unterm.target_pair);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &unterm, "h1"), CT_REPO_ERR_ARG);

    ct_generated_alg empty_seq = base_alg("UB5");
    empty_seq.sequence[0] = '\0';
    CT_ASSERT_EQ(ct_repo_alg_put(db, &empty_seq, "h1"), CT_REPO_ERR_ARG);

    ct_generated_alg optional_empty = base_alg("UB6");
    optional_empty.setup[0] = '\0';
    optional_empty.interchange[0] = '\0';
    optional_empty.insertion[0] = '\0';
    optional_empty.score_parts_json[0] = '\0';
    CT_ASSERT_EQ(ct_repo_alg_put(db, &optional_empty, "h1"), CT_REPO_OK);
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB6", CT_BUFFER_FIXED, 0, "h1", &back),
                 CT_REPO_OK);
    CT_ASSERT_EQ(back.setup[0], '\0');
    CT_ASSERT_EQ(back.interchange[0], '\0');
    CT_ASSERT_EQ(back.insertion[0], '\0');
    CT_ASSERT_EQ(back.score_parts_json[0], '\0');
    CT_ASSERT_EQ(ct_repo_alg_count(db, "h1"), 2);
    ct_db_close(db);
}

CT_SUITE(alg_malformed) {
    sqlite3 *db = algs_db();
    ct_generated_alg a = base_alg("UB");
    char big_hash[CT_ALG_HASH_HEX_MAX + 1];
    memset(big_hash, 'a', sizeof big_hash - 1);
    big_hash[sizeof big_hash - 1] = '\0';
    char full_hash[CT_ALG_HASH_HEX_MAX];
    memset(full_hash, 'b', CT_SHA_HEX_MAX - 1);
    full_hash[CT_SHA_HEX_MAX - 1] = '\0';

    CT_ASSERT_EQ(ct_repo_alg_put(NULL, &a, "h1"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_put(db, NULL, "h1"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, ""), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, big_hash), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, full_hash), CT_REPO_OK);

    ct_generated_alg empty_pair = base_alg("");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &empty_pair, "h1"), CT_REPO_ERR_ARG);

    ct_generated_alg out;
    char big_pair[CT_ALG_PAIR_MAX + 1];
    memset(big_pair, 'c', sizeof big_pair - 1);
    big_pair[sizeof big_pair - 1] = '\0';
    CT_ASSERT_EQ(ct_repo_alg_get(NULL, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "h1", &out),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "h1", NULL),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "", CT_BUFFER_FIXED, 0, "h1", &out),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, big_pair, CT_BUFFER_FIXED, 0, "h1", &out),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, "", &out),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "UB", CT_BUFFER_FIXED, 0, big_hash, &out),
                 CT_REPO_ERR_ARG);

    CT_ASSERT_EQ(ct_repo_alg_count(NULL, "h1"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_count(db, ""), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_count(db, big_hash), CT_REPO_ERR_ARG);

    ct_generated_alg rows[2];
    CT_ASSERT_EQ(ct_repo_alg_list(NULL, "h1", CT_ORBIT_CORNER, rows, 2), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_list(db, "h1", CT_ORBIT_CORNER, NULL, 2), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_list(db, "h1", CT_ORBIT_CORNER, rows, -1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_list(db, "", CT_ORBIT_CORNER, rows, 2), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_list(db, big_hash, CT_ORBIT_CORNER, rows, 2), CT_REPO_ERR_ARG);

    int ids[2];
    CT_ASSERT_EQ(ct_repo_alg_stale(NULL, "h1", ids, 2), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h1", NULL, 2), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "h1", ids, -1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "", ids, 2), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, big_hash, ids, 2), CT_REPO_ERR_ARG);

    CT_ASSERT_EQ(ct_repo_alg_count(db, "h1"), 0);
    ct_db_close(db);
}
