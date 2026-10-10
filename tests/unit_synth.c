#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "framework.h"
#include "gen/score.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    TEST_N = 3,
    TEST_CELLS = 54,
    TEST_ALG_MAX = CT_SYNTH_MAX_MOVES,
    TEST_LINE_MAX = 512,
    TEST_FIXTURE_ROWS = 10,
    TEST_SAMPLE_ENTRIES = 50,
    TEST_COVERAGE_COUNT = 30,
    TEST_QTM_TEMPLATE_MAX = CT_SYNTH_TPL_MAX * 2,
    TEST_SCORING_SAMPLES = 40,
};

static int test_raw(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int test_compact(int face, int row, int col) {
    return face * TEST_N * TEST_N + row * TEST_N + col;
}

static int test_sticker_compact(int orbit, int sticker) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return test_compact(face, row, col);
}

static void test_simulate(const ct_move *seq, int len, uint8_t state[TEST_CELLS]) {
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = TEST_N;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < TEST_N; row++) {
            for (int col = 0; col < TEST_N; col++) {
                c.facelet[test_raw(face, row, col)] = (uint8_t)test_compact(face, row, col);
            }
        }
    }
    for (int i = 0; i < len; i++) {
        ct_apply(&c, seq[i]);
    }
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < TEST_N; row++) {
            for (int col = 0; col < TEST_N; col++) {
                state[test_compact(face, row, col)] = c.facelet[test_raw(face, row, col)];
            }
        }
    }
}

static int test_piece_of(int orbit, int sticker) {
    return orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(sticker) : ct_edge_piece_of(sticker);
}

static bool test_cell_on_pieces(int orbit, int buffer, int t1, int t2, int cell) {
    int pb = test_piece_of(orbit, buffer);
    int p1 = test_piece_of(orbit, t1);
    int p2 = test_piece_of(orbit, t2);
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int p = test_piece_of(orbit, s);
        if (p == pb || p == p1 || p == p2) {
            if (test_sticker_compact(orbit, s) == cell) {
                return true;
            }
        }
    }
    return false;
}

static bool test_case_solved(int orbit, int buffer, int t1, int t2, const ct_move *seq, int len) {
    uint8_t state[TEST_CELLS];
    test_simulate(seq, len, state);
    int cb = test_sticker_compact(orbit, buffer);
    int c1 = test_sticker_compact(orbit, t1);
    int c2 = test_sticker_compact(orbit, t2);
    if (state[c1] != (uint8_t)cb || state[c2] != (uint8_t)c1 || state[cb] != (uint8_t)c2) {
        return false;
    }
    int moved = 0;
    for (int q = 0; q < TEST_CELLS; q++) {
        if (state[q] == (uint8_t)q) {
            continue;
        }
        if (!test_cell_on_pieces(orbit, buffer, t1, t2, q)) {
            return false;
        }
        moved++;
    }
    return moved == CT_TARGET_CORNER_STICKERS_PER_PIECE * CT_SYNTH_CYCLE_LEN ||
           moved == CT_TARGET_EDGE_STICKERS_PER_PIECE * CT_SYNTH_CYCLE_LEN;
}

static int test_cycle_length(const uint8_t state[TEST_CELLS], int start, bool visited[TEST_CELLS]) {
    int perm[TEST_CELLS];
    for (int q = 0; q < TEST_CELLS; q++) {
        perm[state[q]] = q;
    }
    int len = 0;
    int cur = start;
    do {
        visited[cur] = true;
        cur = perm[cur];
        len++;
    } while (cur != start);
    return len;
}

static int test_face_of_letter(char c) {
    switch (c) {
    case 'U':
        return CT_FACE_U;
    case 'D':
        return CT_FACE_D;
    case 'L':
        return CT_FACE_L;
    case 'R':
        return CT_FACE_R;
    case 'F':
        return CT_FACE_F;
    case 'B':
        return CT_FACE_B;
    default:
        return -1;
    }
}

static int test_name_sticker(int orbit, const char *name) {
    int len = (int)strlen(name);
    int want[CT_CUBE_FACE_COUNT] = {0, 0, 0, 0, 0, 0};
    for (int i = 0; i < len; i++) {
        int f = test_face_of_letter(name[i]);
        if (f < 0) {
            return CT_TARGET_NONE;
        }
        want[f]++;
    }
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(orbit, s, &face, &row, &col);
        if (face != test_face_of_letter(name[0])) {
            continue;
        }
        ct_v3 center;
        ct_v3 normal;
        ct_sticker_world(TEST_N, face, row, col, &center, &normal);
        int coords[3] = {center.x / 2, center.y / 2, center.z / 2};
        int have[CT_CUBE_FACE_COUNT] = {0, 0, 0, 0, 0, 0};
        for (int axis = 0; axis < 3; axis++) {
            if (coords[axis] == 0) {
                continue;
            }
            int f = axis == 0   ? (coords[axis] > 0 ? CT_FACE_R : CT_FACE_L)
                    : axis == 1 ? (coords[axis] > 0 ? CT_FACE_U : CT_FACE_D)
                                : (coords[axis] > 0 ? CT_FACE_F : CT_FACE_B);
            have[f]++;
        }
        if (memcmp(want, have, sizeof want) == 0) {
            return s;
        }
    }
    return CT_TARGET_NONE;
}

static int test_trim_field(char *text) {
    int len = (int)strlen(text);
    int start = 0;
    while (start < len && (text[start] == ' ' || text[start] == '\t')) {
        start++;
    }
    int end = len;
    while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\n' ||
                           text[end - 1] == '\r')) {
        end--;
    }
    text[end] = '\0';
    return start;
}

static int test_load_fixture(char orbits[TEST_FIXTURE_ROWS][8], char cycles[TEST_FIXTURE_ROWS][24],
                             char algs[TEST_FIXTURE_ROWS][256]) {
    FILE *file = fopen("tests/fixtures/bh_reference.md", "r");
    if (file == NULL) {
        return -1;
    }
    char line[TEST_LINE_MAX];
    int rows = 0;
    while (fgets(line, sizeof line, file) != NULL) {
        if (line[0] != '|') {
            continue;
        }
        if (strncmp(line, "| orbit", 7) == 0) {
            continue;
        }
        char orbit[8] = {0};
        char cycle[24] = {0};
        char alg[256] = {0};
        if (sscanf(line, "| %7[^|]| %23[^|]| %255[^|]|", orbit, cycle, alg) != 3) {
            continue;
        }
        int orbit_off_check = test_trim_field(orbit);
        if (strcmp(orbit + orbit_off_check, "corner") != 0 &&
            strcmp(orbit + orbit_off_check, "edge") != 0) {
            continue;
        }
        int orbit_off = test_trim_field(orbit);
        int cycle_off = test_trim_field(cycle);
        int alg_off = test_trim_field(alg);
        if (rows >= TEST_FIXTURE_ROWS) {
            fclose(file);
            return -2;
        }
        snprintf(orbits[rows], sizeof orbits[rows], "%s", orbit + orbit_off);
        snprintf(cycles[rows], sizeof cycles[rows], "%s", cycle + cycle_off);
        snprintf(algs[rows], sizeof algs[rows], "%s", alg + alg_off);
        rows++;
    }
    fclose(file);
    return rows;
}

CT_SUITE(unit_synth_table) {
    int size = ct_synth_init();
    CT_ASSERT(size > 0);
    CT_ASSERT_EQ(ct_synth_table_size(), size);
    CT_ASSERT_EQ(ct_synth_init(), size);

    int step = size / TEST_SAMPLE_ENTRIES;
    if (step < 1) {
        step = 1;
    }
    int sampled = 0;
    for (int i = 0; i < size && sampled < TEST_SAMPLE_ENTRIES; i += step) {
        int key[CT_SYNTH_KEY_FIELDS];
        ct_move seq[CT_SYNTH_TPL_MAX];
        int len = ct_synth_table_entry(i, key, seq, CT_SYNTH_TPL_MAX);
        CT_ASSERT(len > 0);
        CT_ASSERT(len <= CT_SYNTH_TPL_MAX);
        CT_ASSERT(key[0] < key[1]);
        CT_ASSERT(key[1] < key[2]);
        CT_ASSERT(key[0] >= 0 && key[2] < TEST_CELLS);
        CT_ASSERT(key[3] == 0 || key[3] == 1);

        uint8_t state[TEST_CELLS];
        test_simulate(seq, len, state);
        bool visited[TEST_CELLS] = {false};
        int cycle_len = test_cycle_length(state, key[0], visited);
        CT_ASSERT_EQ(cycle_len, CT_SYNTH_CYCLE_LEN);
        int perm[TEST_CELLS];
        for (int q = 0; q < TEST_CELLS; q++) {
            perm[state[q]] = q;
        }
        int second = key[3] == 0 ? key[1] : key[2];
        CT_ASSERT_EQ(perm[key[0]], second);
        int moved = 0;
        bool clean = true;
        for (int q = 0; q < TEST_CELLS; q++) {
            if (state[q] == (uint8_t)q) {
                continue;
            }
            moved++;
            bool in_corner = false;
            bool in_edge = false;
            for (int s = 0; s < CT_TARGET_STICKERS; s++) {
                if (test_sticker_compact(CT_TARGET_ORBIT_CORNER, s) == q) {
                    in_corner = true;
                }
                if (test_sticker_compact(CT_TARGET_ORBIT_EDGE, s) == q) {
                    in_edge = true;
                }
            }
            if (in_corner == in_edge) {
                clean = false;
            }
            if (!visited[q]) {
                int other_len = test_cycle_length(state, q, visited);
                if (other_len != CT_SYNTH_CYCLE_LEN) {
                    clean = false;
                }
            }
        }
        CT_ASSERT(clean);
        CT_ASSERT(moved == CT_TARGET_CORNER_STICKERS_PER_PIECE * CT_SYNTH_CYCLE_LEN ||
                  moved == CT_TARGET_EDGE_STICKERS_PER_PIECE * CT_SYNTH_CYCLE_LEN);
        sampled++;
    }
    CT_ASSERT(sampled > 0);
    CT_ASSERT(ct_synth_table_entry(-1, NULL, NULL, 0) < 0);
    CT_ASSERT(ct_synth_table_entry(size, NULL, NULL, 0) < 0);
    int key[CT_SYNTH_KEY_FIELDS];
    CT_ASSERT(ct_synth_table_entry(0, key, NULL, 0) < 0);
    ct_move seq[CT_SYNTH_TPL_MAX];
    CT_ASSERT(ct_synth_table_entry(0, key, seq, 0) < 0);
}

CT_SUITE(unit_synth_direct) {
    ct_config cfg = ct_config_defaults();
    cfg.gen.setup_bfs_depth = 0;
    cfg.gen.fallback_depth = 0;
    ct_synth_result res;
    memset(&res, 0, sizeof res);
    int rc = ct_synth_target(CT_ORBIT_CORNER, 0, 1, 6, &cfg, &res);
    CT_ASSERT_EQ(rc, 0);
    CT_ASSERT(res.found);
    CT_ASSERT(!res.used_setup);
    CT_ASSERT(!res.used_fallback);
    CT_ASSERT(res.len > 0);
    CT_ASSERT(res.qtm > 0);
    CT_ASSERT(res.qtm <= TEST_QTM_TEMPLATE_MAX);
    CT_ASSERT(test_case_solved(CT_ORBIT_CORNER, 0, 1, 6, res.seq, res.len));

    uint8_t state[TEST_CELLS];
    test_simulate(res.seq, res.len, state);
    int perm[TEST_CELLS];
    for (int q = 0; q < TEST_CELLS; q++) {
        perm[state[q]] = q;
    }
    int cb = test_sticker_compact(CT_ORBIT_CORNER, 0);
    int c1 = test_sticker_compact(CT_ORBIT_CORNER, 1);
    int c2 = test_sticker_compact(CT_ORBIT_CORNER, 6);
    CT_ASSERT_EQ(perm[cb], c1);
    CT_ASSERT_EQ(perm[c1], c2);
    CT_ASSERT_EQ(perm[c2], cb);

    ct_synth_result edge;
    memset(&edge, 0, sizeof edge);
    rc = ct_synth_target(CT_ORBIT_EDGE, 2, 1, 4, &cfg, &edge);
    CT_ASSERT_EQ(rc, 0);
    CT_ASSERT(edge.found);
    CT_ASSERT(!edge.used_setup);
    CT_ASSERT(!edge.used_fallback);
    CT_ASSERT(edge.qtm <= TEST_QTM_TEMPLATE_MAX);
    CT_ASSERT(test_case_solved(CT_ORBIT_EDGE, 2, 1, 4, edge.seq, edge.len));

    int expect_penalty = 0;
    static const unsigned face_bit[CT_CUBE_FACE_COUNT] = {
        CT_MOVE_BIT_U, CT_MOVE_BIT_L, CT_MOVE_BIT_F, CT_MOVE_BIT_R, CT_MOVE_BIT_B, CT_MOVE_BIT_D,
    };
    for (int i = 0; i < res.len; i++) {
        unsigned bit = face_bit[res.seq[i].face];
        if ((cfg.gen.preferred_moves & bit) == 0) {
            for (int idx = 0; idx < CT_FACE_COUNT; idx++) {
                if ((1u << idx) == bit) {
                    expect_penalty += (int)cfg.gen.penalty[idx];
                }
            }
        }
    }
    CT_ASSERT_EQ((int)res.subset_penalty, expect_penalty);
}

CT_SUITE(unit_synth_setup) {
    ct_config cfg = ct_config_defaults();
    ct_synth_result res;
    memset(&res, 0, sizeof res);
    int rc = ct_synth_target(CT_ORBIT_CORNER, 0, 1, 2, &cfg, &res);
    CT_ASSERT_EQ(rc, 0);
    CT_ASSERT(res.found);
    CT_ASSERT(res.used_setup);
    CT_ASSERT(!res.used_fallback);
    CT_ASSERT(test_case_solved(CT_ORBIT_CORNER, 0, 1, 2, res.seq, res.len));
    CT_ASSERT(res.len <= CT_SYNTH_MAX_MOVES);
}

CT_SUITE(unit_synth_coverage) {
    ct_config cfg = ct_config_defaults();
    static ct_target_pair corners[378];
    static ct_target_pair edges[440];
    int corner_count = ct_targets_corner(0, corners, 378);
    int edge_count = ct_targets_edge(2, edges, 440);
    CT_ASSERT_EQ(corner_count, 378);
    CT_ASSERT_EQ(edge_count, 440);

    int found = 0;
    int direct = 0;
    int setup = 0;
    int fallback = 0;
    for (int i = 0; i < TEST_COVERAGE_COUNT; i++) {
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(CT_ORBIT_CORNER, 0, corners[i].t1, corners[i].t2, &cfg, &res);
        CT_ASSERT_EQ(rc, 0);
        CT_ASSERT(res.found);
        found += res.found ? 1 : 0;
        direct += !res.used_setup && !res.used_fallback ? 1 : 0;
        setup += res.used_setup ? 1 : 0;
        fallback += res.used_fallback ? 1 : 0;
        CT_ASSERT(
            test_case_solved(CT_ORBIT_CORNER, 0, corners[i].t1, corners[i].t2, res.seq, res.len));
        if (!res.used_setup && !res.used_fallback) {
            CT_ASSERT(res.qtm <= TEST_QTM_TEMPLATE_MAX);
        }
    }
    for (int i = 0; i < TEST_COVERAGE_COUNT; i++) {
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(CT_ORBIT_EDGE, 2, edges[i].t1, edges[i].t2, &cfg, &res);
        CT_ASSERT_EQ(rc, 0);
        CT_ASSERT(res.found);
        found += res.found ? 1 : 0;
        direct += !res.used_setup && !res.used_fallback ? 1 : 0;
        setup += res.used_setup ? 1 : 0;
        fallback += res.used_fallback ? 1 : 0;
        CT_ASSERT(test_case_solved(CT_ORBIT_EDGE, 2, edges[i].t1, edges[i].t2, res.seq, res.len));
        if (!res.used_setup && !res.used_fallback) {
            CT_ASSERT(res.qtm <= TEST_QTM_TEMPLATE_MAX);
        }
    }
    printf("unit_synth_coverage: found=%d direct=%d setup=%d fallback=%d\n", found, direct, setup,
           fallback);
}

CT_SUITE(unit_synth_determinism) {
    ct_config cfg = ct_config_defaults();
    ct_synth_result a;
    ct_synth_result b;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_CORNER, 0, 5, 9, &cfg, &a), 0);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_CORNER, 0, 5, 9, &cfg, &b), 0);
    CT_ASSERT(memcmp(&a, &b, sizeof a) == 0);
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_EDGE, 2, 7, 13, &cfg, &a), 0);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_EDGE, 2, 7, 13, &cfg, &b), 0);
    CT_ASSERT(memcmp(&a, &b, sizeof a) == 0);
}

CT_SUITE(unit_synth_scoring) {
    static ct_target_pair corners[378];
    static ct_target_pair edges[440];
    CT_ASSERT_EQ(ct_targets_corner(0, corners, 378), 378);
    CT_ASSERT_EQ(ct_targets_edge(2, edges, 440), 440);
    ct_config full = ct_config_defaults();
    full.gen.candidate_max = CT_GEN_CANDIDATE_MAX;
    ct_config direct_only = ct_config_defaults();
    direct_only.gen.setup_bfs_depth = 0;
    direct_only.gen.fallback_depth = 0;
    ct_config one_cand = ct_config_defaults();
    one_cand.gen.candidate_max = 1;
    int with_direct = 0;
    int improved = 0;
    int with_single = 0;
    int dominated = 0;
    for (int i = 0; i < TEST_SCORING_SAMPLES; i++) {
        int orbit = i % CT_SYNTH_TWO == 0 ? CT_ORBIT_CORNER : CT_ORBIT_EDGE;
        int buffer = orbit == CT_ORBIT_CORNER ? 0 : 2;
        const ct_target_pair *pair =
            orbit == CT_ORBIT_CORNER ? &corners[i / CT_SYNTH_TWO] : &edges[i / CT_SYNTH_TWO];
        ct_synth_result direct;
        memset(&direct, 0, sizeof direct);
        CT_ASSERT_EQ(ct_synth_target(orbit, buffer, pair->t1, pair->t2, &direct_only, &direct), 0);
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        CT_ASSERT_EQ(ct_synth_target(orbit, buffer, pair->t1, pair->t2, &full, &res), 0);
        CT_ASSERT(res.found);
        CT_ASSERT(test_case_solved(orbit, buffer, pair->t1, pair->t2, res.seq, res.len));
        ct_score_parts res_score;
        CT_ASSERT_EQ(ct_score_alg(res.seq, res.len, &full, &res_score), CT_SCORE_OK);
        ct_synth_result single;
        memset(&single, 0, sizeof single);
        CT_ASSERT_EQ(ct_synth_target(orbit, buffer, pair->t1, pair->t2, &one_cand, &single), 0);
        if (single.found) {
            with_single++;
            CT_ASSERT(test_case_solved(orbit, buffer, pair->t1, pair->t2, single.seq, single.len));
            ct_score_parts single_score;
            CT_ASSERT_EQ(ct_score_alg(single.seq, single.len, &full, &single_score), CT_SCORE_OK);
            if (res_score.total <= single_score.total + 1e-9) {
                dominated++;
            }
        }
        if (direct.found) {
            with_direct++;
            CT_ASSERT(res.found);
            ct_score_parts direct_score;
            CT_ASSERT_EQ(ct_score_alg(direct.seq, direct.len, &full, &direct_score), CT_SCORE_OK);
            if (res_score.total < direct_score.total) {
                improved++;
            }
        }
    }
    printf("unit_synth_scoring: with_direct=%d improved=%d with_single=%d dominated=%d\n",
           with_direct, improved, with_single, dominated);
    CT_ASSERT(with_direct > 0);
    CT_ASSERT(improved > 0);
    CT_ASSERT(with_single > 0);
    CT_ASSERT(with_single == dominated);
}

CT_SUITE(unit_synth_config) {
    ct_config cfg = ct_config_defaults();
    static ct_target_pair corners[378];
    int corner_count = ct_targets_corner(0, corners, 378);
    CT_ASSERT_EQ(corner_count, 378);
    for (int i = 0; i < 5; i++) {
        ct_synth_result full;
        memset(&full, 0, sizeof full);
        CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_CORNER, 0, corners[i].t1, corners[i].t2, &cfg, &full),
                     0);
        CT_ASSERT(full.found);
        cfg.gen.preferred_moves = CT_MOVE_BIT_R;
        ct_synth_result limited;
        memset(&limited, 0, sizeof limited);
        int rc = ct_synth_target(CT_ORBIT_CORNER, 0, corners[i].t1, corners[i].t2, &cfg, &limited);
        CT_ASSERT_EQ(rc, 0);
        if (limited.found) {
            CT_ASSERT(test_case_solved(CT_ORBIT_CORNER, 0, corners[i].t1, corners[i].t2,
                                       limited.seq, limited.len));
        }
        cfg.gen.preferred_moves = CT_GEN_DEFAULT_PREFERRED_MOVES;
    }
    cfg.gen.preferred_moves = CT_MOVE_BIT_R;
    ct_synth_result miss;
    memset(&miss, 0, sizeof miss);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_CORNER, 0, 1, 5, &cfg, &miss), 0);
    CT_ASSERT(!miss.found);
    CT_ASSERT_EQ(miss.len, 0);
    CT_ASSERT_EQ(miss.qtm, 0);
}

CT_SUITE(unit_synth_bh) {
    char orbits[TEST_FIXTURE_ROWS][8];
    char cycles[TEST_FIXTURE_ROWS][24];
    char algs[TEST_FIXTURE_ROWS][256];
    int rows = test_load_fixture(orbits, cycles, algs);
    CT_ASSERT_EQ(rows, TEST_FIXTURE_ROWS);
    ct_config cfg = ct_config_defaults();
    for (int i = 0; i < rows; i++) {
        int orbit = strcmp(orbits[i], "corner") == 0 ? CT_ORBIT_CORNER : CT_ORBIT_EDGE;
        int buffer = orbit == CT_ORBIT_CORNER ? 0 : 2;
        char name_a[8] = {0};
        char name_b[8] = {0};
        char name_c[8] = {0};
        int parsed = sscanf(cycles[i], "%7s %7s %7s", name_a, name_b, name_c);
        CT_ASSERT_EQ(parsed, 3);
        int s_a = test_name_sticker(orbit, name_a);
        int s_b = test_name_sticker(orbit, name_b);
        int s_c = test_name_sticker(orbit, name_c);
        CT_ASSERT(s_a >= 0);
        CT_ASSERT(s_b >= 0);
        CT_ASSERT(s_c >= 0);
        CT_ASSERT(s_a != s_b);
        CT_ASSERT(s_b != s_c);
        CT_ASSERT(s_a != s_c);

        ct_move ref[TEST_ALG_MAX];
        int ref_len = ct_notation_parse(algs[i], TEST_N, ref, TEST_ALG_MAX);
        CT_ASSERT(ref_len > 0);
        uint8_t state[TEST_CELLS];
        test_simulate(ref, ref_len, state);
        int perm[TEST_CELLS];
        for (int q = 0; q < TEST_CELLS; q++) {
            perm[state[q]] = q;
        }
        int c_a = test_sticker_compact(orbit, s_a);
        int c_b = test_sticker_compact(orbit, s_b);
        int c_c = test_sticker_compact(orbit, s_c);
        CT_ASSERT_EQ(perm[c_a], c_b);
        CT_ASSERT_EQ(perm[c_b], c_c);
        CT_ASSERT_EQ(perm[c_c], c_a);

        int names[CT_SYNTH_CYCLE_LEN] = {s_a, s_b, s_c};
        int start = 0;
        while (names[start] != buffer) {
            start++;
        }
        int t1 = names[(start + 1) % CT_SYNTH_CYCLE_LEN];
        int t2 = names[(start + 2) % CT_SYNTH_CYCLE_LEN];

        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(orbit, buffer, t1, t2, &cfg, &res);
        CT_ASSERT_EQ(rc, 0);
        CT_ASSERT(res.found);
        CT_ASSERT(test_case_solved(orbit, buffer, t1, t2, res.seq, res.len));

        uint8_t ours[TEST_CELLS];
        test_simulate(res.seq, res.len, ours);
        int moved_ref = 0;
        int moved_ours = 0;
        for (int q = 0; q < TEST_CELLS; q++) {
            moved_ref += state[q] != (uint8_t)q ? 1 : 0;
            moved_ours += ours[q] != (uint8_t)q ? 1 : 0;
        }
        CT_ASSERT_EQ(moved_ref, moved_ours);
    }
}

CT_SUITE(unit_synth_bad_input) {
    ct_config cfg = ct_config_defaults();
    ct_synth_result res;
    memset(&res, 0, sizeof res);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CENTER, 0, 1, 2, &cfg, &res) < 0);
    CT_ASSERT(!res.found);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CORNER, -1, 1, 2, &cfg, &res) < 0);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CORNER, 0, -1, 2, &cfg, &res) < 0);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CORNER, 0, 1, CT_TARGET_STICKERS, &cfg, &res) < 0);
    CT_ASSERT(ct_synth_target(CT_ORBIT_EDGE, 2, 1, 2, &cfg, &res) < 0);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CORNER, 0, 4, 1, &cfg, &res) < 0);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CORNER, 0, 1, 13, &cfg, &res) < 0);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CORNER, 0, 1, 2, NULL, &res) < 0);
    CT_ASSERT(ct_synth_target(CT_ORBIT_CORNER, 0, 1, 2, &cfg, NULL) < 0);
}

CT_SUITE(unit_synth_stress) {
    ct_sfc64 rng;
    ct_sfc64_init(&rng, 0xC0FFEE);
    ct_config cfg = ct_config_defaults();
    CT_ASSERT(ct_synth_init() > 0);
    int size = ct_synth_table_size();
    CT_ASSERT(size > 0);
    int bad = 0;
    int good = 0;
    int found = 0;
    for (int i = 0; i < 2500; i++) {
        int orbit = (int)ct_sfc64_range(&rng, 8);
        int buffer = (int)ct_sfc64_range(&rng, 32) - 4;
        int t1 = (int)ct_sfc64_range(&rng, 32) - 4;
        int t2 = (int)ct_sfc64_range(&rng, 32) - 4;
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(orbit, buffer, t1, t2, &cfg, &res);
        if (rc < 0) {
            bad++;
            CT_ASSERT(!res.found);
            continue;
        }
        good++;
        if (res.found) {
            found++;
            CT_ASSERT(test_case_solved(orbit, buffer, t1, t2, res.seq, res.len));
            CT_ASSERT(res.len <= CT_SYNTH_MAX_MOVES);
            CT_ASSERT(res.qtm >= res.len);
        }
    }
    CT_ASSERT(bad > 0);
    CT_ASSERT(good > 0);
    CT_ASSERT(found > 0);
    for (int i = 0; i < 500; i++) {
        int index = (int)ct_sfc64_range(&rng, (uint64_t)(size + 8)) - 4;
        int key[CT_SYNTH_KEY_FIELDS];
        ct_move seq[CT_SYNTH_TPL_MAX];
        int len = ct_synth_table_entry(index, key, seq, CT_SYNTH_TPL_MAX);
        if (index < 0 || index >= size) {
            CT_ASSERT(len < 0);
        } else {
            CT_ASSERT(len > 0);
        }
    }
    for (int i = 0; i < 40; i++) {
        ct_config fuzz_cfg = ct_config_defaults();
        unsigned shift = ct_sfc64_range(&rng, CT_FACE_COUNT);
        fuzz_cfg.gen.preferred_moves = 1u << shift;
        fuzz_cfg.gen.setup_bfs_depth = (int)ct_sfc64_range(&rng, 10) - 2;
        fuzz_cfg.gen.fallback_depth = (int)ct_sfc64_range(&rng, 10) - 2;
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(CT_ORBIT_CORNER, 0, 1, 2, &fuzz_cfg, &res);
        CT_ASSERT_EQ(rc, 0);
        if (res.found) {
            CT_ASSERT(test_case_solved(CT_ORBIT_CORNER, 0, 1, 2, res.seq, res.len));
        }
    }
}

CT_SUITE(unit_synth_gates) {
    CT_ASSERT(ct_synth_init() > 0);
    CT_ASSERT_EQ(ct_synth_table_size(), 4464);

    int key[CT_SYNTH_KEY_FIELDS];
    ct_move seq[CT_SYNTH_TPL_MAX];
    CT_ASSERT_EQ(ct_synth_table_entry(-1, key, seq, CT_SYNTH_TPL_MAX), -1);
    CT_ASSERT_EQ(ct_synth_table_entry(ct_synth_table_size(), key, seq, CT_SYNTH_TPL_MAX), -1);
    CT_ASSERT_EQ(ct_synth_table_entry(0, key, seq, CT_SYNTH_TPL_MAX), 8);
    CT_ASSERT_EQ(key[0], 1);
    CT_ASSERT_EQ(key[1], 3);
    CT_ASSERT_EQ(key[2], 46);
    CT_ASSERT_EQ(key[3], 1);
    static const int want_face[8] = {1, 0, 2, 0, 1, 0, 2, 0};
    static const int want_from[8] = {2, 1, 1, 1, 2, 1, 1, 1};
    static const int want_to[8] = {2, 1, 1, 1, 2, 1, 1, 1};
    static const int want_quarter[8] = {2, 3, 2, 1, 2, 3, 2, 1};
    for (int i = 0; i < 8; i++) {
        CT_ASSERT_EQ(seq[i].face, want_face[i]);
        CT_ASSERT_EQ(seq[i].depth_from, want_from[i]);
        CT_ASSERT_EQ(seq[i].depth_to, want_to[i]);
        CT_ASSERT_EQ(seq[i].quarter, want_quarter[i]);
    }
    CT_ASSERT_EQ(ct_synth_table_entry(2, key, seq, CT_SYNTH_TPL_MAX), 8);
    CT_ASSERT_EQ(key[0], 1);
    CT_ASSERT_EQ(key[1], 3);
    CT_ASSERT_EQ(key[2], 46);
    CT_ASSERT_EQ(key[3], 0);

    ct_config direct_only = ct_config_defaults();
    direct_only.gen.setup_bfs_depth = 0;
    direct_only.gen.fallback_depth = 0;
    ct_synth_result res;
    memset(&res, 0, sizeof res);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_EDGE, 6, 2, 0, &direct_only, &res), 0);
    CT_ASSERT(res.found);
    CT_ASSERT(!res.used_setup);
    CT_ASSERT(!res.used_fallback);
    CT_ASSERT(test_case_solved(CT_ORBIT_EDGE, 6, 2, 0, res.seq, res.len));
    memset(&res, 0, sizeof res);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_EDGE, 2, 0, 4, &direct_only, &res), 0);
    CT_ASSERT(!res.found);
    CT_ASSERT_EQ(res.len, 0);

    ct_config wide = ct_config_defaults();
    wide.gen.preferred_moves = CT_MOVE_BIT_U | CT_MOVE_BIT_D | CT_MOVE_BIT_L | CT_MOVE_BIT_R |
                               CT_MOVE_BIT_F | CT_MOVE_BIT_B;
    memset(&res, 0, sizeof res);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_CORNER, 0, 1, 6, &wide, &res), 0);
    CT_ASSERT(res.found);
    CT_ASSERT(test_case_solved(CT_ORBIT_CORNER, 0, 1, 6, res.seq, res.len));
    memset(&res, 0, sizeof res);
    CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_EDGE, 2, 1, 4, &wide, &res), 0);
    CT_ASSERT(res.found);
    CT_ASSERT(test_case_solved(CT_ORBIT_EDGE, 2, 1, 4, res.seq, res.len));
}
