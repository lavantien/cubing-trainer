#include <stdio.h>
#include <string.h>

#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/targets.h"

#define CELL(f, r, c) ((f) * 49 + (r) * 7 + (c))

static const char face_letter[6] = {'U', 'L', 'F', 'R', 'B', 'D'};

static int sticker_cell(int st) {
    int face, row, col;
    ct_sticker_cell(st < 24 ? 0 : 1, st % 24, &face, &row, &col);
    return CELL(face, row, col);
}

static void cell_rc(int cell, int *face, int *row, int *col) {
    *face = cell / 49;
    *row = (cell % 49) / 7;
    *col = cell % 7;
}

int main(int argc, char **argv) {
    ct_cube c;
    ct_cube_init(&c, 3);
    for (int st = 0; st < 48; st++)
        c.facelet[sticker_cell(st)] = (uint8_t)st;
    for (int f = 0; f < 6; f++)
        c.facelet[CELL(f, 1, 1)] = (uint8_t)(48 + f);
    ct_move mv[64];
    int n = ct_notation_parse(argv[1], 3, mv, 64);
    printf("parse=%d\n", n);
    for (int i = 0; i < n; i++)
        ct_apply(&c, mv[i]);
    for (int st = 0; st < 48; st++) {
        int dst_cell = -1;
        for (int f = 0; f < 6 && dst_cell < 0; f++)
            for (int r = 0; r < 3 && dst_cell < 0; r++)
                for (int cl = 0; cl < 3 && dst_cell < 0; cl++)
                    if (c.facelet[CELL(f, r, cl)] == st)
                        dst_cell = CELL(f, r, cl);
        int sf, sr, sc, df, dr, dc;
        cell_rc(sticker_cell(st), &sf, &sr, &sc);
        cell_rc(dst_cell, &df, &dr, &dc);
        if (sticker_cell(st) != dst_cell)
            printf("%c(%d,%d)%s -> %c(%d,%d)%s\n", face_letter[sf], sr, sc,
                   st < 24 ? "c" : "e", face_letter[df], dr, dc, st < 24 ? "c" : "e");
    }
    (void)argc;
    return 0;
}
