#include <stdio.h>
#include "config/config.h"
#include "srs/fsrs.h"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    ct_fsrs_card c = {0};
    c.stability = 10.0;
    c.difficulty = 5.0;
    c.state = 1;
    c.last_review_day = 20000;
    printf("step1 ok\n");
    ct_fsrs_card bad = c;
    bad.difficulty = 1.0 / 0.0;
    ct_fsrs_card out = ct_fsrs_review(&f, &bad, CT_GOOD, 630.0, 20636);
    printf("inf difficulty result: due=%ld stability=%f difficulty=%f\n", out.due_day,
           out.stability, out.difficulty);
    return 0;
}
