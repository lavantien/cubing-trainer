#include <math.h>
#include <stdio.h>

#include "config/config.h"
#include "srs/fsrs.h"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);

    printf("== negative elapsed days ==\n");
    ct_fsrs_card c = {10.0, 5.0, 104, 100, 1, 0, 1};
    double r_neg = ct_fsrs_retrievability(10.0, -60.0);
    printf("R(10, -60) = %f\n", r_neg);
    ct_fsrs_card out = ct_fsrs_review(&f, &c, CT_GOOD, -60.0, 160);
    printf("stability after review at t=-60: %f difficulty: %f\n", out.stability, out.difficulty);
    if (isnan(out.stability)) {
        printf("OBSERVATION: NaN stability reaches the card struct\n");
        long ivl = ct_fsrs_next_interval_days(&f, out.stability);
        printf("interval of NaN stability: %ld\n", ivl);
    }

    printf("== zero and tiny stability ==\n");
    printf("R(0.0, 1.0) = %f\n", ct_fsrs_retrievability(0.0, 1.0));
    printf("R(0.0, 0.0) = %f\n", ct_fsrs_retrievability(0.0, 0.0));
    ct_fsrs_card z = {0.0, 5.0, 100, 99, 1, 0, 1};
    ct_fsrs_card zout = ct_fsrs_review(&f, &z, CT_GOOD, 1.0, 101);
    printf("stability from S=0 good: %f\n", zout.stability);
    ct_fsrs_card zout2 = ct_fsrs_review(&f, &z, CT_AGAIN, 1.0, 101);
    printf("stability from S=0 again: %f\n", zout2.stability);

    printf("== huge elapsed ==\n");
    ct_fsrs_card big = ct_fsrs_review(&f, &c, CT_GOOD, 1e308, 200);
    printf("stability after t=1e308: %f difficulty: %f due: %ld\n", big.stability, big.difficulty,
           big.due_day);

    printf("== grade out of enum range ==\n");
    for (int g = -1; g <= 6; g++) {
        ct_fsrs_card gout = ct_fsrs_review(&f, &c, (ct_grade)g, 4.0, 104);
        printf("learning card grade %d: stability %f difficulty %f\n", g, gout.stability, gout.difficulty);
    }
    for (int g = 0; g <= 1; g++) {
        ct_fsrs_card fresh = {0};
        ct_fsrs_card gout = ct_fsrs_review(&f, &fresh, (ct_grade)g, 0.0, 100);
        printf("fresh card grade %d: stability %f difficulty %f\n", g, gout.stability, gout.difficulty);
    }

    printf("== state out of range treated as learning ==\n");
    ct_fsrs_card s7 = {10.0, 5.0, 104, 104, 2, 0, 7};
    ct_fsrs_card s7out = ct_fsrs_review(&f, &s7, CT_GOOD, 0.0, 104);
    printf("state 7 same-day check: stability %f due %ld\n", s7out.stability, s7out.due_day);

    printf("done\n");
    return 0;
}
