#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "engine/transform.h"
#include "gen/interchange.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

static int raw_of(int face, int row, int col) {
    return face * 49 + row * 7 + col;
}

static int compact_of(int face, int row, int col) {
    return face * 9 + row * 3 + col;
}

static int cell_of(int orbit, int sticker) {
    int face = -1, row = -1, col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return compact_of(face, row, col);
}

static void simulate(const ct_move *seq, int len, uint8_t state[54]) {
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = 3;
    for (int f = 0; f < 6; f++)
        for (int r = 0; r < 3; r++)
            for (int cc = 0; cc < 3; cc++)
                c.facelet[raw_of(f, r, cc)] = (uint8_t)compact_of(f, r, cc);
    for (int i = 0; i < len; i++)
        ct_apply(&c, seq[i]);
    for (int f = 0; f < 6; f++)
        for (int r = 0; r < 3; r++)
            for (int cc = 0; cc < 3; cc++)
                state[compact_of(f, r, cc)] = c.facelet[raw_of(f, r, cc)];
}

int main(void) {
    int want[3] = {cell_of(1, 2), cell_of(1, 5), cell_of(1, 7)};
    printf("cells want: %d %d %d\n", want[0], want[1], want[2]);
    ct_move cands[64];
    int n = ct_candidate_moves(3, cands, 64);
    int singles[32];
    int sn = 0;
    for (int i = 0; i < n; i++)
        if (cands[i].depth_from == 1 && cands[i].depth_to == 1)
            singles[sn++] = i;
    printf("cands=%d singles=%d\n", n, sn);
    int found = 0;
    for (int ai = 0; ai < n && found < 12; ai++) {
        for (int xi = 0; xi < sn && found < 12; xi++) {
            for (int yi = 0; yi < sn && found < 12; yi++) {
                if (cands[singles[yi]].face == cands[singles[xi]].face)
                    continue;
                ct_move shapes[4][6];
                int shape_lens[4] = {3, 5, 5, 5};
                ct_move x = cands[singles[xi]];
                ct_move y = cands[singles[yi]];
                ct_move xi_ = ct_move_inverse(x);
                ct_move yi_ = ct_move_inverse(y);
                ct_move *s0 = shapes[0];
                s0[0] = x;
                s0[1] = y;
                s0[2] = xi_;
                ct_move *s1 = shapes[1];
                s1[0] = x;
                s1[1] = y;
                s1[2] = xi_;
                s1[3] = y;
                s1[4] = yi_;
                ct_move *s2 = shapes[2];
                s2[0] = x;
                s2[1] = y;
                s2[2] = yi_;
                s2[3] = y;
                s2[4] = xi_;
                ct_move *s3 = shapes[3];
                s3[0] = x;
                s3[1] = y;
                s3[2] = x;
                s3[3] = y;
                s3[4] = x;
                for (int shape = 0; shape < 4 && found < 12; shape++) {
                    for (int order = 0; order < 2 && found < 12; order++) {
                        ct_move seq[16];
                        int len = 0;
                        ct_move a = cands[ai];
                        ct_move ia = ct_move_inverse(a);
                        ct_move iinv[6];
                        ct_transform_inverse(shapes[shape], shape_lens[shape], iinv);
                        if (order == 0) {
                            seq[len++] = a;
                            memcpy(seq + len, shapes[shape], shape_lens[shape] * sizeof(ct_move));
                            len += shape_lens[shape];
                            seq[len++] = ia;
                            memcpy(seq + len, iinv, shape_lens[shape] * sizeof(ct_move));
                            len += shape_lens[shape];
                        } else {
                            memcpy(seq + len, shapes[shape], shape_lens[shape] * sizeof(ct_move));
                            len += shape_lens[shape];
                            seq[len++] = a;
                            memcpy(seq + len, iinv, shape_lens[shape] * sizeof(ct_move));
                            len += shape_lens[shape];
                            seq[len++] = ia;
                        }
                        uint8_t state[54];
                        simulate(seq, len, state);
                        int perm[54];
                        for (int q = 0; q < 54; q++)
                            perm[state[q]] = q;
                        int hits = 0;
                        for (int k = 0; k < 3; k++)
                            if (perm[want[k]] != want[k])
                                hits++;
                        if (hits == 3) {
                            int follow = perm[want[0]];
                            int steps = 0;
                            while (follow != want[0] && steps < 9) {
                                follow = perm[follow];
                                steps++;
                            }
                            if (steps == 2) {
                                printf("HIT shape=%d order=%d len=%d:", shape, order, len);
                                for (int i = 0; i < len; i++) {
                                    char buf[8];
                                    ct_notation_format_one(&seq[i], 3, buf, sizeof buf);
                                    printf(" %s", buf);
                                }
                                printf("\n");
                                found++;
                            }
                        }
                    }
                }
            }
        }
    }
    printf("found=%d\n", found);
    return 0;
}
