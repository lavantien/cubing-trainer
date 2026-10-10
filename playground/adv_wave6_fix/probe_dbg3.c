#include "engine/notation.h"
#include "config/config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    ct_move seq[64];
    int len = ct_notation_parse("U", 3, seq, 64);
    printf("len %d face %d from %d to %d quarter %d\n", len, seq[0].face, seq[0].depth_from,
           seq[0].depth_to, seq[0].quarter);
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = 3;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                c.facelet[face * 16 + row * 4 + col] = (uint8_t)(100 + face * 16 + row * 4 + col);
            }
        }
    }
    ct_apply(&c, seq[0]);
    int seen[256] = {0};
    for (int i = 0; i < 96; i++) {
        seen[c.facelet[i]]++;
    }
    for (int v = 0; v < 256; v++) {
        if (seen[v] > 1) {
            printf("value %d appears %d times\n", v, seen[v]);
        }
    }
    for (int face = 0; face < 6; face++) {
        printf("face %d:", face);
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                printf(" %3d", c.facelet[face * 16 + row * 4 + col] - 100);
            }
        }
        printf("\n");
    }
    return 0;
}
