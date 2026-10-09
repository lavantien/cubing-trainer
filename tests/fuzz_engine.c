#include "engine/cube.h"
#include "engine/notation.h"
#include "engine/transform.h"
#include "framework.h"

#include <string.h>

int ct_fuzz_iterations(void);

enum {
    CT_FZ_MAX_LEN = 48,
    CT_FZ_PLANT_MAX = CT_FZ_MAX_LEN + 3,
    CT_FZ_TOKEN_CAP = 8,
    CT_FZ_ALG_TOKENS = 24,
    CT_FZ_ALG_STR_CAP = 192,
    CT_FZ_BYTES_MAX = 64,
    CT_FZ_GARBAGE_CAP = 36,
    CT_FZ_GARBAGE_STR_CAP = 256,
    CT_FZ_BATCH_MAX = 24
};

static const ct_move ct_fz_guard_move = {12345, 12345, 12345, 12345};

#define CT_FZ(cond, seed)                                                                          \
    do {                                                                                           \
        if (cond) {                                                                                \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_eq(__FILE__, __LINE__, #cond, "seed", (unsigned long long)(seed), 0);          \
        }                                                                                          \
    } while (0)

static bool ct_fz_move_eq(ct_move a, ct_move b) {
    return a.face == b.face && a.depth_from == b.depth_from && a.depth_to == b.depth_to &&
           a.quarter == b.quarter;
}

static void ct_fz_fill_random_alg(ct_sfc64 *s, int n, ct_move *alg, int len) {
    for (int i = 0; i < len; i++) {
        alg[i].face = (int)ct_sfc64_range(s, CT_CUBE_FACE_COUNT);
        alg[i].depth_from = 1 + (int)ct_sfc64_range(s, (uint64_t)n);
        alg[i].depth_to =
            alg[i].depth_from + (int)ct_sfc64_range(s, (uint64_t)(n - alg[i].depth_from + 1));
        alg[i].quarter = 1 + (int)ct_sfc64_range(s, 3);
    }
}

static void ct_fz_apply_seq(ct_cube *c, const ct_move *m, int count) {
    for (int i = 0; i < count; i++)
        ct_apply(c, m[i]);
}

static bool ct_fz_facelets_in_range(const ct_cube *c) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++)
        for (int row = 0; row < c->n; row++)
            for (int col = 0; col < c->n; col++)
                if (c->facelet[(face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col] >=
                    CT_CUBE_FACE_COUNT)
                    return false;
    return true;
}

static void ct_fz_histogram(const ct_cube *c, int counts[CT_CUBE_FACE_COUNT]) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++)
        counts[face] = 0;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++)
        for (int row = 0; row < c->n; row++)
            for (int col = 0; col < c->n; col++)
                counts[c->facelet[(face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col]]++;
}

static void ct_fz_mirror_state(const ct_cube *c, ct_cube *out) {
    *out = *c;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++)
        for (int row = 0; row < c->n; row++)
            for (int col = 0; col < c->n; col++) {
                ct_v3 center;
                ct_v3 normal;
                ct_sticker_world(c->n, face, row, col, &center, &normal);
                center.x = -center.x;
                normal.x = -normal.x;
                ct_cell src = ct_world_sticker(center, normal, c->n);
                out->facelet[(face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col] =
                    c->facelet[(src.face * CT_CUBE_STRIDE + src.row) * CT_CUBE_STRIDE + src.col];
            }
}

enum {
    CT_FZ_KIND_BAD = -1,
    CT_FZ_KIND_FACE,
    CT_FZ_KIND_FACE_WIDE,
    CT_FZ_KIND_SLICE,
    CT_FZ_KIND_ROTATION
};

static int ct_fz_letter_face(char c) {
    switch (c) {
    case 'U':
    case 'u':
        return CT_FACE_U;
    case 'D':
    case 'd':
        return CT_FACE_D;
    case 'L':
    case 'l':
        return CT_FACE_L;
    case 'R':
    case 'r':
        return CT_FACE_R;
    case 'F':
    case 'f':
        return CT_FACE_F;
    case 'B':
    case 'b':
        return CT_FACE_B;
    case 'M':
        return CT_FACE_L;
    case 'E':
        return CT_FACE_D;
    case 'S':
        return CT_FACE_F;
    case 'x':
        return CT_FACE_R;
    case 'y':
        return CT_FACE_U;
    case 'z':
        return CT_FACE_F;
    default:
        return -1;
    }
}

static int ct_fz_letter_kind(char c) {
    switch (c) {
    case 'U':
    case 'D':
    case 'L':
    case 'R':
    case 'F':
    case 'B':
        return CT_FZ_KIND_FACE;
    case 'u':
    case 'd':
    case 'l':
    case 'r':
    case 'f':
    case 'b':
        return CT_FZ_KIND_FACE_WIDE;
    case 'M':
    case 'E':
    case 'S':
        return CT_FZ_KIND_SLICE;
    case 'x':
    case 'y':
    case 'z':
        return CT_FZ_KIND_ROTATION;
    default:
        return CT_FZ_KIND_BAD;
    }
}

static bool ct_fz_token_shape(char letter, int digit, int wide, int n, ct_move *exp) {
    int kind = ct_fz_letter_kind(letter);
    if (kind == CT_FZ_KIND_BAD || (kind == CT_FZ_KIND_FACE_WIDE && wide))
        return false;
    exp->face = ct_fz_letter_face(letter);
    if (kind == CT_FZ_KIND_ROTATION || kind == CT_FZ_KIND_SLICE) {
        if (digit != -1 || wide)
            return false;
        if (kind == CT_FZ_KIND_ROTATION) {
            exp->depth_from = 1;
            exp->depth_to = n;
        } else {
            exp->depth_from = (n + 1) / 2;
            exp->depth_to = exp->depth_from;
        }
        return true;
    }
    if (digit != -1 && (digit < 2 || digit > n))
        return false;
    bool is_wide = wide || kind == CT_FZ_KIND_FACE_WIDE;
    if (digit == -1 && is_wide && n < 2)
        return false;
    if (is_wide) {
        exp->depth_from = 1;
        exp->depth_to = digit != -1 ? digit : 2;
    } else {
        exp->depth_from = digit != -1 ? digit : 1;
        exp->depth_to = exp->depth_from;
    }
    return true;
}

static unsigned ct_fz_build_valid_token(ct_sfc64 *s, int n, char *tok, ct_move *exp) {
    static const char faces[] = "UDLRFB";
    static const char slices[] = "MES";
    static const char rotations[] = "xyz";
    unsigned len = 0;
    unsigned cls = (unsigned)ct_sfc64_range(s, 3);
    char letter;
    int digit = -1;
    int wide = 0;
    if (cls == 0) {
        if (n >= 2 && ct_sfc64_range(s, 2) == 0) {
            digit = 2 + (int)ct_sfc64_range(s, (uint64_t)n - 1);
            tok[len++] = (char)('0' + digit);
        }
        letter = faces[ct_sfc64_range(s, sizeof faces - 1)];
        tok[len++] = letter;
        if (n >= 2 && ct_sfc64_range(s, 2) == 0) {
            wide = 1;
            tok[len++] = 'w';
        }
    } else if (cls == 1) {
        letter = slices[ct_sfc64_range(s, sizeof slices - 1)];
        tok[len++] = letter;
    } else {
        letter = rotations[ct_sfc64_range(s, sizeof rotations - 1)];
        tok[len++] = letter;
    }
    unsigned suf = (unsigned)ct_sfc64_range(s, 4);
    if (suf == 1 || suf == 3)
        tok[len++] = '\'';
    if (suf == 2 || suf == 3)
        tok[len++] = '2';
    tok[len] = '\0';
    exp->quarter = suf == 1 ? 3 : (suf == 0 ? 1 : 2);
    ct_fz_token_shape(letter, digit, wide, n, exp);
    return len;
}

CT_SUITE(fuzz_notation_random_tokens) {
    static const char letters[] = "UDLRFBudlrfbMESmesxyzXYZq";
    static const char digits[] = "0123456789";
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0xF0010000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        char tok[CT_FZ_TOKEN_CAP];
        unsigned len = 0;
        int digit = -1;
        if (ct_sfc64_range(&s, 2) == 0) {
            char d = digits[ct_sfc64_range(&s, sizeof digits - 1)];
            digit = d - '0';
            tok[len++] = d;
        }
        char letter = letters[ct_sfc64_range(&s, sizeof letters - 1)];
        tok[len++] = letter;
        int wide = 0;
        if (ct_sfc64_range(&s, 2) == 0) {
            wide = 1;
            tok[len++] = 'w';
        }
        unsigned suf = (unsigned)ct_sfc64_range(&s, 6);
        if (suf == 1 || suf == 2)
            tok[len++] = '\'';
        if (suf == 2 || suf == 3)
            tok[len++] = '2';
        if (suf == 4)
            tok[len++] = 'w';
        if (suf == 5) {
            tok[len++] = '\'';
            tok[len++] = '\'';
        }
        tok[len] = '\0';
        int n = 1 + (int)ct_sfc64_range(&s, CT_CUBE_MAX_N);
        ct_move exp = ct_fz_guard_move;
        int eff_wide = wide || suf == 4;
        bool shape_ok = suf != 5 && ct_fz_token_shape(letter, digit, eff_wide, n, &exp);
        exp.quarter = suf == 1 ? 3 : ((suf == 2 || suf == 3) ? 2 : 1);
        ct_move m = ct_fz_guard_move;
        int r = ct_notation_parse_one(tok, n, &m);
        CT_FZ(r <= 0, seed);
        if (r == 0) {
            CT_FZ(shape_ok, seed);
            CT_FZ(ct_fz_move_eq(m, exp), seed);
            char buf[CT_NOTATION_TOKEN_MAX];
            int flen = ct_notation_format_one(&m, n, buf, sizeof buf);
            CT_FZ(flen >= 1, seed);
            char guard_buf[4];
            memset(guard_buf, '#', sizeof guard_buf);
            CT_FZ(ct_notation_format_one(&m, n, guard_buf, 0) == CT_NOTATION_E_CAP, seed);
            CT_FZ(guard_buf[0] == '#' && guard_buf[1] == '#', seed);
            ct_move back = ct_fz_guard_move;
            CT_FZ(ct_notation_parse_one(buf, n, &back) == 0, seed);
            CT_FZ(ct_fz_move_eq(m, back), seed);
            char buf2[CT_NOTATION_TOKEN_MAX];
            int flen2 = ct_notation_format_one(&back, n, buf2, sizeof buf2);
            CT_FZ(flen2 == flen, seed);
            CT_FZ(strcmp(buf, buf2) == 0, seed);
        } else {
            CT_FZ(ct_fz_move_eq(m, ct_fz_guard_move), seed);
            ct_move cap_out = ct_fz_guard_move;
            int rp = ct_notation_parse(tok, n, &cap_out, 0);
            CT_FZ(rp < 0, seed);
            CT_FZ(ct_fz_move_eq(cap_out, ct_fz_guard_move), seed);
        }
    }
}

CT_SUITE(fuzz_notation_garbage) {
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0x6A8B0000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        char buf[CT_FZ_BYTES_MAX + 1];
        unsigned len = (unsigned)ct_sfc64_range(&s, CT_FZ_BYTES_MAX + 1);
        for (unsigned k = 0; k < len; k++)
            buf[k] = (char)ct_sfc64_range(&s, 256);
        buf[len] = '\0';
        int n = 1 + (int)ct_sfc64_range(&s, CT_CUBE_MAX_N);
        ct_move out[CT_FZ_GARBAGE_CAP];
        int r = ct_notation_parse(buf, n, out, CT_FZ_GARBAGE_CAP);
        CT_FZ(r <= CT_FZ_GARBAGE_CAP, seed);
        if (r > 0) {
            char str[CT_FZ_GARBAGE_STR_CAP];
            int flen = ct_notation_format(out, r, n, str, sizeof str);
            CT_FZ(flen >= 1, seed);
            ct_move back[CT_FZ_GARBAGE_CAP];
            int r2 = ct_notation_parse(str, n, back, CT_FZ_GARBAGE_CAP);
            CT_FZ(r2 == r, seed);
            for (int k = 0; k < r; k++)
                CT_FZ(ct_fz_move_eq(out[k], back[k]), seed);
        }
    }
}

CT_SUITE(fuzz_notation_random_algs) {
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0xA16C0000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        int n = 1 + (int)ct_sfc64_range(&s, CT_CUBE_MAX_N);
        int count = 1 + (int)ct_sfc64_range(&s, CT_FZ_ALG_TOKENS);
        char alg[CT_FZ_ALG_STR_CAP];
        unsigned pos = 0;
        ct_move expected[CT_FZ_ALG_TOKENS];
        for (int t = 0; t < count; t++) {
            if (t > 0)
                alg[pos++] = ' ';
            pos += ct_fz_build_valid_token(&s, n, alg + pos, &expected[t]);
        }
        alg[pos] = '\0';
        ct_move one[CT_FZ_ALG_TOKENS];
        int r1 = ct_notation_parse(alg, n, one, CT_FZ_ALG_TOKENS);
        CT_FZ(r1 == count, seed);
        for (int k = 0; k < count; k++)
            CT_FZ(ct_fz_move_eq(one[k], expected[k]), seed);
        char str[CT_FZ_ALG_STR_CAP];
        int flen = ct_notation_format(one, count, n, str, sizeof str);
        CT_FZ(flen >= 1, seed);
        ct_move two[CT_FZ_ALG_TOKENS];
        int r2 = ct_notation_parse(str, n, two, CT_FZ_ALG_TOKENS);
        CT_FZ(r2 == count, seed);
        for (int k = 0; k < count; k++)
            CT_FZ(ct_fz_move_eq(one[k], two[k]), seed);
    }
}

CT_SUITE(fuzz_transform_inverse) {
    static const int ns[] = {3, 5};
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (size_t f = 0; f < sizeof ns / sizeof ns[0]; f++) {
        int n = ns[f];
        for (int i = 0; i < iters; i++) {
            uint64_t seed = 0x11E5A000u + (uint64_t)i * 8u + (uint64_t)n;
            ct_sfc64 s;
            ct_sfc64_init(&s, seed);
            ct_move alg[CT_FZ_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_FZ_MAX_LEN);
            ct_fz_fill_random_alg(&s, n, alg, len);
            ct_move inv[CT_FZ_MAX_LEN];
            CT_FZ(ct_transform_inverse(alg, len, inv) == len, seed);
            ct_cube fresh;
            ct_cube_init(&fresh, n);
            ct_cube a;
            ct_cube_init(&a, n);
            ct_fz_apply_seq(&a, alg, len);
            ct_fz_apply_seq(&a, inv, len);
            CT_FZ(ct_cube_equal(&a, &fresh), seed);
        }
    }
}

CT_SUITE(fuzz_transform_simplify) {
    static const int ns[] = {3, 5};
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (size_t f = 0; f < sizeof ns / sizeof ns[0]; f++) {
        int n = ns[f];
        for (int i = 0; i < iters; i++) {
            uint64_t seed = 0x51E4B000u + (uint64_t)i * 8u + (uint64_t)n;
            ct_sfc64 s;
            ct_sfc64_init(&s, seed);
            ct_move base[CT_FZ_MAX_LEN];
            int len = 2 + (int)ct_sfc64_range(&s, CT_FZ_MAX_LEN - 1);
            ct_fz_fill_random_alg(&s, n, base, len);
            ct_move planted[CT_FZ_PLANT_MAX];
            int face = (int)ct_sfc64_range(&s, CT_CUBE_FACE_COUNT);
            int quarter = 1 + (int)ct_sfc64_range(&s, 3);
            int variant = (int)ct_sfc64_range(&s, 3);
            const int slots[3] = {0, len / 2, len};
            int slot = (int)ct_sfc64_range(&s, 3);
            int total = 0;
            for (int j = 0; j < slots[slot]; j++)
                planted[total++] = base[j];
            int plant_len = variant == 0 ? 2 : (variant == 1 ? 2 : 3);
            for (int j = 0; j < plant_len; j++) {
                planted[total].face = face;
                planted[total].depth_from = 1;
                planted[total].depth_to = 1;
                planted[total].quarter =
                    variant == 0 && j == 1 ? CT_MOVE_QUARTERS - quarter : quarter;
                total++;
            }
            for (int j = slots[slot]; j < len; j++)
                planted[total++] = base[j];
            ct_move once[CT_FZ_PLANT_MAX];
            int m = ct_transform_simplify(planted, total, once, CT_FZ_PLANT_MAX);
            CT_FZ(m >= 0 && m <= total, seed);
            ct_move twice[CT_FZ_PLANT_MAX];
            int m2 = ct_transform_simplify(once, m, twice, CT_FZ_PLANT_MAX);
            CT_FZ(m2 == m, seed);
            CT_FZ(memcmp(once, twice, (size_t)m * sizeof(ct_move)) == 0, seed);
            ct_cube a;
            ct_cube_init(&a, n);
            ct_fz_apply_seq(&a, planted, total);
            ct_cube b;
            ct_cube_init(&b, n);
            ct_fz_apply_seq(&b, once, m);
            CT_FZ(ct_cube_equal(&a, &b), seed);
        }
    }
}

CT_SUITE(fuzz_transform_rotate_y) {
    static const int ns[] = {3, 5};
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (size_t f = 0; f < sizeof ns / sizeof ns[0]; f++) {
        int n = ns[f];
        for (int i = 0; i < iters; i++) {
            uint64_t seed = 0x707C000u + (uint64_t)i * 8u + (uint64_t)n;
            ct_sfc64 s;
            ct_sfc64_init(&s, seed);
            ct_move alg[CT_FZ_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_FZ_MAX_LEN);
            ct_fz_fill_random_alg(&s, n, alg, len);
            int k = 1 + (int)ct_sfc64_range(&s, 3);
            ct_move rotated[CT_FZ_MAX_LEN];
            CT_FZ(ct_transform_rotate_y(alg, len, k, rotated) == len, seed);
            ct_move y = {CT_FACE_U, 1, n, k};
            ct_cube a;
            ct_cube_init(&a, n);
            ct_fz_apply_seq(&a, alg, len);
            ct_apply(&a, y);
            ct_cube b;
            ct_cube_init(&b, n);
            ct_apply(&b, y);
            ct_fz_apply_seq(&b, rotated, len);
            CT_FZ(ct_cube_equal(&a, &b), seed);
        }
    }
}

CT_SUITE(fuzz_cube_invariants) {
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0xC0FF0000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        int n = 2 + (int)ct_sfc64_range(&s, CT_CUBE_MAX_N - 1);
        ct_cube c;
        ct_cube_init(&c, n);
        ct_move scramble[CT_FZ_BATCH_MAX];
        int slen = 1 + (int)ct_sfc64_range(&s, CT_FZ_BATCH_MAX);
        ct_fz_fill_random_alg(&s, n, scramble, slen);
        for (int k = 0; k < slen; k++) {
            ct_apply(&c, scramble[k]);
            CT_FZ(ct_fz_facelets_in_range(&c), seed);
        }
        int hist[CT_CUBE_FACE_COUNT];
        ct_fz_histogram(&c, hist);
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++)
            CT_FZ(hist[face] == n * n, seed);
        ct_cube start = c;
        ct_move batch[CT_FZ_BATCH_MAX];
        int blen = 1 + (int)ct_sfc64_range(&s, CT_FZ_BATCH_MAX);
        ct_fz_fill_random_alg(&s, n, batch, blen);
        int hist2[CT_CUBE_FACE_COUNT];
        for (int k = 0; k < blen; k++) {
            ct_apply(&c, batch[k]);
            CT_FZ(ct_fz_facelets_in_range(&c), seed);
            ct_fz_histogram(&c, hist2);
            CT_FZ(memcmp(hist, hist2, sizeof hist) == 0, seed);
        }
        for (int k = blen - 1; k >= 0; k--)
            ct_apply(&c, ct_move_inverse(batch[k]));
        CT_FZ(ct_cube_equal(&c, &start), seed);
    }
}

CT_SUITE(fuzz_cube_mirror) {
    static const int ns[] = {3, 5};
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (size_t f = 0; f < sizeof ns / sizeof ns[0]; f++) {
        int n = ns[f];
        for (int i = 0; i < iters; i++) {
            uint64_t seed = 0xAF1FD000u + (uint64_t)i * 8u + (uint64_t)n;
            ct_sfc64 s;
            ct_sfc64_init(&s, seed);
            ct_move pre[CT_FZ_BATCH_MAX];
            int plen = 1 + (int)ct_sfc64_range(&s, CT_FZ_BATCH_MAX);
            ct_fz_fill_random_alg(&s, n, pre, plen);
            ct_cube st;
            ct_cube_init(&st, n);
            ct_fz_apply_seq(&st, pre, plen);
            ct_move alg[CT_FZ_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_FZ_MAX_LEN);
            ct_fz_fill_random_alg(&s, n, alg, len);
            ct_move mirrored[CT_FZ_MAX_LEN];
            CT_FZ(ct_transform_mirror(alg, len, mirrored) == len, seed);
            ct_cube left;
            ct_fz_mirror_state(&st, &left);
            ct_fz_apply_seq(&left, mirrored, len);
            ct_cube right = st;
            ct_fz_apply_seq(&right, alg, len);
            ct_cube right_m;
            ct_fz_mirror_state(&right, &right_m);
            CT_FZ(ct_cube_equal(&left, &right_m), seed);
        }
    }
}
