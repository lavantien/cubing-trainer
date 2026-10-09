#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    ct_synth_init();
    unsigned masks[] = {CT_MOVE_BIT_U | CT_MOVE_BIT_R | CT_MOVE_BIT_F,
                        CT_MOVE_BIT_U | CT_MOVE_BIT_D | CT_MOVE_BIT_L | CT_MOVE_BIT_R |
                            CT_MOVE_BIT_F | CT_MOVE_BIT_B};
    const char *names[] = {"URF", "UDLRFB"};
    ct_target_pair pairs[440];
    int counts[] = {378, 440};
    for (int mk = 0; mk < 2; mk++) {
        for (int orbit = CT_ORBIT_CORNER; orbit <= CT_ORBIT_EDGE; orbit++) {
            int n = orbit == CT_ORBIT_CORNER ? ct_targets_corner(0, pairs, 440)
                                             : ct_targets_edge(2, pairs, 440);
            for (int i = 0; i < n; i++) {
                ct_config cfg = ct_config_defaults();
                cfg.gen.preferred_moves = masks[mk];
                cfg.gen.setup_bfs_depth = 0;
                cfg.gen.fallback_depth = 5;
                ct_synth_result res;
                memset(&res, 0, sizeof res);
                ct_synth_target(orbit, orbit == CT_ORBIT_CORNER ? 0 : 2, pairs[i].t1, pairs[i].t2,
                                &cfg, &res);
                if (res.used_fallback) {
                    printf("%s orbit=%d i=%d t1=%d t2=%d FALLBACK len=%d qtm=%d\n", names[mk],
                           orbit, i, pairs[i].t1, pairs[i].t2, res.len, res.qtm);
                    return 0;
                }
            }
        }
        printf("%s: no fallback with setup skipped\n", names[mk]);
    }
    (void)counts;
    return 0;
}
