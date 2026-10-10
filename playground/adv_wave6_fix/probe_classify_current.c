#include "gen/classify.h"

#include <stdio.h>
#include <string.h>

static const char *const algs[] = {
    "F U2 R' D' R U2 R' D R F'",
    "U' R E' R' U2 R E R' U'",
    "L F R' F' L' F R F'",
    "U' B D' B' U B D B'",
    "D F2 D' B2 D F2 D' B2",
    "L F' L B2 L' F L B2 L2",
    "U R' B2 R F R' B2 R F' U'",
    "F R' U2 R F' R' F U2 F' R",
    "R U2 R' U' L2 U R U' L2 U' R'",
    "U F2 U' F2 U' R2 U F2 U F2 U' R2",
    "U F E F' U' F E' F'",
    "M F R' F' M' F R F'",
    "R' F R' S2 R F' R' S2 R2",
    "F' U R' E R U' R' E' R F",
    "R2 U S' U2 S U R2",
    "U' R2 Uw R2 Uw' F2 Uw R2 Uw' F2 R2 U",
};

int main(void) {
    for (size_t i = 0; i < sizeof algs / sizeof algs[0]; i++) {
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        if (ct_classify_alg(algs[i], &r) != CT_CLASSIFY_OK) {
            printf("%zu parse fail\n", i);
            continue;
        }
        printf("%zu %-40s cls=%-16s moves=%d\n", i, algs[i],
               ct_classification_label(r.cls), r.moves);
    }
    return 0;
}
