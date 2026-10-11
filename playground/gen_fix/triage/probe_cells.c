#include "config/config.h"
#include "gen/targets.h"
#include <stdio.h>

int main(void) {
    for (int orbit = 0; orbit < 2; orbit++) {
        for (int s = 0; s < 4; s++) {
            int face = -1, row = -1, col = -1;
            ct_sticker_cell(orbit, s, &face, &row, &col);
            printf("orbit %d sticker %d -> face %d row %d col %d cell %d\n", orbit, s, face, row,
                   col, face * 9 + row * 3 + col);
        }
    }
    int zero_corner = 0;
    int zero_edge = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1, row = -1, col = -1;
        ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
        if (face * 9 + row * 3 + col == 0) zero_corner = 1;
        ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
        if (face * 9 + row * 3 + col == 0) zero_edge = 1;
    }
    printf("cell0 corner=%d edge=%d\n", zero_corner, zero_edge);
    return 0;
}
