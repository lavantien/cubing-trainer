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

static void identity_map(ct_letter_map *m) {
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        m->corner[i] = (char)(CT_LETTER_FIRST + i);
        m->edge[i] = (char)(CT_LETTER_FIRST + i);
    }
}

CT_SUITE(alg_hash_deterministic) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT_STR_EQ(a, b);
    CT_ASSERT_EQ(strlen(a), 64u);
    for (unsigned i = 0; i < CT_SHA_HEX_MAX - 1; i++) {
        CT_ASSERT((a[i] >= '0' && a[i] <= '9') || (a[i] >= 'a' && a[i] <= 'f'));
    }
}

CT_SUITE(alg_hash_scheme_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 3, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    ct_params_hash(&cfg, 4, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_mode_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    ct_params_hash(&cfg, 1, CT_BUFFER_FLOATING, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER,
                   &map, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_mask_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    cfg.gen.preferred_moves |= CT_MOVE_BIT_B;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_fsrs_inert) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    cfg.fsrs.w[3] += 0.5;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT_STR_EQ(a, b);
}

CT_SUITE(alg_hash_penalty_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    cfg.gen.penalty[4] += 1.0;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_finger_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    cfg.gen.finger_cost[2][CT_FINGER_DIR_CCW] += 0.25;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_bound_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    cfg.gen.setup_bfs_depth = CT_GEN_SETUP_BFS_DEPTH + 1;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_factor_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    cfg.gen.weight_finger = 0.75;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_candidate_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    cfg.gen.candidate_max = CT_GEN_CANDIDATE_MAX + 1;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_buffer_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, 0, CT_3STYLE_EDGE_BUFFER, &map, a);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, 2, CT_3STYLE_EDGE_BUFFER, &map, b);
    CT_ASSERT(strcmp(a, b) != 0);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, 2, &map, a);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, 5, &map, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_map_differs) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char a[CT_ALG_HASH_HEX_MAX];
    char b[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   a);
    char swapped = map.corner[1];
    map.corner[1] = map.corner[2];
    map.corner[2] = swapped;
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
    map.corner[1] = (char)(CT_LETTER_FIRST + 1);
    map.corner[2] = (char)(CT_LETTER_FIRST + 2);
    map.edge[7] = '\0';
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   b);
    CT_ASSERT(strcmp(a, b) != 0);
    ct_letter_map tail_a;
    ct_letter_map tail_b;
    identity_map(&tail_a);
    identity_map(&tail_b);
    tail_a.corner[5] = '\0';
    tail_a.corner[6] = 'G';
    tail_b.corner[5] = '\0';
    tail_b.corner[6] = 'H';
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER,
                   &tail_a, a);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER,
                   &tail_b, b);
    CT_ASSERT(strcmp(a, b) != 0);
}

CT_SUITE(alg_hash_format_pinned) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    static const char canon[] =
        "7|0|0|2|b|"
        "0.000000,0.000000,2.000000,0.000000,2.000000,2.000000,2.000000,2.000000,2.000000,2.000000,"
        "2.000000,2.000000|"
        "0.200000,0.200000,0.200000,0.200000,0.400000,0.400000,0.400000,0.400000,0.800000,0.800000,"
        "0.800000,0.800000|"
        "4|5|12|1.000000|2.000000|1.500000|0.500000|"
        "ABCDEFGHIJKLMNOPQRSTUVWX|ABCDEFGHIJKLMNOPQRSTUVWX";
    char want[CT_SHA_HEX_MAX];
    ct_sha256(canon, (unsigned)(sizeof canon - 1), want);
    char got[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 7, CT_BUFFER_FIXED, 0, 2, &map, got);
    CT_ASSERT_STR_EQ(got, want);
}

static size_t alg_canon_need(const ct_config *cfg, int scheme_id, ct_buffer_mode mode, int bc,
                             int be) {
    char probe[2048];
    size_t need = 0;
    need += (size_t)snprintf(probe, sizeof probe, "%d|%d|%d|%d|%x|", scheme_id, (int)mode, bc, be,
                             cfg->gen.preferred_moves);
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        need +=
            (size_t)snprintf(probe, sizeof probe, i == 0 ? "%.6f" : ",%.6f", cfg->gen.penalty[i]);
    }
    need += (size_t)snprintf(probe, sizeof probe, "|");
    int finger_total = CT_FINGER_FACE_COUNT * CT_FINGER_DIR_COUNT;
    for (int k = 0; k < finger_total; k++) {
        int face = k / CT_FINGER_DIR_COUNT;
        int dir = k % CT_FINGER_DIR_COUNT;
        need += (size_t)snprintf(probe, sizeof probe, k == 0 ? "%.6f" : ",%.6f",
                                 cfg->gen.finger_cost[face][dir]);
    }
    need += (size_t)snprintf(probe, sizeof probe, "|%d|%d|%d|%.6f|%.6f|%.6f|%.6f",
                             cfg->gen.setup_bfs_depth, cfg->gen.fallback_depth,
                             cfg->gen.candidate_max, cfg->gen.weight_len, cfg->gen.weight_subset,
                             cfg->gen.weight_regrip, cfg->gen.weight_finger);
    need += 1 + CT_LETTER_STICKERS + 1 + CT_LETTER_STICKERS;
    return need;
}

static void extreme_cfg(ct_config *cfg) {
    *cfg = ct_config_defaults();
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        cfg->gen.penalty[i] = 1.7e308;
    }
    for (int f = 0; f < CT_FINGER_FACE_COUNT; f++) {
        for (int d = 0; d < CT_FINGER_DIR_COUNT; d++) {
            cfg->gen.finger_cost[f][d] = 1.7e308;
        }
    }
    cfg->gen.weight_len = 1.7e308;
    cfg->gen.weight_subset = 1.7e308;
    cfg->gen.weight_regrip = 1.7e308;
    cfg->gen.weight_finger = 1.7e308;
}

CT_SUITE(alg_hash_fail_closed_on_oversize) {
    ct_config cfg;
    extreme_cfg(&cfg);
    ct_letter_map map_a;
    ct_letter_map map_b;
    identity_map(&map_a);
    identity_map(&map_b);
    map_b.corner[11] = 'Y';
    map_b.corner[12] = 'L';
    char ha[CT_ALG_HASH_HEX_MAX];
    char hb[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map_a,
                   ha);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map_b,
                   hb);
    CT_ASSERT_EQ(strlen(ha), 0u);
    CT_ASSERT_EQ(strlen(hb), 0u);

    sqlite3 *db = algs_db();
    ct_generated_alg a = base_alg("AB");
    CT_ASSERT_EQ(ct_repo_alg_put(db, &a, ha), CT_REPO_ERR_ARG);
    ct_generated_alg out;
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "AB", CT_BUFFER_FIXED, 0, ha, &out),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_count(db, ha), CT_REPO_ERR_ARG);
    ct_db_close(db);

    ct_config bounded = ct_config_defaults();
    bounded.gen.penalty[3] = CT_GEN_DOUBLE_MAX;
    bounded.gen.finger_cost[4][1] = CT_GEN_DOUBLE_MAX;
    bounded.gen.weight_len = CT_GEN_DOUBLE_MAX;
    ct_params_hash(&bounded, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER,
                   &map_a, ha);
    ct_params_hash(&bounded, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER,
                   &map_b, hb);
    CT_ASSERT_EQ(strlen(ha), 64u);
    CT_ASSERT_EQ(strlen(hb), 64u);
    CT_ASSERT(strcmp(ha, hb) != 0);
    CT_ASSERT(alg_canon_need(&bounded, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER,
                             CT_3STYLE_EDGE_BUFFER) < CT_ALG_CANON_MAX);
}

CT_SUITE(alg_hash_canonical_boundary) {
    ct_config base = ct_config_defaults();
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        base.gen.penalty[i] = 1e308;
    }
    for (int f = 0; f < CT_FINGER_FACE_COUNT; f++) {
        for (int d = 0; d < CT_FINGER_DIR_COUNT; d++) {
            base.gen.finger_cost[f][d] = 1e308;
        }
    }
    base.gen.weight_len = 1e308;
    ct_letter_map map;
    identity_map(&map);
    char hex[CT_ALG_HASH_HEX_MAX];
    int below = -1;
    int at_or_above = -1;
    double knob = 1.0;
    for (int k = 0; k <= 308; k++) {
        ct_config cfg = base;
        cfg.gen.weight_finger = knob;
        size_t need = alg_canon_need(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER,
                                     CT_3STYLE_EDGE_BUFFER);
        ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER,
                       &map, hex);
        if (need < CT_ALG_CANON_MAX) {
            CT_ASSERT_EQ(strlen(hex), 64u);
            below = k;
        } else {
            CT_ASSERT_EQ(strlen(hex), 0u);
            if (at_or_above < 0) {
                at_or_above = k;
            }
        }
        knob *= 10.0;
    }
    CT_ASSERT(below >= 0);
    CT_ASSERT(at_or_above >= 0);
    CT_ASSERT_EQ(at_or_above, below + 1);
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
    CT_ASSERT_EQ(ct_repo_alg_drop_orbit(db, "", CT_ORBIT_CORNER), CT_REPO_ERR_ARG);
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

CT_SUITE(alg_read_boundaries) {
    sqlite3 *db = algs_db();
    ct_letter_map map;
    identity_map(&map);
    char long_seq[CT_ALG_SEQ_MAX + 1];
    memset(long_seq, 'R', CT_ALG_SEQ_MAX);
    long_seq[CT_ALG_SEQ_MAX] = '\0';
    char long_json[CT_ALG_PARTS_MAX + 1];
    memset(long_json, 'j', CT_ALG_PARTS_MAX);
    long_json[CT_ALG_PARTS_MAX] = '\0';
    char *err = NULL;
    int rc = sqlite3_exec(db,
                          "INSERT INTO generated_alg(orbit, target_pair, buffer_mode, category,"
                          " sequence, classification, setup, interchange, insertion, qtm, score,"
                          " score_parts_json, params_hash, created_at)"
                          " VALUES(1, 'ZZ', 0, 0, 'x', 0, NULL, NULL, NULL, 1, 1.0, NULL, 'hx',"
                          " '2026-01-01T00:00:00Z')",
                          NULL, NULL, &err);
    sqlite3_free(err);
    CT_ASSERT_EQ(rc, SQLITE_OK);
    sqlite3_stmt *st = NULL;
    CT_ASSERT_EQ(sqlite3_prepare_v2(db,
                                    "UPDATE generated_alg SET sequence = ?, score_parts_json = ?"
                                    " WHERE target_pair = 'ZZ'",
                                    -1, &st, NULL),
                 SQLITE_OK);
    sqlite3_bind_text(st, 1, long_seq, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 2, long_json, -1, SQLITE_STATIC);
    CT_ASSERT_EQ(sqlite3_step(st), SQLITE_DONE);
    sqlite3_finalize(st);

    ct_generated_alg got;
    memset(&got, 0xAA, sizeof got);
    CT_ASSERT_EQ(ct_repo_alg_get(db, CT_ORBIT_EDGE, "ZZ", CT_BUFFER_FIXED, 0, "hx", &got),
                 CT_REPO_OK);
    CT_ASSERT_EQ(strlen(got.sequence), (size_t)(CT_ALG_SEQ_MAX - 1));
    CT_ASSERT_EQ(strlen(got.score_parts_json), (size_t)(CT_ALG_PARTS_MAX - 1));
    CT_ASSERT_EQ(got.setup[0], '\0');
    CT_ASSERT_EQ(got.interchange[0], '\0');
    CT_ASSERT_EQ(got.insertion[0], '\0');
    CT_ASSERT_STR_EQ(got.created_at, "2026-01-01T00:00:00Z");

    ct_generated_alg listed[8];
    memset(listed, 0xAA, sizeof listed);
    CT_ASSERT_EQ(ct_repo_alg_list(db, "hx", CT_ORBIT_EDGE, listed, 8), 1);
    CT_ASSERT_EQ(listed[1].orbit, 0);
    CT_ASSERT_EQ(listed[1].target_pair[0], '\0');
    CT_ASSERT_EQ(listed[1].sequence[0], '\0');

    CT_ASSERT_EQ(ct_repo_alg_list(db, "hx", CT_ORBIT_EDGE, NULL, 1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_list(db, "hx", CT_ORBIT_EDGE, NULL, 0), CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "hx", NULL, 1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_alg_stale(db, "hx", NULL, 0), CT_REPO_OK);

    char null_cfg[CT_ALG_HASH_HEX_MAX];
    memset(null_cfg, 'q', sizeof null_cfg - 1);
    null_cfg[sizeof null_cfg - 1] = '\0';
    ct_params_hash(NULL, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
                   null_cfg);
    CT_ASSERT_EQ(null_cfg[0], '\0');
    ct_config cfg = ct_config_defaults();
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, NULL,
                   null_cfg);
    CT_ASSERT_EQ(null_cfg[0], '\0');
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
