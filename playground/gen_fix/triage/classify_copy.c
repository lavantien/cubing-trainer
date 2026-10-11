#include "gen/classify.h"

#include "engine/notation.h"
#include "engine/transform.h"
#include "gen/score.h"
#include "gen/targets.h"

#include <stdio.h>
int dbg_corner_swaps;
int dbg_edge_swaps;
int dbg_corner_ret;
int dbg_edge_ret;
#include <string.h>

enum {
    CL_SPLIT_VARIANTS = CT_MOVE_QUARTERS - 1,
    CL_PAIR_COUNT = CT_CLASSIFY_SPECIAL_LEN,
    CL_CYCLIC_HEAD = 2 * (CT_CLASSIFY_INSERT_LEN + 2),
    CL_CYCLIC_C_OFF = CL_CYCLIC_HEAD / 2,
    CL_CYCLE_PIECES = 3,
    CL_PURE_MOVES = 2 * (CT_CLASSIFY_INSERT_LEN + 1),
    CL_CONJ_MOVES = CL_PURE_MOVES + 1,
    CL_ORTH_MOVES = CL_PURE_MOVES + 1 + 1,
    CL_SPECIAL_MOVES = 2 * (CT_CLASSIFY_SPECIAL_LEN + 1),
    CL_HSP_MOVES = 2 * (1 + 1),
};

typedef struct {
    ct_move m[CT_CLASSIFY_MOVES_MAX];
    int len;
} cl_seq;

typedef struct {
    ct_move m[CT_CLASSIFY_MOVES_MAX + 1];
    int len;
} cl_exp;

typedef struct {
    int p;
    int q;
} cl_pair;

typedef struct {
    bool corner;
    bool edge;
    bool corner_exact;
    bool edge_exact;
    int flipped;
    int twisted;
    int corner_swaps;
    int edge_swaps;
} cl_perm_info;

static const char cl_labels[CT_CLS_UNKNOWN + 1][CT_CLASSIFY_LABEL_MAX] = {
    "pure", "A9",  "B9",  "orthogonal", "cyclic shift", "column", "per special", "half slice-plane",
    "SP5",  "SP7", "SP9", "parity",     "flip",         "twist",  "unknown",
};

static const char cl_aliases[CT_CLS_UNKNOWN + 1][CT_CLASSIFY_LABEL_MAX] = {
    "pure",      "A9",           "B9",          "perpendicular",
    "collision", "simultaneous", "per special", "half slice-plane",
    "SP5",       "SP7",          "SP9",         "parity",
    "flip",      "twist",        "unknown",
};

static bool cl_corner_cell[CT_CLASSIFY_CELLS];
static bool cl_edge_cell[CT_CLASSIFY_CELLS];
static bool cl_cells_ready;

static const cl_pair cl_pairs[CL_PAIR_COUNT] = {
    {1, 1},
    {1, CT_CLASSIFY_INSERT_LEN},
    {CT_CLASSIFY_INSERT_LEN, 1},
    {1, CT_CLASSIFY_SPECIAL_LEN},
    {CT_CLASSIFY_SPECIAL_LEN, 1},
};

enum { CL_CYCLIC_SLOT = CT_CLASSIFY_INSERT_LEN };

static int cl_raw_of(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int cl_compact_of(int face, int row, int col) {
    return face * CT_CLASSIFY_N * CT_CLASSIFY_N + row * CT_CLASSIFY_N + col;
}

static void cl_cells_init(void) {
    if (cl_cells_ready) {
        return;
    }
    for (int q = 0; q < CT_CLASSIFY_CELLS; q++) {
        cl_corner_cell[q] = false;
        cl_edge_cell[q] = false;
    }
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
        cl_corner_cell[cl_compact_of(face, row, col)] = true;
        ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
        cl_edge_cell[cl_compact_of(face, row, col)] = true;
    }
    cl_cells_ready = true;
}

static void cl_simulate(const ct_move *seq, int len, uint8_t out[CT_CLASSIFY_CELLS]) {
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = CT_CLASSIFY_N;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_CLASSIFY_N; row++) {
            for (int col = 0; col < CT_CLASSIFY_N; col++) {
                c.facelet[cl_raw_of(face, row, col)] = (uint8_t)cl_compact_of(face, row, col);
            }
        }
    }
    for (int i = 0; i < len; i++) {
        ct_apply(&c, seq[i]);
    }
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_CLASSIFY_N; row++) {
            for (int col = 0; col < CT_CLASSIFY_N; col++) {
                out[cl_compact_of(face, row, col)] = c.facelet[cl_raw_of(face, row, col)];
            }
        }
    }
}

static int cl_sticker_at(int orbit, int cell) {
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(orbit, s, &face, &row, &col);
        if (cl_compact_of(face, row, col) == cell) {
            return s;
        }
    }
    return CT_TARGET_NONE;
}

static int cl_piece_of(int orbit, int sticker) {
    return orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(sticker) : ct_edge_piece_of(sticker);
}

static bool cl_centers_fixed(const uint8_t st[CT_CLASSIFY_CELLS]) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        int mid = cl_compact_of(face, 1, 1);
        if (st[mid] != (uint8_t)mid) {
            return false;
        }
    }
    return true;
}

static bool cl_orbit_transposition(const uint8_t st[CT_CLASSIFY_CELLS], int orbit, int *swaps) {
    int dest[CT_CLASSIFY_CELLS];
    for (int q = 0; q < CT_CLASSIFY_CELLS; q++) {
        dest[st[q]] = q;
    }
    bool seen[CT_TARGET_STICKERS] = {false};
    int count = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int piece = cl_piece_of(orbit, s);
        if (seen[piece]) {
            continue;
        }
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(orbit, s, &face, &row, &col);
        int cell = cl_compact_of(face, row, col);
        int t = cl_sticker_at(orbit, dest[cell]);
        if (t == CT_TARGET_NONE) {
            return false;
        }
        int target = cl_piece_of(orbit, t);
        seen[piece] = true;
        if (target != piece) {
            if (seen[target]) {
                return false;
            }
            seen[target] = true;
            count++;
        }
    }
    *swaps = count;
    return true;
}

static bool cl_orbit_home(const uint8_t st[CT_CLASSIFY_CELLS], int orbit) {
    int dest[CT_CLASSIFY_CELLS];
    for (int q = 0; q < CT_CLASSIFY_CELLS; q++) {
        dest[st[q]] = q;
    }
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(orbit, s, &face, &row, &col);
        int cell = cl_compact_of(face, row, col);
        int t = cl_sticker_at(orbit, dest[cell]);
        if (t == CT_TARGET_NONE) {
            return false;
        }
        if (cl_piece_of(orbit, t) != cl_piece_of(orbit, s)) {
            return false;
        }
    }
    return true;
}

static bool cl_move_same(ct_move a, ct_move b) {
    return a.face == b.face && a.depth_from == b.depth_from && a.depth_to == b.depth_to &&
           a.quarter == b.quarter;
}

static bool cl_orbit_exact_cycle(const uint8_t st[CT_CLASSIFY_CELLS], int orbit) {
    int cells[CT_TARGET_STICKERS];
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(orbit, s, &face, &row, &col);
        cells[s] = cl_compact_of(face, row, col);
    }
    int per = orbit == CT_ORBIT_CORNER ? CT_TARGET_CORNER_STICKERS_PER_PIECE
                                       : CT_TARGET_EDGE_STICKERS_PER_PIECE;
    int moved = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        if (st[cells[s]] != (uint8_t)cells[s]) {
            moved++;
        }
    }
    if (moved != per * CL_CYCLE_PIECES) {
        return false;
    }
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int c = cells[s];
        if (st[c] == (uint8_t)c) {
            continue;
        }
        int c1 = st[c];
        int c2 = st[c1];
        if (c1 == c || c2 == c || c2 == c1 || st[c2] != (uint8_t)c) {
            return false;
        }
    }
    return true;
}

static void cl_perm_scan(const uint8_t st[CT_CLASSIFY_CELLS], cl_perm_info *info) {
    int dest[CT_CLASSIFY_CELLS];
    for (int q = 0; q < CT_CLASSIFY_CELLS; q++) {
        dest[st[q]] = q;
    }
    info->corner = true;
    info->edge = true;
    info->flipped = 0;
    info->twisted = 0;
    for (int q = 0; q < CT_CLASSIFY_CELLS; q++) {
        if (st[q] == (uint8_t)q) {
            continue;
        }
        info->corner = info->corner && cl_corner_cell[q];
        info->edge = info->edge && cl_edge_cell[q];
    }
    info->corner_exact = cl_orbit_exact_cycle(st, CT_ORBIT_CORNER);
    info->edge_exact = cl_orbit_exact_cycle(st, CT_ORBIT_EDGE);
    info->corner_swaps = 0;
    info->edge_swaps = 0;
    dbg_corner_ret = cl_orbit_transposition(st, CT_ORBIT_CORNER, &info->corner_swaps);
    if (!dbg_corner_ret) {
        info->corner_swaps = -1;
    }
    dbg_edge_ret = cl_orbit_transposition(st, CT_ORBIT_EDGE, &info->edge_swaps);
    if (!dbg_edge_ret) {
        info->edge_swaps = -1;
    }
    dbg_corner_swaps = info->corner_swaps;
    dbg_edge_swaps = info->edge_swaps;
    if (cl_orbit_home(st, CT_ORBIT_CORNER) && cl_orbit_home(st, CT_ORBIT_EDGE)) {
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int face = -1;
            int row = -1;
            int col = -1;
            ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
            int cell = cl_compact_of(face, row, col);
            int mate = cl_sticker_at(CT_ORBIT_EDGE, dest[cell]);
            if (mate != CT_TARGET_NONE && mate != s) {
                info->flipped++;
            }
        }
        info->flipped /= CT_TARGET_EDGE_STICKERS_PER_PIECE;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int face = -1;
            int row = -1;
            int col = -1;
            ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
            int cell = cl_compact_of(face, row, col);
            int mate = cl_sticker_at(CT_ORBIT_CORNER, dest[cell]);
            if (mate != CT_TARGET_NONE && mate != s) {
                info->twisted++;
            }
        }
        info->twisted /= CT_TARGET_CORNER_STICKERS_PER_PIECE;
    }
}

static bool cl_is_parity(const uint8_t st[CT_CLASSIFY_CELLS]) {
    cl_perm_info info;
    cl_perm_scan(st, &info);
    return cl_centers_fixed(st) && info.corner_swaps == 1 && info.edge_swaps == 1;
}

static bool cl_is_flip(const uint8_t st[CT_CLASSIFY_CELLS]) {
    cl_perm_info info;
    cl_perm_scan(st, &info);
    return cl_centers_fixed(st) && info.corner_swaps == 0 && info.edge_swaps == 0 &&
           info.flipped > 0 && info.twisted == 0;
}

static bool cl_is_twist(const uint8_t st[CT_CLASSIFY_CELLS]) {
    cl_perm_info info;
    cl_perm_scan(st, &info);
    return cl_centers_fixed(st) && info.corner_swaps == 0 && info.edge_swaps == 0 &&
           info.twisted > 0 && info.flipped == 0;
}

static bool cl_seq_equal(const ct_move *a, int alen, const ct_move *b, int blen) {
    if (alen != blen) {
        return false;
    }
    return memcmp(a, b, (size_t)alen * sizeof *a) == 0;
}

static int cl_split_variants(ct_move m, ct_move out[CL_SPLIT_VARIANTS][2]) {
    out[0][0] = m;
    int n = 1;
    for (int first = 1; first < CT_MOVE_QUARTERS && n < CL_SPLIT_VARIANTS; first++) {
        int second = (m.quarter - first + CT_MOVE_QUARTERS) % CT_MOVE_QUARTERS;
        if (second == 0) {
            continue;
        }
        ct_move a = m;
        ct_move b = m;
        a.quarter = first;
        b.quarter = second;
        out[n][0] = a;
        out[n][1] = b;
        n++;
    }
    return n;
}

static void cl_expand(const cl_seq *s, int pos, int variant, cl_exp *out) {
    out->len = 0;
    for (int i = 0; i < s->len; i++) {
        if (i != pos) {
            out->m[out->len++] = s->m[i];
            continue;
        }
        ct_move vars[CL_SPLIT_VARIANTS][2];
        int vn = cl_split_variants(s->m[i], vars);
        int pick = variant;
        if (pick >= vn) {
            pick = 0;
        }
        if (pick == 0) {
            out->m[out->len++] = s->m[i];
        } else {
            out->m[out->len++] = vars[pick][0];
            out->m[out->len++] = vars[pick][1];
        }
    }
}

static bool cl_try_pair(const cl_exp *e, const cl_seq *s, int k, int p, int q) {
    if (k + p + q > e->len) {
        return false;
    }
    if (2 * (k + p + q) < s->len) {
        return false;
    }
    ct_move t[CT_CLASSIFY_MOVES_MAX + CT_CLASSIFY_SETUP_MAX * 2 + CT_CLASSIFY_PART_MAX * 2];
    int n = 0;
    memcpy(t, e->m, (size_t)k * sizeof *t);
    n = k;
    memcpy(t + n, e->m + k, (size_t)(p + q) * sizeof *t);
    n += p + q;
    ct_move p_inv[CT_CLASSIFY_PART_MAX];
    ct_move q_inv[CT_CLASSIFY_PART_MAX];
    int p_inv_len = ct_transform_inverse(e->m + k, p, p_inv);
    int q_inv_len = ct_transform_inverse(e->m + k + p, q, q_inv);
    memcpy(t + n, p_inv, (size_t)p_inv_len * sizeof *t);
    n += p_inv_len;
    memcpy(t + n, q_inv, (size_t)q_inv_len * sizeof *t);
    n += q_inv_len;
    ct_move k_inv[CT_CLASSIFY_SETUP_MAX + 1];
    int k_inv_len = ct_transform_inverse(e->m, k, k_inv);
    memcpy(t + n, k_inv, (size_t)k_inv_len * sizeof *t);
    n += k_inv_len;
    ct_move simp[CT_CLASSIFY_MOVES_MAX];
    int simp_len = ct_transform_simplify(t, n, simp, CT_CLASSIFY_MOVES_MAX);
    if (simp_len < 0) {
        return false;
    }
    return cl_seq_equal(simp, simp_len, s->m, s->len);
}

static bool cl_try_cyclic(const cl_exp *e, const cl_seq *s, int k) {
    if (k + CL_CYCLIC_HEAD > e->len) {
        return false;
    }
    const ct_move *a = e->m + k;
    if (a[0].face == a[1].face) {
        return false;
    }
    if (e->m[k + 2].quarter != CT_MOVE_QUARTERS / 2) {
        return false;
    }
    const ct_move *c = e->m + k + CL_CYCLIC_C_OFF;
    if (!cl_move_same(c[0], a[1]) || !cl_move_same(c[1], a[0])) {
        return false;
    }
    ct_move t[CT_CLASSIFY_MOVES_MAX + CT_CLASSIFY_SETUP_MAX * 2 + CT_CLASSIFY_PART_MAX * 2];
    int n = 0;
    memcpy(t, e->m, (size_t)k * sizeof *t);
    n = k;
    memcpy(t + n, a, (size_t)2 * sizeof *t);
    n += 2;
    t[n++] = e->m[k + 2];
    ct_move a_inv[2];
    ct_transform_inverse(a, 2, a_inv);
    memcpy(t + n, a_inv, sizeof a_inv);
    n += 2;
    memcpy(t + n, c, (size_t)2 * sizeof *t);
    n += 2;
    t[n++] = e->m[k + 2];
    ct_move c_inv[2];
    ct_transform_inverse(c, 2, c_inv);
    memcpy(t + n, c_inv, sizeof c_inv);
    n += 2;
    ct_move k_inv[CT_CLASSIFY_SETUP_MAX + 1];
    int k_inv_len = ct_transform_inverse(e->m, k, k_inv);
    memcpy(t + n, k_inv, (size_t)k_inv_len * sizeof *t);
    n += k_inv_len;
    ct_move simp[CT_CLASSIFY_MOVES_MAX];
    int simp_len = ct_transform_simplify(t, n, simp, CT_CLASSIFY_MOVES_MAX);
    if (simp_len < 0) {
        return false;
    }
    return cl_seq_equal(simp, simp_len, s->m, s->len);
}

static void cl_fill_parts(ct_classify_result *r, const cl_exp *e, int k, int p, int q) {
    r->setup_len = k;
    memcpy(r->setup, e->m, (size_t)k * sizeof *r->setup);
    if (p == 1) {
        r->interchange_len = 1;
        r->interchange[0] = e->m[k];
        r->insertion_len = q;
        memcpy(r->insertion, e->m + k + 1, (size_t)q * sizeof *r->insertion);
        r->insert_first = false;
    } else {
        r->insertion_len = p;
        memcpy(r->insertion, e->m + k, (size_t)p * sizeof *r->insertion);
        r->interchange_len = q;
        memcpy(r->interchange, e->m + k + p, (size_t)q * sizeof *r->interchange);
        r->insert_first = true;
    }
}

static ct_classification cl_length_class(ct_classification cls, bool corner, bool edge, int n) {
    bool ok = false;
    switch (cls) {
    case CT_CLS_PURE:
        ok = n == CL_PURE_MOVES;
        break;
    case CT_CLS_A9:
        ok = n == CL_CONJ_MOVES;
        break;
    case CT_CLS_B9:
        ok = edge && n == CL_CONJ_MOVES;
        break;
    case CT_CLS_ORTHOGONAL:
        ok = n == CL_ORTH_MOVES;
        break;
    case CT_CLS_CYCLIC_SHIFT:
        ok = corner && n == CL_ORTH_MOVES;
        break;
    case CT_CLS_COLUMN:
        ok = corner && n == CT_CLASSIFY_COLUMN_MOVES;
        break;
    case CT_CLS_PER_SPECIAL:
        ok = corner && n == CL_SPECIAL_MOVES;
        break;
    case CT_CLS_HALF_SLICE_PLANE:
        ok = edge && n == CL_HSP_MOVES;
        break;
    case CT_CLS_SP5:
        ok = edge && n == CT_CLASSIFY_SP5_MOVES;
        break;
    case CT_CLS_SP7:
        ok = edge && n == CT_CLASSIFY_SP7_MOVES;
        break;
    case CT_CLS_SP9:
        ok = edge && n == CT_CLASSIFY_SP9_MOVES;
        break;
    default:
        ok = false;
        break;
    }
    return ok ? cls : CT_CLS_UNKNOWN;
}

static ct_classification cl_decide(const cl_exp *e, int k, int p, int q, int n,
                                   const cl_perm_info *info) {
    bool corner = info->corner && !info->edge;
    bool edge = info->edge && !info->corner;
    ct_classification structural = CT_CLS_UNKNOWN;
    if (p == CT_CLASSIFY_SPECIAL_LEN || q == CT_CLASSIFY_SPECIAL_LEN) {
        structural = CT_CLS_PER_SPECIAL;
    } else if (p == 1 && q == 1) {
        if (n == CT_CLASSIFY_SP5_MOVES) {
            structural = CT_CLS_SP5;
        } else if (n == CT_CLASSIFY_SP7_MOVES) {
            structural = CT_CLS_SP7;
        } else if (n == CT_CLASSIFY_SP9_MOVES) {
            structural = CT_CLS_SP9;
        } else {
            structural = CT_CLS_HALF_SLICE_PLANE;
        }
    } else {
        const ct_move *three = p == CT_CLASSIFY_INSERT_LEN ? e->m + k : e->m + k + 1;
        const ct_move *one = p == 1 ? e->m + k : e->m + k + CT_CLASSIFY_INSERT_LEN;
        bool sp_shape = cl_move_same(three[1], one[0]);
        bool tail_is_insert = q == CT_CLASSIFY_INSERT_LEN;
        if (k == 0) {
            structural = CT_CLS_PURE;
        } else if (sp_shape && n == CT_CLASSIFY_SP9_MOVES) {
            structural = CT_CLS_SP9;
        } else if (2 * (k + p + q) == n) {
            structural = CT_CLS_ORTHOGONAL;
        } else if (corner && n == CT_CLASSIFY_COLUMN_MOVES) {
            structural = CT_CLS_COLUMN;
        } else {
            structural = corner || tail_is_insert ? CT_CLS_A9 : CT_CLS_B9;
        }
    }
    return cl_length_class(structural, corner, edge, n);
}

static void cl_fmt(const ct_move *m, int len, char *out, unsigned cap) {
    int w = ct_notation_format(m, len, CT_CLASSIFY_N, out, cap);
    if (w < 0) {
        out[0] = '\0';
    }
}

static void cl_set_label(ct_classify_result *r) {
    snprintf(r->label, sizeof r->label, "%s", cl_labels[r->cls]);
}

static void cl_rationale(ct_classify_result *r, const cl_perm_info *info) {
    char setup[CT_CLASSIFY_FMT_MAX];
    char inter[CT_CLASSIFY_FMT_MAX];
    char insert[CT_CLASSIFY_FMT_MAX];
    cl_fmt(r->setup, r->setup_len, setup, sizeof setup);
    cl_fmt(r->interchange, r->interchange_len, inter, sizeof inter);
    cl_fmt(r->insertion, r->insertion_len, insert, sizeof insert);
    switch (r->cls) {
    case CT_CLS_PURE:
        snprintf(r->rationale, sizeof r->rationale,
                 "pure commutator A B A' B': interchange %s, insertion %s", inter, insert);
        break;
    case CT_CLS_A9:
        snprintf(r->rationale, sizeof r->rationale,
                 "A9: setup %s cancels into the insert, interchange %s, insertion %s", setup, inter,
                 insert);
        break;
    case CT_CLS_B9:
        snprintf(r->rationale, sizeof r->rationale,
                 "B9: setup %s cancels into the interchange, interchange %s, insertion %s", setup,
                 inter, insert);
        break;
    case CT_CLS_ORTHOGONAL:
        snprintf(r->rationale, sizeof r->rationale,
                 "orthogonal: conjugated pure commutator without cancellation, setup %s", setup);
        break;
    case CT_CLS_CYCLIC_SHIFT:
        snprintf(r->rationale, sizeof r->rationale,
                 "cyclic shift A B A' C B C': half turn %s, shift pair %s", inter, insert);
        break;
    case CT_CLS_COLUMN:
        snprintf(r->rationale, sizeof r->rationale,
                 "column: setup %s into a canceled commutator, interchange %s, insertion %s", setup,
                 inter, insert);
        break;
    case CT_CLS_PER_SPECIAL:
        snprintf(r->rationale, sizeof r->rationale,
                 "per special: half turn %s, five move insertion %s", inter, insert);
        break;
    case CT_CLS_HALF_SLICE_PLANE:
        snprintf(r->rationale, sizeof r->rationale,
                 "half slice-plane: single move interchange %s and insertion %s", inter, insert);
        break;
    case CT_CLS_SP5:
    case CT_CLS_SP7:
    case CT_CLS_SP9:
        snprintf(r->rationale, sizeof r->rationale,
                 "%s: conjugated half slice-plane, setup %s, interchange %s, insertion %s",
                 cl_labels[r->cls], setup, inter, insert);
        break;
    case CT_CLS_PARITY:
        snprintf(r->rationale, sizeof r->rationale,
                 "parity: one corner pair and one edge pair "
                 "swapped with orientation preserved");
        break;
    case CT_CLS_FLIP:
        snprintf(r->rationale, sizeof r->rationale, "flip: %d edge pieces flipped in place",
                 info->flipped);
        break;
    case CT_CLS_TWIST:
        snprintf(r->rationale, sizeof r->rationale, "twist: %d corner pieces twisted in place",
                 info->twisted);
        break;
    default:
        snprintf(r->rationale, sizeof r->rationale,
                 "no commutator, parity, flip, or twist structure matched");
        break;
    }
}

int ct_classify(const ct_move *seq, int count, ct_classify_result *out) {
    if (seq == NULL || out == NULL || count < 0) {
        return CT_CLASSIFY_E_ARG;
    }
    memset(out, 0, sizeof *out);
    out->cls = CT_CLS_UNKNOWN;
    cl_cells_init();
    if (count == 0) {
        cl_set_label(out);
        snprintf(out->rationale, sizeof out->rationale, "empty sequence");
        return CT_CLASSIFY_OK;
    }
    if (count > CT_CLASSIFY_MOVES_MAX) {
        cl_set_label(out);
        snprintf(out->rationale, sizeof out->rationale, "sequence longer than the classify bound");
        return CT_CLASSIFY_OK;
    }
    cl_seq s;
    int len = ct_transform_simplify(seq, count, s.m, CT_CLASSIFY_MOVES_MAX);
    if (len < 0) {
        cl_set_label(out);
        snprintf(out->rationale, sizeof out->rationale, "invalid move in sequence");
        return CT_CLASSIFY_OK;
    }
    s.len = len;
    out->moves = s.len;
    out->qtm = ct_count_qtm(s.m, s.len);
    uint8_t st[CT_CLASSIFY_CELLS];
    cl_simulate(s.m, s.len, st);
    cl_perm_info info;
    cl_perm_scan(st, &info);
    if (cl_is_parity(st)) {
        out->cls = CT_CLS_PARITY;
    } else if (cl_is_flip(st)) {
        out->cls = CT_CLS_FLIP;
    } else if (cl_is_twist(st)) {
        out->cls = CT_CLS_TWIST;
    } else {
        bool corner_cycle = info.corner && !info.edge && info.corner_exact;
        bool edge_cycle = info.edge && !info.corner && info.edge_exact;
        bool found = false;
        if (corner_cycle || edge_cycle) {
            cl_exp plain;
            plain.len = s.len;
            memcpy(plain.m, s.m, (size_t)s.len * sizeof *s.m);
            cl_exp exps[CT_CLASSIFY_MOVES_MAX * 2 + 1];
            int exp_n = 0;
            exps[exp_n++] = plain;
            for (int pos = 0; pos < s.len && exp_n < (int)(sizeof exps / sizeof exps[0]); pos++) {
                ct_move vars[CL_SPLIT_VARIANTS][2];
                int vn = cl_split_variants(s.m[pos], vars);
                for (int v = 1; v < vn; v++) {
                    cl_exp e;
                    cl_expand(&s, pos, v, &e);
                    exps[exp_n++] = e;
                }
            }
            for (int ei = 0; ei < exp_n && !found; ei++) {
                const cl_exp *e = &exps[ei];
                for (int k = 0; k <= CT_CLASSIFY_SETUP_MAX && !found; k++) {
                    for (int pi = 0; pi < CL_PAIR_COUNT && !found; pi++) {
                        if (pi == CL_CYCLIC_SLOT) {
                            if (cl_try_cyclic(e, &s, k) &&
                                cl_length_class(CT_CLS_CYCLIC_SHIFT, corner_cycle, edge_cycle,
                                                s.len) == CT_CLS_CYCLIC_SHIFT) {
                                out->setup_len = k;
                                memcpy(out->setup, e->m, (size_t)k * sizeof *out->setup);
                                out->insertion_len = 2;
                                out->insertion[0] = e->m[k];
                                out->insertion[1] = e->m[k + 1];
                                out->interchange_len = 1;
                                out->interchange[0] = e->m[k + 2];
                                out->cyclic = true;
                                out->cls = CT_CLS_CYCLIC_SHIFT;
                                found = true;
                                break;
                            }
                        }
                        if (cl_try_pair(e, &s, k, cl_pairs[pi].p, cl_pairs[pi].q)) {
                            ct_classification cls =
                                cl_decide(e, k, cl_pairs[pi].p, cl_pairs[pi].q, s.len, &info);
                            if (cls == CT_CLS_UNKNOWN) {
                                continue;
                            }
                            cl_fill_parts(out, e, k, cl_pairs[pi].p, cl_pairs[pi].q);
                            out->cls = cls;
                            found = true;
                        }
                    }
                }
            }
        }
    }
    cl_set_label(out);
    cl_rationale(out, &info);
    return CT_CLASSIFY_OK;
}

int ct_classify_alg(const char *alg, ct_classify_result *out) {
    if (alg == NULL || out == NULL) {
        return CT_CLASSIFY_E_ARG;
    }
    ct_move seq[CT_CLASSIFY_MOVES_MAX];
    int n = ct_notation_parse(alg, CT_CLASSIFY_N, seq, CT_CLASSIFY_MOVES_MAX);
    if (n < 0) {
        return n;
    }
    return ct_classify(seq, n, out);
}

const char *ct_classification_label(ct_classification cls) {
    if (cls < CT_CLS_PURE || cls > CT_CLS_UNKNOWN) {
        return cl_labels[CT_CLS_UNKNOWN];
    }
    return cl_labels[cls];
}

const char *ct_classification_alias(ct_classification cls) {
    if (cls < CT_CLS_PURE || cls > CT_CLS_UNKNOWN) {
        return cl_aliases[CT_CLS_UNKNOWN];
    }
    return cl_aliases[cls];
}

ct_classification ct_classification_from_label(const char *label) {
    if (label == NULL) {
        return CT_CLS_UNKNOWN;
    }
    for (int i = CT_CLS_PURE; i <= CT_CLS_UNKNOWN; i++) {
        if (strcmp(label, cl_labels[i]) == 0) {
            return (ct_classification)i;
        }
    }
    return CT_CLS_UNKNOWN;
}
