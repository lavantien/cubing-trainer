#include <stdio.h>

#include "gen/targets.h"

enum { PROBE_PAIR_CAP = CT_TARGET_STICKERS * CT_TARGET_STICKERS };

int main(void) {
    static ct_target_pair pairs[PROBE_PAIR_CAP];
    int nc = ct_targets_corner(0, pairs, PROBE_PAIR_CAP);
    printf("corner count %d first t1 %d t2 %d key %c%c\n", nc, pairs[0].t1, pairs[0].t2,
           'A' + pairs[0].t1, 'A' + pairs[0].t2);
    int ne = ct_targets_edge(2, pairs, PROBE_PAIR_CAP);
    printf("edge count %d first t1 %d t2 %d key %c%c\n", ne, pairs[0].t1, pairs[0].t2,
           'A' + pairs[0].t1, 'A' + pairs[0].t2);
    printf("corner last key %c%c\n", 'A' + pairs[nc - 1].t1, 'A' + pairs[nc - 1].t2);
    printf("edge last key %c%c\n", 'A' + pairs[ne - 1].t1, 'A' + pairs[ne - 1].t2);
    return 0;
}
