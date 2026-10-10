#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/score.h"
#include "gen/synth.h"

#include <stdio.h>
#include <string.h>

static void run(const char *tag, int orbit, int buffer, int t1, int t2, ct_config *cfg) {
    ct_synth_result res;
    memset(&res, 0, sizeof res);
    int rc = ct_synth_target(orbit, buffer, t1, t2, cfg, &res);
    ct_score_parts p;
    double total = -1;
    if (res.found && ct_score_alg(res.seq, res.len, cfg, &p) == CT_SCORE_OK) {
        total = p.total;
    }
    char text[256] = {0};
    if (res.found) {
        ct_notation_format(res.seq, res.len, 3, text, sizeof text);
    }
    printf("%s rc=%d found=%d setup=%d len=%d qtm=%d total=%.2f %s\n", tag, rc, res.found,
           res.used_setup, res.len, res.qtm, total, text);
}

int main(void) {
    if (ct_synth_init() < 0) {
        return 1;
    }
    ct_config full = ct_config_defaults();
    full.gen.candidate_max = CT_GEN_CANDIDATE_MAX;
    ct_config direct_only = ct_config_defaults();
    direct_only.gen.setup_bfs_depth = 0;
    direct_only.gen.fallback_depth = 0;
    ct_config def = ct_config_defaults();
    run("corner 1,6 full", CT_ORBIT_CORNER, 0, 1, 6, &full);
    run("corner 1,6 direct", CT_ORBIT_CORNER, 0, 1, 6, &direct_only);
    run("corner 1,6 default", CT_ORBIT_CORNER, 0, 1, 6, &def);
    run("corner 1,2 full", CT_ORBIT_CORNER, 0, 1, 2, &full);
    run("edge 1,4 full", CT_ORBIT_EDGE, 2, 1, 4, &full);
    run("edge 1,4 direct", CT_ORBIT_EDGE, 2, 1, 4, &direct_only);
    run("edge 0,3 full", CT_ORBIT_EDGE, 2, 0, 3, &full);
    return 0;
}
