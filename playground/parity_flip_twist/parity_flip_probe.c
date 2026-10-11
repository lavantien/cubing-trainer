#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/targets.h"

enum {
    PROBE_MOVES = 64,
    PROBE_STICKERS = CT_TARGET_STICKERS,
    PROBE_EDGES = 12,
    PROBE_CORNERS = 8,
    PROBE_NAME = 8,
    PROBE_OK = 0,
    PROBE_FAIL = 1
};

static const char probe_faces[CT_CUBE_FACE_COUNT] = {'U', 'L', 'F', 'R', 'B', 'D'};

static int probe_fail = PROBE_OK;

static void probe_check(int cond, const char *what) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", what);
        probe_fail = PROBE_FAIL;
    }
}

static int probe_flat(int face, int row, int col) {
    return face * 9 + row * 3 + col;
}

static int probe_cell(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int probe_flat_of_cell(int cell) {
    return probe_flat(cell / (CT_CUBE_STRIDE * CT_CUBE_STRIDE),
                      (cell % (CT_CUBE_STRIDE * CT_CUBE_STRIDE)) / CT_CUBE_STRIDE,
                      cell % CT_CUBE_STRIDE);
}

static int probe_cell_of_sticker(int orbit, int sticker) {
    int face = 0;
    int row = 0;
    int col = 0;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return probe_cell(face, row, col);
}

static int probe_sticker_of_cell(int orbit, int cell) {
    for (int s = 0; s < PROBE_STICKERS; s++) {
        if (probe_cell_of_sticker(orbit, s) == cell)
            return s;
    }
    return -1;
}

static void probe_identity(ct_cube *c) {
    ct_cube_init(c, 3);
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++)
                c->facelet[probe_cell(face, row, col)] =
                    (uint8_t)probe_flat(face, row, col);
        }
    }
}

static int probe_cell_of_home(const ct_cube *c, int cell) {
    int want = probe_flat_of_cell(cell);
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                if (c->facelet[probe_cell(face, row, col)] == (uint8_t)want)
                    return probe_cell(face, row, col);
            }
        }
    }
    return -1;
}

static int probe_face_axis(int face) {
    if (face == CT_FACE_L || face == CT_FACE_R)
        return 0;
    if (face == CT_FACE_U || face == CT_FACE_D)
        return 1;
    return 2;
}

static char probe_axis_letter(int axis, int sign) {
    if (axis == 0)
        return sign > 0 ? 'R' : 'L';
    if (axis == 1)
        return sign > 0 ? 'U' : 'D';
    return sign > 0 ? 'F' : 'B';
}

static void probe_name(int orbit, int sticker, char *out, unsigned cap) {
    int face = 0;
    int row = 0;
    int col = 0;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    ct_v3 center;
    ct_v3 normal;
    ct_sticker_world(3, face, row, col, &center, &normal);
    int vals[3] = {center.x, center.y, center.z};
    int face_axis = probe_face_axis(face);
    char others[2];
    int n = 0;
    for (int a = 0; a < 3 && n < 2; a++) {
        if (a == face_axis || vals[a] == 0)
            continue;
        others[n++] = probe_axis_letter(a, vals[a] > 0 ? 1 : -1);
    }
    if (orbit == CT_TARGET_ORBIT_EDGE)
        snprintf(out, cap, "%c%c", probe_faces[face], others[0]);
    else
        snprintf(out, cap, "%c%c%c", probe_faces[face], others[0], others[1]);
}

static int probe_ud_sticker(int orbit, int piece, int (*piece_of)(int)) {
    for (int pass = 0; pass < 2; pass++) {
        for (int s = 0; s < PROBE_STICKERS; s++) {
            if (piece_of(s) != piece)
                continue;
            int face = 0;
            int row = 0;
            int col = 0;
            ct_sticker_cell(orbit, s, &face, &row, &col);
            if (pass == 0 && (face == CT_FACE_U || face == CT_FACE_D))
                return s;
            if (pass == 1 && (face == CT_FACE_F || face == CT_FACE_B))
                return s;
        }
    }
    for (int s = 0; s < PROBE_STICKERS; s++) {
        if (piece_of(s) == piece)
            return s;
    }
    return -1;
}

static int probe_edge_oriented(int home_sticker, int now_sticker) {
    int hf = 0;
    int hr = 0;
    int hc = 0;
    int nf = 0;
    int nr = 0;
    int nc = 0;
    ct_sticker_cell(CT_TARGET_ORBIT_EDGE, home_sticker, &hf, &hr, &hc);
    ct_sticker_cell(CT_TARGET_ORBIT_EDGE, now_sticker, &nf, &nr, &nc);
    if (hf == CT_FACE_U || hf == CT_FACE_D)
        return nf == CT_FACE_U || nf == CT_FACE_D;
    return nf == CT_FACE_F || nf == CT_FACE_B;
}

static void probe_perm(const ct_cube *c, int orbit, int *perm) {
    int (*piece_of)(int) = orbit == 0 ? ct_corner_piece_of : ct_edge_piece_of;
    int pieces = orbit == 0 ? PROBE_CORNERS : PROBE_EDGES;
    for (int p = 0; p < pieces; p++) {
        int s = probe_ud_sticker(orbit, p, piece_of);
        int cell = probe_cell_of_sticker(orbit, s);
        int now = probe_cell_of_home(c, cell);
        int ns = probe_sticker_of_cell(orbit, now);
        perm[p] = piece_of(ns);
    }
}

static void probe_describe(const ct_cube *c, const char *label) {
    printf("effect %s\n", label);
    for (int orbit = 0; orbit < 2; orbit++) {
        int (*piece_of)(int) = orbit == 0 ? ct_corner_piece_of : ct_edge_piece_of;
        int pieces = orbit == 0 ? PROBE_CORNERS : PROBE_EDGES;
        const char *kind = orbit == 0 ? "corner" : "edge";
        int perm[PROBE_EDGES];
        probe_perm(c, orbit, perm);
        int visited[PROBE_EDGES] = {0};
        for (int p = 0; p < pieces; p++) {
            if (visited[p] || perm[p] == p)
                continue;
            printf("  %s perm cycle:", kind);
            int q = p;
            do {
                char a[PROBE_NAME];
                char b[PROBE_NAME];
                probe_name(orbit, probe_ud_sticker(orbit, q, piece_of), a, sizeof a);
                probe_name(orbit, probe_ud_sticker(orbit, perm[q], piece_of), b,
                           sizeof b);
                printf(" %s->%s", a, b);
                visited[q] = 1;
                q = perm[q];
            } while (q != p);
            printf("\n");
        }
        for (int p = 0; p < pieces; p++) {
            int s = probe_ud_sticker(orbit, p, piece_of);
            int cell = probe_cell_of_sticker(orbit, s);
            int now = probe_cell_of_home(c, cell);
            int ns = probe_sticker_of_cell(orbit, now);
            char a[PROBE_NAME];
            probe_name(orbit, s, a, sizeof a);
            if (perm[p] != p) {
                char b[PROBE_NAME];
                int nf = 0;
                int nr = 0;
                int nc = 0;
                ct_sticker_cell(orbit, ns, &nf, &nr, &nc);
                probe_name(orbit, probe_ud_sticker(orbit, perm[p], piece_of), b,
                           sizeof b);
                if (orbit == CT_TARGET_ORBIT_CORNER)
                    printf("  corner %s at %s UD sticker on %c\n", a, b,
                           probe_faces[nf]);
                else
                    printf("  edge %s at %s oriented=%d\n", a, b,
                           probe_edge_oriented(s, ns));
            } else if (orbit == CT_TARGET_ORBIT_EDGE) {
                if (!probe_edge_oriented(s, ns))
                    printf("  edge flip in place: %s\n", a);
            } else {
                int nf = 0;
                int nr = 0;
                int nc = 0;
                ct_sticker_cell(orbit, ns, &nf, &nr, &nc);
                if (nf != CT_FACE_U && nf != CT_FACE_D)
                    printf("  corner twist in place: %s UD sticker now on %c\n", a,
                           probe_faces[nf]);
            }
        }
    }
    printf("  centers:");
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        int now = probe_cell_of_home(c, probe_cell(face, 1, 1));
        printf(" %c%c", probe_faces[face],
               probe_faces[now / (CT_CUBE_STRIDE * CT_CUBE_STRIDE)]);
    }
    printf("\n");
}

static void probe_strip_brackets(char *alg) {
    size_t w = 0;
    for (size_t r = 0; alg[r] != '\0'; r++) {
        if (alg[r] != '(' && alg[r] != ')' && alg[r] != ',')
            alg[w++] = alg[r];
    }
    alg[w] = '\0';
}

static void probe_expand_runs(const char *src, char *dst, unsigned cap) {
    unsigned w = 0;
    for (size_t r = 0; src[r] != '\0' && w + 4 < cap; r++) {
        char c = src[r];
        if (c == ' ' || c == '-') {
            if (w > 0 && dst[w - 1] != ' ')
                dst[w++] = ' ';
            continue;
        }
        dst[w++] = c;
        while ((src[r + 1] == '\'' || src[r + 1] == '2') && w + 2 < cap) {
            r++;
            dst[w++] = src[r];
        }
        if (src[r + 1] != '\0')
            dst[w++] = ' ';
    }
    while (w > 0 && dst[w - 1] == ' ')
        w--;
    dst[w] = '\0';
}

static int probe_parse(const char *alg, ct_move *out, int cap) {
    char buf[512];
    snprintf(buf, sizeof buf, "%s", alg);
    probe_strip_brackets(buf);
    return ct_notation_parse(buf, 3, out, cap);
}

static int probe_apply(ct_cube *c, const char *alg) {
    ct_move moves[PROBE_MOVES];
    int n = probe_parse(alg, moves, PROBE_MOVES);
    if (n <= 0) {
        fprintf(stderr, "PARSE FAIL %s -> %d\n", alg, n);
        probe_fail = PROBE_FAIL;
        return 0;
    }
    for (int k = 0; k < n; k++)
        ct_apply(c, moves[k]);
    return 1;
}

static void probe_grammar(const char *alg, const char *label) {
    ct_move moves[PROBE_MOVES];
    char what[256];
    int n = ct_notation_parse(alg, 3, moves, PROBE_MOVES);
    snprintf(what, sizeof what, "grammar clean %s", label);
    probe_check(n > 0, what);
}

static int probe_solved_state(const ct_cube *c) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                if (c->facelet[probe_cell(face, row, col)] !=
                    (uint8_t)probe_flat(face, row, col))
                    return 0;
            }
        }
    }
    return 1;
}

static int probe_centers_solved(const ct_cube *c) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        if (c->facelet[probe_cell(face, 1, 1)] != (uint8_t)probe_flat(face, 1, 1))
            return 0;
    }
    return 1;
}

static int probe_parity(const ct_cube *c, int orbit) {
    int perm[PROBE_EDGES];
    probe_perm(c, orbit, perm);
    int pieces = orbit == 0 ? PROBE_CORNERS : PROBE_EDGES;
    int swaps = 0;
    int visited[PROBE_EDGES] = {0};
    for (int p = 0; p < pieces; p++) {
        if (visited[p] || perm[p] == p)
            continue;
        int len = 0;
        int q = p;
        do {
            visited[q] = 1;
            q = perm[q];
            len++;
        } while (q != p);
        swaps += len - 1;
    }
    return swaps % 2;
}

static int probe_moved(const ct_cube *c, int orbit) {
    int perm[PROBE_EDGES];
    probe_perm(c, orbit, perm);
    int pieces = orbit == 0 ? PROBE_CORNERS : PROBE_EDGES;
    int moved = 0;
    for (int p = 0; p < pieces; p++) {
        if (perm[p] != p)
            moved++;
    }
    return moved;
}

static void probe_effect(const char *alg, const char *label) {
    ct_cube c;
    probe_identity(&c);
    if (!probe_apply(&c, alg))
        return;
    probe_describe(&c, label);
}

static int probe_sticker_named(int orbit, const char *name) {
    for (int s = 0; s < PROBE_STICKERS; s++) {
        char a[PROBE_NAME];
        probe_name(orbit, s, a, sizeof a);
        if (strcmp(a, name) == 0)
            return s;
    }
    return -1;
}

static void probe_swap_alg(const char *alg, int orbit, const char *from, const char *to,
                           const char *label) {
    ct_cube c;
    probe_identity(&c);
    if (!probe_apply(&c, alg))
        return;
    char what[256];
    snprintf(what, sizeof what, "%s moves exactly 2 pieces", label);
    probe_check(probe_moved(&c, orbit) == 2, what);
    int from_s = probe_sticker_named(orbit, from);
    int cell = probe_cell_of_sticker(orbit, from_s);
    int now = probe_cell_of_home(&c, cell);
    int ns = probe_sticker_of_cell(orbit, now);
    char landed[PROBE_NAME];
    probe_name(orbit, ns, landed, sizeof landed);
    snprintf(what, sizeof what, "%s sticker %s lands sticker exact on %s got %s", label,
             from, to, landed);
    probe_check(strcmp(landed, to) == 0, what);
    snprintf(what, sizeof what, "%s centers solved", label);
    probe_check(probe_centers_solved(&c), what);
}

static void probe_setup_swap(ct_cube *c, const char *setup, const char *swap) {
    if (setup[0] != '\0') {
        if (!probe_apply(c, setup))
            return;
        if (!probe_apply(c, swap))
            return;
        ct_move undo[PROBE_MOVES];
        int n = probe_parse(setup, undo, PROBE_MOVES);
        if (n <= 0) {
            probe_fail = PROBE_FAIL;
            return;
        }
        for (int m = n - 1; m >= 0; m--)
            ct_apply(c, ct_move_inverse(undo[m]));
    } else {
        probe_apply(c, swap);
    }
}

static void probe_side_pair(const ct_cube *c, int orbit, char *out0, char *out1) {
    int f = 0;
    out0[0] = '\0';
    out1[0] = '\0';
    for (int s = 0; s < PROBE_STICKERS && f < 2; s++) {
        int cell = probe_cell_of_sticker(orbit, s);
        int now = probe_cell_of_home(c, cell);
        int ns = probe_sticker_of_cell(orbit, now);
        int (*piece_of)(int) = orbit == 0 ? ct_corner_piece_of : ct_edge_piece_of;
        if (piece_of(ns) != piece_of(s)) {
            probe_name(orbit, ns, f == 0 ? out0 : out1, PROBE_NAME);
            f++;
        }
    }
}

static void probe_run(void) {
    const char *t_perm = "R U R' U' R' F R2 U' R' U' R U R' F'";
    const char *y_perm = "R U' R' U' R U R' F' R U R' U' R' F R";
    const char *op_parity = "R U' R' U' R U R D R' U' R D' R' U2 R' U'";
    const char *jb_perm = "R U R' F' R U R' U' R' F R2 U' R' U'";
    const char *bld_y = "F R U' R' U' R U R' F' R U R' U' R' F R F'";
    const char *m2_parity1 = "M2 U' M U2 M U' M' U2 M' U2";
    const char *m2_parity2 = "D' L2 D M2 D' L2 D";
    const char *m2_flip = "M' U M' U M' U2 M U M U M U2";
    const char *twist3 = "L' U' L U' L' U2 L R U R' U R U2 R'";
    const char *twist1 = "R' U R2' U' R2 U' R' U R U R' U' R2 U' R2 U";
    const char *twist2 = "R U R2' U' R2 U' R' U R U R' U' R2 U' R2 U R2";
    const char *twist4 = "R U2 R' U' R U' R' L' U2 L U L' U L";
    const char *poch_odd = "(r2' U' r2) (R' U) (L' U2') (R U' R' U2 R) (L U') (r2' U)";

    probe_grammar(op_parity, "op.08 parity");
    probe_grammar(jb_perm, "op.40 bldtrainer parity");
    probe_grammar(m2_parity1, "m2 parity option 1");
    probe_grammar(m2_parity2, "m2 parity option 2");
    probe_grammar(m2_flip, "m2 buffer flip");
    probe_grammar("F2 M' U M' U M' U2 M U M U M U2 F2", "m2 buffer flip row text");
    probe_grammar(twist3, "corner twist");

    probe_effect(t_perm, "op.01 T perm");
    probe_effect(y_perm, "op.02 Y perm");
    probe_effect(op_parity, "op.08 parity");
    probe_effect(jb_perm, "bldtrainer Jb perm");
    probe_effect(bld_y, "bldtrainer Y perm");
    probe_effect(m2_parity1, "m2 parity option 1");
    probe_effect(m2_parity2, "m2 parity option 2");
    probe_effect(m2_flip, "m2 buffer flip");
    probe_effect("F2 M' U M' U M' U2 M U M U M U2 F2", "m2 flip F2 conjugate");
    probe_effect("L F M' U M' U M' U2 M U M U M U2 F' L'",
                 "pochmann example 1 orient edges conjugate");
    probe_effect(twist1, "pochmann orient corners 1");
    probe_effect(twist2, "pochmann orient corners 2");
    probe_effect(twist3, "pochmann orient corners 3 draft row");
    probe_effect(twist4, "pochmann orient corners 4");
    probe_effect(poch_odd, "pochmann M2R2 odd parity observation");

    probe_swap_alg(t_perm, CT_TARGET_ORBIT_EDGE, "UR", "UL", "op.01 T perm");
    probe_swap_alg(y_perm, CT_TARGET_ORBIT_CORNER, "LUB", "DRF", "op.02 Y perm");
    probe_swap_alg(jb_perm, CT_TARGET_ORBIT_EDGE, "UF", "UR", "bldtrainer Jb perm");
    probe_swap_alg(bld_y, CT_TARGET_ORBIT_CORNER, "ULB", "URF", "bldtrainer Y perm");

    {
        ct_cube c;
        probe_identity(&c);
        probe_apply(&c, op_parity);
        probe_check(probe_parity(&c, CT_TARGET_ORBIT_EDGE) == 1, "op.08 edge parity odd");
        probe_check(probe_parity(&c, CT_TARGET_ORBIT_CORNER) == 1,
                    "op.08 corner parity odd");
        probe_check(probe_moved(&c, CT_TARGET_ORBIT_EDGE) == 2, "op.08 moves 2 edges");
        probe_check(probe_moved(&c, CT_TARGET_ORBIT_CORNER) == 2,
                    "op.08 moves 2 corners");
        probe_check(probe_centers_solved(&c), "op.08 centers solved");
    }

    {
        ct_cube c;
        probe_identity(&c);
        probe_apply(&c, m2_flip);
        probe_check(probe_moved(&c, CT_TARGET_ORBIT_EDGE) == 0,
                    "m2 flip pure no edge permutation");
        probe_check(probe_moved(&c, CT_TARGET_ORBIT_CORNER) == 0,
                    "m2 flip pure corners untouched");
        probe_check(probe_centers_solved(&c), "m2 flip pure centers solved");
    }

    for (int t = 0; t < 4; t++) {
        const char *alg = t == 0 ? twist1 : t == 1 ? twist2 : t == 2 ? twist3 : twist4;
        ct_cube c;
        probe_identity(&c);
        probe_apply(&c, alg);
        char what[128];
        snprintf(what, sizeof what, "twist %d pure no corner permutation", t + 1);
        probe_check(probe_moved(&c, CT_TARGET_ORBIT_CORNER) == 0, what);
        snprintf(what, sizeof what, "twist %d pure edges untouched", t + 1);
        probe_check(probe_moved(&c, CT_TARGET_ORBIT_EDGE) == 0, what);
        snprintf(what, sizeof what, "twist %d pure centers solved", t + 1);
        probe_check(probe_centers_solved(&c), what);
    }

    {
        ct_cube c;
        probe_identity(&c);
        probe_apply(&c, "U' L2 D' U R2 B' D' U' L2 B2 R' U' B' F' L U2 F R2 U'");
        const char *edge_setups[] = {
            "Lw D L2", "", "Lw D' L2", "L Dw' L", "L2", "D' Lw D L2",
            "D L2", "Dw2 L", "Dw L'", "L'", "Lw' D L2", "Dw L", "L2",
        };
        for (unsigned k = 0; k < sizeof edge_setups / sizeof edge_setups[0]; k++)
            probe_setup_swap(&c, edge_setups[k], t_perm);
        probe_apply(&c, op_parity);
        const char *corner_setups[] = {
            "R2 F", "D", "D F'", "F2 D", "", "R' F", "R D'", "F R'", "F' D",
        };
        for (unsigned k = 0; k < sizeof corner_setups / sizeof corner_setups[0]; k++)
            probe_setup_swap(&c, corner_setups[k], y_perm);
        probe_check(probe_solved_state(&c), "jperm bld example 2 end to end solved");
    }

    {
        char y_side0[PROBE_NAME];
        char y_side1[PROBE_NAME];
        {
            ct_cube y_ref;
            probe_identity(&y_ref);
            probe_apply(&y_ref, y_perm);
            probe_side_pair(&y_ref, CT_TARGET_ORBIT_EDGE, y_side0, y_side1);
        }
        printf("op.02 edge side effect pair: %s %s\n", y_side0, y_side1);

        for (int opt = 1; opt <= 2; opt++) {
            ct_cube m2_ref;
            probe_identity(&m2_ref);
            probe_apply(&m2_ref,
                        "L' D2 B2 D2 B D F D R' F' U D L2 B2 D B2 L2 B2 U' B2 R2");
            ct_cube c = m2_ref;
            const char *targets[] = {
                "B' R B", "B' R' B", "M U2 M U2", "B L' B'", "U2 M' U2 M'",
                "U' L' U", "U' L2 U", "M2", "U' L U", "U R U'", "B' R2 B",
            };
            const int special[] = {0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0};
            for (unsigned k = 0; k < sizeof targets / sizeof targets[0]; k++) {
                if (special[k])
                    probe_apply(&c, targets[k]);
                else
                    probe_setup_swap(&c, targets[k], "M2");
            }
            probe_apply(&c, opt == 1 ? m2_parity1 : m2_parity2);
            char what[128];
            snprintf(what, sizeof what, "jperm m2 example 1 option %d edges except 2",
                     opt);
            probe_check(probe_moved(&c, CT_TARGET_ORBIT_EDGE) == 2 &&
                            probe_parity(&c, CT_TARGET_ORBIT_EDGE) == 1,
                        what);
            snprintf(what, sizeof what,
                     "jperm m2 example 1 option %d corners unchanged by edge phase", opt);
            int corners_same = 1;
            for (int s = 0; s < PROBE_STICKERS; s++) {
                int cell = probe_cell_of_sticker(CT_TARGET_ORBIT_CORNER, s);
                if (probe_cell_of_home(&c, cell) != probe_cell_of_home(&m2_ref, cell))
                    corners_same = 0;
            }
            probe_check(corners_same, what);
            snprintf(what, sizeof what, "jperm m2 example 1 option %d centers solved",
                     opt);
            probe_check(probe_centers_solved(&c), what);
            char s0[PROBE_NAME];
            char s1[PROBE_NAME];
            probe_side_pair(&c, CT_TARGET_ORBIT_EDGE, s0, s1);
            printf("m2 parity option %d leftover edge pair: %s %s\n", opt, s0, s1);
            int match = (strcmp(s0, y_side0) == 0 && strcmp(s1, y_side1) == 0) ||
                        (strcmp(s0, y_side1) == 0 && strcmp(s1, y_side0) == 0);
            snprintf(what, sizeof what, "m2 option %d leftover pair equals op.02 side",
                     opt);
            probe_check(match, what);
        }
    }

    {
        const char *scramble1 =
            "B2 D2 F D' U2 R' B2 U L2 R' D2 L' U2 R' U B R2 D L F R2 U F D2 R2";
        ct_cube c1;
        probe_identity(&c1);
        probe_apply(&c1, scramble1);
        const char *pairs1[] = {
            "U'LU-M2-U'L'U-URU'-M2-UR'U'",
            "D-U'R2U-M-U'R2U-M'-D'",
            "UR'U'-M2-URU'-U'L'U-M2-U'LU",
            "BLB'-M2-BL'B'-B'R'B-M2-B'RB",
            "R'URU'-MU2MU2M2'-UR'U'R",
            "LU'L'U-M2-U'LUL'-B'RB-M2-B'R'B",
        };
        char expanded[512];
        for (unsigned k = 0; k < sizeof pairs1 / sizeof pairs1[0]; k++) {
            probe_expand_runs(pairs1[k], expanded, sizeof expanded);
            probe_apply(&c1, expanded);
        }
        probe_describe(&c1, "cubefreak example 1 end state");
        char what[128];
        snprintf(what, sizeof what, "cubefreak example 1 edges solved");
        probe_check(probe_moved(&c1, CT_TARGET_ORBIT_EDGE) == 0, what);
        int flipped1 = 0;
        for (int p = 0; p < PROBE_EDGES; p++) {
            int s = probe_ud_sticker(CT_TARGET_ORBIT_EDGE, p, ct_edge_piece_of);
            int cell = probe_cell_of_sticker(CT_TARGET_ORBIT_EDGE, s);
            int now = probe_cell_of_home(&c1, cell);
            int ns = probe_sticker_of_cell(CT_TARGET_ORBIT_EDGE, now);
            if (!probe_edge_oriented(s, ns))
                flipped1++;
        }
        snprintf(what, sizeof what, "cubefreak example 1 no flipped edges remain");
        probe_check(flipped1 == 0, what);
        snprintf(what, sizeof what, "cubefreak example 1 centers solved");
        probe_check(probe_centers_solved(&c1), what);
    }

    {
        const char *scramble =
            "D F' D R2 B2 U F L' F' L2 U2 L2 B R2 B' R U' D B R' F' D2 B' L' R2";
        ct_cube c;
        probe_identity(&c);
        probe_apply(&c, scramble);
        const char *pairs[] = {
            "URU'-M2-UR'U'-R'URU'-M2-UR'U'R",
            "RB'R'B-M2-B'RBR'-R'UR'U'-M2-URU'R",
            "L'BLB'-M2-BL'B'L-U'L'U-M2-U'LU",
            "BLB'-M'U2MU2-BL'B'",
            "M-BL'B'-M-BLB'-M2'",
        };
        printf("cubefreak example 2 erratum: printed pair 4 setup BL'B' contradicts the "
               "page prep table where LD is BLB', corrected form used, verbatim form "
               "cycles the UL piece instead of DL and misses the claimed end state\n");
        char expanded[512];
        for (unsigned k = 0; k < sizeof pairs / sizeof pairs[0]; k++) {
            probe_expand_runs(pairs[k], expanded, sizeof expanded);
            probe_apply(&c, expanded);
            char label[128];
            snprintf(label, sizeof label, "cubefreak example 2 pair %u alone", k + 1);
            probe_effect(expanded, label);
        }
        probe_describe(&c, "cubefreak example 2 after pairs before orient");
        probe_apply(&c, "F2 M' U M' U M' U2 M U M U M U2 F2");
        probe_describe(&c, "cubefreak example 2 end state");
        char what[128];
        ct_cube ref;
        probe_identity(&ref);
        probe_apply(&ref, scramble);
        int same = 1;
        for (int s = 0; s < PROBE_STICKERS; s++) {
            int cell = probe_cell_of_sticker(CT_TARGET_ORBIT_CORNER, s);
            if (probe_cell_of_home(&c, cell) != probe_cell_of_home(&ref, cell))
                same = 0;
        }
        snprintf(what, sizeof what, "cubefreak example 2 corners unchanged by edge solve");
        probe_check(same, what);
        char s0[PROBE_NAME];
        char s1[PROBE_NAME];
        probe_side_pair(&c, CT_TARGET_ORBIT_EDGE, s0, s1);
        printf("cubefreak example 2 leftover edge pair: %s %s\n", s0, s1);
        printf("cubefreak example 2 moved edges: %d\n",
               probe_moved(&c, CT_TARGET_ORBIT_EDGE));
    }
}

int main(void) {
    probe_run();
    if (probe_fail == PROBE_OK)
        printf("parity flip twist probe ok\n");
    return probe_fail;
}
