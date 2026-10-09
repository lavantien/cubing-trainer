#ifndef CT_PACKS_PROVENANCE_H
#define CT_PACKS_PROVENANCE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CT_B1 = 1,
    CT_B2 = CT_B1 << 1,
    CT_B4 = CT_B1 << 2,
    CT_B8 = CT_B1 << 3,
    CT_B16 = CT_B1 << 4,
    CT_B32 = CT_B1 << 5,
    CT_B64 = CT_B1 << 6,
    CT_B128 = CT_B8 * CT_B16,
    CT_B256 = CT_B16 * CT_B16,
    CT_SHA_WORD_BITS = CT_B32,
    CT_SHA_WORD_BYTES = CT_B4,
    CT_SHA_WORD_COUNT = CT_B8,
    CT_SHA_BLOCK_BYTES = CT_B64,
    CT_SHA_ROUND_COUNT = CT_B64,
    CT_SHA_HEX_MAX = CT_B64 + CT_B1,
    CT_SHA_ROTR_2 = CT_B2,
    CT_SHA_ROTR_3 = CT_B2 + CT_B1,
    CT_SHA_ROTR_6 = CT_B4 + CT_B2,
    CT_SHA_ROTR_7 = CT_B4 + CT_B2 + CT_B1,
    CT_SHA_ROTR_10 = CT_B8 + CT_B2,
    CT_SHA_ROTR_11 = CT_B8 + CT_B2 + CT_B1,
    CT_SHA_ROTR_13 = CT_B8 + CT_B4 + CT_B1,
    CT_SHA_ROTR_17 = CT_B16 + CT_B1,
    CT_SHA_ROTR_18 = CT_B16 + CT_B2,
    CT_SHA_ROTR_19 = CT_B16 + CT_B2 + CT_B1,
    CT_SHA_ROTR_22 = CT_B16 + CT_B4 + CT_B2,
    CT_SHA_ROTR_25 = CT_B16 + CT_B8 + CT_B1,
    CT_SHA_SCHED_BACK_2 = CT_B2,
    CT_SHA_SCHED_BACK_7 = CT_B8 - CT_B1,
    CT_SHA_SCHED_BACK_15 = CT_B16 - CT_B1,
    CT_SHA_SCHED_BACK_16 = CT_B16,
    CT_SHA_DERIVE_SHIFT_SQRT = CT_SHA_WORD_BITS * CT_B2,
    CT_SHA_DERIVE_SHIFT_CBRT = CT_SHA_WORD_BITS * (CT_B2 + CT_B1),
    CT_SHA_DERIVE_BOUND_SHIFT = CT_SHA_WORD_BITS + CT_B8
};

void ct_sha256(const void *data, unsigned len, char hex_out[CT_SHA_HEX_MAX]);
void ct_pack_source_record(char *out, unsigned cap, const char *url, const char *retrieved_at,
                           const char *alg);

#ifdef __cplusplus
}
#endif

#endif
