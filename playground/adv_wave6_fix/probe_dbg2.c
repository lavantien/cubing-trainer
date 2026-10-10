#include "engine/notation.h"
#include "config/config.h"
#include "gen/targets.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { N = 3, CELLS = 54 };

static int raw_of(int face, int row, int col) {
    return face * 4 * 4 + row * 4 + col;
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

static void simulate(const char *alg, uint8_t st[CELLS]) {
    ct_move seq[64];
    int len = ct_notation_parse(alg, N, seq, 64);
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

int main(void) {
    static const char *const algs[] = {"U", "R", "S", "M", "E"};
    for (size_t i = 0; i < sizeof algs / sizeof algs[0]; i++) {
        uint8_t st[CELLS];
        simulate(algs[i], st);
        printf("alg %s\n", algs[i]);
        int seen[CELLS];
        for (int q = 0; q < CELLS; q++) {
            seen[q] = 0;
        }
        for (int q = 0; q < CELLS; q++) {
            seen[st[q]]++;
        }
        for (int q = 0; q < CELLS; q++) {
            if (seen[q] != 1) {
                printf("  source cell %d used %d times\n", q, seen[q]);
            }
        }
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int c = sticker_compact(CT_ORBIT_CORNER, s);
            if (st[c] != (uint8_t)c) {
                printf("  corner sticker %d cell %d st %d\n", s, c, st[c]);
            }
        }
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int c = sticker_compact(CT_ORBIT_EDGE, s);
            if (st[c] != (uint8_t)c) {
                printf("  edge sticker %d cell %d st %d\n", s, c, st[c]);
            }
        }
    }
    return 0;
}
