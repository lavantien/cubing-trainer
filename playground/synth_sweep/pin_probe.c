#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    ct_synth_init();
    ct_config cfg = ct_config_defaults();
    ct_target_pair pairs[440];
    int corner_n = ct_targets_corner(0, pairs, 440);
    for (int i = 0; i < 40; i++) {
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        ct_synth_target(CT_ORBIT_CORNER, 0, pairs[i].t1, pairs[i].t2, &cfg, &res);
        char alg[128] = "";
        ct_notation_format(res.seq, res.len, 3, alg, sizeof alg);
        printf("corner i=%d t1=%d t2=%d found=%d setup=%d fallback=%d qtm=%d alg=%s\n", i,
               pairs[i].t1, pairs[i].t2, res.found, res.used_setup, res.used_fallback, res.qtm,
               alg);
    }
    int edge_n = ct_targets_edge(2, pairs, 440);
    for (int i = 0; i < 40; i++) {
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        ct_synth_target(CT_ORBIT_EDGE, 2, pairs[i].t1, pairs[i].t2, &cfg, &res);
        char alg[128] = "";
        ct_notation_format(res.seq, res.len, 3, alg, sizeof alg);
        printf("edge i=%d t1=%d t2=%d found=%d setup=%d fallback=%d qtm=%d alg=%s\n", i,
               pairs[i].t1, pairs[i].t2, res.found, res.used_setup, res.used_fallback, res.qtm,
               alg);
    }
    (void)corner_n;
    (void)edge_n;
    return 0;
}
