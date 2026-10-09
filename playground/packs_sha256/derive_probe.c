#include <stdio.h>

enum { CT_B1 = 1 };
enum { CT_B8 = CT_B1 << 3, CT_B16 = CT_B1 << 4, CT_B128 = CT_B8 * CT_B16 };

typedef unsigned _BitInt(CT_B128) ct_u128;

static unsigned next_prime(unsigned *n) {
    for (;;) {
        unsigned c = *n;
        unsigned d = 2;
        int prime = 1;
        while (d * d <= c) {
            if (c % d == 0) {
                prime = 0;
                break;
            }
            d += 1;
        }
        *n = c + 1;
        if (prime) return c;
    }
}

static unsigned derive_sqrt(unsigned p) {
    ct_u128 target = (ct_u128)p << 64;
    ct_u128 lo = 0;
    ct_u128 hi = (ct_u128)1 << 40;
    while (lo + 1 < hi) {
        ct_u128 mid = lo + (hi - lo) / 2;
        if (mid * mid <= target) lo = mid;
        else hi = mid;
    }
    return (unsigned)lo;
}

static unsigned derive_cbrt(unsigned p) {
    ct_u128 target = (ct_u128)p << 96;
    ct_u128 lo = 0;
    ct_u128 hi = (ct_u128)1 << 40;
    while (lo + 1 < hi) {
        ct_u128 mid = lo + (hi - lo) / 2;
        if (mid * mid * mid <= target) lo = mid;
        else hi = mid;
    }
    return (unsigned)lo;
}

int main(void) {
    unsigned n = 2;
    printf("H:\n");
    for (int i = 0; i < 8; i++) {
        unsigned p = next_prime(&n);
        printf("%08x %u\n", derive_sqrt(p), p);
    }
    n = 2;
    printf("K:\n");
    for (int i = 0; i < 64; i++) {
        unsigned p = next_prime(&n);
        printf("%08x %u\n", derive_cbrt(p), p);
    }
    return 0;
}
