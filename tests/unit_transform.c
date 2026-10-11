#include <string.h>

#include "engine/cube.h"
#include "engine/transform.h"
#include "framework.h"

enum {
    CT_T_ROUNDS = 200,
    CT_T_MAX_LEN = 48,
    CT_T_CAP = 256,
    CT_T_SENTINEL = 99,
    CT_T_SMALL_LEN = 8,
    CT_T_PLANT_MAX = CT_T_MAX_LEN + 3
};

static const int ct_t_ns[] = {3, 5};

static void fill_random_alg(ct_sfc64 *s, int n, ct_move *alg, int len) {
    for (int i = 0; i < len; i++) {
        alg[i].face = (int)ct_sfc64_range(s, CT_CUBE_FACE_COUNT);
        alg[i].depth_from = 1 + (int)ct_sfc64_range(s, (uint64_t)n);
        alg[i].depth_to =
            alg[i].depth_from + (int)ct_sfc64_range(s, (uint64_t)(n - alg[i].depth_from + 1));
        alg[i].quarter = 1 + (int)ct_sfc64_range(s, 3);
    }
}

static void apply_seq(ct_cube *c, const ct_move *m, int count) {
    for (int i = 0; i < count; i++)
        ct_apply(c, m[i]);
}

static bool seq_equal(const ct_move *a, const ct_move *b, int count) {
    return memcmp(a, b, (size_t)count * sizeof(ct_move)) == 0;
}

static bool face_all(const ct_cube *c, int face, int value) {
    for (int row = 0; row < c->n; row++)
        for (int col = 0; col < c->n; col++)
            if (c->facelet[(face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col] != value)
                return false;
    return true;
}

static void mirror_state(const ct_cube *c, ct_cube *out) {
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

CT_SUITE(transform_inverse_golden) {
    ct_move alg[3] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 2}, {CT_FACE_F, 2, 3, 3}};
    ct_move out[3];
    CT_ASSERT_EQ(ct_transform_inverse(alg, 3, out), 3);
    CT_ASSERT_EQ(out[0].face, CT_FACE_F);
    CT_ASSERT_EQ(out[0].depth_from, 2);
    CT_ASSERT_EQ(out[0].depth_to, 3);
    CT_ASSERT_EQ(out[0].quarter, 1);
    CT_ASSERT_EQ(out[1].face, CT_FACE_U);
    CT_ASSERT_EQ(out[1].depth_from, 1);
    CT_ASSERT_EQ(out[1].depth_to, 1);
    CT_ASSERT_EQ(out[1].quarter, 2);
    CT_ASSERT_EQ(out[2].face, CT_FACE_R);
    CT_ASSERT_EQ(out[2].depth_from, 1);
    CT_ASSERT_EQ(out[2].depth_to, 1);
    CT_ASSERT_EQ(out[2].quarter, 3);
    ct_move half = {CT_FACE_B, 1, 4, 2};
    ct_move half_out[1];
    CT_ASSERT_EQ(ct_transform_inverse(&half, 1, half_out), 1);
    CT_ASSERT_EQ(half_out[0].face, CT_FACE_B);
    CT_ASSERT_EQ(half_out[0].depth_from, 1);
    CT_ASSERT_EQ(half_out[0].depth_to, 4);
    CT_ASSERT_EQ(half_out[0].quarter, 2);
}

CT_SUITE(transform_inverse_simulation) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_sfc64 s;
            ct_sfc64_init(&s, 0x11E5u + (uint64_t)n * CT_T_ROUNDS + (uint64_t)round);
            ct_move alg[CT_T_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN);
            fill_random_alg(&s, n, alg, len);
            ct_move inv[CT_T_MAX_LEN];
            CT_ASSERT_EQ(ct_transform_inverse(alg, len, inv), len);
            ct_cube fresh;
            ct_cube_init(&fresh, n);
            ct_cube a;
            ct_cube_init(&a, n);
            apply_seq(&a, alg, len);
            apply_seq(&a, inv, len);
            CT_ASSERT(ct_cube_equal(&a, &fresh));
            ct_cube b;
            ct_cube_init(&b, n);
            apply_seq(&b, inv, len);
            apply_seq(&b, alg, len);
            CT_ASSERT(ct_cube_equal(&b, &fresh));
        }
    }
}

CT_SUITE(transform_mirror_golden) {
    ct_move alg[3] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}};
    ct_move out[3];
    CT_ASSERT_EQ(ct_transform_mirror(alg, 3, out), 3);
    CT_ASSERT_EQ(out[0].face, CT_FACE_L);
    CT_ASSERT_EQ(out[0].quarter, 3);
    CT_ASSERT_EQ(out[1].face, CT_FACE_U);
    CT_ASSERT_EQ(out[1].quarter, 3);
    CT_ASSERT_EQ(out[2].face, CT_FACE_L);
    CT_ASSERT_EQ(out[2].quarter, 1);
    const int exp_face[CT_CUBE_FACE_COUNT] = {
        [CT_FACE_U] = CT_FACE_U, [CT_FACE_L] = CT_FACE_R, [CT_FACE_F] = CT_FACE_F,
        [CT_FACE_R] = CT_FACE_L, [CT_FACE_B] = CT_FACE_B, [CT_FACE_D] = CT_FACE_D,
    };
    const int exp_quarter[3] = {3, 2, 1};
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++)
        for (int quarter = 1; quarter <= 3; quarter++) {
            ct_move m = {face, 1, 1, quarter};
            ct_move r[1];
            CT_ASSERT_EQ(ct_transform_mirror(&m, 1, r), 1);
            CT_ASSERT_EQ(r[0].face, exp_face[face]);
            CT_ASSERT_EQ(r[0].quarter, exp_quarter[quarter - 1]);
            CT_ASSERT_EQ(r[0].depth_from, 1);
            CT_ASSERT_EQ(r[0].depth_to, 1);
        }
    ct_move deep = {CT_FACE_R, 2, 4, 1};
    ct_move deep_out[1];
    CT_ASSERT_EQ(ct_transform_mirror(&deep, 1, deep_out), 1);
    CT_ASSERT_EQ(deep_out[0].face, CT_FACE_L);
    CT_ASSERT_EQ(deep_out[0].depth_from, 2);
    CT_ASSERT_EQ(deep_out[0].depth_to, 4);
    CT_ASSERT_EQ(deep_out[0].quarter, 3);
}

CT_SUITE(transform_mirror_involution) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        ct_sfc64 s;
        ct_sfc64_init(&s, 0x5EEDu + (uint64_t)n);
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_move alg[CT_T_MAX_LEN];
            ct_move once[CT_T_MAX_LEN];
            ct_move twice[CT_T_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN);
            fill_random_alg(&s, n, alg, len);
            CT_ASSERT_EQ(ct_transform_mirror(alg, len, once), len);
            CT_ASSERT_EQ(ct_transform_mirror(once, len, twice), len);
            CT_ASSERT(seq_equal(alg, twice, len));
        }
    }
}

CT_SUITE(transform_mirror_inverse_commute) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        ct_sfc64 s;
        ct_sfc64_init(&s, 0xC0DEu + (uint64_t)n);
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_move alg[CT_T_MAX_LEN];
            ct_move mirrored[CT_T_MAX_LEN];
            ct_move inv_of_mirror[CT_T_MAX_LEN];
            ct_move inverted[CT_T_MAX_LEN];
            ct_move mirror_of_inv[CT_T_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN);
            fill_random_alg(&s, n, alg, len);
            CT_ASSERT_EQ(ct_transform_mirror(alg, len, mirrored), len);
            CT_ASSERT_EQ(ct_transform_inverse(mirrored, len, inv_of_mirror), len);
            CT_ASSERT_EQ(ct_transform_inverse(alg, len, inverted), len);
            CT_ASSERT_EQ(ct_transform_mirror(inverted, len, mirror_of_inv), len);
            CT_ASSERT(seq_equal(inv_of_mirror, mirror_of_inv, len));
        }
    }
}

CT_SUITE(transform_mirror_state_simulation) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        ct_cube solved;
        ct_cube_init(&solved, n);
        ct_cube mirrored_solved;
        mirror_state(&solved, &mirrored_solved);
        CT_ASSERT(face_all(&mirrored_solved, CT_FACE_U, CT_FACE_U));
        CT_ASSERT(face_all(&mirrored_solved, CT_FACE_L, CT_FACE_R));
        CT_ASSERT(face_all(&mirrored_solved, CT_FACE_F, CT_FACE_F));
        CT_ASSERT(face_all(&mirrored_solved, CT_FACE_R, CT_FACE_L));
        CT_ASSERT(face_all(&mirrored_solved, CT_FACE_B, CT_FACE_B));
        CT_ASSERT(face_all(&mirrored_solved, CT_FACE_D, CT_FACE_D));
        ct_sfc64 s;
        ct_sfc64_init(&s, 0xAF1Fu + (uint64_t)n);
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_move alg[CT_T_MAX_LEN];
            ct_move mirrored[CT_T_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN);
            fill_random_alg(&s, n, alg, len);
            CT_ASSERT_EQ(ct_transform_mirror(alg, len, mirrored), len);
            ct_cube a;
            ct_cube_init(&a, n);
            apply_seq(&a, alg, len);
            ct_cube a_mirrored;
            mirror_state(&a, &a_mirrored);
            ct_cube b = mirrored_solved;
            apply_seq(&b, mirrored, len);
            CT_ASSERT(ct_cube_equal(&a_mirrored, &b));
        }
    }
}

CT_SUITE(transform_rotate_y_golden) {
    const int y1[CT_CUBE_FACE_COUNT] = {
        [CT_FACE_U] = CT_FACE_U, [CT_FACE_L] = CT_FACE_B, [CT_FACE_F] = CT_FACE_L,
        [CT_FACE_R] = CT_FACE_F, [CT_FACE_B] = CT_FACE_R, [CT_FACE_D] = CT_FACE_D,
    };
    const int y2[CT_CUBE_FACE_COUNT] = {
        [CT_FACE_U] = CT_FACE_U, [CT_FACE_L] = CT_FACE_R, [CT_FACE_F] = CT_FACE_B,
        [CT_FACE_R] = CT_FACE_L, [CT_FACE_B] = CT_FACE_F, [CT_FACE_D] = CT_FACE_D,
    };
    const int y3[CT_CUBE_FACE_COUNT] = {
        [CT_FACE_U] = CT_FACE_U, [CT_FACE_L] = CT_FACE_F, [CT_FACE_F] = CT_FACE_R,
        [CT_FACE_R] = CT_FACE_B, [CT_FACE_B] = CT_FACE_L, [CT_FACE_D] = CT_FACE_D,
    };
    const int *tables[3] = {y1, y2, y3};
    for (int k = 1; k <= 3; k++)
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++)
            for (int quarter = 1; quarter <= 3; quarter++) {
                ct_move m = {face, 1, 1, quarter};
                ct_move r[1];
                CT_ASSERT_EQ(ct_transform_rotate_y(&m, 1, k, r), 1);
                CT_ASSERT_EQ(r[0].face, tables[k - 1][face]);
                CT_ASSERT_EQ(r[0].quarter, quarter);
                CT_ASSERT_EQ(r[0].depth_from, 1);
                CT_ASSERT_EQ(r[0].depth_to, 1);
            }
    for (int k = 1; k <= 3; k++) {
        ct_move deep = {CT_FACE_R, 2, 4, 2};
        ct_move deep_out[1];
        CT_ASSERT_EQ(ct_transform_rotate_y(&deep, 1, k, deep_out), 1);
        CT_ASSERT_EQ(deep_out[0].depth_from, 2);
        CT_ASSERT_EQ(deep_out[0].depth_to, 4);
        CT_ASSERT_EQ(deep_out[0].quarter, 2);
    }
}

CT_SUITE(transform_rotate_y_simulation) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        for (int k = 1; k <= 3; k++)
            for (int round = 0; round < CT_T_ROUNDS; round++) {
                ct_sfc64 s;
                ct_sfc64_init(&s, 0x707u + ((uint64_t)n * 3u + (uint64_t)k) * CT_T_ROUNDS +
                                      (uint64_t)round);
                ct_move alg[CT_T_MAX_LEN];
                int len = 1 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN);
                fill_random_alg(&s, n, alg, len);
                ct_move rotated[CT_T_MAX_LEN];
                CT_ASSERT_EQ(ct_transform_rotate_y(alg, len, k, rotated), len);
                ct_move y = {CT_FACE_U, 1, n, k};
                ct_cube a;
                ct_cube_init(&a, n);
                apply_seq(&a, alg, len);
                ct_apply(&a, y);
                ct_cube b;
                ct_cube_init(&b, n);
                ct_apply(&b, y);
                apply_seq(&b, rotated, len);
                CT_ASSERT(ct_cube_equal(&a, &b));
            }
    }
}

CT_SUITE(transform_rotate_y_fourfold) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        ct_sfc64 s;
        ct_sfc64_init(&s, 0xF047u + (uint64_t)n);
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_move alg[CT_T_MAX_LEN];
            ct_move cur[CT_T_MAX_LEN];
            ct_move next[CT_T_MAX_LEN];
            int len = 1 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN);
            fill_random_alg(&s, n, alg, len);
            int k = 1 + (int)ct_sfc64_range(&s, 3);
            memcpy(cur, alg, (size_t)len * sizeof(ct_move));
            for (int times = 0; times < 4; times++) {
                CT_ASSERT_EQ(ct_transform_rotate_y(cur, len, k, next), len);
                memcpy(cur, next, (size_t)len * sizeof(ct_move));
            }
            CT_ASSERT(seq_equal(alg, cur, len));
        }
    }
}

CT_SUITE(transform_conjugate_expansion) {
    ct_move setup[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 2, 3, 2}};
    ct_move inner[3] = {{CT_FACE_F, 1, 1, 3}, {CT_FACE_D, 1, 1, 1}, {CT_FACE_B, 3, 3, 2}};
    ct_move out[7];
    CT_ASSERT_EQ(ct_transform_conjugate(setup, 2, inner, 3, out, 7), 7);
    CT_ASSERT_EQ(out[0].face, CT_FACE_R);
    CT_ASSERT_EQ(out[0].quarter, 1);
    CT_ASSERT_EQ(out[1].face, CT_FACE_U);
    CT_ASSERT_EQ(out[1].depth_from, 2);
    CT_ASSERT_EQ(out[1].depth_to, 3);
    CT_ASSERT_EQ(out[1].quarter, 2);
    CT_ASSERT_EQ(out[2].face, CT_FACE_F);
    CT_ASSERT_EQ(out[2].quarter, 3);
    CT_ASSERT_EQ(out[3].face, CT_FACE_D);
    CT_ASSERT_EQ(out[3].quarter, 1);
    CT_ASSERT_EQ(out[4].face, CT_FACE_B);
    CT_ASSERT_EQ(out[4].quarter, 2);
    CT_ASSERT_EQ(out[5].face, CT_FACE_U);
    CT_ASSERT_EQ(out[5].depth_from, 2);
    CT_ASSERT_EQ(out[5].depth_to, 3);
    CT_ASSERT_EQ(out[5].quarter, 2);
    CT_ASSERT_EQ(out[6].face, CT_FACE_R);
    CT_ASSERT_EQ(out[6].quarter, 3);
    CT_ASSERT_EQ(ct_transform_conjugate(setup, 2, inner, 3, out, 6), -1);
}

CT_SUITE(transform_conjugate_simulation) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        ct_sfc64 s;
        ct_sfc64_init(&s, 0x2A2Bu + (uint64_t)n);
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_move setup[CT_T_SMALL_LEN];
            ct_move inner[CT_T_SMALL_LEN];
            ct_move inv[CT_T_SMALL_LEN];
            ct_move conj[3 * CT_T_SMALL_LEN];
            int setup_len = 1 + (int)ct_sfc64_range(&s, CT_T_SMALL_LEN);
            int inner_len = 1 + (int)ct_sfc64_range(&s, CT_T_SMALL_LEN);
            fill_random_alg(&s, n, setup, setup_len);
            fill_random_alg(&s, n, inner, inner_len);
            CT_ASSERT_EQ(ct_transform_inverse(setup, setup_len, inv), setup_len);
            int total = ct_transform_conjugate(setup, setup_len, inner, inner_len, conj,
                                               3 * CT_T_SMALL_LEN);
            CT_ASSERT_EQ(total, 2 * setup_len + inner_len);
            ct_cube a;
            ct_cube_init(&a, n);
            apply_seq(&a, setup, setup_len);
            apply_seq(&a, inner, inner_len);
            apply_seq(&a, inv, setup_len);
            ct_cube b;
            ct_cube_init(&b, n);
            apply_seq(&b, conj, total);
            CT_ASSERT(ct_cube_equal(&a, &b));
        }
        ct_move setup[1] = {{CT_FACE_R, 1, 1, 1}};
        ct_move out[2];
        CT_ASSERT_EQ(ct_transform_conjugate(setup, 1, NULL, 0, out, 2), 2);
        ct_cube c;
        ct_cube_init(&c, n);
        apply_seq(&c, out, 2);
        CT_ASSERT(ct_cube_solved(&c));
        CT_ASSERT_EQ(ct_transform_conjugate(NULL, 0, NULL, 0, out, 2), 0);
    }
}

CT_SUITE(transform_simplify_golden) {
    ct_move in[CT_T_CAP];
    ct_move out[CT_T_CAP];
    int m;

    ct_move merge_pair[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}};
    m = ct_transform_simplify(merge_pair, 2, out, CT_T_CAP);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].face, CT_FACE_R);
    CT_ASSERT_EQ(out[0].quarter, 2);

    ct_move cancel_pair[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}};
    CT_ASSERT_EQ(ct_transform_simplify(cancel_pair, 2, out, CT_T_CAP), 0);

    ct_move triple[3] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}};
    m = ct_transform_simplify(triple, 3, out, CT_T_CAP);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].quarter, 3);

    for (int i = 0; i < 5; i++) {
        in[i].face = CT_FACE_R;
        in[i].depth_from = 1;
        in[i].depth_to = 1;
        in[i].quarter = 1;
    }
    m = ct_transform_simplify(in, 5, out, CT_T_CAP);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].quarter, 1);

    ct_move buried[4] = {
        {CT_FACE_U, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}, {CT_FACE_F, 1, 1, 1}};
    m = ct_transform_simplify(buried, 4, out, CT_T_CAP);
    CT_ASSERT_EQ(m, 2);
    CT_ASSERT_EQ(out[0].face, CT_FACE_U);
    CT_ASSERT_EQ(out[0].quarter, 1);
    CT_ASSERT_EQ(out[1].face, CT_FACE_F);
    CT_ASSERT_EQ(out[1].quarter, 1);

    ct_move cross[4] = {
        {CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 1}, {CT_FACE_U, 1, 1, 3}, {CT_FACE_R, 1, 1, 2}};
    m = ct_transform_simplify(cross, 4, out, CT_T_CAP);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].face, CT_FACE_R);
    CT_ASSERT_EQ(out[0].quarter, 3);

    ct_move cascade[4] = {
        {CT_FACE_R, 1, 1, 2}, {CT_FACE_U, 1, 1, 1}, {CT_FACE_U, 1, 1, 3}, {CT_FACE_R, 1, 1, 2}};
    CT_ASSERT_EQ(ct_transform_simplify(cascade, 4, out, CT_T_CAP), 0);

    ct_move depths[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 2, 2, 1}};
    m = ct_transform_simplify(depths, 2, out, CT_T_CAP);
    CT_ASSERT_EQ(m, 2);
    CT_ASSERT(seq_equal(depths, out, 2));

    ct_move spread[3] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 2}, {CT_FACE_R, 1, 1, 1}};
    m = ct_transform_simplify(spread, 3, out, CT_T_CAP);
    CT_ASSERT_EQ(m, 3);
    CT_ASSERT(seq_equal(spread, out, 3));
}

CT_SUITE(transform_simplify_properties) {
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        ct_sfc64 s;
        ct_sfc64_init(&s, 0x51E4u + (uint64_t)n);
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_move alg[CT_T_MAX_LEN];
            ct_move once[CT_T_CAP];
            ct_move twice[CT_T_CAP];
            int len = 2 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN - 1);
            fill_random_alg(&s, n, alg, len);
            int m = ct_transform_simplify(alg, len, once, CT_T_CAP);
            CT_ASSERT(m >= 0);
            CT_ASSERT(m <= len);
            int m2 = ct_transform_simplify(once, m, twice, CT_T_CAP);
            CT_ASSERT_EQ(m2, m);
            CT_ASSERT(seq_equal(once, twice, m));
            ct_cube a;
            ct_cube_init(&a, n);
            apply_seq(&a, alg, len);
            ct_cube b;
            ct_cube_init(&b, n);
            apply_seq(&b, once, m);
            CT_ASSERT(ct_cube_equal(&a, &b));
        }
    }
}

CT_SUITE(transform_simplify_planted) {
    const int plant_lens[3] = {2, 2, 3};
    const int plant_quarters[3][3] = {{1, 3, 0}, {1, 1, 0}, {1, 1, 1}};
    for (size_t i = 0; i < sizeof ct_t_ns / sizeof ct_t_ns[0]; i++) {
        int n = ct_t_ns[i];
        ct_sfc64 s;
        ct_sfc64_init(&s, 0xFA7u + (uint64_t)n);
        for (int round = 0; round < CT_T_ROUNDS; round++) {
            ct_move base[CT_T_MAX_LEN];
            int len = 2 + (int)ct_sfc64_range(&s, CT_T_MAX_LEN - 1);
            fill_random_alg(&s, n, base, len);
            for (int variant = 0; variant < 3; variant++) {
                int plant_len = plant_lens[variant];
                int at[3] = {0, len / 2, len};
                int slot = (int)ct_sfc64_range(&s, 3);
                ct_move planted[CT_T_PLANT_MAX];
                int total = 0;
                for (int j = 0; j < at[slot]; j++)
                    planted[total++] = base[j];
                for (int j = 0; j < plant_len; j++) {
                    planted[total].face = CT_FACE_R;
                    planted[total].depth_from = 1;
                    planted[total].depth_to = 1;
                    planted[total].quarter = plant_quarters[variant][j];
                    total++;
                }
                for (int j = at[slot]; j < len; j++)
                    planted[total++] = base[j];
                ct_move once[CT_T_PLANT_MAX];
                int m = ct_transform_simplify(planted, total, once, CT_T_PLANT_MAX);
                CT_ASSERT(m >= 0);
                CT_ASSERT(m <= total);
                ct_cube a;
                ct_cube_init(&a, n);
                apply_seq(&a, planted, total);
                ct_cube b;
                ct_cube_init(&b, n);
                apply_seq(&b, once, m);
                CT_ASSERT(ct_cube_equal(&a, &b));
            }
        }
    }
}

CT_SUITE(transform_simplify_cap_output_bound) {
    ct_move out[CT_T_CAP];
    ct_move again[CT_T_CAP];
    ct_move plant = {CT_T_SENTINEL, CT_T_SENTINEL, CT_T_SENTINEL, CT_T_SENTINEL};
    ct_cube a;
    ct_cube b;
    int m;
    int m2;

    ct_move reviewer[3] = {{CT_FACE_U, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}};
    m = ct_transform_simplify(reviewer, 3, out, 1);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].face, CT_FACE_U);
    CT_ASSERT_EQ(out[0].depth_from, 1);
    CT_ASSERT_EQ(out[0].depth_to, 1);
    CT_ASSERT_EQ(out[0].quarter, 1);
    ct_cube_init(&a, 3);
    apply_seq(&a, reviewer, 3);
    ct_cube_init(&b, 3);
    apply_seq(&b, out, m);
    CT_ASSERT(ct_cube_equal(&a, &b));
    m2 = ct_transform_simplify(out, m, again, 1);
    CT_ASSERT_EQ(m2, m);
    CT_ASSERT(seq_equal(out, again, m));

    ct_move telescope[5] = {{CT_FACE_U, 1, 1, 1},
                            {CT_FACE_R, 1, 1, 1},
                            {CT_FACE_F, 1, 1, 1},
                            {CT_FACE_F, 1, 1, 3},
                            {CT_FACE_R, 1, 1, 3}};
    m = ct_transform_simplify(telescope, 5, out, 1);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].face, CT_FACE_U);
    CT_ASSERT_EQ(out[0].quarter, 1);
    ct_cube_init(&a, 3);
    apply_seq(&a, telescope, 5);
    ct_cube_init(&b, 3);
    apply_seq(&b, out, m);
    CT_ASSERT(ct_cube_equal(&a, &b));
    m2 = ct_transform_simplify(out, m, again, 1);
    CT_ASSERT_EQ(m2, m);
    CT_ASSERT(seq_equal(out, again, m));

    ct_move shoulder[4] = {
        {CT_FACE_U, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}, {CT_FACE_F, 1, 1, 1}, {CT_FACE_F, 1, 1, 3}};
    m = ct_transform_simplify(shoulder, 4, out, 2);
    CT_ASSERT_EQ(m, 2);
    CT_ASSERT_EQ(out[0].face, CT_FACE_U);
    CT_ASSERT_EQ(out[1].face, CT_FACE_R);
    ct_cube_init(&a, 3);
    apply_seq(&a, shoulder, 4);
    ct_cube_init(&b, 3);
    apply_seq(&b, out, m);
    CT_ASSERT(ct_cube_equal(&a, &b));
    m2 = ct_transform_simplify(out, m, again, 2);
    CT_ASSERT_EQ(m2, m);
    CT_ASSERT(seq_equal(out, again, m));

    ct_move buried_merge[5] = {{CT_FACE_U, 1, 1, 1},
                               {CT_FACE_U, 1, 1, 1},
                               {CT_FACE_R, 1, 1, 1},
                               {CT_FACE_R, 1, 1, 3},
                               {CT_FACE_U, 1, 1, 3}};
    m = ct_transform_simplify(buried_merge, 5, out, 1);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].face, CT_FACE_U);
    CT_ASSERT_EQ(out[0].quarter, 1);
    ct_cube_init(&a, 3);
    apply_seq(&a, buried_merge, 5);
    ct_cube_init(&b, 3);
    apply_seq(&b, out, m);
    CT_ASSERT(ct_cube_equal(&a, &b));
    m2 = ct_transform_simplify(out, m, again, 1);
    CT_ASSERT_EQ(m2, m);
    CT_ASSERT(seq_equal(out, again, m));

    ct_move head_cancel[3] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}, {CT_FACE_U, 1, 1, 1}};
    m = ct_transform_simplify(head_cancel, 3, out, 1);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(out[0].face, CT_FACE_U);
    CT_ASSERT_EQ(out[0].quarter, 1);
    ct_cube_init(&a, 3);
    apply_seq(&a, head_cancel, 3);
    ct_cube_init(&b, 3);
    apply_seq(&b, out, m);
    CT_ASSERT(ct_cube_equal(&a, &b));

    ct_move full_quarters[4] = {
        {CT_FACE_U, 1, 1, 1}, {CT_FACE_U, 1, 1, 1}, {CT_FACE_U, 1, 1, 1}, {CT_FACE_U, 1, 1, 1}};
    CT_ASSERT_EQ(ct_transform_simplify(full_quarters, 4, out, 0), 0);

    ct_move overflow[3] = {{CT_FACE_U, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}, {CT_FACE_F, 1, 1, 1}};
    for (int i = 0; i < CT_T_CAP; i++)
        out[i] = plant;
    CT_ASSERT_EQ(ct_transform_simplify(overflow, 3, out, 2), -1);
    bool intact = true;
    for (int i = 0; i < CT_T_CAP; i++)
        if (!seq_equal(&out[i], &plant, 1))
            intact = false;
    CT_ASSERT(intact);
}

CT_SUITE(transform_simplify_assembly_bound) {
    ct_move full[CT_TRANSFORM_ASSEMBLY_MAX];
    ct_move out[CT_TRANSFORM_ASSEMBLY_MAX];
    for (int i = 0; i < CT_TRANSFORM_ASSEMBLY_MAX; i++) {
        full[i].face = i % CT_CUBE_FACE_COUNT;
        full[i].depth_from = 1;
        full[i].depth_to = 1;
        full[i].quarter = 1;
    }
    int m = ct_transform_simplify(full, CT_TRANSFORM_ASSEMBLY_MAX, out, CT_TRANSFORM_ASSEMBLY_MAX);
    CT_ASSERT_EQ(m, CT_TRANSFORM_ASSEMBLY_MAX);
    CT_ASSERT(seq_equal(full, out, CT_TRANSFORM_ASSEMBLY_MAX));
    CT_ASSERT_EQ(
        ct_transform_simplify(full, CT_TRANSFORM_ASSEMBLY_MAX, out, CT_TRANSFORM_ASSEMBLY_MAX - 1),
        -1);

    ct_move over[CT_TRANSFORM_ASSEMBLY_MAX + 1];
    for (int i = 0; i < CT_TRANSFORM_ASSEMBLY_MAX + 1; i++) {
        over[i].face = i % CT_CUBE_FACE_COUNT;
        over[i].depth_from = 1;
        over[i].depth_to = 1;
        over[i].quarter = 1;
    }
    CT_ASSERT_EQ(ct_transform_simplify(over, CT_TRANSFORM_ASSEMBLY_MAX + 1, out,
                                       CT_TRANSFORM_ASSEMBLY_MAX + 1),
                 -1);

    ct_move collapse[CT_TRANSFORM_ASSEMBLY_MAX];
    int half = CT_TRANSFORM_ASSEMBLY_MAX / 2;
    for (int i = 0; i < half; i++) {
        collapse[i].face = i % CT_CUBE_FACE_COUNT;
        collapse[i].depth_from = 1;
        collapse[i].depth_to = 1;
        collapse[i].quarter = 1;
    }
    for (int i = 0; i < half; i++) {
        collapse[half + i] = collapse[half - 1 - i];
        collapse[half + i].quarter = CT_MOVE_QUARTERS - collapse[half + i].quarter;
    }
    CT_ASSERT_EQ(ct_transform_simplify(collapse, CT_TRANSFORM_ASSEMBLY_MAX, out, 0), 0);
    CT_ASSERT_EQ(ct_transform_simplify(collapse, CT_TRANSFORM_ASSEMBLY_MAX, out, 1), 0);

    ct_cube a;
    ct_cube_init(&a, 3);
    apply_seq(&a, collapse, CT_TRANSFORM_ASSEMBLY_MAX);
    CT_ASSERT(ct_cube_solved(&a));
}

CT_SUITE(transform_edges) {
    ct_move out[8];
    CT_ASSERT_EQ(ct_transform_inverse(NULL, 0, out), 0);
    CT_ASSERT_EQ(ct_transform_mirror(NULL, 0, out), 0);
    CT_ASSERT_EQ(ct_transform_rotate_y(NULL, 0, 1, out), 0);
    CT_ASSERT_EQ(ct_transform_conjugate(NULL, 0, NULL, 0, out, 0), 0);
    CT_ASSERT_EQ(ct_transform_simplify(NULL, 0, out, 0), 0);
    ct_move alg[1] = {{CT_FACE_U, 1, 1, 1}};
    CT_ASSERT_EQ(ct_transform_inverse(NULL, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_mirror(NULL, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_rotate_y(NULL, 1, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_simplify(NULL, 1, out, 4), -1);
    CT_ASSERT_EQ(ct_transform_conjugate(NULL, 1, alg, 1, out, 8), -1);
    CT_ASSERT_EQ(ct_transform_conjugate(alg, 1, NULL, 1, out, 8), -1);
    ct_move guard[CT_T_CAP];
    ct_move sentinel = {CT_T_SENTINEL, CT_T_SENTINEL, CT_T_SENTINEL, CT_T_SENTINEL};
    for (int i = 0; i < CT_T_CAP; i++)
        guard[i] = sentinel;
    ct_move two[2] = {{CT_FACE_U, 1, 1, 1}, {CT_FACE_F, 1, 1, 1}};
    CT_ASSERT_EQ(ct_transform_simplify(two, 2, guard, 1), -1);
    bool intact = true;
    for (int i = 0; i < CT_T_CAP; i++)
        if (!seq_equal(&guard[i], &sentinel, 1))
            intact = false;
    CT_ASSERT(intact);
    for (int i = 0; i < CT_T_CAP; i++)
        guard[i] = sentinel;
    ct_move cancel[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}};
    CT_ASSERT_EQ(ct_transform_simplify(cancel, 2, guard, 1), 0);
    ct_move bad_face[1] = {{CT_CUBE_FACE_COUNT, 1, 1, 1}};
    CT_ASSERT_EQ(ct_transform_mirror(bad_face, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_rotate_y(bad_face, 1, 1, out), -1);
    ct_move bad_quarter[1] = {{CT_FACE_U, 1, 1, 0}};
    CT_ASSERT_EQ(ct_transform_inverse(bad_quarter, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_simplify(bad_quarter, 1, out, 4), -1);
    ct_move bad_depth[1] = {{CT_FACE_U, 2, 1, 1}};
    CT_ASSERT_EQ(ct_transform_rotate_y(bad_depth, 1, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_conjugate(bad_depth, 1, alg, 1, out, 8), -1);
    ct_move half[1] = {{CT_FACE_U, 1, 1, 2}};
    CT_ASSERT_EQ(ct_transform_inverse(half, 1, out), 1);
    CT_ASSERT_EQ(out[0].quarter, 2);
    CT_ASSERT_EQ(ct_transform_mirror(half, 1, out), 1);
    CT_ASSERT_EQ(out[0].quarter, 2);

    ct_move over_quarter[1] = {{CT_FACE_U, 1, 1, CT_MOVE_QUARTERS}};
    CT_ASSERT_EQ(ct_transform_inverse(over_quarter, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_mirror(over_quarter, 1, out), -1);
    ct_move zero_depth[1] = {{CT_FACE_U, 0, 0, 1}};
    CT_ASSERT_EQ(ct_transform_inverse(zero_depth, 1, out), -1);
    CT_ASSERT_EQ(ct_transform_rotate_y(zero_depth, 1, 1, out), -1);

    CT_ASSERT_EQ(ct_transform_inverse(NULL, 0, NULL), 0);
    CT_ASSERT_EQ(ct_transform_mirror(NULL, 0, NULL), 0);
    CT_ASSERT_EQ(ct_transform_rotate_y(NULL, 0, 2, NULL), 0);
    CT_ASSERT_EQ(ct_transform_simplify(NULL, 0, NULL, 0), 0);

    CT_ASSERT_EQ(ct_transform_conjugate(alg, -1, alg, 0, out, 8), -1);
    CT_ASSERT_EQ(ct_transform_conjugate(alg, 1, alg, -1, out, 8), -1);
    CT_ASSERT_EQ(ct_transform_conjugate(alg, 1, alg, 1, out, 3), 3);
    CT_ASSERT_EQ(ct_transform_conjugate(alg, 0, alg, 1, NULL, 1), -1);
    CT_ASSERT_EQ(ct_transform_conjugate(alg, 0, alg, 0, NULL, 0), 0);
    CT_ASSERT_EQ(ct_transform_simplify(alg, -1, out, 4), -1);
}
