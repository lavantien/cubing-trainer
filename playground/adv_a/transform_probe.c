#include <stdio.h>
#include <string.h>

#include "engine/cube.h"
#include "engine/notation.h"
#include "engine/transform.h"

static int failures = 0;
static int checks = 0;

static void check(const char *label, int cond) {
    checks++;
    if (!cond) {
        printf("FAIL %s\n", label);
        failures++;
    }
}

static void fmt_move(const ct_move *m, int n, char *buf, size_t cap) {
    int len = ct_notation_format_one(m, n, buf, (unsigned)cap);
    if (len < 0)
        snprintf(buf, cap, "?%d", len);
}

static void apply_seq(ct_cube *c, const ct_move *m, int count) {
    for (int i = 0; i < count; i++)
        ct_apply(c, m[i]);
}

static bool seq_same(const ct_move *a, const ct_move *b, int count) {
    return memcmp(a, b, (size_t)count * sizeof(ct_move)) == 0;
}

static unsigned long long rng = 0x9E3779B97F4A7C15ULL;

static unsigned long long next_rng(void) {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static void fill_alg(int n, ct_move *alg, int len) {
    for (int i = 0; i < len; i++) {
        alg[i].face = (int)(next_rng() % 6);
        alg[i].depth_from = (int)(next_rng() % (unsigned long long)n) + 1;
        alg[i].depth_to =
            alg[i].depth_from + (int)(next_rng() % (unsigned long long)(n - alg[i].depth_from + 1));
        alg[i].quarter = (int)(next_rng() % 3) + 1;
    }
}

static void mirror_state(const ct_cube *c, ct_cube *out) {
    *out = *c;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < c->n; row++) {
            for (int col = 0; col < c->n; col++) {
                ct_cell dst;
                for (int f = 0; f < 6; f++) {
                    for (int r = 0; r < c->n; r++) {
                        for (int cc = 0; cc < c->n; cc++) {
                            ct_v3 s1;
                            ct_v3 n1;
                            ct_sticker_world(c->n, f, r, cc, &s1, &n1);
                            ct_v3 s2;
                            ct_v3 n2;
                            ct_sticker_world(c->n, face, row, col, &s2, &n2);
                            s2.x = -s2.x;
                            n2.x = -n2.x;
                            if (s1.x == s2.x && s1.y == s2.y && s1.z == s2.z && n1.x == n2.x &&
                                n1.y == n2.y && n1.z == n2.z) {
                                dst.face = f;
                                dst.row = r;
                                dst.col = cc;
                            }
                        }
                    }
                }
                out->facelet[(face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col] =
                    c->facelet[(dst.face * CT_CUBE_STRIDE + dst.row) * CT_CUBE_STRIDE + dst.col];
            }
        }
    }
}

int main(void) {
    printf("== crafted simplify cases ==\n");
    {
        struct {
            const char *alg;
            const char *want;
        } simplify_cases[] = {
            {"R R' R", "R"}, {"R R'", ""}, {"R2 R2", ""}, {"R R R R", ""},
            {"R U R' U'", "R U R' U'"}, {"Rw R2 Rw'", "Rw R2 Rw'"}, {"x x'", ""},
            {"R R' U U'", ""}, {"R2 R", "R'"}, {"R' R2", "R"},
            {"R U2 U2 R'", ""}, {"R2 U R2 U", "R2 U R2 U"}, {"R R2 R' R", "R'"},
        };
        for (size_t k = 0; k < sizeof simplify_cases / sizeof simplify_cases[0]; k++) {
            ct_move in[16];
            int count = ct_notation_parse(simplify_cases[k].alg, 3, in, 16);
            check("crafted parse", count > 0);
            ct_move outm[16];
            int m = ct_transform_simplify(in, count, outm, 16);
            char got[128];
            int pos = 0;
            for (int i = 0; i < m; i++) {
                char one[8];
                fmt_move(&outm[i], 3, one, sizeof one);
                if (i)
                    got[pos++] = ' ';
                int len = (int)strlen(one);
                memcpy(got + pos, one, (size_t)len);
                pos += len;
            }
            got[pos] = '\0';
            char label[128];
            snprintf(label, sizeof label, "simplify '%s' -> '%s' (got '%s')", simplify_cases[k].alg,
                     simplify_cases[k].want, got);
            check(label, strcmp(got, simplify_cases[k].want) == 0);
        }
    }

    printf("== simplify equivalence and fixed point, random algs, n=2..7 ==\n");
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        for (int round = 0; round < 60; round++) {
            ct_move alg[40];
            int len = (int)(next_rng() % 40) + 1;
            fill_alg(n, alg, len);
            ct_move once[44];
            int m1 = ct_transform_simplify(alg, len, once, 44);
            check("simplify rc >= 0", m1 >= 0);
            ct_move twice[44];
            int m2 = ct_transform_simplify(once, m1, twice, 44);
            check("simplify fixed point length", m2 == m1);
            check("simplify fixed point content", seq_same(once, twice, m1));
            ct_cube a;
            ct_cube_init(&a, n);
            apply_seq(&a, alg, len);
            ct_cube b;
            ct_cube_init(&b, n);
            apply_seq(&b, once, m1);
            check("simplify preserves permutation", ct_cube_equal(&a, &b));
            for (int i = 1; i < m1; i++) {
                check("simplify leaves no adjacent same-span pair",
                      once[i].face != once[i - 1].face ||
                          once[i].depth_from != once[i - 1].depth_from ||
                          once[i].depth_to != once[i - 1].depth_to);
            }
        }
    }

    printf("== inverse, mirror, rotate_y random equivalences ==\n");
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        for (int round = 0; round < 30; round++) {
            ct_move alg[40];
            int len = (int)(next_rng() % 40) + 1;
            fill_alg(n, alg, len);

            ct_move inv[40];
            check("inverse rc", ct_transform_inverse(alg, len, inv) == len);
            ct_cube a;
            ct_cube_init(&a, n);
            apply_seq(&a, alg, len);
            apply_seq(&a, inv, len);
            check("alg inverse alg == id", ct_cube_solved(&a));

            ct_move mir[40];
            check("mirror rc", ct_transform_mirror(alg, len, mir) == len);
            ct_move mir2[40];
            ct_transform_mirror(mir, len, mir2);
            check("mirror mirror == id", seq_same(alg, mir2, len));
            ct_cube st;
            ct_cube_init(&st, n);
            apply_seq(&st, alg, len);
            ct_cube want;
            mirror_state(&st, &want);
            ct_cube start;
            ct_cube_init(&start, n);
            ct_cube mstart;
            mirror_state(&start, &mstart);
            apply_seq(&mstart, mir, len);
            check("mirror(alg on solved) == mirror(alg) on mirror(solved)", ct_cube_equal(&mstart, &want));

            for (int k = 0; k < 4; k++) {
                ct_move rot[40];
                check("rotate_y rc", ct_transform_rotate_y(alg, len, k, rot) == len);
                ct_move rot4a[40];
                ct_move rot4b[40];
                ct_transform_rotate_y(alg, len, k, rot4a);
                ct_transform_rotate_y(rot4a, len, 4 - k, rot4b);
                check("rotate_y k then 4-k == id", seq_same(alg, rot4b, len));
                ct_move y = {CT_FACE_U, 1, n, k % 4};
                ct_cube left;
                ct_cube_init(&left, n);
                apply_seq(&left, alg, len);
                ct_apply(&left, y);
                ct_cube right;
                ct_cube_init(&right, n);
                ct_apply(&right, y);
                apply_seq(&right, rot, len);
                check("rotate_y conjugation equivalence", ct_cube_equal(&left, &right));
            }
        }
    }

    printf("== conjugate assembly and invalid inputs ==\n");
    {
        ct_move setup[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 2, 2}};
        ct_move inner[2] = {{CT_FACE_F, 1, 1, 3}, {CT_FACE_D, 1, 1, 1}};
        ct_move out[8];
        int total = ct_transform_conjugate(setup, 2, inner, 2, out, 8);
        check("conjugate rc", total == 6);
        ct_cube a;
        ct_cube_init(&a, 3);
        ct_move seq[6];
        memcpy(seq, setup, sizeof setup);
        memcpy(seq + 2, inner, sizeof inner);
        ct_move inv[2];
        ct_transform_inverse(setup, 2, inv);
        memcpy(seq + 4, inv, sizeof inv);
        apply_seq(&a, seq, 6);
        ct_cube b;
        ct_cube_init(&b, 3);
        apply_seq(&b, out, total);
        check("conjugate equals manual assembly", ct_cube_equal(&a, &b));
        check("conjugate cap", ct_transform_conjugate(setup, 2, inner, 2, out, 5) == -1);
        check("conjugate null setup", ct_transform_conjugate(NULL, 0, inner, 2, out, 8) == 2);
        ct_move bad = {CT_FACE_R, 1, 1, 0};
        check("inverse invalid quarter", ct_transform_inverse(&bad, 1, out) == -1);
        bad.quarter = 4;
        check("inverse quarter 4", ct_transform_inverse(&bad, 1, out) == -1);
        bad.quarter = 1;
        bad.depth_to = 0;
        check("inverse bad span", ct_transform_inverse(&bad, 1, out) == -1);
        check("inverse count -1", ct_transform_inverse(NULL, -1, out) == -1);
        check("mirror null in", ct_transform_mirror(NULL, 2, out) == -1);
        check("rotate_y null out", ct_transform_rotate_y(setup, 2, 1, NULL) == -1);
        check("simplify cap 0 with cancels", ct_transform_simplify(setup, 2, out, 0) == -1);
        ct_move cancels[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}};
        check("simplify cap 1 all cancel", ct_transform_simplify(cancels, 2, out, 1) == 0);
        ct_move peak[3] = {{CT_FACE_U, 1, 1, 1}, {CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}};
        int peak_final = ct_transform_simplify(peak, 3, out, 1);
        printf("observation: simplify(U R R', cap=1) = %d, final result is 1 move\n", peak_final);
        check("simplify cap equals final length may fail", peak_final == -1 || peak_final == 1);
    }

    printf("checks=%d failures=%d\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
