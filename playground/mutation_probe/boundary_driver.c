#include "config/config.h"
#include "srs/fsrs.h"

#include <math.h>
#include <stdio.h>

int main(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    f.request_retention = 0.6;
    f.max_ivl_days = pow(0.6, 1.0 / CT_FSRS_DECAY) - 1.0;
    double raw = (CT_FSRS_FACTOR / CT_FSRS_FACTOR) * (pow(f.request_retention, 1.0 / CT_FSRS_DECAY) - 1.0);
    printf("max=%.17g raw=%.17g equal=%d ivl=%ld\n", f.max_ivl_days, raw,
           raw == f.max_ivl_days, ct_fsrs_next_interval_days(&f, CT_FSRS_FACTOR));
    return 0;
}
