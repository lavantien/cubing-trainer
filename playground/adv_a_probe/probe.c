#include "engine/cube.h"
#include "gen/interchange.h"
#include "gen/targets.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(bool ok, const char *what) {
    if (!ok) {
        failures++;
        printf("FAIL %s\n", what);
    }
}

static const int corner_rc[4][2] = {{0, 0}, {0, 2}, {2, 2}, {2, 0}};
static const int edge_rc[4][2] = {{0, 1}, {1, 2}, {2, 1}, {1, 0}};

static int my_raw(int face, int row, int col) {
    return face * 49 + row * 7 + col;
}

static int my_compact(int n, int face, int row, int col) {
    return face * n * n + row * n + col;
}

static int my_cell(int orbit, int sticker, int n) {
    int face = sticker / 4;
    int pos = sticker % 4;
    const int *rc = orbit == 0 ? corner_rc[pos] : edge_rc[pos];
    return my_compact(n, face, rc[0], rc[1]);
}

static void my_perm(ct_move m, int n, uint8_t *perm) {
    memset(perm, 0xFF, 294);
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = n;
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < n; r++) {
            for (int q = 0; q < n; q++) {
                c.facelet[my_raw(f, r, q)] = (uint8_t)my_compact(n, f, r, q);
            }
        }
    }
    ct_apply(&c, m);
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < n; r++) {
            for (int q = 0; q < n; q++) {
                perm[c.facelet[my_raw(f, r, q)]] = (uint8_t)my_compact(n, f, r, q);
            }
        }
    }
}

static bool my_inter(const uint8_t *perm, int a, int b) {
    return perm[a] == (uint8_t)b && perm[b] == (uint8_t)a;
}

static bool move_eq(ct_move a, ct_move b) {
    return a.face == b.face && a.depth_from == b.depth_from && a.depth_to == b.depth_to &&
           a.quarter == b.quarter;
}

static int my_candidates(int n, ct_move *out) {
    int total = 0;
    int half = (n + 1) / 2;
    for (int face = 0; face < 6; face++) {
        for (int from = 1; from <= half; from++) {
            for (int to = from; to <= half; to++) {
                for (int q = 1; q <= 3; q++) {
                    out[total++] = (ct_move){face, from, to, q};
                }
            }
        }
    }
    (void)out;
    return total;
}

static void print_pairs(const char *label, ct_move m, int n, int orbit) {
    uint8_t perm[294];
    my_perm(m, n, perm);
    printf("%s orbit%d:", label, orbit);
    for (int a = 0; a < 24; a++) {
        for (int b = a + 1; b < 24; b++) {
            if (my_inter(perm, my_cell(orbit, a, n), my_cell(orbit, b, n))) {
                printf(" (%d,%d)", a, b);
            }
        }
    }
    printf("\n");
}

static const int want_u2_corner[6][2] = {{0,2},{1,3},{4,12},{5,13},{8,16},{9,17}};
static const int want_u2_edge[4][2] = {{0,2},{1,3},{4,12},{8,16}};
static const int want_r2_corner[6][2] = {{12,14},{13,15},{1,21},{2,22},{9,19},{10,16}};
static const int want_r2_edge[4][2] = {{12,14},{13,15},{1,21},{9,19}};
static const int want_m2_edge[4][2] = {{0,20},{2,22},{8,18},{10,16}};

static bool table_match(const uint8_t *perm, int n, int orbit, const int (*tab)[2], int cnt) {
    int hits = 0;
    for (int a = 0; a < 24; a++) {
        for (int b = a + 1; b < 24; b++) {
            bool got = my_inter(perm, my_cell(orbit, a, n), my_cell(orbit, b, n));
            bool want = false;
            for (int i = 0; i < cnt; i++) {
                if ((tab[i][0] == a && tab[i][1] == b) || (tab[i][0] == b && tab[i][1] == a)) {
                    want = true;
                }
            }
            if (got != want) {
                printf("MISMATCH orbit%d (%d,%d) got=%d want=%d\n", orbit, a, b, got, want);
                return false;
            }
            if (got) {
                hits++;
            }
        }
    }
    return hits == cnt;
}

int main(void) {
    ct_move u2 = {CT_FACE_U, 1, 1, 2};
    ct_move r2 = {CT_FACE_R, 1, 1, 2};
    ct_move m2 = {CT_FACE_L, 2, 2, 2};
    uint8_t perm[294];

    print_pairs("U2", u2, 3, 0);
    print_pairs("U2", u2, 3, 1);
    print_pairs("R2", r2, 3, 0);
    print_pairs("R2", r2, 3, 1);
    print_pairs("M2", m2, 3, 0);
    print_pairs("M2", m2, 3, 1);

    my_perm(u2, 3, perm);
    check(table_match(perm, 3, 0, want_u2_corner, 6), "U2 corner table");
    check(table_match(perm, 3, 1, want_u2_edge, 4), "U2 edge table");
    my_perm(r2, 3, perm);
    check(table_match(perm, 3, 0, want_r2_corner, 6), "R2 corner table");
    check(table_match(perm, 3, 1, want_r2_edge, 4), "R2 edge table");
    my_perm(m2, 3, perm);
    check(table_match(perm, 3, 0, NULL, 0), "M2 corner empty");
    check(table_match(perm, 3, 1, want_m2_edge, 4), "M2 edge table");

    for (int n = 2; n <= 7; n++) {
        ct_move cands[200];
        int count = my_candidates(n < 7 ? n : 7, cands);
        if (n == 7) {
            count = 0;
        }
        if (n <= 6) {
            for (int i = 0; i < count; i++) {
                uint8_t api[294];
                memset(api, 0x00, sizeof api);
                int rc = ct_sticker_perm(cands[i], n, api);
                char buf[64];
                snprintf(buf, sizeof buf, "perm rc n=%d i=%d", n, i);
                check(rc == 0, buf);
                my_perm(cands[i], n, perm);
                snprintf(buf, sizeof buf, "perm match n=%d i=%d", n, i);
                check(memcmp(api, perm, 294) == 0, buf);
                bool seen[216] = {false};
                bool bij = true;
                for (int c2 = 0; c2 < 6 * n * n; c2++) {
                    if (api[c2] >= 6 * n * n || seen[api[c2]]) {
                        bij = false;
                    }
                    if (api[c2] < 6 * n * n) {
                        seen[api[c2]] = true;
                    }
                }
                for (int c2 = 0; c2 < 6 * n * n; c2++) {
                    if (!seen[c2]) {
                        bij = false;
                    }
                }
                for (int c2 = 6 * n * n; c2 < 294; c2++) {
                    if (api[c2] != 0xFF) {
                        bij = false;
                    }
                }
                snprintf(buf, sizeof buf, "perm bijection n=%d i=%d", n, i);
                check(bij, buf);
            }
        }
    }

    for (int n = 3; n <= 6; n++) {
        ct_move cands[200];
        int count = my_candidates(n, cands);
        uint8_t perms[200][294];
        for (int i = 0; i < count; i++) {
            my_perm(cands[i], n, perms[i]);
        }
        int expected_total = 6 * ((n + 1) / 2) * ((n + 1) / 2 + 1) / 2 * 3;
        char buf[64];
        snprintf(buf, sizeof buf, "candidate count n=%d", n);
        check(ct_candidate_moves(n, NULL, 0) == expected_total, buf);
        for (int orbit = 0; orbit <= 1; orbit++) {
            for (int a = 0; a < 24; a++) {
                for (int b = 0; b < 24; b++) {
                    if (a == b) {
                        continue;
                    }
                    int ca = my_cell(orbit, a, n);
                    int cb = my_cell(orbit, b, n);
                    ct_move res[200];
                    for (int i = 0; i < 200; i++) {
                        res[i] = (ct_move){-7, -7, -7, -7};
                    }
                    int api = ct_interchanges_for(orbit, a, b, n, res, 200);
                    snprintf(buf, sizeof buf, "api rc n=%d orbit=%d (%d,%d)", n, orbit, a, b);
                    check(api >= 0, buf);
                    int guards_ok = 1;
                    if (api < 200) {
                        for (int i = api; i < 200; i++) {
                            if (res[i].face != -7) {
                                guards_ok = 0;
                            }
                        }
                    }
                    snprintf(buf, sizeof buf, "api guards n=%d orbit=%d (%d,%d)", n, orbit, a, b);
                    check(guards_ok, buf);
                    for (int i = 0; i < count; i++) {
                        bool in_api = false;
                        for (int j = 0; j < api; j++) {
                            if (move_eq(res[j], cands[i])) {
                                in_api = true;
                            }
                        }
                        if (my_inter(perms[i], ca, cb) != in_api) {
                            printf("FAIL completeness n=%d orbit=%d (%d,%d) move f%d d%d-%d q%d\n",
                                   n, orbit, a, b, cands[i].face, cands[i].depth_from,
                                   cands[i].depth_to, cands[i].quarter);
                            failures++;
                            i = count;
                            break;
                        }
                    }
                }
            }
        }
    }

    {
        ct_move res[4];
        for (int i = 0; i < 4; i++) {
            res[i] = (ct_move){-7, -7, -7, -7};
        }
        int c1 = ct_interchanges_for(0, 0, 2, 3, res, 1);
        check(c1 == 2, "cap1 count");
        check(res[0].face == CT_FACE_U && res[0].depth_from == 1 && res[0].depth_to == 1 &&
                  res[0].quarter == 2, "cap1 first");
        check(res[1].face == -7, "cap1 guard");
        check(ct_interchanges_for(0, 0, 2, 3, NULL, -9) == 2, "null negative cap");
        int caps[] = {-100, -1, 0, 1, 2, 3, 99};
        for (unsigned ci = 0; ci < sizeof caps / sizeof caps[0]; ci++) {
            ct_move buf[200];
            for (int i = 0; i < 200; i++) {
                buf[i] = (ct_move){-7, -7, -7, -7};
            }
            int got = ct_interchanges_for(0, 0, 2, 3, buf, caps[ci]);
            check(got == 2, "cap sweep count");
            int lim = caps[ci] < 0 ? 0 : caps[ci];
            if (lim > 200) {
                lim = 200;
            }
            bool ok = true;
            for (int i = lim; i < 200; i++) {
                if (buf[i].face != -7) {
                    ok = false;
                }
            }
            check(ok, "cap sweep guards");
        }
    }

    {
        ct_move big[185];
        for (int i = 0; i < 185; i++) {
            big[i] = (ct_move){-7, -7, -7, -7};
        }
        int got = ct_candidate_moves(7, big, 180);
        check(got == 180, "candidate n7 count");
        bool ok = big[179].face == 5 && big[179].depth_from == 4 && big[179].depth_to == 4 &&
                  big[179].quarter == 3;
        check(ok, "candidate n7 last");
        check(big[180].face == -7, "candidate n7 guard");
        check(ct_candidate_moves(2, NULL, 0) == 18, "candidate n2");
    }

    {
        uint8_t p[294];
        check(ct_sticker_perm((ct_move){0, 1, 1, 1}, 7, p) < 0, "perm n7 reject");
        check(ct_sticker_perm((ct_move){0, 1, 1, 1}, 1, p) < 0, "perm n1 reject");
        check(ct_sticker_perm((ct_move){0, 3, 3, 1}, 3, p) == 0, "perm deep slab ok");
        check(ct_sticker_perm((ct_move){0, 1, 4, 1}, 3, p) < 0, "perm depth reject");
        check(ct_sticker_perm((ct_move){5, 1, 1, 2}, 2, p) == 0, "perm n2 ok");
    }

    if (failures == 0) {
        printf("PROBE ALL OK\n");
    } else {
        printf("PROBE FAILURES=%d\n", failures);
    }
    return failures != 0;
}
