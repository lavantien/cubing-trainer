#include "engine/cube.h"
#include "engine/notation.h"

#include <stdio.h>
#include <string.h>

static int raw_of(int face, int row, int col) {
    return face * 49 + row * 7 + col;
}

static int compact_of(int face, int row, int col) {
    return face * 9 + row * 3 + col;
}

static void sticker_name(int cell, char *out) {
    static const char fl[3][2] = {{'L', 'R'}, {'D', 'U'}, {'B', 'F'}};
    int face = cell / 9;
    int row = (cell % 9) / 3;
    int col = cell % 3;
    ct_v3 center;
    ct_v3 normal;
    ct_sticker_world(3, face, row, col, &center, &normal);
    int coords[3] = {center.x / 2, center.y / 2, center.z / 2};
    int naxis = normal.x != 0 ? 0 : (normal.y != 0 ? 1 : 2);
    out[0] = fl[naxis][coords[naxis] > 0];
    int pos = 1;
    for (int axis = 0; axis < 3; axis++) {
        if (axis != naxis && coords[axis] != 0)
            out[pos++] = fl[axis][coords[axis] > 0];
    }
    out[pos] = 0;
}

int main(int argc, char **argv) {
    if (argc < 2)
        return 1;
    ct_move moves[32];
    int count = ct_notation_parse(argv[1], 3, moves, 32);
    if (count < 0) {
        printf("parse %d\n", count);
        return 1;
    }
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = 3;
    for (int f = 0; f < 6; f++)
        for (int r = 0; r < 3; r++)
            for (int cc = 0; cc < 3; cc++)
                c.facelet[raw_of(f, r, cc)] = (uint8_t)compact_of(f, r, cc);
    for (int i = 0; i < count; i++)
        ct_apply(&c, moves[i]);
    uint8_t state[54];
    for (int f = 0; f < 6; f++)
        for (int r = 0; r < 3; r++)
            for (int cc = 0; cc < 3; cc++)
                state[compact_of(f, r, cc)] = c.facelet[raw_of(f, r, cc)];
    int perm[54];
    for (int q = 0; q < 54; q++)
        perm[state[q]] = q;
    int moved = 0;
    bool visited[54] = {false};
    for (int q = 0; q < 54; q++) {
        if (perm[q] == q)
            continue;
        moved++;
        if (visited[q])
            continue;
        int cur = q;
        int len = 0;
        printf("cycle:");
        do {
            visited[cur] = true;
            char name[4];
            sticker_name(cur, name);
            printf(" %d(%s)", cur, name);
            cur = perm[cur];
            len++;
        } while (cur != q);
        printf(" len=%d\n", len);
    }
    printf("moved=%d\n", moved);
    return 0;
}
