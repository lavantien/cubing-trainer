#include "srs/fsrs.h"

#include <math.h>
#include <stdio.h>

int main(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    double r = ct_fsrs_retrievability(10.0, 4.0);
    printf("R=%.17g\n", r);
    printf("w4=%.17g w5=%.17g w6=%.17g w7=%.17g\n", f.w[4], f.w[5], f.w[6], f.w[7]);
    printf("w8=%.17g w9=%.17g w10=%.17g w11=%.17g\n", f.w[8], f.w[9], f.w[10], f.w[11]);
    printf("w12=%.17g w13=%.17g w14=%.17g w15=%.17g\n", f.w[12], f.w[13], f.w[14], f.w[15]);
    printf("w16=%.17g w17=%.17g w18=%.17g\n", f.w[16], f.w[17], f.w[18]);
    ct_fsrs_card c = {0};
    c.stability = 10.0;
    c.difficulty = 5.0;
    c.due_day = 104;
    c.last_review_day = 100;
    c.reps = 1;
    c.state = 1;
    for (int g = 1; g <= 4; g++) {
        ct_fsrs_card out = ct_fsrs_review(&f, &c, (ct_grade)g, 4.0, 104);
        printf("g=%d S=%.17g D=%.17g\n", g, out.stability, out.difficulty);
    }
    ct_fsrs_card sd = c;
    sd.last_review_day = 104;
    for (int g = 1; g <= 4; g++) {
        ct_fsrs_card out = ct_fsrs_review(&f, &sd, (ct_grade)g, 0.0, 104);
        printf("sd g=%d S=%.17g\n", g, out.stability);
    }
    return 0;
}
