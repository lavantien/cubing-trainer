#include <stdio.h>
#include <string.h>

#include "engine/cube.h"
#include "engine/notation.h"
#include "engine/transform.h"

static int failures = 0;

static void failf(const char *fmt, int a, int b) {
    printf("FAIL ");
    printf(fmt, a, b);
    printf("\n");
    failures++;
}

enum { U = CT_FACE_U, L = CT_FACE_L, F = CT_FACE_F, R = CT_FACE_R, B = CT_FACE_B, D = CT_FACE_D };

static const int my_row_dir[6][3] = {
    {0, 0, 1}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, 0, -1},
};
static const int my_col_dir[6][3] = {
    {1, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0},
};
static const int my_normal[6][3] = {
    {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 0, -1}, {0, -1, 0},
};

static void my_world(int n, int face, int row, int col, int *center, int *normal) {
    int h = n - 1;
    for (int k = 0; k < 3; k++) {
        center[k] = my_normal[face][k] * h + my_row_dir[face][k] * (2 * row - h) +
                    my_col_dir[face][k] * (2 * col - h);
        normal[k] = my_normal[face][k];
    }
}

static int my_find_cell(int n, const int *center, const int *normal, int *face, int *row, int *col) {
    for (int f = 0; f < 6; f++) {
        if (normal[0] != my_normal[f][0] || normal[1] != my_normal[f][1] || normal[2] != my_normal[f][2]) {
            continue;
        }
        int h = n - 1;
        int along_row = 0;
        int along_col = 0;
        for (int k = 0; k < 3; k++) {
            along_row += center[k] * my_row_dir[f][k];
            along_col += center[k] * my_col_dir[f][k];
        }
        *face = f;
        *row = (along_row + h) / 2;
        *col = (along_col + h) / 2;
        return 1;
    }
    return 0;
}

static void my_rot90(int *v, int axis) {
    int x = v[0];
    int y = v[1];
    int z = v[2];
    if (axis == 0) {
        v[0] = x;
        v[1] = -z;
        v[2] = y;
    } else if (axis == 1) {
        v[0] = z;
        v[1] = y;
        v[2] = -x;
    } else {
        v[0] = -y;
        v[1] = x;
        v[2] = z;
    }
}

static int my_axis_of(int face) {
    const int *nrm = my_normal[face];
    if (nrm[0] != 0)
        return 0;
    if (nrm[1] != 0)
        return 1;
    return 2;
}

static int my_sign_of(int face) {
    const int *nrm = my_normal[face];
    if (nrm[0] != 0)
        return nrm[0];
    if (nrm[1] != 0)
        return nrm[1];
    return nrm[2];
}

static void my_apply(ct_cube *c, ct_move m) {
    ct_cube src = *c;
    int axis = my_axis_of(m.face);
    int sign = my_sign_of(m.face);
    int h = c->n - 1;
    int lo = h - 2 * (m.depth_to - 1);
    int hi = h - 2 * (m.depth_from - 1);
    int steps = ((-sign * m.quarter) % 4 + 4) % 4;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < c->n; row++) {
            for (int col = 0; col < c->n; col++) {
                int center[3];
                int normal[3];
                my_world(c->n, face, row, col, center, normal);
                int u = 0;
                for (int k = 0; k < 3; k++)
                    u += center[k] * my_normal[m.face][k];
                int rot = (u >= lo && u <= hi) ? steps : 0;
                for (int k = 0; k < rot; k++) {
                    my_rot90(center, axis);
                    my_rot90(normal, axis);
                }
                int df;
                int dr;
                int dc;
                if (!my_find_cell(c->n, center, normal, &df, &dr, &dc)) {
                    printf("FAIL my_apply could not map cell\n");
                    failures++;
                    continue;
                }
                c->facelet[df * CT_CUBE_STRIDE * CT_CUBE_STRIDE + dr * CT_CUBE_STRIDE + dc] =
                    src.facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col];
            }
        }
    }
}

static void check_my_apply_matches(const char *label, ct_move m, int n) {
    ct_cube a;
    ct_cube b;
    ct_cube_init(&a, n);
    ct_cube_init(&b, n);
    for (int face = 0; face < 6; face++)
        for (int i = 0; i < CT_CUBE_STRIDE * CT_CUBE_STRIDE; i++)
            a.facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + i] =
                b.facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + i] =
                    (uint8_t)(face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + i);
    ct_apply(&a, m);
    my_apply(&b, m);
    if (!ct_cube_equal(&a, &b)) {
        printf("FAIL %s n=%d face=%d span=%d..%d q=%d diverges from independent model\n", label, n,
               m.face, m.depth_from, m.depth_to, m.quarter);
        failures++;
    }
}

static ct_move mv(int face, int from, int to, int quarter) {
    ct_move m = {face, from, to, quarter};
    return m;
}

static void check_true(const char *label, int cond, int a, int b) {
    if (!cond)
        failf(label, a, b);
}

static uint8_t at(const ct_cube *c, int face, int row, int col) {
    return c->facelet[(face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col];
}

struct corner_expect {
    int face;
    int row;
    int col;
    int dest_face;
    int dest_row;
    int dest_col;
};

static void check_corner_cycle(const char *label, int n, int face, const struct corner_expect *table,
                               int count) {
    for (int k = 0; k < count; k++) {
        ct_cube c;
        ct_cube_init(&c, n);
        memset(c.facelet, 100, sizeof c.facelet);
        c.facelet[table[k].face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + table[k].row * CT_CUBE_STRIDE +
                  table[k].col] = 201;
        ct_apply(&c, mv(face, 1, 1, 1));
        int hits = 0;
        int hit_face = -1;
        int hit_row = -1;
        int hit_col = -1;
        for (int f = 0; f < 6; f++) {
            for (int r = 0; r < n; r++) {
                for (int cc = 0; cc < n; cc++) {
                    if (at(&c, f, r, cc) == 201) {
                        hits++;
                        hit_face = f;
                        hit_row = r;
                        hit_col = cc;
                    }
                }
            }
        }
        if (hits != 1 || hit_face != table[k].dest_face || hit_row != table[k].dest_row ||
            hit_col != table[k].dest_col) {
            printf("FAIL %s sticker %d,%d,%d expected at %d,%d,%d got hits=%d at %d,%d,%d\n", label,
                   table[k].face, table[k].row, table[k].col, table[k].dest_face, table[k].dest_row,
                   table[k].dest_col, hits, hit_face, hit_row, hit_col);
            failures++;
        }
    }
}

static void run_alg(ct_cube *c, const char *alg) {
    ct_move moves[64];
    int count = ct_notation_parse(alg, c->n, moves, 64);
    if (count < 0) {
        printf("FAIL alg parse '%s' rc=%d\n", alg, count);
        failures++;
        return;
    }
    for (int k = 0; k < count; k++)
        ct_apply(c, moves[k]);
}

int main(void) {
    printf("== 1. independent geometric model equivalence, all n, faces, spans, quarters ==\n");
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        for (int face = 0; face < 6; face++) {
            for (int from = 1; from <= n; from++) {
                for (int to = from; to <= n; to++) {
                    for (int q = 1; q <= 3; q++) {
                        check_my_apply_matches("full sweep", mv(face, from, to, q), n);
                        check_my_apply_matches("full sweep inverse", mv(face, from, to, 4 - q), n);
                    }
                }
            }
        }
    }
    printf("sweep done, failures so far: %d\n", failures);

    printf("== 2. corner and edge strip cycles from first principles (n=3, quarter turns) ==\n");
    struct corner_expect r_cycle[] = {
        {U, 0, 2, B, 2, 0}, {F, 0, 2, U, 0, 2}, {F, 2, 2, U, 2, 2}, {D, 0, 2, F, 0, 2},
        {B, 2, 0, D, 0, 2},
    };
    check_corner_cycle("R", 3, R, r_cycle, 5);
    struct corner_expect l_cycle[] = {
        {U, 0, 0, F, 0, 0}, {U, 2, 0, F, 2, 0}, {B, 0, 2, U, 2, 0}, {D, 2, 0, B, 0, 2},
        {F, 2, 0, D, 2, 0},
    };
    check_corner_cycle("L", 3, L, l_cycle, 5);
    struct corner_expect d_cycle[] = {
        {F, 2, 0, R, 2, 0}, {F, 2, 2, R, 2, 2}, {R, 2, 2, B, 2, 2}, {B, 2, 2, L, 2, 2},
        {L, 2, 2, F, 2, 2},
    };
    check_corner_cycle("D", 3, D, d_cycle, 5);
    struct corner_expect b_cycle[] = {
        {U, 0, 0, L, 2, 0}, {U, 0, 2, L, 0, 0}, {R, 0, 2, U, 0, 0}, {D, 2, 2, R, 0, 2},
        {L, 0, 0, D, 2, 0},
    };
    check_corner_cycle("B", 3, B, b_cycle, 5);
    struct corner_expect u_cycle[] = {
        {F, 0, 0, L, 0, 0}, {F, 0, 2, L, 0, 2}, {R, 0, 0, F, 0, 0}, {B, 0, 0, R, 0, 0},
        {L, 0, 0, B, 0, 0}, {L, 0, 2, B, 0, 2},
    };
    check_corner_cycle("U", 3, U, u_cycle, 6);
    struct corner_expect f_cycle[] = {
        {U, 2, 0, R, 0, 0}, {U, 2, 2, R, 2, 0}, {L, 0, 2, U, 2, 2}, {D, 0, 2, L, 2, 2},
        {R, 0, 0, D, 0, 2},
    };
    check_corner_cycle("F", 3, F, f_cycle, 5);

    printf("== 3. group identities from cubing knowledge ==\n");
    for (int rep = 0; rep < 2; rep++) {
        ct_cube c;
        ct_cube_init(&c, 3);
        for (int k = 0; k < 105; k++)
            run_alg(&c, "R U");
        check_true("(R U)^105 == id rep=%d", ct_cube_solved(&c), rep, 0);
        ct_cube_init(&c, 3);
        for (int k = 0; k < 6; k++)
            run_alg(&c, "R U R' U'");
        check_true("(R U R' U')^6 == id rep=%d", ct_cube_solved(&c), rep, 0);
        ct_cube_init(&c, 3);
        for (int k = 0; k < 6; k++)
            run_alg(&c, "R U R' U'");
        ct_cube d;
        ct_cube_init(&d, 3);
        for (int k = 0; k < 5; k++)
            run_alg(&d, "R U R' U'");
        check_true("sexy 5 != id rep=%d", !ct_cube_equal(&c, &d), rep, 0);
        ct_cube_init(&c, 3);
        run_alg(&c, "M E S M E S M E S M E S M E S M E S M E S M E S");
        check_true("(M E S)^12 == id rep=%d", ct_cube_solved(&c), rep, 0);
        ct_cube_init(&c, 3);
        run_alg(&c, "x x x x");
        check_true("x^4 == id rep=%d", ct_cube_solved(&c), rep, 0);
        ct_cube scrambled;
        ct_cube_init(&scrambled, 3);
        run_alg(&scrambled, "F B' R L' U D' F B' R L' U D'");
        ct_cube rot = scrambled;
        ct_cube via_y = scrambled;
        run_alg(&rot, "y");
        run_alg(&via_y, "z x z'");
        check_true("y == z x z' rep=%d", ct_cube_equal(&rot, &via_y), rep, 0);
        via_y = scrambled;
        run_alg(&via_y, "z x' z'");
        ct_cube yinv = scrambled;
        run_alg(&yinv, "y'");
        check_true("y' == z x' z' rep=%d", ct_cube_equal(&yinv, &via_y), rep, 0);
        check_true("y != z x' z' rep=%d", !ct_cube_equal(&rot, &via_y), rep, 0);
        via_y = scrambled;
        run_alg(&via_y, "x z");
        check_true("y != x z rep=%d", !ct_cube_equal(&rot, &via_y), rep, 0);
        ct_cube xy = scrambled;
        run_alg(&xy, "x y x y x y");
        check_true("(x y)^3 == id rep=%d", ct_cube_equal(&xy, &scrambled), rep, 0);
    }

    printf("== 4. wide and inner algebra, all n ==\n");
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        for (int face = 0; face < 6; face++) {
            for (int q = 1; q <= 3; q++) {
                for (int d = 2; d <= n; d++) {
                    ct_cube a;
                    ct_cube_init(&a, n);
                    ct_apply(&a, mv(face, 1, d, q));
                    ct_apply(&a, mv(face, 1, d - 1, 4 - q));
                    ct_cube b;
                    ct_cube_init(&b, n);
                    ct_apply(&b, mv(face, d, d, q));
                    char label[64];
                    snprintf(label, sizeof label, "wide-to-%d then wide-to-%d inverse == single d n=%%d", d, d - 1);
                    check_true(label, ct_cube_equal(&a, &b), n, face);
                }
            }
        }
        for (int face = 0; face < 6; face++) {
            for (int q = 1; q <= 3; q++) {
                ct_cube a;
                ct_cube_init(&a, n);
                ct_apply(&a, mv(face, 1, n, q));
                ct_cube b;
                ct_cube_init(&b, n);
                for (int d = 1; d <= n; d++)
                    ct_apply(&b, mv(face, d, d, q));
                char label[64];
                snprintf(label, sizeof label, "rotation == product of single layers n=%%d face=%%d");
                check_true(label, ct_cube_equal(&a, &b), n, face);
            }
        }
    }

    printf("== 5. scramble + inverse round trips with fresh seeds ==\n");
    {
        unsigned long long s = 88172645463325252ULL;
        for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
            ct_move moves[97];
            ct_cube c;
            ct_cube_init(&c, n);
            for (int k = 0; k < 96; k++) {
                s ^= s << 13;
                s ^= s >> 7;
                s ^= s << 17;
                moves[k].face = (int)(s % 6);
                moves[k].depth_from = (int)(s % (unsigned long long)n) + 1;
                moves[k].depth_to =
                    moves[k].depth_from + (int)((s >> 8) % (unsigned long long)(n - moves[k].depth_from + 1));
                moves[k].quarter = (int)((s >> 16) % 3) + 1;
                ct_apply(&c, moves[k]);
                for (int face = 0; face < 6; face++) {
                    for (int i = 0; i < n; i++) {
                        for (int j = 0; j < n; j++) {
                            if (at(&c, face, i, j) >= 6) {
                                failf("facelet out of range n=%d", n, 0);
                            }
                        }
                    }
                }
            }
            for (int k = 95; k >= 0; k--)
                ct_apply(&c, ct_move_inverse(moves[k]));
            check_true("xorshift96 scramble+inverse n=%d", ct_cube_solved(&c), n, 0);
        }
    }

    printf("== 6. solved-state stickers untouched outside slab ==\n");
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        for (int face = 0; face < 6; face++) {
            for (int d = 2; d < n; d++) {
                ct_cube c;
                ct_cube_init(&c, n);
                ct_apply(&c, mv(face, d, d, 1));
                int face_rows_untouched = 1;
                for (int row = 0; row < n; row++)
                    for (int col = 0; col < n; col++)
                        if (at(&c, face, row, col) != face)
                            face_rows_untouched = 0;
                check_true("inner move leaves its own face intact n=%d", face_rows_untouched, n, face);
                int opposite_untouched = 1;
                int opp = -1;
                for (int k = 0; k < 6; k++) {
                    if (my_normal[k][0] == -my_normal[face][0] && my_normal[k][1] == -my_normal[face][1] &&
                        my_normal[k][2] == -my_normal[face][2])
                        opp = k;
                }
                if (d < n) {
                    for (int row = 0; row < n; row++)
                        for (int col = 0; col < n; col++)
                            if (at(&c, opp, row, col) != opp)
                                opposite_untouched = 0;
                    check_true("inner move leaves opposite face intact n=%d", opposite_untouched, n, face);
                }
            }
        }
    }

    printf("total failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
