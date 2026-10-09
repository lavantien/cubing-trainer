#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    ct_synth_init();
    ct_target_pair pairs[440];
    int n = ct_targets_corner(0, pairs, 440);
    int misses = 0;
    for (int i = 0; i < n; i++) {
        ct_config cfg = ct_config_defaults();
        cfg.gen.preferred_moves = CT_MOVE_BIT_R;
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(CT_ORBIT_CORNER, 0, pairs[i].t1, pairs[i].t2, &cfg, &res);
        if (rc != 0 || !res.found) {
            printf("i=%d t1=%d t2=%d rc=%d found=%d\n", i, pairs[i].t1, pairs[i].t2, rc, res.found);
            misses++;
            if (misses >= 5)
                break;
        }
    }
    printf("misses=%d\n", misses);
    return 0;
}
