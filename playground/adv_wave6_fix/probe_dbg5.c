#include "engine/notation.h"
#include "config/config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int x;
    int y;
    int z;
} lv3;

static lv3 lrot(lv3 v, int axis) {
    lv3 r = v;
    if (axis == 0) {
        r.y = -v.z;
        r.z = v.y;
    } else if (axis == 1) {
        r.x = v.z;
        r.z = -v.x;
    } else {
        r.x = -v.y;
        r.y = v.x;
    }
    return r;
}

int main(void) {
    ct_move seq[64];
    int len = ct_notation_parse("U", 3, seq, 64);
    (void)len;
    const int n = 3;
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = n;
    for (int i = 0; i < (int)sizeof c.facelet; i++) {
        c.facelet[i] = (uint8_t)(100 + i);
    }
    ct_apply(&c, seq[0]);
    uint8_t via_engine[96];
    memcpy(via_engine, c.facelet, sizeof via_engine);

    memset(&c, 0, sizeof c);
    c.n = n;
    for (int i = 0; i < (int)sizeof c.facelet; i++) {
        c.facelet[i] = (uint8_t)(100 + i);
    }
    ct_cube src = c;
    const ct_face_frame *fr = &ct_frames[seq[0].face];
    int axis = fr->normal.x != 0 ? 0 : fr->normal.y != 0 ? 1 : 2;
    int sign = fr->normal.x != 0 ? fr->normal.x : fr->normal.y != 0 ? fr->normal.y : fr->normal.z;
    int h = n - 1;
    int u_lo = h - 2 * (seq[0].depth_to - 1);
    int u_hi = h - 2 * (seq[0].depth_from - 1);
    int steps = ((-sign * seq[0].quarter) % 4 + 4) % 4;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                ct_v3 center;
                ct_v3 normal;
                ct_sticker_world(n, face, row, col, &center, &normal);
                int u = center.x * fr->normal.x + center.y * fr->normal.y + center.z * fr->normal.z;
                int rot = (u >= u_lo && u <= u_hi) ? steps : 0;
                lv3 lc = {center.x, center.y, center.z};
                lv3 ln = {normal.x, normal.y, normal.z};
                for (int k = 0; k < rot; k++) {
                    lc = lrot(lc, axis);
                    ln = lrot(ln, axis);
                }
                ct_v3 out_center = {lc.x, lc.y, lc.z};
                ct_v3 out_normal = {ln.x, ln.y, ln.z};
                ct_cell dst = ct_world_sticker(out_center, out_normal, n);
                c.facelet[dst.face * 16 + dst.row * 4 + dst.col] =
                    src.facelet[face * 16 + row * 4 + col];
            }
        }
    }
    for (int i = 0; i < 96; i++) {
        if (via_engine[i] != c.facelet[i]) {
            printf("raw %d engine %d inline %d\n", i, via_engine[i], c.facelet[i]);
        }
    }
    return 0;
}
