#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/classify.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

static int raw_of(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int compact_of(int face, int row, int col) {
    return face * CT_CLASSIFY_N * CT_CLASSIFY_N + row * CT_CLASSIFY_N + col;
}

static void simulate(const ct_move *seq, int len, uint8_t out[CT_CLASSIFY_CELLS]) {
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = CT_CLASSIFY_N;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_CLASSIFY_N; row++) {
            for (int col = 0; col < CT_CLASSIFY_N; col++) {
                c.facelet[raw_of(face, row, col)] = (uint8_t)compact_of(face, row, col);
            }
        }
    }
    for (int i = 0; i < len; i++) {
        ct_apply(&c, seq[i]);
    }
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_CLASSIFY_N; row++) {
            for (int col = 0; col < CT_CLASSIFY_N; col++) {
                out[compact_of(face, row, col)] = c.facelet[raw_of(face, row, col)];
            }
        }
    }
}

static int sticker_cell(int orbit, int sticker) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return compact_of(face, row, col);
}

static void report(const char *alg) {
    ct_move seq[CT_CLASSIFY_MOVES_MAX];
    int n = ct_notation_parse(alg, CT_CLASSIFY_N, seq, CT_CLASSIFY_MOVES_MAX);
    if (n < 0) {
        printf("%-46s parse error %d\n", alg, n);
        return;
    }
    uint8_t st[CT_CLASSIFY_CELLS];
    simulate(seq, n, st);
    int dest[CT_CLASSIFY_CELLS];
    for (int q = 0; q < CT_CLASSIFY_CELLS; q++) {
        dest[st[q]] = q;
    }
    for (int orbit = CT_ORBIT_CORNER; orbit <= CT_ORBIT_EDGE; orbit++) {
        const char *name = orbit == CT_ORBIT_CORNER ? "corner" : "edge";
        int moved = 0;
        int displaced = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int cell = sticker_cell(orbit, s);
            if (dest[cell] == cell) {
                continue;
            }
            moved++;
            int other = -1;
            for (int t = 0; t < CT_TARGET_STICKERS; t++) {
                if (sticker_cell(orbit, t) == dest[cell]) {
                    other = t;
                    break;
                }
            }
            int self_piece = orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(s) : ct_edge_piece_of(s);
            int other_piece =
                orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(other) : ct_edge_piece_of(other);
            if (other_piece != self_piece) {
                displaced++;
            }
        }
        printf("%-46s %-6s moved=%d displaced=%d\n", alg, name, moved, displaced);
    }
    ct_classify_result r;
    memset(&r, 0, sizeof r);
    ct_classify_alg(alg, &r);
    printf("%-46s -> %-17s %s\n", "", ct_classification_label(r.cls), r.rationale);
}

int main(void) {
    report("R U R' U' R U R' U' R U R' U'");
    report("R' D R U2 R' D' R U2");
    report("R D' R' D R D' R' D U D' R D R' D' R D R' U'");
    report("R D' R' D R D' R' D U2 R D' R' D R D' R' D U2");
    report("R' D' R D U R' D R D' R' D' R D U'");
    report("M' U M' U M' U2 M U M U M U2");
    report("R U R' U' R' F R2 U' R' U' R U R' F'");
    return 0;
}
