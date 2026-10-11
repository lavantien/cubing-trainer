#include "engine/cube.h"
#include "engine/notation.h"
#include "config/config.h"
#include "gen/targets.h"
#include <stdio.h>
#include <string.h>

enum { CELLS = 54, N = 3 };

static int compact_of(int face, int row, int col) {
    return face * N * N + row * N + col;
}

static int raw_of(int face, int row, int col) {
    return face * 16 + row * 4 + col;
}

int main(void) {
    const char *alg = "L F R' F' L' F R F' R2 U' R' U' R U R U R U' R";
    ct_move seq[64];
    int n = ct_notation_parse(alg, N, seq, 64);
    printf("parsed %d\n", n);
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
    for (int i = 0; i < n; i++) {
        ct_apply(&c, seq[i]);
    }
    uint8_t st[CELLS];
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < N; row++) {
            for (int col = 0; col < N; col++) {
                st[compact_of(face, row, col)] = c.facelet[raw_of(face, row, col)];
            }
        }
    }
    for (int orbit = 0; orbit < 2; orbit++) {
        int piece[24];
        int np = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int face = -1, row = -1, col = -1;
            ct_sticker_cell(orbit, s, &face, &row, &col);
            piece[np++] = orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(s) : ct_edge_piece_of(s);
        }
        int maxp = 0;
        for (int s = 0; s < np; s++) {
            if (piece[s] > maxp) maxp = piece[s];
        }
        bool moved[32] = {false};
        int moved_n = 0;
        for (int s = 0; s < np; s++) {
            int face = -1, row = -1, col = -1;
            ct_sticker_cell(orbit, s, &face, &row, &col);
            int cell = compact_of(face, row, col);
            if (st[cell] != (uint8_t)cell) {
                moved[s] = true;
                moved_n++;
            }
        }
        printf("orbit %d moved stickers %d\n", orbit, moved_n);
        int seen[32] = {0};
        for (int s = 0; s < np; s++) {
            if (seen[piece[s]] || !moved[s]) continue;
            int len = 0;
            int cur = s;
            int cyc[24];
            while (!seen[piece[cur]]) {
                seen[piece[cur]] = 1;
                cyc[len++] = piece[cur];
                int face = -1, row = -1, col = -1;
                ct_sticker_cell(orbit, cur, &face, &row, &col);
                int cell = compact_of(face, row, col);
                int dest_cell = -1;
                for (int q = 0; q < CELLS; q++) {
                    if (st[q] == (uint8_t)cell) dest_cell = q;
                }
                int next = -1;
                for (int t = 0; t < np; t++) {
                    int face2 = -1, row2 = -1, col2 = -1;
                    ct_sticker_cell(orbit, t, &face2, &row2, &col2);
                    if (compact_of(face2, row2, col2) == dest_cell) next = t;
                }
                cur = next;
            }
            printf("  piece cycle len %d:", len);
            for (int i = 0; i < len; i++) printf(" %d", cyc[i]);
            printf("\n");
        }
    }
    return 0;
}
