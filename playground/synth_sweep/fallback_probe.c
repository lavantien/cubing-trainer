#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    ct_synth_init();
    unsigned masks[] = {
        CT_MOVE_BIT_U | CT_MOVE_BIT_R,
        CT_MOVE_BIT_U | CT_MOVE_BIT_D | CT_MOVE_BIT_L | CT_MOVE_BIT_R | CT_MOVE_BIT_F |
            CT_MOVE_BIT_B,
    };
    const char *names[] = {"UR", "UDLRFB"};
    ct_target_pair pairs[440];
    int corner_n = ct_targets_corner(0, pairs, 440);
    int edge_n = ct_targets_edge(2, pairs, 440);
    for (int mk = 0; mk < 2; mk++) {
        for (int i = 0; i < corner_n; i++) {
            ct_config cfg = ct_config_defaults();
            cfg.gen.preferred_moves = masks[mk];
            cfg.gen.setup_bfs_depth = 1;
            cfg.gen.fallback_depth = 5;
            ct_synth_result res;
            memset(&res, 0, sizeof res);
            ct_synth_target(CT_ORBIT_CORNER, 0, pairs[i].t1, pairs[i].t2, &cfg, &res);
            if (res.used_fallback) {
                printf("%s corner i=%d t1=%d t2=%d FALLBACK len=%d qtm=%d\n", names[mk], i,
                       pairs[i].t1, pairs[i].t2, res.len, res.qtm);
                mk = 99;
                break;
            }
        }
        if (mk == 99)
            break;
        printf("%s corners: no fallback through %d\n", names[mk], corner_n);
        for (int i = 0; i < edge_n; i++) {
            ct_config cfg = ct_config_defaults();
            cfg.gen.preferred_moves = masks[mk];
            cfg.gen.setup_bfs_depth = 1;
            cfg.gen.fallback_depth = 5;
            ct_synth_result res;
            memset(&res, 0, sizeof res);
            ct_synth_target(CT_ORBIT_EDGE, 2, pairs[i].t1, pairs[i].t2, &cfg, &res);
            if (res.used_fallback) {
                printf("%s edge i=%d t1=%d t2=%d FALLBACK len=%d qtm=%d\n", names[mk], i,
                       pairs[i].t1, pairs[i].t2, res.len, res.qtm);
                mk = 99;
                break;
            }
        }
        if (mk == 99)
            break;
        printf("%s edges: no fallback through %d\n", names[mk], edge_n);
    }
    return 0;
}
