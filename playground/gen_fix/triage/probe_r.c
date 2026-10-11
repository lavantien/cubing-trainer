#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include <stdio.h>
#include <string.h>

static int raw_of(int face, int row, int col) {
    return face * 16 + row * 4 + col;
}

int main(void) {
    ct_move seq[8];
    int n = ct_notation_parse("R", 3, seq, 8);
    printf("n=%d face=%d depth=%d-%d q=%d\n", n, seq[0].face, seq[0].depth_from, seq[0].depth_to, seq[0].quarter);
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
    int disp = 0;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                disp += c.facelet[raw_of(face, row, col)] != (uint8_t)(face * 9 + row * 3 + col);
            }
        }
    }
    printf("displaced=%d\n", disp);
    return 0;
}
