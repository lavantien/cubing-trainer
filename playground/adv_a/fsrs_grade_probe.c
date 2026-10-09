#include <stdio.h>

#include "config/config.h"
#include "srs/fsrs.h"

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int g = argc > 1 ? (int)argv[1][0] - '0' : 0;
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    ct_fsrs_card fresh = {0};
    ct_fsrs_card out = ct_fsrs_review(&f, &fresh, (ct_grade)g, 0.0, 100);
    printf("fresh card grade %d: stability %f difficulty %f\n", g, out.stability, out.difficulty);
    return 0;
}
