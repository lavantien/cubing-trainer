#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"

#include <stdio.h>
#include <string.h>

static int piece_of(int orbit, int s) {
    return orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(s) : ct_edge_piece_of(s);
}

int main(void) {
    ct_synth_init();
    unsigned masks[] = {CT_MOVE_BIT_U | CT_MOVE_BIT_R, CT_MOVE_BIT_U | CT_MOVE_BIT_F};
    const char *names[] = {"UR", "UF"};
    int depths[] = {5, 6};
    for (int mk = 0; mk < 2; mk++) {
        for (int orbit = CT_ORBIT_CORNER; orbit <= CT_ORBIT_EDGE; orbit++) {
            for (int b = 0; b < 24; b++) {
                for (int t1 = 0; t1 < 24; t1++) {
                    if (piece_of(orbit, t1) == piece_of(orbit, b))
                        continue;
                    for (int t2 = 0; t2 < 24; t2++) {
                        if (piece_of(orbit, t2) == piece_of(orbit, b))
                            continue;
                        if (piece_of(orbit, t2) == piece_of(orbit, t1))
                            continue;
                        ct_config cfg = ct_config_defaults();
                        cfg.gen.preferred_moves = masks[mk];
                        cfg.gen.setup_bfs_depth = 1;
                        cfg.gen.fallback_depth = depths[mk];
                        ct_synth_result res;
                        memset(&res, 0, sizeof res);
                        ct_synth_target(orbit, b, t1, t2, &cfg, &res);
                        if (res.used_fallback) {
                            printf("%s orbit=%d b=%d t1=%d t2=%d FALLBACK len=%d qtm=%d\n",
                                   names[mk], orbit, b, t1, t2, res.len, res.qtm);
                            return 0;
                        }
                    }
                }
            }
        }
        printf("%s: no fallback across all triples\n", names[mk]);
    }
    return 0;
}
