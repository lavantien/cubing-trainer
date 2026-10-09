#include "framework.h"

#include <stdlib.h>

CT_SUITE(unit_smoke) {
    CT_ASSERT(true);
    CT_ASSERT(!false);
    CT_ASSERT_EQ(2 + 2, 4);
    CT_ASSERT_EQ((1ull << 63) + 1, 9223372036854775809ull);
    CT_ASSERT_DBL_EQ(1.0, 1.5, 0.5);
    CT_ASSERT_DBL_EQ(0.25, -0.25, 0.5);
    CT_ASSERT(!ct_dbl_within(1.0, 2.0, 0.5));
    CT_ASSERT_STR_EQ("cubing", "cubing");
    CT_ASSERT(ct_str_eq("r u r'", "r u r'"));
    CT_ASSERT(!ct_str_eq("alpha", "beta"));

    ct_sfc64 same_a;
    ct_sfc64 same_b;
    ct_sfc64 other;
    ct_sfc64_init(&same_a, 42);
    ct_sfc64_init(&same_b, 42);
    ct_sfc64_init(&other, 43);
    for (int i = 0; i < 16; i++) {
        CT_ASSERT_EQ(ct_sfc64_next(&same_a), ct_sfc64_next(&same_b));
    }
    uint64_t diff = 0;
    for (int i = 0; i < 16; i++) {
        diff |= ct_sfc64_next(&same_a) ^ ct_sfc64_next(&other);
    }
    CT_ASSERT(diff != 0);

    static const uint64_t gold_first[4] = {
        0x406a26168ad7417eull,
        0xcf8acc48879262e9ull,
        0x7aeb56e3063c19a4ull,
        0xf1696c4fc37efd82ull,
    };
    ct_sfc64 gold;
    ct_sfc64_init(&gold, 123456789);
    for (int i = 0; i < 4; i++) {
        CT_ASSERT_EQ(ct_sfc64_next(&gold), gold_first[i]);
    }

    ct_sfc64 uni;
    ct_sfc64_init(&uni, 7);
    double lo = 1.0;
    double hi = 0.0;
    bool out_of_range = false;
    for (int i = 0; i < 10000; i++) {
        double v = ct_sfc64_uniform(&uni);
        if (v < 0.0 || v >= 1.0) {
            out_of_range = true;
        }
        if (v < lo) {
            lo = v;
        }
        if (v > hi) {
            hi = v;
        }
    }
    CT_ASSERT(!out_of_range);
    CT_ASSERT(lo >= 0.0);
    CT_ASSERT(hi < 1.0);
    CT_ASSERT(hi > 0.5);
    CT_ASSERT(lo < 0.5);

    ct_sfc64 one;
    ct_sfc64_init(&one, 9);
    bool hit_nonzero = false;
    for (int i = 0; i < 10000; i++) {
        if (ct_sfc64_range(&one, 1) != 0) {
            hit_nonzero = true;
        }
    }
    CT_ASSERT(!hit_nonzero);

    ct_sfc64 two;
    ct_sfc64_init(&two, 11);
    unsigned seen = 0;
    for (int i = 0; i < 10000; i++) {
        seen |= 1u << (unsigned)ct_sfc64_range(&two, 2);
    }
    CT_ASSERT_EQ(seen, 3u);
}

static void expected_fail_body(void) {
    CT_ASSERT(0 == 1);
}

__attribute__((constructor)) static void expected_fail_register(void) {
    const char *gate = getenv("CT_EXPECTED_FAIL");
    if (gate != NULL && *gate != '\0') {
        ct_suite_register("expected_fail", expected_fail_body);
    }
}
