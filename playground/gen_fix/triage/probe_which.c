#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include <stdio.h>
#include <string.h>

static int raw_of(int face, int row, int col) {
    return face * 49 + row * 7 + col;
}

int main(void) {
    ct_move seq[8];
    ct_notation_parse("R", 3, seq, 8);
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
    ct_apply(&c, seq[0]);
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                int cell = face * 9 + row * 3 + col;
                int raw = raw_of(face, row, col);
                if (c.facelet[raw] != (uint8_t)cell) {
                    printf("face %d row %d col %d (cell %d): %d -> %d\n", face, row, col, cell,
                           cell, c.facelet[raw]);
                }
            }
        }
    }
    return 0;
}
