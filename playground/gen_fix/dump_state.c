#include "engine/cube.h"
#include "engine/notation.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        ct_move seq[32];
        int n = ct_notation_parse(argv[i], 3, seq, 32);
        ct_cube c;
        memset(&c, 0, sizeof c);
        c.n = 3;
        for (int f = 0; f < 6; f++) {
            for (int r = 0; r < 3; r++) {
                for (int col = 0; col < 3; col++) {
                    c.facelet[f * 49 + r * 7 + col] = (uint8_t)(f * 9 + r * 3 + col);
                }
            }
        }
        for (int j = 0; j < n; j++) {
            ct_apply(&c, seq[j]);
        }
        printf("alg=%s\n", argv[i]);
        for (int f = 0; f < 6; f++) {
            for (int r = 0; r < 3; r++) {
                for (int col = 0; col < 3; col++) {
                    int q = f * 9 + r * 3 + col;
                    uint8_t v = c.facelet[f * 49 + r * 7 + col];
                    if (v != (uint8_t)q) {
                        printf("  moved cell %d (face %d row %d col %d) -> %d\n", q, f, r, col, v);
                    }
                }
            }
        }
    }
    return 0;
}
