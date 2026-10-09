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
    for (int depth = 5; depth <= 8; depth++) {
        for (int i = 0; i < n; i++) {
            ct_config cfg = ct_config_defaults();
            cfg.gen.preferred_moves = CT_MOVE_BIT_U | CT_MOVE_BIT_R | CT_MOVE_BIT_F;
            cfg.gen.setup_bfs_depth = 0;
            cfg.gen.fallback_depth = depth;
            ct_synth_result res;
            memset(&res, 0, sizeof res);
            ct_synth_target(CT_ORBIT_CORNER, 0, pairs[i].t1, pairs[i].t2, &cfg, &res);
            if (res.used_fallback) {
                printf("depth=%d i=%d t1=%d t2=%d FALLBACK len=%d qtm=%d\n", depth, i, pairs[i].t1,
                       pairs[i].t2, res.len, res.qtm);
                return 0;
            }
        }
        printf("depth=%d: none\n", depth);
    }
    return 0;
}
