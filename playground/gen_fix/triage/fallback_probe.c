#include "config/config.h"
#include "gen/synth.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    ct_config cfg = ct_config_defaults();
    cfg.gen.setup_bfs_depth = 0;
    ct_synth_result r;
    memset(&r, 0, sizeof r);
    int rc = ct_synth_target(CT_ORBIT_EDGE, 2, 0, 4, &cfg, &r);
    printf("rc=%d found=%d len=%d qtm=%d fallback=%d\n", rc, r.found, r.len, r.qtm,
           r.used_fallback);
    return 0;
}
