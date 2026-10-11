#include <stdio.h>
#include <string.h>

#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/targets.h"

static int priority(int face) {
    if (face == CT_TARGET_FACE_U || face == CT_TARGET_FACE_D) {
        return 2;
    }
    if (face == CT_TARGET_FACE_F || face == CT_TARGET_FACE_B) {
        return 1;
    }
    return 0;
}

static int hi_cell(int face, int row, int col) {
    int cell = face * 9 + row * 3 + col;
    for (int s = 0; s < 24; s++) {
        int sface = -1;
        int srow = -1;
        int scol = -1;
        ct_sticker_cell(CT_TARGET_ORBIT_EDGE, s, &sface, &srow, &scol);
        if (sface != face || srow != row || scol != col) {
            continue;
        }
        int piece = ct_edge_piece_of(s);
        int mate = -1;
        for (int t = 0; t < 24; t++) {
            if (t != s && ct_edge_piece_of(t) == piece) {
                mate = t;
            }
        }
        int mface = -1;
        int mrow = -1;
        int mcol = -1;
        ct_sticker_cell(CT_TARGET_ORBIT_EDGE, mate, &mface, &mrow, &mcol);
        return priority(face) > priority(mface);
    }
    return cell;
}

int main(void) {
    static const char *const parity_rows[] = {
        "R U R' F' R U R' U' R' F R2 U' R' U'",
        "U D' R' F' R U R' U' R' F R2 U' R' U' R D",
    };
    for (size_t r = 0; r < sizeof parity_rows / sizeof parity_rows[0]; r++) {
        ct_move seq[64];
        int n = ct_notation_parse(parity_rows[r], 3, seq, 64);
        if (n < 0) {
            printf("row %zu parse failed %d\n", r, n);
            return 1;
        }
        ct_cube c;
        memset(&c, 0, sizeof c);
        c.n = 3;
        for (int face = 0; face < 6; face++) {
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    c.facelet[face * 49 + row * 7 + col] = (uint8_t)(face * 9 + row * 3 + col);
                }
            }
        }
        for (int i = 0; i < n; i++) {
            ct_apply(&c, seq[i]);
        }
        printf("row %zu moved edges:", r);
        for (int s = 0; s < 24; s++) {
            int face = -1;
            int row = -1;
            int col = -1;
            ct_sticker_cell(CT_TARGET_ORBIT_EDGE, s, &face, &row, &col);
            int cell = face * 9 + row * 3 + col;
            int src = c.facelet[face * 49 + row * 7 + col];
            if (src == cell) {
                continue;
            }
            int src_face = src / 9;
            int src_row = (src % 9) / 3;
            int src_col = src % 3;
            int oriented = hi_cell(src_face, src_row, src_col) == hi_cell(face, row, col);
            printf(" slot(face%d,row%d,col%d)->from(face%d) %s", face, row, col, src_face,
                   oriented ? "oriented" : "FLIPPED");
        }
        printf("\n");
    }
    return 0;
}
