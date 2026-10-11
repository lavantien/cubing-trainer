#include "config/config.h"
#include "gen/targets.h"
#include "engine/cube.h"
#include <stdio.h>

static int compact(int orbit, int sticker) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return face * 9 + row * 3 + col;
}

int main(void) {
    printf("corner cells 0..7:");
    for (int s = 0; s < 24; s += 3) {
        printf(" %d", compact(CT_ORBIT_CORNER, s));
    }
    printf("\nedge cells 0,2,4,6..:");
    for (int s = 0; s < 24; s += 2) {
        printf(" %d", compact(CT_ORBIT_EDGE, s));
    }
    printf("\ncorner piece_of: 0=%d 1=%d 2=%d 3=%d\n", ct_corner_piece_of(0), ct_corner_piece_of(1),
           ct_corner_piece_of(2), ct_corner_piece_of(3));
    printf("edge piece_of: 0=%d 2=%d 4=%d 6=%d\n", ct_edge_piece_of(0), ct_edge_piece_of(2),
           ct_edge_piece_of(4), ct_edge_piece_of(6));
    return 0;
}
