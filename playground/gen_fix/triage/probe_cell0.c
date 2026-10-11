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
    const char *algs[] = {"L F R' F' L' F R F'", "R2 S' R2 S", "U M' U2 M U"};
    for (int a = 0; a < 3; a++) {
        ct_move seq[64];
        int n = ct_notation_parse(algs[a], 3, seq, 64);
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
        int corner_disp = 0;
        int edge_disp = 0;
        int other_disp = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int face = -1, row = -1, col = -1;
            ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
            int cell = face * 9 + row * 3 + col;
            corner_disp += st[cell] != (uint8_t)cell;
            ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
            cell = face * 9 + row * 3 + col;
            edge_disp += st[cell] != (uint8_t)cell;
        }
        for (int q = 0; q < 54; q++) {
            int known = 0;
            for (int s = 0; s < CT_TARGET_STICKERS && !known; s++) {
                int face = -1, row = -1, col = -1;
                ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
                known |= face * 9 + row * 3 + col == q;
                ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
                known |= face * 9 + row * 3 + col == q;
            }
            if (!known && st[q] != (uint8_t)q) other_disp++;
        }
        printf("%-24s st0=%2d corner_disp=%d edge_disp=%d other_disp=%d\n", algs[a], st[0],
               corner_disp, edge_disp, other_disp);
    }
    return 0;
}
