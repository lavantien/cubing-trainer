#include "gen/classify.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *composite = "L F R' F' L' F R F' R2 U' R' U' R U R U R U' R";
    ct_classify_result r;
    memset(&r, 0, sizeof r);
    int rc = ct_classify_alg(composite, &r);
    printf("rc=%d cls=%d %s | %s\n", rc, (int)r.cls, r.label, r.rationale);
    return 0;
}
