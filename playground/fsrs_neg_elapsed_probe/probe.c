#include "srs/fsrs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static ct_fsrs probe_engine(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    return f;
}

int main(int argc, char **argv) {
    int mode = argc > 1 ? atoi(argv[1]) : 0;
    ct_fsrs f = probe_engine();
    ct_fsrs_card c = {0};
    c.state = 1;
    c.stability = 0.1;
    c.difficulty = 5.0;
    c.last_review_day = 100;
    c.due_day = 100;
    if (mode == 0) {
        ct_fsrs_card out = ct_fsrs_review(&f, &c, CT_GOOD, -1.0, 100);
        printf("neg elapsed: stability=%f difficulty=%f due=%ld r=%f\n", out.stability,
               out.difficulty, out.due_day, ct_fsrs_retrievability(0.1, -1.0));
    } else {
        c.stability = 3.173;
        c.last_review_day = LONG_MAX - 40000;
        c.due_day = LONG_MAX - 40000;
        ct_fsrs_card out = ct_fsrs_review(&f, &c, CT_GOOD, 4.0, LONG_MAX);
        printf("longmax today: stability=%f due=%ld\n", out.stability, out.due_day);
    }
    return 0;
}
