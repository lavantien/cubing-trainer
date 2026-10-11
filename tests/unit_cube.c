#include <string.h>

#include "engine/cube.h"
#include "framework.h"

enum { CT_SCRAMBLE_LEN = 200 };

static const int ct_cycle_ns[] = {2, 3, 4, 5, 7};
static const int ct_invariant_ns[] = {2, 3, 5, 7};
static const int ct_scramble_ns[] = {4, 7};

static uint8_t at(const ct_cube *c, int face, int row, int col) {
    return c->facelet[(face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col];
}

static bool row_all(const ct_cube *c, int face, int row, int value) {
    for (int col = 0; col < c->n; col++) {
        if (at(c, face, row, col) != value)
            return false;
    }
    return true;
}

static bool col_all(const ct_cube *c, int face, int col, int value) {
    for (int row = 0; row < c->n; row++) {
        if (at(c, face, row, col) != value)
            return false;
    }
    return true;
}

static bool colors_valid(const ct_cube *c) {
    int counts[CT_CUBE_FACE_COUNT] = {0};
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < c->n; row++) {
            for (int col = 0; col < c->n; col++) {
                int v = at(c, face, row, col);
                if (v >= CT_CUBE_FACE_COUNT)
                    return false;
                counts[v]++;
            }
        }
    }
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        if (counts[face] != c->n * c->n)
            return false;
    }
    return true;
}

CT_SUITE(cube_frame_handedness) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        const ct_face_frame *fr = &ct_frames[face];
        ct_v3 cross = {fr->row.y * fr->col.z - fr->row.z * fr->col.y,
                       fr->row.z * fr->col.x - fr->row.x * fr->col.z,
                       fr->row.x * fr->col.y - fr->row.y * fr->col.x};
        CT_ASSERT_EQ(cross.x, fr->normal.x);
        CT_ASSERT_EQ(cross.y, fr->normal.y);
        CT_ASSERT_EQ(cross.z, fr->normal.z);
    }
}

CT_SUITE(cube_world_miss) {
    ct_cell miss = ct_world_sticker((ct_v3){0, 0, 0}, (ct_v3){2, 0, 0}, 3);
    CT_ASSERT_EQ(miss.face, -1);
    CT_ASSERT_EQ(miss.row, -1);
    CT_ASSERT_EQ(miss.col, -1);
}

CT_SUITE(cube_face_self_rotation) {
    ct_cube c;
    ct_cube_init(&c, 3);
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            c.facelet[(CT_FACE_R * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col] =
                (uint8_t)(0x10 + row * 3 + col);
        }
    }
    ct_move r = {CT_FACE_R, 1, 1, 1};
    ct_apply(&c, r);
    bool changed = false;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            if (at(&c, CT_FACE_R, row, col) != (uint8_t)(0x10 + row * 3 + col))
                changed = true;
        }
    }
    CT_ASSERT(changed);

    ct_cube d;
    ct_cube_init(&d, 3);
    d.facelet[(CT_FACE_F * CT_CUBE_STRIDE + 0) * CT_CUBE_STRIDE + 2] = CT_FACE_D;
    ct_apply(&d, r);
    CT_ASSERT_EQ(at(&d, CT_FACE_U, 0, 2), CT_FACE_D);
}

CT_SUITE(cube_solved_edges) {
    ct_cube c;
    ct_cube_init(&c, 3);
    c.facelet[(0 * CT_CUBE_STRIDE + 0) * CT_CUBE_STRIDE + 1] = CT_FACE_F;
    CT_ASSERT(!ct_cube_solved(&c));
    ct_cube d;
    ct_cube_init(&d, 3);
    d.facelet[(0 * CT_CUBE_STRIDE + 1) * CT_CUBE_STRIDE + 0] = CT_FACE_F;
    CT_ASSERT(!ct_cube_solved(&d));

    ct_cube two;
    ct_cube_init(&two, 2);
    ct_cube three;
    ct_cube_init(&three, 3);
    CT_ASSERT(!ct_cube_equal(&two, &three));
    CT_ASSERT(ct_cube_equal(&two, &two));
}

CT_SUITE(cube_slab_layers) {
    int n = 5;
    ct_move w2 = {CT_FACE_F, 1, 2, 1};
    ct_cube a;
    ct_cube_init(&a, n);
    ct_apply(&a, w2);
    bool row4_moved = false;
    bool row3_moved = false;
    for (int col = 0; col < n; col++) {
        if (at(&a, CT_FACE_U, 4, col) != CT_FACE_U)
            row4_moved = true;
        if (at(&a, CT_FACE_U, 3, col) != CT_FACE_U)
            row3_moved = true;
    }
    CT_ASSERT(row4_moved);
    CT_ASSERT(row3_moved);

    ct_move w3 = {CT_FACE_F, 1, 3, 1};
    ct_cube b;
    ct_cube_init(&b, n);
    ct_apply(&b, w3);
    CT_ASSERT(row_all(&b, CT_FACE_U, 1, CT_FACE_U));

    ct_move mid = {CT_FACE_F, 3, 4, 1};
    ct_cube m;
    ct_cube_init(&m, n);
    ct_apply(&m, mid);
    CT_ASSERT(!row_all(&m, CT_FACE_U, 1, CT_FACE_U));
    CT_ASSERT(!row_all(&m, CT_FACE_U, 2, CT_FACE_U));
    CT_ASSERT(row_all(&m, CT_FACE_U, 3, CT_FACE_U));
}

CT_SUITE(cube_move_legal) {
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        ct_move ok = {CT_FACE_U, 1, n, 1};
        CT_ASSERT(ct_move_legal(&ok, n));
        ct_move wide = {CT_FACE_D, 1, n, CT_MOVE_QUARTERS - 1};
        CT_ASSERT(ct_move_legal(&wide, n));
    }
    int n = 3;
    ct_move depth_zero = {CT_FACE_U, 0, 1, 1};
    CT_ASSERT(!ct_move_legal(&depth_zero, n));
    ct_move depth_neg = {CT_FACE_U, -1, 1, 1};
    CT_ASSERT(!ct_move_legal(&depth_neg, n));
    ct_move depth_beyond = {CT_FACE_U, 1, n + 1, 1};
    CT_ASSERT(!ct_move_legal(&depth_beyond, n));
    ct_move depth_to_below = {CT_FACE_U, 2, 1, 1};
    CT_ASSERT(!ct_move_legal(&depth_to_below, n));
    ct_move quarter_zero = {CT_FACE_U, 1, 1, 0};
    CT_ASSERT(!ct_move_legal(&quarter_zero, n));
    ct_move quarter_neg = {CT_FACE_U, 1, 1, -1};
    CT_ASSERT(!ct_move_legal(&quarter_neg, n));
    ct_move quarter_four = {CT_FACE_U, 1, 1, CT_MOVE_QUARTERS};
    CT_ASSERT(!ct_move_legal(&quarter_four, n));
    ct_move face_below = {CT_FACE_U - 1, 1, 1, 1};
    CT_ASSERT(!ct_move_legal(&face_below, n));
    ct_move face_above = {CT_FACE_D + 1, 1, 1, 1};
    CT_ASSERT(!ct_move_legal(&face_above, n));
    ct_move ok3 = {CT_FACE_R, 1, 1, 2};
    CT_ASSERT(!ct_move_legal(&ok3, 1));
    CT_ASSERT(!ct_move_legal(&ok3, 0));
    CT_ASSERT(!ct_move_legal(&ok3, -3));
    CT_ASSERT(!ct_move_legal(&ok3, CT_CUBE_MAX_N + 1));
    CT_ASSERT(!ct_move_legal(NULL, n));
}

CT_SUITE(cube_init) {
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        ct_cube c;
        ct_cube_init(&c, n);
        CT_ASSERT_EQ(c.n, n);
        CT_ASSERT(ct_cube_solved(&c));
        bool clean = true;
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
            for (int row = 0; row < n; row++) {
                for (int col = 0; col < n; col++) {
                    if (at(&c, face, row, col) != face)
                        clean = false;
                }
            }
        }
        CT_ASSERT(clean);
        ct_cube fresh;
        ct_cube_init(&fresh, n);
        CT_ASSERT(ct_cube_equal(&c, &fresh));
    }
}

CT_SUITE(cube_frame_convention) {
    ct_v3 center;
    ct_v3 normal;
    ct_sticker_world(3, CT_FACE_U, 0, 0, &center, &normal);
    CT_ASSERT_EQ(normal.x, 0);
    CT_ASSERT_EQ(normal.y, 1);
    CT_ASSERT_EQ(normal.z, 0);
    CT_ASSERT_EQ(center.x, -2);
    CT_ASSERT_EQ(center.y, 2);
    CT_ASSERT_EQ(center.z, -2);
    ct_sticker_world(3, CT_FACE_F, 0, 0, &center, &normal);
    CT_ASSERT_EQ(normal.x, 0);
    CT_ASSERT_EQ(normal.y, 0);
    CT_ASSERT_EQ(normal.z, 1);
    CT_ASSERT_EQ(center.x, -2);
    CT_ASSERT_EQ(center.y, 2);
    CT_ASSERT_EQ(center.z, 2);
}

CT_SUITE(cube_round_trip) {
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
            for (int row = 0; row < n; row++) {
                for (int col = 0; col < n; col++) {
                    ct_v3 center;
                    ct_v3 normal;
                    ct_sticker_world(n, face, row, col, &center, &normal);
                    ct_cell back = ct_world_sticker(center, normal, n);
                    CT_ASSERT_EQ(back.face, face);
                    CT_ASSERT_EQ(back.row, row);
                    CT_ASSERT_EQ(back.col, col);
                }
            }
        }
    }
}

CT_SUITE(cube_turn_cycle) {
    for (size_t i = 0; i < sizeof ct_cycle_ns / sizeof ct_cycle_ns[0]; i++) {
        int n = ct_cycle_ns[i];
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
            int depths[2][2] = {{1, 1}, {1, n}};
            for (int d = 0; d < 2; d++) {
                ct_move q1 = {face, depths[d][0], depths[d][1], 1};
                ct_move q2 = {face, depths[d][0], depths[d][1], 2};
                ct_move q3 = {face, depths[d][0], depths[d][1], 3};
                ct_cube c;
                ct_cube_init(&c, n);
                for (int k = 0; k < 4; k++)
                    ct_apply(&c, q1);
                CT_ASSERT(ct_cube_solved(&c));
                for (int k = 0; k < 2; k++)
                    ct_apply(&c, q2);
                CT_ASSERT(ct_cube_solved(&c));
                ct_cube once;
                ct_cube_init(&once, n);
                ct_apply(&once, q3);
                ct_cube thrice;
                ct_cube_init(&thrice, n);
                for (int k = 0; k < 3; k++)
                    ct_apply(&thrice, q1);
                CT_ASSERT(ct_cube_equal(&once, &thrice));
                ct_cube undo = once;
                ct_apply(&undo, ct_move_inverse(q3));
                CT_ASSERT(ct_cube_solved(&undo));
                ct_cube undo2;
                ct_cube_init(&undo2, n);
                ct_apply(&undo2, q1);
                ct_apply(&undo2, ct_move_inverse(q1));
                ct_apply(&undo2, q2);
                ct_apply(&undo2, ct_move_inverse(q2));
                CT_ASSERT(ct_cube_solved(&undo2));
            }
        }
    }
}

CT_SUITE(cube_wide) {
    int n = 5;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        ct_move half = {face, 1, 2, 2};
        ct_cube c;
        ct_cube_init(&c, n);
        ct_apply(&c, half);
        ct_apply(&c, half);
        CT_ASSERT(ct_cube_solved(&c));

        ct_move wide = {face, 1, 2, 1};
        ct_move inner = {face, 1, 1, 1};
        ct_move outer = {face, 2, 2, 1};
        ct_cube w;
        ct_cube_init(&w, n);
        ct_apply(&w, wide);
        ct_cube inner_then_outer;
        ct_cube_init(&inner_then_outer, n);
        ct_apply(&inner_then_outer, inner);
        ct_apply(&inner_then_outer, outer);
        CT_ASSERT(ct_cube_equal(&w, &inner_then_outer));
        ct_cube outer_then_inner;
        ct_cube_init(&outer_then_inner, n);
        ct_apply(&outer_then_inner, outer);
        ct_apply(&outer_then_inner, inner);
        CT_ASSERT(ct_cube_equal(&w, &outer_then_inner));
    }
}

CT_SUITE(cube_u_grid) {
    int n = 3;
    ct_cube c;
    ct_cube_init(&c, n);
    ct_move r = {CT_FACE_R, 1, 1, 1};
    ct_apply(&c, r);
    uint8_t grid[CT_CUBE_STRIDE][CT_CUBE_STRIDE];
    for (int row = 0; row < n; row++) {
        for (int col = 0; col < n; col++) {
            grid[row][col] = at(&c, CT_FACE_U, row, col);
        }
    }
    bool mixed = false;
    for (int row = 0; row < n; row++) {
        for (int col = 0; col < n; col++) {
            if (grid[row][col] != CT_FACE_U)
                mixed = true;
        }
    }
    CT_ASSERT(mixed);
    ct_move u = {CT_FACE_U, 1, 1, 1};
    ct_apply(&c, u);
    bool rotated = true;
    for (int row = 0; row < n; row++) {
        for (int col = 0; col < n; col++) {
            if (at(&c, CT_FACE_U, row, col) != grid[n - 1 - col][row])
                rotated = false;
        }
    }
    CT_ASSERT(rotated);
    CT_ASSERT_EQ(at(&c, CT_FACE_U, 0, n - 1), grid[0][0]);
}

CT_SUITE(cube_u_strips) {
    int n = 3;
    ct_cube c;
    ct_cube_init(&c, n);
    ct_move u = {CT_FACE_U, 1, 1, 1};
    ct_apply(&c, u);
    CT_ASSERT(row_all(&c, CT_FACE_F, 0, CT_FACE_R));
    CT_ASSERT(row_all(&c, CT_FACE_L, 0, CT_FACE_F));
    CT_ASSERT(row_all(&c, CT_FACE_B, 0, CT_FACE_L));
    CT_ASSERT(row_all(&c, CT_FACE_R, 0, CT_FACE_B));
    CT_ASSERT(!ct_cube_solved(&c));
    CT_ASSERT(colors_valid(&c));
}

CT_SUITE(cube_f_strips) {
    int n = 3;
    ct_cube c;
    ct_cube_init(&c, n);
    ct_move f = {CT_FACE_F, 1, 1, 1};
    ct_apply(&c, f);
    CT_ASSERT(col_all(&c, CT_FACE_R, 0, CT_FACE_U));
    CT_ASSERT(col_all(&c, CT_FACE_L, n - 1, CT_FACE_D));
    CT_ASSERT(row_all(&c, CT_FACE_U, n - 1, CT_FACE_L));
    CT_ASSERT(row_all(&c, CT_FACE_D, 0, CT_FACE_R));
    CT_ASSERT(!ct_cube_solved(&c));
    CT_ASSERT(colors_valid(&c));
}

CT_SUITE(cube_move_invariants) {
    for (size_t i = 0; i < sizeof ct_invariant_ns / sizeof ct_invariant_ns[0]; i++) {
        int n = ct_invariant_ns[i];
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
            for (int quarter = 1; quarter <= 3; quarter++) {
                for (int d1 = 1; d1 <= n; d1++) {
                    for (int d2 = d1; d2 <= n; d2++) {
                        ct_cube c;
                        ct_cube_init(&c, n);
                        ct_move m = {face, d1, d2, quarter};
                        ct_apply(&c, m);
                        CT_ASSERT(colors_valid(&c));
                    }
                }
            }
        }
    }
}

CT_SUITE(cube_scramble_inverse) {
    for (size_t i = 0; i < sizeof ct_scramble_ns / sizeof ct_scramble_ns[0]; i++) {
        int n = ct_scramble_ns[i];
        ct_cube c;
        ct_cube_init(&c, n);
        ct_sfc64 s;
        ct_sfc64_init(&s, 0xC0FFEEu + (uint64_t)n);
        ct_move moves[CT_SCRAMBLE_LEN];
        for (int k = 0; k < CT_SCRAMBLE_LEN; k++) {
            moves[k].face = (int)ct_sfc64_range(&s, CT_CUBE_FACE_COUNT);
            moves[k].depth_from = 1 + (int)ct_sfc64_range(&s, (uint64_t)n);
            moves[k].depth_to = moves[k].depth_from +
                                (int)ct_sfc64_range(&s, (uint64_t)(n - moves[k].depth_from + 1));
            moves[k].quarter = 1 + (int)ct_sfc64_range(&s, 3);
            ct_apply(&c, moves[k]);
            CT_ASSERT(colors_valid(&c));
        }
        for (int k = CT_SCRAMBLE_LEN - 1; k >= 0; k--) {
            ct_apply(&c, ct_move_inverse(moves[k]));
        }
        CT_ASSERT(ct_cube_solved(&c));
        ct_cube fresh;
        ct_cube_init(&fresh, n);
        CT_ASSERT(ct_cube_equal(&c, &fresh));
    }
}
