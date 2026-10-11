#include "gen/classify.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *algs[] = {"L F R' F' L' F R F'", "U' B D' B' U B D B'",
                          "D F2 D' B2 D F2 D' B2"};
    for (int i = 0; i < 3; i++) {
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        ct_classify_alg(algs[i], &r);
        printf("%-24s cls=%d label=%s\n", algs[i], (int)r.cls, r.label);
    }
    return 0;
}
