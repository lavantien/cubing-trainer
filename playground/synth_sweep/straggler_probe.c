#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    ct_synth_init();
    for (int depth = 5; depth <= 8; depth++) {
        ct_config cfg = ct_config_defaults();
        cfg.gen.fallback_depth = depth;
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(CT_ORBIT_EDGE, 2, 5, 7, &cfg, &res);
        printf("depth=%d rc=%d found=%d setup=%d fallback=%d len=%d qtm=%d\n", depth, rc, res.found,
               res.used_setup, res.used_fallback, res.len, res.qtm);
        if (res.found) {
            break;
        }
    }
    return 0;
}
