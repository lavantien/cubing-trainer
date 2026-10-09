#include "packs/provenance.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef unsigned _BitInt(CT_B128) ct_u128;

static const char CT_SHA_HEX[] = "0123456789abcdef";

static uint32_t ct_sha_h0[CT_SHA_WORD_COUNT];
static uint32_t ct_sha_k[CT_SHA_ROUND_COUNT];
static int ct_sha_ready;

static unsigned ct_sha_next_prime(unsigned *n) {
    for (;;) {
        unsigned c = *n;
        unsigned d = CT_B2;
        int prime = 1;
        while (d * d <= c) {
            if (c % d == 0) {
                prime = 0;
                break;
            }
            d += 1;
        }
        *n = c + 1;
        if (prime)
            return c;
    }
}

static uint32_t ct_sha_derive_root(unsigned p, int cube) {
    unsigned shift = cube ? CT_SHA_DERIVE_SHIFT_CBRT : CT_SHA_DERIVE_SHIFT_SQRT;
    ct_u128 target = (ct_u128)p << shift;
    ct_u128 lo = 0;
    ct_u128 hi = (ct_u128)CT_B1 << CT_SHA_DERIVE_BOUND_SHIFT;
    while (lo + 1 < hi) {
        ct_u128 mid = lo + (hi - lo) / CT_B2;
        ct_u128 v = cube ? mid * mid * mid : mid * mid;
        if (v <= target) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return (uint32_t)lo;
}

static void ct_sha_init(void) {
    if (ct_sha_ready)
        return;
    unsigned n = CT_B2;
    for (int i = 0; i < CT_SHA_WORD_COUNT; i++) {
        ct_sha_h0[i] = ct_sha_derive_root(ct_sha_next_prime(&n), 0);
    }
    n = CT_B2;
    for (int i = 0; i < CT_SHA_ROUND_COUNT; i++) {
        ct_sha_k[i] = ct_sha_derive_root(ct_sha_next_prime(&n), 1);
    }
    ct_sha_ready = 1;
}

static uint32_t ct_sha_rotr(uint32_t x, unsigned n) {
    return (x >> n) | (uint32_t)(x << (CT_SHA_WORD_BITS - n));
}

static uint32_t ct_sha_lower_sigma0(uint32_t x) {
    return ct_sha_rotr(x, CT_SHA_ROTR_7) ^ ct_sha_rotr(x, CT_SHA_ROTR_18) ^ (x >> CT_SHA_ROTR_3);
}

static uint32_t ct_sha_lower_sigma1(uint32_t x) {
    return ct_sha_rotr(x, CT_SHA_ROTR_17) ^ ct_sha_rotr(x, CT_SHA_ROTR_19) ^ (x >> CT_SHA_ROTR_10);
}

static uint32_t ct_sha_upper_sigma0(uint32_t x) {
    return ct_sha_rotr(x, CT_SHA_ROTR_2) ^ ct_sha_rotr(x, CT_SHA_ROTR_13) ^
           ct_sha_rotr(x, CT_SHA_ROTR_22);
}

static uint32_t ct_sha_upper_sigma1(uint32_t x) {
    return ct_sha_rotr(x, CT_SHA_ROTR_6) ^ ct_sha_rotr(x, CT_SHA_ROTR_11) ^
           ct_sha_rotr(x, CT_SHA_ROTR_25);
}

static uint32_t ct_sha_load_be32(const unsigned char *p) {
    uint32_t v = 0;
    for (int i = 0; i < CT_SHA_WORD_BYTES; i++) {
        v = (v << CT_B8) | (uint32_t)p[i];
    }
    return v;
}

static void ct_sha_compress(uint32_t h[CT_SHA_WORD_COUNT], const unsigned char *block) {
    uint32_t w[CT_SHA_ROUND_COUNT];
    int words = CT_SHA_BLOCK_BYTES / CT_SHA_WORD_BYTES;
    for (int t = 0; t < words; t++) {
        w[t] = ct_sha_load_be32(block + (size_t)t * CT_SHA_WORD_BYTES);
    }
    for (int t = words; t < CT_SHA_ROUND_COUNT; t++) {
        uint32_t s0 = ct_sha_lower_sigma0(w[t - CT_SHA_SCHED_BACK_15]);
        uint32_t s1 = ct_sha_lower_sigma1(w[t - CT_SHA_SCHED_BACK_2]);
        w[t] = w[t - CT_SHA_SCHED_BACK_16] + s0 + w[t - CT_SHA_SCHED_BACK_7] + s1;
    }
    uint32_t a = h[0];
    uint32_t b = h[1];
    uint32_t c = h[2];
    uint32_t d = h[3];
    uint32_t e = h[4];
    uint32_t f = h[5];
    uint32_t g = h[6];
    uint32_t hh = h[7];
    for (int t = 0; t < CT_SHA_ROUND_COUNT; t++) {
        uint32_t t1 = hh + ct_sha_upper_sigma1(e) + ((e & f) ^ (~e & g)) + ct_sha_k[t] + w[t];
        uint32_t t2 = ct_sha_upper_sigma0(a) + ((a & b) ^ (a & c) ^ (b & c));
        hh = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
}

void ct_sha256(const void *data, unsigned len, char hex_out[CT_SHA_HEX_MAX]) {
    if (hex_out == NULL)
        return;
    if (data == NULL)
        len = 0;
    ct_sha_init();
    uint32_t h[CT_SHA_WORD_COUNT];
    memcpy(h, ct_sha_h0, sizeof h);
    const unsigned char *p = (const unsigned char *)data;
    unsigned done = 0;
    while (len - done >= (unsigned)CT_SHA_BLOCK_BYTES) {
        ct_sha_compress(h, p + done);
        done += (unsigned)CT_SHA_BLOCK_BYTES;
    }
    unsigned rem = len - done;
    unsigned char tail[CT_SHA_BLOCK_BYTES * CT_B2];
    memset(tail, 0, sizeof tail);
    memcpy(tail, p + done, rem);
    tail[rem] = (unsigned char)(CT_B8 * CT_B16);
    if (rem >= (unsigned)(CT_SHA_BLOCK_BYTES - CT_B8)) {
        ct_sha_compress(h, tail);
        memset(tail, 0, sizeof tail);
    }
    unsigned long long bits = (unsigned long long)len * CT_B8;
    for (int b = 0; b < CT_B8; b++) {
        tail[CT_SHA_BLOCK_BYTES - CT_B8 + b] =
            (unsigned char)(bits >> (CT_B8 * (CT_B8 - CT_B1 - b)));
    }
    ct_sha_compress(h, tail);
    for (int i = 0; i < CT_SHA_WORD_COUNT; i++) {
        for (int j = 0; j < CT_B8; j++) {
            hex_out[i * CT_B8 + j] =
                CT_SHA_HEX[(h[i] >> (CT_B4 * (CT_B8 - CT_B1 - j))) & (CT_B16 - CT_B1)];
        }
    }
    hex_out[CT_SHA_HEX_MAX - CT_B1] = '\0';
}

void ct_pack_source_record(char *out, unsigned cap, const char *url, const char *retrieved_at,
                           const char *alg) {
    if (out == NULL || cap == 0)
        return;
    snprintf(out, cap, "%s|%s|%s", url != NULL ? url : "", retrieved_at != NULL ? retrieved_at : "",
             alg != NULL ? alg : "");
}
