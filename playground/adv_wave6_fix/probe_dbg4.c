#include "engine/notation.h"
#include "engine/transform.h"
#include "config/config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    ct_move seq[64];
    int len = ct_notation_parse("U", 3, seq, 64);
    (void)len;
    const int n = 3;
    int writes[6][4][4] = {{{0}}};
    int h = n - 1;
    int axis = 1;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                ct_v3 center;
                ct_v3 normal;
                ct_sticker_world(n, face, row, col, &center, &normal);
                int u = center.x * 0 + center.y * 1 + center.z * 0;
                int rot = (u == h) ? 1 : 0;
                for (int k = 0; k < rot; k++) {
                    int nx = center.z;
                    int nz = -center.x;
                    center.x = nx;
                    center.z = nz;
                    int nnx = normal.z;
                    int nnz = -normal.x;
                    normal.x = nnx;
                    normal.z = nnz;
                }
                ct_cell dst = ct_world_sticker(center, normal, n);
                if (dst.row < 0 || dst.row >= n || dst.col < 0 || dst.col >= n || dst.face < 0) {
                    printf("BAD dst from face %d row %d col %d -> face %d row %d col %d\n", face,
                           row, col, dst.face, dst.row, dst.col);
                    continue;
                }
                writes[dst.face][dst.row][dst.col]++;
            }
        }
    }
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (writes[f][r][c] != (r < n && c < n ? 1 : 0)) {
                    printf("face %d row %d col %d writes %d\n", f, r, c, writes[f][r][c]);
                }
            }
        }
    }
    return 0;
}
