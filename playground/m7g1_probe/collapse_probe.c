#include <stdio.h>

#include "engine/cube.h"
#include "engine/transform.h"

static void dump(const char *label, const ct_move *m, int len) {
    printf("%s len=%d:", label, len);
    for (int i = 0; i < len; i++)
        printf(" (%d,%d,%d,%d)", m[i].face, m[i].depth_from, m[i].depth_to, m[i].quarter);
    printf("\n");
}

int main(void) {
    printf("assembly max=%d\n", CT_TRANSFORM_ASSEMBLY_MAX);
    int half = CT_TRANSFORM_ASSEMBLY_MAX / 2;
    ct_move collapse[CT_TRANSFORM_ASSEMBLY_MAX];
    for (int i = 0; i < half; i++) {
        collapse[i].face = i % CT_CUBE_FACE_COUNT;
        collapse[i].depth_from = 1;
        collapse[i].depth_to = 1;
        collapse[i].quarter = 1;
        collapse[half + i] = collapse[half - 1 - i];
        collapse[half + i].quarter = CT_MOVE_QUARTERS - collapse[half + i].quarter;
    }
    dump("collapse", collapse, CT_TRANSFORM_ASSEMBLY_MAX);
    ct_move out[CT_TRANSFORM_ASSEMBLY_MAX];
    int m = ct_transform_simplify(collapse, CT_TRANSFORM_ASSEMBLY_MAX, out, 0);
    printf("cap0 rc=%d\n", m);
    if (m > 0)
        dump("out", out, m);
    ct_move buried[5] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 1},
                         {CT_FACE_U, 1, 1, 1},  {CT_FACE_U, 1, 1, 3},
                         {CT_FACE_R, 1, 1, 3}};
    int mb = ct_transform_simplify(buried, 5, out, 8);
    printf("buried rc=%d\n", mb);
    if (mb > 0)
        dump("buried out", out, mb);
    return 0;
}
