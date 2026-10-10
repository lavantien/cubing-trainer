#include "engine/cube.h"
#include "framework.h"
#include "gen/interchange.h"
#include "gen/targets.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { CT_TEST_PERM_CELLS = CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE };

static const int u2_corner_pairs[6][2] = {
    {0, 2}, {1, 3}, {8, 16}, {9, 17}, {4, 12}, {5, 13},
};
static const int u2_edge_pairs[4][2] = {
    {0, 2},
    {1, 3},
    {8, 16},
    {4, 12},
};
static const int r2_corner_pairs[6][2] = {
    {12, 14}, {13, 15}, {1, 21}, {2, 22}, {9, 19}, {10, 16},
};
static const int r2_edge_pairs[4][2] = {
    {12, 14},
    {13, 15},
    {1, 21},
    {9, 19},
};
static const int m2_edge_pairs[4][2] = {
    {0, 20},
    {2, 22},
    {8, 18},
    {10, 16},
};
static const int m2_corner_pairs[1][2] = {
    {-1, -1},
};
static const ct_move quarter_samples[5] = {
    {CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 1}, {CT_FACE_F, 1, 1, 1},
    {CT_FACE_R, 1, 2, 1}, {CT_FACE_L, 2, 2, 1},
};

static int test_raw(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int test_compact(int n, int face, int row, int col) {
    return face * n * n + row * n + col;
}

static int test_sticker_compact(int orbit, int sticker, int n) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return test_compact(n, face, row, col);
}

static void test_identity_cube(ct_cube *c, int n) {
    memset(c, 0, sizeof *c);
    c->n = n;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                c->facelet[test_raw(face, row, col)] = (uint8_t)test_compact(n, face, row, col);
            }
        }
    }
}

static void test_extract(const ct_cube *c, int n, uint8_t *perm) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                perm[c->facelet[test_raw(face, row, col)]] =
                    (uint8_t)test_compact(n, face, row, col);
            }
        }
    }
}

static void test_perm(ct_move m, int n, uint8_t *perm) {
    memset(perm, 0xFF, (size_t)CT_TEST_PERM_CELLS);
    ct_cube c;
    test_identity_cube(&c, n);
    ct_apply(&c, m);
    test_extract(&c, n, perm);
}

static bool test_interchanges(const uint8_t *perm, int a, int b) {
    return perm[a] == (uint8_t)b && perm[b] == (uint8_t)a;
}

static bool test_move_eq(ct_move a, ct_move b) {
    return a.face == b.face && a.depth_from == b.depth_from && a.depth_to == b.depth_to &&
           a.quarter == b.quarter;
}

static bool test_move_in(const ct_move *ms, int count, ct_move m) {
    for (int i = 0; i < count; i++) {
        if (test_move_eq(ms[i], m)) {
            return true;
        }
    }
    return false;
}

static int test_candidates(int n, ct_move *out) {
    int total = 0;
    int half = (n + 1) / 2;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int from = 1; from <= half; from++) {
            for (int to = from; to <= half; to++) {
                for (int quarter = 1; quarter < CT_MOVE_QUARTERS; quarter++) {
                    if (out != NULL) {
                        out[total] = (ct_move){face, from, to, quarter};
                    }
                    total++;
                }
            }
        }
    }
    return total;
}

static bool pair_in_table(const int (*tab)[2], int count, int a, int b) {
    for (int i = 0; i < count; i++) {
        if ((tab[i][0] == a && tab[i][1] == b) || (tab[i][0] == b && tab[i][1] == a)) {
            return true;
        }
    }
    return false;
}

static bool perm_pairs_match_golden(const uint8_t *perm, int n, int orbit, const int (*tab)[2],
                                    int tab_n) {
    int hits = 0;
    for (int a = 0; a < CT_TARGET_STICKERS; a++) {
        for (int b = a + 1; b < CT_TARGET_STICKERS; b++) {
            bool inter = test_interchanges(perm, test_sticker_compact(orbit, a, n),
                                           test_sticker_compact(orbit, b, n));
            if (inter != pair_in_table(tab, tab_n, a, b)) {
                return false;
            }
            if (inter) {
                hits++;
            }
        }
    }
    return hits == tab_n;
}

CT_SUITE(unit_interchange) {
    uint8_t perm[CT_TEST_PERM_CELLS];
    uint8_t other[CT_TEST_PERM_CELLS];
    static ct_move cands[CT_INTERCHANGE_MAX_CANDIDATES];
    static uint8_t cand_perms[54][CT_TEST_PERM_CELLS];
    ct_move res[CT_INTERCHANGE_MAX_CANDIDATES];

    struct {
        int orbit;
        int sticker;
        int face;
        int row;
        int col;
    } cell_cases[] = {
        {CT_TARGET_ORBIT_CORNER, 0, CT_FACE_U, 0, 0},
        {CT_TARGET_ORBIT_CORNER, 1, CT_FACE_U, 0, 2},
        {CT_TARGET_ORBIT_CORNER, 2, CT_FACE_U, 2, 2},
        {CT_TARGET_ORBIT_CORNER, 12, CT_FACE_R, 0, 0},
        {CT_TARGET_ORBIT_CORNER, 16, CT_FACE_B, 0, 0},
        {CT_TARGET_ORBIT_CORNER, 17, CT_FACE_B, 0, 2},
        {CT_TARGET_ORBIT_CORNER, 20, CT_FACE_D, 0, 0},
        {CT_TARGET_ORBIT_CORNER, 21, CT_FACE_D, 0, 2},
        {CT_TARGET_ORBIT_EDGE, 0, CT_FACE_U, 0, 1},
        {CT_TARGET_ORBIT_EDGE, 2, CT_FACE_U, 2, 1},
        {CT_TARGET_ORBIT_EDGE, 8, CT_FACE_F, 0, 1},
        {CT_TARGET_ORBIT_EDGE, 12, CT_FACE_R, 0, 1},
        {CT_TARGET_ORBIT_EDGE, 16, CT_FACE_B, 0, 1},
        {CT_TARGET_ORBIT_EDGE, 20, CT_FACE_D, 0, 1},
        {CT_TARGET_ORBIT_EDGE, 22, CT_FACE_D, 2, 1},
    };
    for (size_t i = 0; i < sizeof cell_cases / sizeof cell_cases[0]; i++) {
        int face = -9;
        int row = -9;
        int col = -9;
        ct_sticker_cell(cell_cases[i].orbit, cell_cases[i].sticker, &face, &row, &col);
        CT_ASSERT_EQ(face, cell_cases[i].face);
        CT_ASSERT_EQ(row, cell_cases[i].row);
        CT_ASSERT_EQ(col, cell_cases[i].col);
    }
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(CT_TARGET_ORBIT_CORNER, s, &face, &row, &col);
        ct_v3 center;
        ct_v3 normal;
        ct_sticker_world(3, face, row, col, &center, &normal);
        CT_ASSERT(center.x == 2 || center.x == -2);
        CT_ASSERT(center.y == 2 || center.y == -2);
        CT_ASSERT(center.z == 2 || center.z == -2);
        ct_sticker_cell(CT_TARGET_ORBIT_EDGE, s, &face, &row, &col);
        ct_sticker_world(3, face, row, col, &center, &normal);
        int zeros = (center.x == 0) + (center.y == 0) + (center.z == 0);
        CT_ASSERT_EQ(zeros, 1);
        CT_ASSERT(center.x == 2 || center.x == -2 || center.x == 0);
        CT_ASSERT(center.y == 2 || center.y == -2 || center.y == 0);
        CT_ASSERT(center.z == 2 || center.z == -2 || center.z == 0);
    }
    for (int p = 0; p < CT_TARGET_CORNER_PIECES; p++) {
        ct_v3 ref = {999, 999, 999};
        int seen = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            if (ct_corner_piece_of(s) != p) {
                continue;
            }
            int face = -1;
            int row = -1;
            int col = -1;
            ct_sticker_cell(CT_TARGET_ORBIT_CORNER, s, &face, &row, &col);
            ct_v3 center;
            ct_v3 normal;
            ct_sticker_world(3, face, row, col, &center, &normal);
            if (seen == 0) {
                ref = center;
            } else {
                CT_ASSERT_EQ(center.x, ref.x);
                CT_ASSERT_EQ(center.y, ref.y);
                CT_ASSERT_EQ(center.z, ref.z);
            }
            seen++;
        }
        CT_ASSERT_EQ(seen, CT_TARGET_CORNER_STICKERS_PER_PIECE);
    }
    for (int p = 0; p < CT_TARGET_EDGE_PIECES; p++) {
        ct_v3 ref = {999, 999, 999};
        int seen = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            if (ct_edge_piece_of(s) != p) {
                continue;
            }
            int face = -1;
            int row = -1;
            int col = -1;
            ct_sticker_cell(CT_TARGET_ORBIT_EDGE, s, &face, &row, &col);
            ct_v3 center;
            ct_v3 normal;
            ct_sticker_world(3, face, row, col, &center, &normal);
            if (seen == 0) {
                ref = center;
            } else {
                CT_ASSERT_EQ(center.x, ref.x);
                CT_ASSERT_EQ(center.y, ref.y);
                CT_ASSERT_EQ(center.z, ref.z);
            }
            seen++;
        }
        CT_ASSERT_EQ(seen, CT_TARGET_EDGE_STICKERS_PER_PIECE);
    }

    int cand_count = test_candidates(3, cands);
    CT_ASSERT_EQ(cand_count, 54);
    for (int i = 0; i < cand_count; i++) {
        ct_cube c;
        test_identity_cube(&c, 3);
        ct_apply(&c, cands[i]);
        ct_apply(&c, ct_move_inverse(cands[i]));
        test_extract(&c, 3, perm);
        bool identity = true;
        for (int cell = 0; cell < 54; cell++) {
            identity = identity && perm[cell] == cell;
        }
        CT_ASSERT(identity);
    }
    ct_move r1 = {CT_FACE_R, 1, 1, 1};
    ct_move r2 = {CT_FACE_R, 1, 1, 2};
    test_perm(r1, 3, perm);
    test_perm(r2, 3, other);
    bool composed = true;
    for (int cell = 0; cell < 54; cell++) {
        composed = composed && perm[perm[cell]] == other[cell];
    }
    CT_ASSERT(composed);
    test_perm(r1, 3, perm);
    CT_ASSERT_EQ(perm[test_compact(3, CT_FACE_U, 0, 2)], test_compact(3, CT_FACE_B, 2, 0));
    CT_ASSERT_EQ(perm[test_compact(3, CT_FACE_F, 0, 2)], test_compact(3, CT_FACE_U, 0, 2));
    CT_ASSERT_EQ(perm[test_compact(3, CT_FACE_B, 2, 0)], test_compact(3, CT_FACE_D, 0, 2));
    CT_ASSERT_EQ(perm[test_compact(3, CT_FACE_D, 0, 2)], test_compact(3, CT_FACE_F, 0, 2));
    CT_ASSERT_EQ(ct_sticker_perm(r1, 3, other), 0);
    CT_ASSERT_EQ(other[test_compact(3, CT_FACE_U, 0, 2)], test_compact(3, CT_FACE_B, 2, 0));
    CT_ASSERT_EQ(other[test_compact(3, CT_FACE_F, 0, 2)], test_compact(3, CT_FACE_U, 0, 2));
    CT_ASSERT_EQ(other[test_compact(3, CT_FACE_B, 2, 0)], test_compact(3, CT_FACE_D, 0, 2));
    CT_ASSERT_EQ(other[test_compact(3, CT_FACE_D, 0, 2)], test_compact(3, CT_FACE_F, 0, 2));
    for (int i = 0; i < cand_count; i++) {
        test_perm(cands[i], 3, perm);
        bool seen[54] = {false};
        bool bijection = true;
        for (int cell = 0; cell < 54; cell++) {
            if (seen[perm[cell]]) {
                bijection = false;
            }
            seen[perm[cell]] = true;
        }
        CT_ASSERT(bijection);
    }

    ct_move u2 = {CT_FACE_U, 1, 1, 2};
    test_perm(u2, 3, perm);
    CT_ASSERT(perm_pairs_match_golden(perm, 3, CT_TARGET_ORBIT_CORNER, u2_corner_pairs, 6));
    CT_ASSERT(perm_pairs_match_golden(perm, 3, CT_TARGET_ORBIT_EDGE, u2_edge_pairs, 4));
    for (int a = 0; a < CT_TARGET_STICKERS; a++) {
        for (int b = 0; b < CT_TARGET_STICKERS; b++) {
            if (a == b) {
                continue;
            }
            int cnt = ct_interchanges_for(CT_TARGET_ORBIT_CORNER, a, b, 3, res, 54);
            CT_ASSERT(cnt >= 0);
            CT_ASSERT_EQ(test_move_in(res, cnt, u2), pair_in_table(u2_corner_pairs, 6, a, b));
        }
    }

    for (int i = 0; i < 5; i++) {
        test_perm(quarter_samples[i], 3, perm);
        for (int a = 0; a < CT_TARGET_STICKERS; a++) {
            for (int b = a + 1; b < CT_TARGET_STICKERS; b++) {
                CT_ASSERT(!test_interchanges(perm,
                                             test_sticker_compact(CT_TARGET_ORBIT_CORNER, a, 3),
                                             test_sticker_compact(CT_TARGET_ORBIT_CORNER, b, 3)));
                CT_ASSERT(!test_interchanges(perm, test_sticker_compact(CT_TARGET_ORBIT_EDGE, a, 3),
                                             test_sticker_compact(CT_TARGET_ORBIT_EDGE, b, 3)));
            }
        }
    }

    test_perm(r2, 3, perm);
    CT_ASSERT(perm_pairs_match_golden(perm, 3, CT_TARGET_ORBIT_CORNER, r2_corner_pairs, 6));
    CT_ASSERT(perm_pairs_match_golden(perm, 3, CT_TARGET_ORBIT_EDGE, r2_edge_pairs, 4));
    ct_move m2 = {CT_FACE_L, 2, 2, 2};
    test_perm(m2, 3, perm);
    CT_ASSERT(perm_pairs_match_golden(perm, 3, CT_TARGET_ORBIT_CORNER, m2_corner_pairs, 0));
    CT_ASSERT(perm_pairs_match_golden(perm, 3, CT_TARGET_ORBIT_EDGE, m2_edge_pairs, 4));
    CT_ASSERT(test_interchanges(perm, test_compact(3, CT_FACE_U, 1, 1),
                                test_compact(3, CT_FACE_D, 1, 1)));
    CT_ASSERT(test_interchanges(perm, test_compact(3, CT_FACE_F, 1, 1),
                                test_compact(3, CT_FACE_B, 1, 1)));
    int m2_hits = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 2, 22, 3, res, 54);
    CT_ASSERT(m2_hits > 0);
    CT_ASSERT(test_move_in(res, m2_hits, m2));
    int m2_hits_b = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 8, 18, 3, res, 54);
    CT_ASSERT(m2_hits_b > 0);
    CT_ASSERT(test_move_in(res, m2_hits_b, m2));
    int m2_hits_c = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 0, 20, 3, res, 54);
    CT_ASSERT(m2_hits_c > 0);
    CT_ASSERT(test_move_in(res, m2_hits_c, m2));
    int m2_hits_d = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 10, 16, 3, res, 54);
    CT_ASSERT(m2_hits_d > 0);
    CT_ASSERT(test_move_in(res, m2_hits_d, m2));
    int corner_u2 = ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 3, res, 54);
    CT_ASSERT(corner_u2 > 0);
    CT_ASSERT(!test_move_in(res, corner_u2, m2));

    for (int i = 0; i < cand_count; i++) {
        test_perm(cands[i], 3, perm);
        CT_ASSERT_EQ(ct_sticker_perm(cands[i], 3, other), 0);
        CT_ASSERT(memcmp(perm, other, 54) == 0);
    }
    ct_move ok = {CT_FACE_R, 1, 1, 1};
    CT_ASSERT(ct_sticker_perm((ct_move){CT_FACE_R, 1, 1, 0}, 3, other) < 0);
    CT_ASSERT(ct_sticker_perm((ct_move){CT_FACE_R, 1, 1, 4}, 3, other) < 0);
    CT_ASSERT(ct_sticker_perm((ct_move){6, 1, 1, 1}, 3, other) < 0);
    CT_ASSERT(ct_sticker_perm((ct_move){-1, 1, 1, 1}, 3, other) < 0);
    CT_ASSERT(ct_sticker_perm((ct_move){CT_FACE_R, 0, 1, 1}, 3, other) < 0);
    CT_ASSERT(ct_sticker_perm((ct_move){CT_FACE_R, 2, 1, 1}, 3, other) < 0);
    CT_ASSERT(ct_sticker_perm((ct_move){CT_FACE_R, 1, 4, 1}, 3, other) < 0);
    CT_ASSERT(ct_sticker_perm(ok, 1, other) < 0);
    CT_ASSERT(ct_sticker_perm(ok, 0, other) < 0);
    CT_ASSERT(ct_sticker_perm(ok, -2, other) < 0);
    CT_ASSERT(ct_sticker_perm(ok, 7, other) < 0);
    CT_ASSERT_EQ(ct_sticker_perm(ok, 2, other), 0);
    CT_ASSERT_EQ(ct_sticker_perm(ok, 6, other), 0);

    CT_ASSERT_EQ(ct_candidate_moves(1, NULL, 0), 0);
    CT_ASSERT_EQ(ct_candidate_moves(0, NULL, 0), 0);
    CT_ASSERT_EQ(ct_candidate_moves(-3, NULL, 0), 0);
    CT_ASSERT_EQ(ct_candidate_moves(2, NULL, 0), 18);
    CT_ASSERT_EQ(ct_candidate_moves(3, NULL, 0), 54);
    CT_ASSERT_EQ(ct_candidate_moves(4, NULL, 0), 54);
    CT_ASSERT_EQ(ct_candidate_moves(5, NULL, 0), 108);
    CT_ASSERT_EQ(ct_candidate_moves(6, NULL, 0), 108);
    CT_ASSERT_EQ(ct_candidate_moves(7, NULL, 0), 180);
    CT_ASSERT_EQ(ct_candidate_moves(3, cands, 54), 54);
    for (int i = 0; i < 54; i++) {
        CT_ASSERT(cands[i].face >= CT_FACE_U && cands[i].face <= CT_FACE_D);
        CT_ASSERT(cands[i].quarter >= 1 && cands[i].quarter <= CT_MOVE_QUARTERS - 1);
        CT_ASSERT(cands[i].depth_from >= 1 && cands[i].depth_from <= cands[i].depth_to);
        CT_ASSERT(cands[i].depth_to <= 2);
        for (int j = i + 1; j < 54; j++) {
            CT_ASSERT(!test_move_eq(cands[i], cands[j]));
        }
    }
    ct_move small[4];
    for (int i = 0; i < 4; i++) {
        small[i] = (ct_move){-9, -9, -9, -9};
    }
    CT_ASSERT_EQ(ct_candidate_moves(3, small, 3), 54);
    CT_ASSERT(test_move_eq(small[0], cands[0]));
    CT_ASSERT(test_move_eq(small[1], cands[1]));
    CT_ASSERT(test_move_eq(small[2], cands[2]));
    CT_ASSERT_EQ(small[3].face, -9);
    for (int i = 0; i < 4; i++) {
        small[i] = (ct_move){-9, -9, -9, -9};
    }
    CT_ASSERT_EQ(ct_candidate_moves(3, small, 0), 54);
    CT_ASSERT_EQ(small[0].face, -9);
    CT_ASSERT_EQ(ct_candidate_moves(3, small, -5), 54);
    CT_ASSERT_EQ(small[0].face, -9);

    CT_ASSERT(ct_interchanges_for(2, 0, 2, 3, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(-1, 0, 2, 3, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 5, 5, 3, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, -1, 2, 3, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, CT_TARGET_STICKERS, 3, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 0, -7, 3, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 2, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 7, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 0, res, 54) < 0);
    CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, -4, res, 54) < 0);
    for (int n = 4; n <= 6; n++) {
        CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, n, res, 54) < 0);
        CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 2, 22, n, res, 54) < 0);
        CT_ASSERT(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, n, NULL, 0) < 0);
    }
    for (int i = 0; i < 4; i++) {
        res[i] = (ct_move){-9, -9, -9, -9};
    }
    CT_ASSERT_EQ(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 3, res, 1), 2);
    CT_ASSERT(test_move_eq(res[0], (ct_move){CT_FACE_U, 1, 1, 2}));
    CT_ASSERT_EQ(res[1].face, -9);
    for (int i = 0; i < 4; i++) {
        res[i] = (ct_move){-9, -9, -9, -9};
    }
    CT_ASSERT_EQ(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 3, res, 0), 2);
    CT_ASSERT_EQ(res[0].face, -9);
    CT_ASSERT_EQ(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 3, res, -5), 2);
    CT_ASSERT_EQ(res[0].face, -9);
    CT_ASSERT_EQ(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 3, NULL, 0), 2);
    CT_ASSERT_EQ(ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 3, res, 54), 2);
    CT_ASSERT(test_move_in(res, 2, (ct_move){CT_FACE_U, 1, 1, 2}));
    CT_ASSERT(test_move_in(res, 2, (ct_move){CT_FACE_U, 1, 2, 2}));

    CT_ASSERT_EQ(ct_candidate_moves(3, cands, CT_INTERCHANGE_MAX_CANDIDATES), 54);
    for (int i = 0; i < cand_count; i++) {
        test_perm(cands[i], 3, cand_perms[i]);
    }
    for (int orbit = CT_TARGET_ORBIT_CORNER; orbit <= CT_TARGET_ORBIT_EDGE; orbit++) {
        for (int a = 0; a < CT_TARGET_STICKERS; a++) {
            for (int b = 0; b < CT_TARGET_STICKERS; b++) {
                if (a == b) {
                    continue;
                }
                int ca = test_sticker_compact(orbit, a, 3);
                int cb = test_sticker_compact(orbit, b, 3);
                int api_cnt = ct_interchanges_for(orbit, a, b, 3, res, 54);
                CT_ASSERT(api_cnt >= 0);
                bool same = true;
                for (int i = 0; i < cand_count; i++) {
                    if (test_interchanges(cand_perms[i], ca, cb) !=
                        test_move_in(res, api_cnt, cands[i])) {
                        same = false;
                    }
                }
                CT_ASSERT(same);
            }
        }
    }
}
