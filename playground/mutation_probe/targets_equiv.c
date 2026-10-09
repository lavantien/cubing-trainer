#include "gen/targets.h"

#include <stdio.h>

static unsigned long long h = 1469598103934665603ULL;

static void mix(long v) {
    h ^= (unsigned long long)v;
    h *= 1099511628211ULL;
}

int main(void) {
    static ct_target_pair big[512];
    for (int s = -3; s <= 27; s++) {
        mix(ct_corner_piece_of(s));
        mix(ct_edge_piece_of(s));
    }
    for (int b = -2; b <= 25; b++) {
        for (int capi = 0; capi < 4; capi++) {
            int cap = capi == 0 ? 0 : (capi == 1 ? -7 : 500);
            ct_target_pair *out = capi == 3 ? NULL : big;
            if (capi != 3) {
                for (int i = 0; i < 512; i++) {
                    big[i].t1 = -99;
                    big[i].t2 = -99;
                }
            }
            mix(ct_targets_corner(b, out, cap));
            if (capi != 3) {
                for (int i = 0; i < 512; i++) {
                    mix(big[i].t1);
                    mix(big[i].t2);
                }
            }
            if (capi != 3) {
                for (int i = 0; i < 512; i++) {
                    big[i].t1 = -99;
                    big[i].t2 = -99;
                }
            }
            mix(ct_targets_edge(b, out, cap));
            if (capi != 3) {
                for (int i = 0; i < 512; i++) {
                    mix(big[i].t1);
                    mix(big[i].t2);
                }
            }
        }
    }
    for (int o = -2; o <= 3; o++) {
        for (int s = -2; s <= 25; s++) {
            int f = -7;
            int r = -7;
            int c = -7;
            ct_sticker_cell(o, s, &f, &r, &c);
            mix(f);
            mix(r);
            mix(c);
        }
    }
    printf("%016llx\n", h);
    return 0;
}
