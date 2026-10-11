#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/targets.h"
#include <stdio.h>
#include <string.h>

static int raw_of(int face, int row, int col) {
    return face * 49 + row * 7 + col;
}

int main(void) {
    const char *cands[] = {
        "R U' R U R U R U' R' U' R2",
        "R2 U R U R' U' R' U' R' U R2",
        "R2 U' R' U' R U R U R U' R",
        "F2 U L R' F2 L' R U' F2 U2",
        "M2 U M U2 M' U M2",
        "L F R' F' L' F R F'",
        "L F R' F' L' F R F' R2 U' R' U' R U R U R U' R",
        "L F R' F' L' F R F' R U' R U R U R U' R' U' R2",
    };
    int centers[6] = {4, 13, 22, 31, 40, 49};
    for (int a = 0; a < 8; a++) {
        ct_move seq[64];
        int n = ct_notation_parse(cands[a], 3, seq, 64);
        if (n <= 0) {
            printf("%-32s parse=%d\n", cands[a], n);
            continue;
        }
        ct_cube c;
        memset(&c, 0, sizeof c);
        c.n = 3;
        for (int face = 0; face < 6; face++) {
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    c.facelet[raw_of(face, row, col)] = (uint8_t)(face * 9 + row * 3 + col);
                }
            }
        }
        for (int i = 0; i < n; i++) {
            ct_apply(&c, seq[i]);
        }
        uint8_t st[54];
        for (int face = 0; face < 6; face++) {
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    st[face * 9 + row * 3 + col] = c.facelet[raw_of(face, row, col)];
                }
            }
        }
        int centers_ok = 1;
        for (int i = 0; i < 6; i++) {
            centers_ok &= st[centers[i]] == (uint8_t)centers[i];
        }
        int corner_disp = 0;
        int edge_disp = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int face = -1, row = -1, col = -1;
            ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
            corner_disp += st[face * 9 + row * 3 + col] != (uint8_t)(face * 9 + row * 3 + col);
            ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
            edge_disp += st[face * 9 + row * 3 + col] != (uint8_t)(face * 9 + row * 3 + col);
        }
        printf("%-32s n=%2d centers=%d corner=%2d edge=%2d\n", cands[a], n, centers_ok, corner_disp,
               edge_disp);
    }
    return 0;
}
