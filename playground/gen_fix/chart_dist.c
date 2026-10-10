#include "config/config.h"
#include "engine/cube.h"
#include "gen/classify.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    if (ct_synth_init() < 0) {
        return 1;
    }
    ct_config cfg = ct_config_defaults();
    cfg.gen.candidate_max = CT_GEN_CANDIDATE_MAX;
    ct_target_pair pairs[CT_TARGET_STICKERS * CT_TARGET_STICKERS];
    int counts[2][CT_CLS_UNKNOWN + 1] = {0};
    int unknown_lens[2][24] = {0};
    int found = 0;
    int setup = 0;
    for (int orbit = 0; orbit < 2; orbit++) {
        int total = orbit == 0 ? ct_targets_corner(0, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS)
                               : ct_targets_edge(2, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
        for (int k = 0; k < total; k++) {
            ct_synth_result res;
            memset(&res, 0, sizeof res);
            int rc = ct_synth_target(orbit, orbit == 0 ? 0 : 2, pairs[k].t1, pairs[k].t2, &cfg,
                                     &res);
            if (rc != 0 || !res.found) {
                printf("MISS orbit=%d t1=%d t2=%d\n", orbit, pairs[k].t1, pairs[k].t2);
                continue;
            }
            found++;
            setup += res.used_setup;
            ct_classify_result r;
            memset(&r, 0, sizeof r);
            ct_classify(res.seq, res.len, &r);
            counts[orbit][r.cls]++;
            if (r.cls == CT_CLS_UNKNOWN && r.moves < 24) {
                unknown_lens[orbit][r.moves]++;
            }
        }
    }
    printf("found=%d setup=%d\n", found, setup);
    for (int o = 0; o < 2; o++) {
        printf("orbit%d:", o);
        for (int c = 0; c <= CT_CLS_UNKNOWN; c++) {
            if (counts[o][c]) {
                printf(" %s:%d", ct_classification_label((ct_classification)c), counts[o][c]);
            }
        }
        printf("\n");
        printf("orbit%d unknown lens:", o);
        for (int l = 0; l < 24; l++) {
            if (unknown_lens[o][l]) {
                printf(" %d:%d", l, unknown_lens[o][l]);
            }
        }
        printf("\n");
    }
    return 0;
}
