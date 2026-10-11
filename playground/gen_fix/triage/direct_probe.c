#include "config/config.h"
#include "gen/synth.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    ct_config cfg = ct_config_defaults();
    cfg.gen.setup_bfs_depth = 0;
    cfg.gen.fallback_depth = 0;
    struct { int orbit; int b; int t1; int t2; } cases[] = {
        {CT_ORBIT_EDGE, 2, 0, 4},
        {CT_ORBIT_EDGE, 6, 2, 0},
        {CT_ORBIT_CORNER, 0, 1, 6},
        {CT_ORBIT_EDGE, 2, 1, 4},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        ct_synth_result r;
        memset(&r, 0, sizeof r);
        int rc = ct_synth_target(cases[i].orbit, cases[i].b, cases[i].t1, cases[i].t2, &cfg, &r);
        printf("case %zu orbit=%d b=%d t1=%d t2=%d rc=%d found=%d len=%d setup=%d\n", i,
               cases[i].orbit, cases[i].b, cases[i].t1, cases[i].t2, rc, r.found, r.len,
               r.used_setup);
    }
    return 0;
}
