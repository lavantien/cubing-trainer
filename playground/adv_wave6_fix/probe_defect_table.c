#include "engine/notation.h"
#include "gen/classify.h"
#include "gen/targets.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { N = 3, CELLS = 54 };

static const char *const algs[] = {
    "L F R' F' L' F R F'",
    "U' B D' B' U B D B'",
    "D F2 D' B2 D F2 D' B2",
    "R B L B' R' B L' B'",
    "U B' D' B U' B' D B",
    "L F' L B2 L' F L B2 L2",
    "R2 B2 R F R' B2 R F' R",
    "U R' B2 R F R' B2 R F' U'",
    "F R' U2 R F' R' F U2 F' R",
    "D B' R2 B D' B' D R2 D' B",
    "R U2 R' U' L2 U R U' L2 U' R'",
    "U F2 U' F2 U' R2 U F2 U F2 U' R2",
    "R2 S' R2 S",
    "U M' U2 M U",
    "M2 U M' U2 M U M2",
    "U' M U2 M' U2 M U2 M' U'",
    "U F E F' U' F E' F'",
    "M F R' F' M' F R F'",
    "R' F R' S2 R F' R' S2 R2",
    "U F' E2 F U F' E2 F U2",
    "F' U R' E R U' R' E' R F",
    "F U2 R' D' R U2 R' D R F'",
    "U' R E' R' U2 R E R' U'",
    "R2 U S' U2 S U R2",
    "U' R2 Uw R2 Uw' F2 Uw R2 Uw' F2 R2 U",
};

static int raw_of(int face, int row, int col) {
    return face * 7 * 7 + row * 7 + col;
}

static int compact_of(int face, int row, int col) {
    return face * N * N + row * N + col;
}

static int sticker_compact(int orbit, int sticker) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return compact_of(face, row, col);
}

static void simulate(const ct_move *seq, int len, uint8_t st[CELLS]) {
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = N;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < N; row++) {
            for (int col = 0; col < N; col++) {
                c.facelet[raw_of(face, row, col)] = (uint8_t)compact_of(face, row, col);
            }
        }
    }
    for (int i = 0; i < len; i++) {
        ct_apply(&c, seq[i]);
    }
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < N; row++) {
            for (int col = 0; col < N; col++) {
                st[compact_of(face, row, col)] = c.facelet[raw_of(face, row, col)];
            }
        }
    }
}

static int face_class(int face) {
    if (face == CT_TARGET_FACE_U || face == CT_TARGET_FACE_D)
        return 0;
    if (face == CT_TARGET_FACE_F || face == CT_TARGET_FACE_B)
        return 1;
    return 2;
}

static int face_of_cell(int cell) {
    return cell / (N * N);
}

int main(void) {
    bool corner_ud[CT_TARGET_STICKERS] = {false};
    bool edge_top[CT_TARGET_STICKERS] = {false};
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int cf = -1;
        int cr = -1;
        int cc = -1;
        ct_sticker_cell(CT_ORBIT_CORNER, s, &cf, &cr, &cc);
        corner_ud[s] = face_class(cf) == 0;
    }
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int piece = ct_edge_piece_of(s);
        int best = 99;
        int best_s = -1;
        for (int t = 0; t < CT_TARGET_STICKERS; t++) {
            if (ct_edge_piece_of(t) != piece)
                continue;
            int tf = -1;
            int tr = -1;
            int tc = -1;
            ct_sticker_cell(CT_ORBIT_EDGE, t, &tf, &tr, &tc);
            int cls = face_class(tf);
            if (cls < best) {
                best = cls;
                best_s = t;
            }
        }
        edge_top[best_s] = true;
    }
    bool corner_ud_cell[CELLS] = {false};
    bool edge_top_cell[CELLS] = {false};
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        corner_ud_cell[sticker_compact(CT_ORBIT_CORNER, s)] = corner_ud[s];
        edge_top_cell[sticker_compact(CT_ORBIT_EDGE, s)] = edge_top[s];
    }
    for (size_t i = 0; i < sizeof algs / sizeof algs[0]; i++) {
        ct_move seq[64];
        int len = ct_notation_parse(algs[i], N, seq, 64);
        if (len <= 0) {
            printf("%2zu %-40s parse fail\n", i, algs[i]);
            continue;
        }
        uint8_t st[CELLS];
        simulate(seq, len, st);
        int cdefect = 0;
        int edefect = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int c = sticker_compact(CT_ORBIT_CORNER, s);
            if (st[c] != (uint8_t)c && corner_ud_cell[st[c]] != corner_ud_cell[c])
                cdefect++;
            int e = sticker_compact(CT_ORBIT_EDGE, s);
            if (st[e] != (uint8_t)e && edge_top_cell[st[e]] != edge_top_cell[e])
                edefect++;
        }
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        ct_classify_alg(algs[i], &r);
        printf("%2zu %-40s cls=%-14s corner_defect_cells=%d edge_defect_cells=%d\n", i, algs[i],
               ct_classification_label(r.cls), cdefect, edefect);
    }
    return 0;
}
