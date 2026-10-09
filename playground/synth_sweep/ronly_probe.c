#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    ct_synth_init();
    ct_config cfg = ct_config_defaults();
    cfg.gen.preferred_moves = CT_MOVE_BIT_R;
    ct_synth_result res;
    memset(&res, 0, sizeof res);
    int rc = ct_synth_target(CT_ORBIT_CORNER, 0, 1, 2, &cfg, &res);
    printf("rc=%d found=%d setup=%d fallback=%d len=%d\n", rc, res.found, res.used_setup,
           res.used_fallback, res.len);
    return 0;
}
