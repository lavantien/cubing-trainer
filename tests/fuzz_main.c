#include "config/config.h"

#include <stdlib.h>

enum { CT_FUZZ_MULT_CAP = 1000000, CT_FUZZ_ITER_CAP = 2000000000 };

int ct_fuzz_iterations(void) {
    ct_config cfg = ct_config_defaults();
    long long iters = cfg.fuzz.iterations;
    const char *env = getenv("CT_FUZZ_LOOPS");
    if (env != NULL && env[0] != '\0') {
        char *end = NULL;
        long long mult = strtoll(env, &end, CT_DECIMAL_BASE);
        if (end != NULL && *end == '\0' && mult >= 1 && mult <= CT_FUZZ_MULT_CAP)
            iters *= mult;
    }
    if (iters > CT_FUZZ_ITER_CAP)
        iters = CT_FUZZ_ITER_CAP;
    return (int)iters;
}
