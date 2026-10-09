#include <stdint.h>
#include <stdio.h>

#include "../../tests/framework.h"

static uint64_t ref_splitmix64(uint64_t *x) {
    uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t ref_next(uint64_t *s) {
    const uint64_t tmp = s[0] + s[1] + s[3]++;
    s[0] = s[1] ^ (s[1] >> 11);
    s[1] = s[2] + (s[2] << 3);
    s[2] = ((s[2] << 24) | (s[2] >> 40)) + tmp;
    return tmp;
}

static void ref_init(uint64_t *s, uint64_t seed) {
    uint64_t sm = seed;
    s[0] = ref_splitmix64(&sm);
    s[1] = ref_splitmix64(&sm);
    s[2] = ref_splitmix64(&sm);
    s[3] = ref_splitmix64(&sm);
    for (int i = 0; i < 12; i++) {
        (void)ref_next(s);
    }
}

int main(void) {
    uint64_t bad = 0;
    uint64_t draws = 0;
    for (uint64_t k = 0; k < 8; k++) {
        uint64_t seed = k * 0x9E3779B97F4A7C15ULL + 1;
        uint64_t ref[4];
        ct_sfc64 mine;
        ref_init(ref, seed);
        ct_sfc64_init(&mine, seed);
        for (int i = 0; i < 1000; i++) {
            uint64_t a = ref_next(ref);
            uint64_t b = ct_sfc64_next(&mine);
            draws++;
            if (a != b) {
                bad++;
            }
        }
    }
    if (bad != 0) {
        printf("crosscheck FAIL: %llu of %llu outputs diverge\n",
               (unsigned long long)bad, (unsigned long long)draws);
        return 1;
    }
    printf("crosscheck ok: %llu outputs identical across 8 seeds\n", (unsigned long long)draws);
    uint64_t gold[4];
    ref_init(gold, 123456789);
    printf("seed 123456789 first outputs:");
    for (int i = 0; i < 4; i++) {
        printf(" %016llx", (unsigned long long)ref_next(gold));
    }
    printf("\n");
    return 0;
}
