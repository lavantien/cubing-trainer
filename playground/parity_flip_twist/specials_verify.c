#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/targets.h"

enum {
    SV_MOVES = 48,
    SV_STICKERS = CT_TARGET_STICKERS,
    SV_EDGES = 12,
    SV_CORNERS = 8,
    SV_LINE = 1024,
    SV_OK = 0,
    SV_FAIL = 1
};

static const char *const sv_corner_names[SV_STICKERS] = {
    "UBL", "UBR", "UFR", "UFL", "LUB", "LUF", "LDF", "LDB",
    "FUL", "FUR", "FDR", "FDL", "RUF", "RUB", "RDB", "RDF",
    "BUR", "BUL", "BDL", "BDR", "DFL", "DFR", "DBR", "DBL",
};

static const char *const sv_edge_names[SV_STICKERS] = {
    "UB", "UR", "UF", "UL", "LU", "LF", "LD", "LB",
    "FU", "FR", "FD", "FL", "RU", "RB", "RD", "RF",
    "BU", "BL", "BD", "BR", "DF", "DR", "DB", "DL",
};

static int sv_fail = SV_OK;
static int sv_checked = 0;

static void sv_check(int cond, const char *what) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", what);
        sv_fail = SV_FAIL;
    }
}

static int sv_flat(int face, int row, int col) {
    return face * 9 + row * 3 + col;
}

static int sv_cell(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int sv_cell_of_sticker(int orbit, int sticker) {
    int face = 0;
    int row = 0;
    int col = 0;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return sv_cell(face, row, col);
}

static int sv_sticker_of_cell(int orbit, int cell) {
    for (int s = 0; s < SV_STICKERS; s++) {
        if (sv_cell_of_sticker(orbit, s) == cell)
            return s;
    }
    return -1;
}

static void sv_identity(ct_cube *c) {
    ct_cube_init(c, 3);
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++)
                c->facelet[sv_cell(face, row, col)] = (uint8_t)sv_flat(face, row, col);
        }
    }
}

static int sv_cell_of_home(const ct_cube *c, int cell) {
    int want = sv_flat(cell / (CT_CUBE_STRIDE * CT_CUBE_STRIDE),
                       (cell % (CT_CUBE_STRIDE * CT_CUBE_STRIDE)) / CT_CUBE_STRIDE,
                       cell % CT_CUBE_STRIDE);
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                if (c->facelet[sv_cell(face, row, col)] == (uint8_t)want)
                    return sv_cell(face, row, col);
            }
        }
    }
    return -1;
}

static int sv_ud_sticker(int orbit, int piece, int (*piece_of)(int)) {
    for (int pass = 0; pass < 2; pass++) {
        for (int s = 0; s < SV_STICKERS; s++) {
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
    return -1;
}

static void sv_perm(const ct_cube *c, int orbit, int *perm) {
    int (*piece_of)(int) = orbit == 0 ? ct_corner_piece_of : ct_edge_piece_of;
    int pieces = orbit == 0 ? SV_CORNERS : SV_EDGES;
    for (int p = 0; p < pieces; p++) {
        int s = sv_ud_sticker(orbit, p, piece_of);
        int cell = sv_cell_of_sticker(orbit, s);
        int now = sv_cell_of_home(c, cell);
        int ns = sv_sticker_of_cell(orbit, now);
        perm[p] = piece_of(ns);
    }
}

static int sv_moved(const ct_cube *c, int orbit) {
    int perm[SV_EDGES];
    sv_perm(c, orbit, perm);
    int pieces = orbit == 0 ? SV_CORNERS : SV_EDGES;
    int moved = 0;
    for (int p = 0; p < pieces; p++) {
        if (perm[p] != p)
            moved++;
    }
    return moved;
}

static int sv_centers_solved(const ct_cube *c) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        if (c->facelet[sv_cell(face, 1, 1)] != (uint8_t)sv_flat(face, 1, 1))
            return 0;
    }
    return 1;
}

static int sv_edge_oriented(int home_sticker, int now_sticker) {
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

static int sv_corner_twisted(const ct_cube *c, int piece) {
    int s = sv_ud_sticker(CT_TARGET_ORBIT_CORNER, piece, ct_corner_piece_of);
    int cell = sv_cell_of_sticker(CT_TARGET_ORBIT_CORNER, s);
    int now = sv_cell_of_home(c, cell);
    int ns = sv_sticker_of_cell(CT_TARGET_ORBIT_CORNER, now);
    int nf = 0;
    int nr = 0;
    int nc = 0;
    ct_sticker_cell(CT_TARGET_ORBIT_CORNER, ns, &nf, &nr, &nc);
    return nf != CT_FACE_U && nf != CT_FACE_D;
}

static int sv_face_of_sticker(int orbit, int sticker) {
    int face = 0;
    int row = 0;
    int col = 0;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return face;
}

static int sv_twist_sign(const ct_cube *c, int piece) {
    int s = sv_ud_sticker(CT_TARGET_ORBIT_CORNER, piece, ct_corner_piece_of);
    int home_face = sv_face_of_sticker(CT_TARGET_ORBIT_CORNER, s);
    int cell = sv_cell_of_sticker(CT_TARGET_ORBIT_CORNER, s);
    int now = sv_cell_of_home(c, cell);
    int now_face = now / (CT_CUBE_STRIDE * CT_CUBE_STRIDE);
    int third_face = -1;
    for (int q = 0; q < SV_STICKERS; q++) {
        if (ct_corner_piece_of(q) != piece)
            continue;
        int fq = sv_face_of_sticker(CT_TARGET_ORBIT_CORNER, q);
        if (fq != home_face && fq != now_face) {
            third_face = fq;
            break;
        }
    }
    if (third_face < 0)
        return 0;
    ct_v3 a = ct_frames[home_face].normal;
    ct_v3 b = ct_frames[now_face].normal;
    ct_v3 t = ct_frames[third_face].normal;
    int cross_x = a.y * b.z - a.z * b.y;
    int cross_y = a.z * b.x - a.x * b.z;
    int cross_z = a.x * b.y - a.y * b.x;
    int dot = cross_x * t.x + cross_y * t.y + cross_z * t.z;
    return dot > 0 ? 1 : -1;
}

static int sv_name_sticker(int orbit, const char *name, int len) {
    const char *const *names = orbit == 0 ? sv_corner_names : sv_edge_names;
    int name_len = orbit == 0 ? 3 : 2;
    if (len != name_len)
        return -1;
    for (int s = 0; s < SV_STICKERS; s++) {
        if (strncmp(names[s], name, (size_t)len) == 0)
            return s;
    }
    return -1;
}

static int sv_apply(ct_cube *c, const char *alg) {
    ct_move moves[SV_MOVES];
    int n = ct_notation_parse(alg, 3, moves, SV_MOVES);
    if (n <= 0) {
        fprintf(stderr, "PARSE FAIL %s -> %d\n", alg, n);
        sv_fail = SV_FAIL;
        return 0;
    }
    for (int k = 0; k < n; k++)
        ct_apply(c, moves[k]);
    return 1;
}

static int sv_edge_flipped(const ct_cube *c, int piece) {
    int s = sv_ud_sticker(CT_TARGET_ORBIT_EDGE, piece, ct_edge_piece_of);
    int cell = sv_cell_of_sticker(CT_TARGET_ORBIT_EDGE, s);
    int now = sv_cell_of_home(c, cell);
    int ns = sv_sticker_of_cell(CT_TARGET_ORBIT_EDGE, now);
    return ct_edge_piece_of(ns) == piece && !sv_edge_oriented(s, ns);
}

static void sv_check_flips(const ct_cube *c, const char *position, const char *key) {
    char what[SV_LINE];
    int a = sv_name_sticker(CT_TARGET_ORBIT_EDGE, position, 2);
    int b = sv_name_sticker(CT_TARGET_ORBIT_EDGE, position + 5, 2);
    snprintf(what, sizeof what, "%s flip names parse", key);
    sv_check(a >= 0 && b >= 0, what);
    if (a < 0 || b < 0)
        return;
    int pa = ct_edge_piece_of(a);
    int pb = ct_edge_piece_of(b);
    snprintf(what, sizeof what, "%s flips exactly the two named edges", key);
    sv_check(sv_moved(c, CT_TARGET_ORBIT_EDGE) == 0, what);
    snprintf(what, sizeof what, "%s edge %d flipped", key, pa);
    sv_check(sv_edge_flipped(c, pa), what);
    snprintf(what, sizeof what, "%s edge %d flipped", key, pb);
    sv_check(sv_edge_flipped(c, pb), what);
    int flipped_count = 0;
    for (int p = 0; p < SV_EDGES; p++) {
        if (sv_edge_flipped(c, p))
            flipped_count++;
    }
    snprintf(what, sizeof what, "%s exactly two edges flipped", key);
    sv_check(flipped_count == 2, what);
    snprintf(what, sizeof what, "%s corners untouched", key);
    sv_check(sv_moved(c, CT_TARGET_ORBIT_CORNER) == 0, what);
    int twisted = 0;
    for (int p = 0; p < SV_CORNERS; p++) {
        if (sv_corner_twisted(c, p))
            twisted++;
    }
    snprintf(what, sizeof what, "%s no corners twisted", key);
    sv_check(twisted == 0, what);
    snprintf(what, sizeof what, "%s centers solved", key);
    sv_check(sv_centers_solved(c), what);
}

static void sv_check_twists(const ct_cube *c, const char *position, const char *key) {
    char what[SV_LINE];
    int a = sv_name_sticker(CT_TARGET_ORBIT_CORNER, position, 3);
    int b = sv_name_sticker(CT_TARGET_ORBIT_CORNER, position + 6, 3);
    snprintf(what, sizeof what, "%s twist names parse", key);
    sv_check(a >= 0 && b >= 0, what);
    if (a < 0 || b < 0)
        return;
    int pa = ct_corner_piece_of(a);
    int pb = ct_corner_piece_of(b);
    snprintf(what, sizeof what, "%s twists exactly the two named corners", key);
    sv_check(sv_moved(c, CT_TARGET_ORBIT_CORNER) == 0, what);
    snprintf(what, sizeof what, "%s corner %d twisted", key, pa);
    sv_check(sv_corner_twisted(c, pa), what);
    snprintf(what, sizeof what, "%s corner %d twisted", key, pb);
    sv_check(sv_corner_twisted(c, pb), what);
    int twisted = 0;
    for (int p = 0; p < SV_CORNERS; p++) {
        if (sv_corner_twisted(c, p))
            twisted++;
    }
    snprintf(what, sizeof what, "%s exactly two corners twisted", key);
    sv_check(twisted == 2, what);
    snprintf(what, sizeof what, "%s first named corner twists ccw", key);
    sv_check(sv_twist_sign(c, pa) == 1, what);
    snprintf(what, sizeof what, "%s second named corner twists cw", key);
    sv_check(sv_twist_sign(c, pb) == -1, what);
    snprintf(what, sizeof what, "%s edges untouched", key);
    sv_check(sv_moved(c, CT_TARGET_ORBIT_EDGE) == 0, what);
    int flipped = 0;
    for (int p = 0; p < SV_EDGES; p++) {
        if (sv_edge_flipped(c, p))
            flipped++;
    }
    snprintf(what, sizeof what, "%s no edges flipped", key);
    sv_check(flipped == 0, what);
    snprintf(what, sizeof what, "%s centers solved", key);
    sv_check(sv_centers_solved(c), what);
}

static void sv_check_parity(const ct_cube *c, const char *position, const char *key) {
    char what[SV_LINE];
    int e1 = sv_name_sticker(CT_TARGET_ORBIT_EDGE, position, 2);
    int e2 = sv_name_sticker(CT_TARGET_ORBIT_EDGE, position + 3, 2);
    int c1 = sv_name_sticker(CT_TARGET_ORBIT_CORNER, position + 7, 3);
    int c2 = sv_name_sticker(CT_TARGET_ORBIT_CORNER, position + 11, 3);
    snprintf(what, sizeof what, "%s parity names parse", key);
    sv_check(e1 >= 0 && e2 >= 0 && c1 >= 0 && c2 >= 0, what);
    if (e1 < 0 || e2 < 0 || c1 < 0 || c2 < 0)
        return;
    int pe1 = ct_edge_piece_of(e1);
    int pe2 = ct_edge_piece_of(e2);
    int pc1 = ct_corner_piece_of(c1);
    int pc2 = ct_corner_piece_of(c2);
    int perm[SV_EDGES];
    sv_perm(c, CT_TARGET_ORBIT_EDGE, perm);
    snprintf(what, sizeof what, "%s edges transposed exactly", key);
    sv_check(perm[pe1] == pe2 && perm[pe2] == pe1 && sv_moved(c, CT_TARGET_ORBIT_EDGE) == 2,
             what);
    for (int p = 0; p < SV_EDGES; p++) {
        if (p == pe1 || p == pe2)
            continue;
        snprintf(what, sizeof what, "%s edge %d oriented", key, p);
        sv_check(!sv_edge_flipped(c, p), what);
    }
    sv_perm(c, CT_TARGET_ORBIT_CORNER, perm);
    snprintf(what, sizeof what, "%s corners transposed exactly", key);
    sv_check(perm[pc1] == pc2 && perm[pc2] == pc1 && sv_moved(c, CT_TARGET_ORBIT_CORNER) == 2,
             what);
    snprintf(what, sizeof what, "%s centers solved", key);
    sv_check(sv_centers_solved(c), what);
}

static void sv_check_ltct(const ct_cube *c, const char *position, const char *key) {
    char what[SV_LINE];
    int e1 = sv_name_sticker(CT_TARGET_ORBIT_EDGE, position, 2);
    int e2 = sv_name_sticker(CT_TARGET_ORBIT_EDGE, position + 3, 2);
    const char *c1p = position + 7;
    const char *bracket = strchr(position, '[');
    const char *comma = strchr(position, ',');
    int c1 = sv_name_sticker(CT_TARGET_ORBIT_CORNER, c1p, 3);
    int second_len = bracket != NULL ? (int)(bracket - (c1p + 4)) : -1;
    int c2 = sv_name_sticker(CT_TARGET_ORBIT_CORNER, c1p + 4, second_len);
    int c3 = bracket != NULL ? sv_name_sticker(CT_TARGET_ORBIT_CORNER, bracket + 1, 3) : -1;
    snprintf(what, sizeof what, "%s ltct names parse %d %d %d %d", key, e1, e2, c1, c2);
    sv_check(e1 >= 0 && e2 >= 0 && c1 >= 0 && c2 >= 0 && c3 >= 0 && comma != NULL, what);
    if (e1 < 0 || e2 < 0 || c1 < 0 || c2 < 0 || c3 < 0)
        return;
    int pe1 = ct_edge_piece_of(e1);
    int pe2 = ct_edge_piece_of(e2);
    int pc1 = ct_corner_piece_of(c1);
    int pc2 = ct_corner_piece_of(c2);
    int pc3 = ct_corner_piece_of(c3);
    int perm[SV_EDGES];
    sv_perm(c, CT_TARGET_ORBIT_EDGE, perm);
    snprintf(what, sizeof what, "%s edges transposed exactly", key);
    sv_check(perm[pe1] == pe2 && perm[pe2] == pe1 && sv_moved(c, CT_TARGET_ORBIT_EDGE) == 2,
             what);
    sv_perm(c, CT_TARGET_ORBIT_CORNER, perm);
    snprintf(what, sizeof what, "%s corners c1 c2 transposed exactly", key);
    sv_check(perm[pc1] == pc2 && perm[pc2] == pc1 && sv_moved(c, CT_TARGET_ORBIT_CORNER) == 2,
             what);
    snprintf(what, sizeof what, "%s bracketed corner twisted in place", key);
    sv_check(perm[pc3] == pc3 && sv_corner_twisted(c, pc3), what);
    snprintf(what, sizeof what, "%s centers solved", key);
    sv_check(sv_centers_solved(c), what);
}

int main(void) {
    FILE *f = fopen("db/seeds/packs/bld_3style_specials.tsv", "rb");
    if (f == NULL) {
        fprintf(stderr, "FAIL open tsv\n");
        return SV_FAIL;
    }
    char line[SV_LINE];
    if (fgets(line, sizeof line, f) == NULL) {
        fprintf(stderr, "FAIL header\n");
        fclose(f);
        return SV_FAIL;
    }
    int category_counts[4] = {0, 0, 0, 0};
    while (fgets(line, sizeof line, f) != NULL) {
        char *nl = strchr(line, '\n');
        if (nl != NULL)
            *nl = '\0';
        char *tab1 = strchr(line, '\t');
        char *tab2 = tab1 != NULL ? strchr(tab1 + 1, '\t') : NULL;
        char *tab4 = tab2 != NULL ? strchr(tab2 + 1, '\t') : NULL;
        char *tab5 = tab4 != NULL ? strchr(tab4 + 1, '\t') : NULL;
        if (tab1 == NULL || tab2 == NULL || tab4 == NULL || tab5 == NULL) {
            fprintf(stderr, "FAIL row shape %s\n", line);
            sv_fail = SV_FAIL;
            continue;
        }
        *tab1 = '\0';
        *tab2 = '\0';
        *tab4 = '\0';
        *tab5 = '\0';
        const char *key = line;
        const char *alg = tab1 + 1;
        const char *front = tab2 + 1;
        const char *position = strstr(front, "\"position\":\"");
        if (position == NULL) {
            fprintf(stderr, "FAIL position missing %s\n", key);
            sv_fail = SV_FAIL;
            continue;
        }
        position += strlen("\"position\":\"");
        char pos[128];
        size_t plen = strcspn(position, "\"");
        if (plen >= sizeof pos) {
            fprintf(stderr, "FAIL position too long %s\n", key);
            sv_fail = SV_FAIL;
            continue;
        }
        memcpy(pos, position, plen);
        pos[plen] = '\0';
        ct_cube c;
        sv_identity(&c);
        if (!sv_apply(&c, alg))
            continue;
        sv_checked++;
        if (strncmp(key, "flip.", 5) == 0) {
            sv_check_flips(&c, pos, key);
            category_counts[0]++;
        } else if (strncmp(key, "twist.", 6) == 0) {
            sv_check_twists(&c, pos, key);
            category_counts[1]++;
        } else if (strncmp(key, "parity.", 7) == 0) {
            sv_check_parity(&c, pos, key);
            category_counts[2]++;
        } else if (strncmp(key, "ltct.", 5) == 0) {
            sv_check_ltct(&c, pos, key);
            category_counts[3]++;
        } else {
            fprintf(stderr, "FAIL unknown prefix %s\n", key);
            sv_fail = SV_FAIL;
        }
    }
    fclose(f);
    printf("checked %d rows: flip %d twist %d parity %d ltct %d\n", sv_checked,
           category_counts[0], category_counts[1], category_counts[2], category_counts[3]);
    if (sv_fail == SV_OK && sv_checked == 85 && category_counts[0] == 25 &&
        category_counts[1] == 23 && category_counts[2] == 26 && category_counts[3] == 11)
        printf("specials verify ok\n");
    else
        sv_fail = SV_FAIL;
    return sv_fail;
}
