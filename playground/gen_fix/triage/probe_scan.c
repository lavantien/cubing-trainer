#include "gen/classify.h"
#include <stdio.h>
#include <string.h>

extern int dbg_corner_swaps;
extern int dbg_edge_swaps;
extern int dbg_corner_ret;
extern int dbg_edge_ret;

static void run(const char *alg) {
    ct_classify_result r;
    memset(&r, 0, sizeof r);
    int rc = ct_classify_alg(alg, &r);
    printf("%-58s rc=%d cls=%2d corner(%d,%d) edge(%d,%d)\n", alg, rc, (int)r.cls,
           dbg_corner_ret, dbg_corner_swaps, dbg_edge_ret, dbg_edge_swaps);
}

int main(void) {
    run("L F R' F' L' F R F'");
    run("R2 U' R' U' R U R U R U' R");
    run("L F R' F' L' F R F' R2 U' R' U' R U R U R U' R");
    run("R U R' U' R' F R2 U' R' U' R U R' F'");
    return 0;
}
