#include "gen/synth.h"

#include "engine/transform.h"
#include "gen/interchange.h"
#include "gen/score.h"
#include "gen/targets.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t key[CT_SYNTH_KEY_FIELDS];
    uint8_t len;
    ct_move seq[CT_SYNTH_TPL_MAX];
} ct_synth_entry;

typedef struct {
    uint8_t state[CT_SYNTH_CELLS];
    int parent;
    ct_move move;
} ct_synth_node;

typedef struct {
    ct_synth_node *nodes;
    int count;
    int *hash;
} ct_synth_side;

typedef struct {
    ct_move seq[CT_SYNTH_MAX_MOVES];
    int len;
    bool setup;
} ct_synth_cand;

typedef struct {
    ct_move moves[CT_CUBE_FACE_COUNT * (CT_MOVE_QUARTERS - 1)];
    int move_n;
    ct_cube cube;
    ct_move path[CT_SYNTH_SETUP_MAX];
    int depth;
    int max_depth;
    int orbit;
    int cb;
    int c1;
    int c2;
    ct_synth_cand *cands;
    int cand_cap;
    int cand_n;
    bool hit;
} ct_synth_setup_ctx;

static ct_synth_entry *synth_entries;
static int synth_hash[CT_SYNTH_HASH_MAX];
static int synth_count;
static bool synth_built;
static bool synth_corner_cell[CT_SYNTH_CELLS];
static bool synth_edge_cell[CT_SYNTH_CELLS];
static int synth_corner_sticker_of[CT_SYNTH_CELLS];
static int synth_edge_sticker_of[CT_SYNTH_CELLS];

static const unsigned synth_face_bits[CT_CUBE_FACE_COUNT] = {
    CT_MOVE_BIT_U, CT_MOVE_BIT_L, CT_MOVE_BIT_F, CT_MOVE_BIT_R, CT_MOVE_BIT_B, CT_MOVE_BIT_D,
};

static int synth_raw_of(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int synth_compact_of(int face, int row, int col) {
    return face * CT_SYNTH_N * CT_SYNTH_N + row * CT_SYNTH_N + col;
}

static void synth_value_identity(ct_cube *c) {
    memset(c, 0, sizeof *c);
    c->n = CT_SYNTH_N;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_SYNTH_N; row++) {
            for (int col = 0; col < CT_SYNTH_N; col++) {
                c->facelet[synth_raw_of(face, row, col)] =
                    (uint8_t)synth_compact_of(face, row, col);
            }
        }
    }
}

static void synth_read_state(const ct_cube *c, uint8_t state[CT_SYNTH_CELLS]) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_SYNTH_N; row++) {
            for (int col = 0; col < CT_SYNTH_N; col++) {
                state[synth_compact_of(face, row, col)] = c->facelet[synth_raw_of(face, row, col)];
            }
        }
    }
}

static void synth_apply_seq(ct_cube *c, const ct_move *seq, int len) {
    for (int i = 0; i < len; i++) {
        ct_apply(c, seq[i]);
    }
}

static void synth_cycle_key(int x, int y, int z, int sorted[CT_SYNTH_CYCLE_LEN], int *dir) {
    sorted[0] = x;
    sorted[1] = y;
    sorted[2] = z;
    for (int i = 1; i < CT_SYNTH_CYCLE_LEN; i++) {
        int cur = sorted[i];
        int j = i - 1;
        while (j >= 0 && sorted[j] > cur) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = cur;
    }
    int next = sorted[0] == x ? y : (sorted[0] == y ? z : x);
    *dir = next == sorted[1] ? 0 : 1;
}

static uint32_t synth_key_hash(int a, int b, int c, int dir) {
    uint32_t h = (uint32_t)dir;
    h = h * (uint32_t)CT_SYNTH_CELLS + (uint32_t)c;
    h = h * (uint32_t)CT_SYNTH_CELLS + (uint32_t)b;
    h = h * (uint32_t)CT_SYNTH_CELLS + (uint32_t)a;
    return h;
}

static int synth_slot(int a, int b, int c, int dir) {
    uint32_t h = synth_key_hash(a, b, c, dir) % (uint32_t)CT_SYNTH_HASH_MAX;
    while (synth_hash[h] != 0) {
        const ct_synth_entry *e = &synth_entries[synth_hash[h] - 1];
        if (e->key[0] == (uint8_t)a && e->key[1] == (uint8_t)b && e->key[2] == (uint8_t)c &&
            e->key[3] == (uint8_t)dir) {
            break;
        }
        h = (h + 1) % (uint32_t)CT_SYNTH_HASH_MAX;
    }
    return (int)h;
}

static bool synth_move_less(const ct_move *x, const ct_move *y) {
    if (x->face != y->face) {
        return x->face < y->face;
    }
    if (x->depth_from != y->depth_from) {
        return x->depth_from < y->depth_from;
    }
    if (x->depth_to != y->depth_to) {
        return x->depth_to < y->depth_to;
    }
    return x->quarter < y->quarter;
}

static bool synth_seq_better(const ct_move *a, int alen, const ct_move *b, int blen) {
    if (alen != blen) {
        return alen < blen;
    }
    for (int i = 0; i < alen; i++) {
        if (synth_move_less(&a[i], &b[i])) {
            return true;
        }
        if (synth_move_less(&b[i], &a[i])) {
            return false;
        }
    }
    return false;
}

static void synth_table_insert(int a, int b, int c, int dir, const ct_move *seq, int len) {
    int slot = synth_slot(a, b, c, dir);
    if (synth_hash[slot] == 0) {
        if (synth_count >= CT_SYNTH_KEY_MAX) {
            return;
        }
        ct_synth_entry *e = &synth_entries[synth_count];
        e->key[0] = (uint8_t)a;
        e->key[1] = (uint8_t)b;
        e->key[2] = (uint8_t)c;
        e->key[3] = (uint8_t)dir;
        e->len = (uint8_t)len;
        memcpy(e->seq, seq, (size_t)len * sizeof *seq);
        synth_hash[slot] = synth_count + 1;
        synth_count++;
        return;
    }
    ct_synth_entry *e = &synth_entries[synth_hash[slot] - 1];
    if (synth_seq_better(seq, len, e->seq, e->len)) {
        e->len = (uint8_t)len;
        memcpy(e->seq, seq, (size_t)len * sizeof *seq);
    }
}

static const ct_synth_entry *synth_table_find(int a, int b, int c, int dir) {
    int slot = synth_slot(a, b, c, dir);
    if (synth_hash[slot] == 0) {
        return NULL;
    }
    return &synth_entries[synth_hash[slot] - 1];
}

static void synth_analyze(const ct_move *seq, int len) {
    ct_cube cube;
    synth_value_identity(&cube);
    synth_apply_seq(&cube, seq, len);
    uint8_t state[CT_SYNTH_CELLS];
    synth_read_state(&cube, state);
    int moved[CT_SYNTH_CELLS];
    int moved_n = 0;
    bool corner = true;
    bool edge = true;
    for (int q = 0; q < CT_SYNTH_CELLS; q++) {
        if (state[q] == (uint8_t)q) {
            continue;
        }
        moved[moved_n++] = q;
        corner = corner && synth_corner_cell[q];
        edge = edge && synth_edge_cell[q];
    }
    if (moved_n == 0 || corner == edge) {
        return;
    }
    int per = corner ? CT_TARGET_CORNER_STICKERS_PER_PIECE : CT_TARGET_EDGE_STICKERS_PER_PIECE;
    if (moved_n != per * CT_SYNTH_CYCLE_LEN) {
        return;
    }
    int perm[CT_SYNTH_CELLS];
    for (int q = 0; q < CT_SYNTH_CELLS; q++) {
        perm[state[q]] = q;
    }
    bool visited[CT_SYNTH_CELLS] = {false};
    for (int i = 0; i < moved_n; i++) {
        int start = moved[i];
        if (visited[start]) {
            continue;
        }
        int cyc[CT_SYNTH_CYCLE_LEN];
        int cur = start;
        int cycle_len = 0;
        while (cycle_len < CT_SYNTH_CYCLE_LEN && !visited[cur]) {
            visited[cur] = true;
            cyc[cycle_len] = cur;
            cycle_len++;
            cur = perm[cur];
        }
        if (cycle_len != CT_SYNTH_CYCLE_LEN || cur != start) {
            return;
        }
        int sorted[CT_SYNTH_CYCLE_LEN];
        int dir = 0;
        synth_cycle_key(cyc[0], cyc[1], cyc[2], sorted, &dir);
        synth_table_insert(sorted[0], sorted[1], sorted[2], dir, seq, len);
    }
}

static void synth_try_pair(const ct_move *a, const ct_move *ins, int ins_len, bool a_first) {
    ct_move inv_ins[CT_SYNTH_TPL_MAX];
    ct_transform_inverse(ins, ins_len, inv_ins);
    ct_move seq[CT_SYNTH_TPL_MAX];
    int len = 0;
    if (a_first) {
        seq[len++] = *a;
        memcpy(seq + len, ins, (size_t)ins_len * sizeof *ins);
        len += ins_len;
        seq[len++] = ct_move_inverse(*a);
        memcpy(seq + len, inv_ins, (size_t)ins_len * sizeof *ins);
        len += ins_len;
    } else {
        memcpy(seq + len, ins, (size_t)ins_len * sizeof *ins);
        len += ins_len;
        seq[len++] = *a;
        memcpy(seq + len, inv_ins, (size_t)ins_len * sizeof *ins);
        len += ins_len;
        seq[len++] = ct_move_inverse(*a);
    }
    synth_analyze(seq, len);
}

static bool synth_dedup_perms(const ct_move *x, const ct_move *y) {
    uint8_t px[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE];
    uint8_t py[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE];
    if (ct_sticker_perm(*x, CT_SYNTH_N, px) != 0) {
        return false;
    }
    if (ct_sticker_perm(*y, CT_SYNTH_N, py) != 0) {
        return false;
    }
    return memcmp(px, py, (size_t)CT_SYNTH_CELLS) == 0;
}

static void synth_init_cells(void) {
    for (int q = 0; q < CT_SYNTH_CELLS; q++) {
        synth_corner_cell[q] = false;
        synth_edge_cell[q] = false;
        synth_corner_sticker_of[q] = CT_TARGET_NONE;
        synth_edge_sticker_of[q] = CT_TARGET_NONE;
    }
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = CT_TARGET_NONE;
        int row = CT_TARGET_NONE;
        int col = CT_TARGET_NONE;
        ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
        int corner_idx = synth_compact_of(face, row, col);
        synth_corner_cell[corner_idx] = true;
        synth_corner_sticker_of[corner_idx] = s;
        ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
        int edge_idx = synth_compact_of(face, row, col);
        synth_edge_cell[edge_idx] = true;
        synth_edge_sticker_of[edge_idx] = s;
    }
}

static int synth_build(void) {
    synth_init_cells();
    ct_move cands[CT_INTERCHANGE_MAX_CANDIDATES];
    int cand_n = ct_candidate_moves(CT_SYNTH_N, cands, CT_INTERCHANGE_MAX_CANDIDATES);
    if (cand_n <= 0) {
        return -1;
    }
    bool keep[CT_INTERCHANGE_MAX_CANDIDATES];
    int uniq_n = 0;
    int uniq[CT_INTERCHANGE_MAX_CANDIDATES];
    for (int i = 0; i < cand_n; i++) {
        keep[i] = true;
        for (int j = 0; j < i; j++) {
            if (!keep[j]) {
                continue;
            }
            if (synth_dedup_perms(&cands[i], &cands[j])) {
                keep[i] = false;
                break;
            }
        }
        if (keep[i]) {
            uniq[uniq_n++] = i;
        }
    }
    for (int ai = 0; ai < uniq_n; ai++) {
        const ct_move *a = &cands[uniq[ai]];
        for (int ii = 0; ii < uniq_n; ii++) {
            synth_try_pair(a, &cands[uniq[ii]], 1, true);
            synth_try_pair(a, &cands[uniq[ii]], 1, false);
        }
        for (int xi = 0; xi < uniq_n; xi++) {
            const ct_move *x = &cands[uniq[xi]];
            ct_move x_inv = ct_move_inverse(*x);
            ct_move ins[CT_SYNTH_INSERT_LEN];
            ins[0] = *x;
            for (int yi = 0; yi < uniq_n; yi++) {
                if (cands[uniq[yi]].face == x->face) {
                    continue;
                }
                ct_move y = cands[uniq[yi]];
                ins[1] = y;
                ins[CT_SYNTH_TWO] = x_inv;
                synth_try_pair(a, ins, CT_SYNTH_CYCLE_LEN, true);
                synth_try_pair(a, ins, CT_SYNTH_CYCLE_LEN, false);
                ins[CT_SYNTH_TWO] = x_inv;
                ins[CT_SYNTH_CYCLE_LEN] = y;
                ins[CT_SYNTH_INSERT_LEN - CT_SYNTH_ONE] = x_inv;
                synth_try_pair(a, ins, CT_SYNTH_INSERT_LEN, true);
                synth_try_pair(a, ins, CT_SYNTH_INSERT_LEN, false);
                ins[CT_SYNTH_TWO] = *x;
                ins[CT_SYNTH_INSERT_LEN - CT_SYNTH_ONE] = *x;
                synth_try_pair(a, ins, CT_SYNTH_INSERT_LEN, true);
                synth_try_pair(a, ins, CT_SYNTH_INSERT_LEN, false);
            }
        }
    }
    return synth_count;
}

int ct_synth_init(void) {
    if (synth_built) {
        return synth_count;
    }
    synth_entries = calloc((size_t)CT_SYNTH_KEY_MAX, sizeof *synth_entries);
    if (synth_entries == NULL) {
        return -1;
    }
    memset(synth_hash, 0, sizeof synth_hash);
    synth_count = 0;
    if (synth_build() < 0) {
        free(synth_entries);
        synth_entries = NULL;
        return -1;
    }
    synth_built = true;
    return synth_count;
}

int ct_synth_table_size(void) {
    if (!synth_built) {
        return -1;
    }
    return synth_count;
}

int ct_synth_table_entry(int index, int key[CT_SYNTH_KEY_FIELDS], ct_move *out, int cap) {
    if (!synth_built || index < 0 || index >= synth_count || key == NULL) {
        return -1;
    }
    const ct_synth_entry *e = &synth_entries[index];
    if (out == NULL || cap < (int)e->len) {
        return -1;
    }
    for (int i = 0; i < CT_SYNTH_KEY_FIELDS; i++) {
        key[i] = e->key[i];
    }
    memcpy(out, e->seq, (size_t)e->len * sizeof *out);
    return e->len;
}

static int synth_sticker_cell_of(int orbit, int sticker) {
    int face = CT_TARGET_NONE;
    int row = CT_TARGET_NONE;
    int col = CT_TARGET_NONE;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return synth_compact_of(face, row, col);
}

static int synth_piece_of(int orbit, int sticker) {
    return orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(sticker) : ct_edge_piece_of(sticker);
}

static bool synth_case_valid(int orbit, int buffer, int t1, int t2) {
    if (orbit != CT_ORBIT_CORNER && orbit != CT_ORBIT_EDGE) {
        return false;
    }
    if (buffer < 0 || t1 < 0 || t2 < 0) {
        return false;
    }
    if (buffer >= CT_TARGET_STICKERS || t1 >= CT_TARGET_STICKERS || t2 >= CT_TARGET_STICKERS) {
        return false;
    }
    int pb = synth_piece_of(orbit, buffer);
    int p1 = synth_piece_of(orbit, t1);
    int p2 = synth_piece_of(orbit, t2);
    return pb != p1 && p1 != p2 && pb != p2;
}

static bool synth_validate(const ct_move *seq, int len, int orbit, int cb, int c1, int c2) {
    ct_cube cube;
    synth_value_identity(&cube);
    synth_apply_seq(&cube, seq, len);
    uint8_t state[CT_SYNTH_CELLS];
    synth_read_state(&cube, state);
    if (state[c1] != (uint8_t)cb || state[c2] != (uint8_t)c1 || state[cb] != (uint8_t)c2) {
        return false;
    }
    bool on_pieces[CT_SYNTH_CELLS] = {false};
    int key_cells[CT_SYNTH_CYCLE_LEN] = {cb, c1, c2};
    for (int i = 0; i < CT_SYNTH_CYCLE_LEN; i++) {
        int sticker = synth_corner_sticker_of[key_cells[i]];
        if (orbit == CT_ORBIT_EDGE) {
            sticker = synth_edge_sticker_of[key_cells[i]];
        }
        if (sticker == CT_TARGET_NONE) {
            return false;
        }
        int piece = synth_piece_of(orbit, sticker);
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            if (synth_piece_of(orbit, s) == piece) {
                on_pieces[synth_sticker_cell_of(orbit, s)] = true;
            }
        }
    }
    for (int q = 0; q < CT_SYNTH_CELLS; q++) {
        if (state[q] != (uint8_t)q && !on_pieces[q]) {
            return false;
        }
    }
    return true;
}

static bool synth_fill(ct_synth_result *out, const ct_config *cfg, int orbit, const ct_move *seq,
                       int len, bool setup, bool fallback) {
    ct_score_parts parts;
    if (ct_score_alg(seq, len, cfg, orbit, &parts) != CT_SCORE_OK) {
        return false;
    }
    memcpy(out->seq, seq, (size_t)len * sizeof *seq);
    out->len = len;
    out->qtm = parts.qtm;
    out->subset_penalty = parts.subset_penalty;
    out->found = true;
    out->used_setup = setup;
    out->used_fallback = fallback;
    return true;
}

static void synth_cand_add(ct_synth_cand *cands, int *cand_n, int cand_cap, const ct_move *seq,
                           int len, bool setup) {
    if (cands == NULL || *cand_n >= cand_cap) {
        return;
    }
    memcpy(cands[*cand_n].seq, seq, (size_t)len * sizeof *seq);
    cands[*cand_n].len = len;
    cands[*cand_n].setup = setup;
    (*cand_n)++;
}

static bool synth_lookup_seq(int x, int y, int z, ct_move *out, int *out_len) {
    int sorted[CT_SYNTH_CYCLE_LEN];
    int dir = 0;
    synth_cycle_key(x, y, z, sorted, &dir);
    for (int k = 0; k < CT_SYNTH_TWO; k++) {
        int want = k == 0 ? dir : dir ^ 1;
        const ct_synth_entry *e = synth_table_find(sorted[0], sorted[1], sorted[2], want);
        if (e == NULL) {
            continue;
        }
        if (want == dir) {
            memcpy(out, e->seq, (size_t)e->len * sizeof *out);
            *out_len = e->len;
        } else {
            *out_len = ct_transform_inverse(e->seq, e->len, out);
        }
        return *out_len > 0;
    }
    return false;
}

static int synth_subset_moves(unsigned mask, ct_move *out) {
    int n = 0;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        if ((mask & synth_face_bits[face]) == 0) {
            continue;
        }
        for (int quarter = 1; quarter < CT_MOVE_QUARTERS; quarter++) {
            out[n++] = (ct_move){face, 1, 1, quarter};
        }
    }
    return n;
}

static void synth_setup_try(ct_synth_setup_ctx *ctx) {
    uint8_t state[CT_SYNTH_CELLS];
    synth_read_state(&ctx->cube, state);
    int targets[CT_SYNTH_CYCLE_LEN] = {ctx->cb, ctx->c1, ctx->c2};
    int q[CT_SYNTH_CYCLE_LEN];
    for (int i = 0; i < CT_SYNTH_CYCLE_LEN; i++) {
        q[i] = -1;
        for (int cell = 0; cell < CT_SYNTH_CELLS; cell++) {
            if (state[cell] == (uint8_t)targets[i]) {
                q[i] = cell;
                break;
            }
        }
        if (q[i] < 0) {
            return;
        }
    }
    ct_move inner[CT_SYNTH_TPL_MAX];
    int inner_len = 0;
    if (!synth_lookup_seq(q[0], q[1], q[2], inner, &inner_len)) {
        return;
    }
    ct_move assembled[CT_SYNTH_MAX_MOVES];
    int assembled_len = ct_transform_conjugate(ctx->path, ctx->depth, inner, inner_len, assembled,
                                               CT_SYNTH_MAX_MOVES);
    if (assembled_len < 0) {
        return;
    }
    ct_move simplified[CT_SYNTH_MAX_MOVES];
    int simplified_len =
        ct_transform_simplify(assembled, assembled_len, simplified, CT_SYNTH_MAX_MOVES);
    if (simplified_len < 0) {
        return;
    }
    if (!synth_validate(simplified, simplified_len, ctx->orbit, ctx->cb, ctx->c1, ctx->c2)) {
        return;
    }
    synth_cand_add(ctx->cands, &ctx->cand_n, ctx->cand_cap, simplified, simplified_len, true);
    if (ctx->cand_n >= ctx->cand_cap) {
        ctx->hit = true;
    }
}

static void synth_setup_dfs(ct_synth_setup_ctx *ctx, int depth) {
    if (ctx->hit) {
        return;
    }
    if (depth > 0) {
        ctx->depth = depth;
        synth_setup_try(ctx);
        if (ctx->hit) {
            return;
        }
    }
    if (depth >= ctx->max_depth) {
        return;
    }
    for (int i = 0; i < ctx->move_n && !ctx->hit; i++) {
        if (depth > 0 && ctx->path[depth - 1].face == ctx->moves[i].face) {
            continue;
        }
        ctx->path[depth] = ctx->moves[i];
        ct_apply(&ctx->cube, ctx->moves[i]);
        synth_setup_dfs(ctx, depth + 1);
        ct_apply(&ctx->cube, ct_move_inverse(ctx->moves[i]));
    }
}

static void synth_setup_search(int orbit, int cb, int c1, int c2, const ct_move *moves, int move_n,
                               int max_depth, ct_synth_cand *cands, int cand_cap, int *cand_n) {
    ct_synth_setup_ctx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.move_n = move_n;
    memcpy(ctx.moves, moves, (size_t)move_n * sizeof *moves);
    synth_value_identity(&ctx.cube);
    ctx.orbit = orbit;
    ctx.cb = cb;
    ctx.c1 = c1;
    ctx.c2 = c2;
    ctx.cands = cands;
    ctx.cand_cap = cand_cap;
    ctx.cand_n = *cand_n;
    for (int depth = 1; depth <= max_depth && !ctx.hit; depth++) {
        ctx.max_depth = depth;
        synth_setup_dfs(&ctx, 0);
    }
    *cand_n = ctx.cand_n;
}

enum { CT_SYNTH_STATE_MULT = (1u << (CT_SYNTH_TWO + CT_SYNTH_TWO + CT_SYNTH_ONE)) - CT_SYNTH_ONE };

static uint32_t synth_state_hash(const uint8_t *state) {
    uint32_t h = 0;
    for (int i = 0; i < CT_SYNTH_CELLS; i++) {
        h = h * CT_SYNTH_STATE_MULT + state[i];
    }
    return h;
}

static int synth_side_slot(const ct_synth_side *side, const uint8_t *state) {
    uint32_t h = synth_state_hash(state) % (uint32_t)CT_SYNTH_FALLBACK_HASH;
    while (side->hash[h] != 0) {
        if (memcmp(side->nodes[side->hash[h] - 1].state, state, (size_t)CT_SYNTH_CELLS) == 0) {
            break;
        }
        h = (h + 1) % (uint32_t)CT_SYNTH_FALLBACK_HASH;
    }
    return (int)h;
}

static int synth_variant_states(int orbit, int cb, int c1, int c2,
                                uint8_t states[][CT_SYNTH_CELLS]) {
    int key_cells[CT_SYNTH_CYCLE_LEN] = {cb, c1, c2};
    int stickers[CT_SYNTH_CYCLE_LEN];
    int mates[CT_SYNTH_CYCLE_LEN][CT_TARGET_CORNER_STICKERS_PER_PIECE];
    int mate_n[CT_SYNTH_CYCLE_LEN];
    for (int i = 0; i < CT_SYNTH_CYCLE_LEN; i++) {
        const int *sticker_of =
            orbit == CT_ORBIT_CORNER ? synth_corner_sticker_of : synth_edge_sticker_of;
        stickers[i] = sticker_of[key_cells[i]];
        if (stickers[i] == CT_TARGET_NONE) {
            return 0;
        }
    }
    for (int i = 0; i < CT_SYNTH_CYCLE_LEN; i++) {
        int piece = synth_piece_of(orbit, stickers[i]);
        mate_n[i] = 0;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            if (synth_piece_of(orbit, s) != piece || s == stickers[i]) {
                continue;
            }
            mates[i][mate_n[i]++] = s;
        }
    }
    int variants = 1;
    for (int i = 0; i < CT_SYNTH_CYCLE_LEN; i++) {
        if (mate_n[i] > 1) {
            variants *= CT_SYNTH_TWO;
        }
    }
    for (int v = 0; v < variants; v++) {
        uint8_t *state = states[v];
        for (int q = 0; q < CT_SYNTH_CELLS; q++) {
            state[q] = (uint8_t)q;
        }
        state[c1] = (uint8_t)cb;
        state[c2] = (uint8_t)c1;
        state[cb] = (uint8_t)c2;
        int bits = v;
        for (int i = 0; i < CT_SYNTH_CYCLE_LEN; i++) {
            int next = (i + 1) % CT_SYNTH_CYCLE_LEN;
            int flip = 0;
            if (mate_n[i] > 1) {
                flip = bits % CT_SYNTH_TWO;
                bits /= CT_SYNTH_TWO;
            }
            for (int j = 0; j < mate_n[i]; j++) {
                int dst_j = flip == 1 ? mate_n[i] - 1 - j : j;
                state[synth_sticker_cell_of(orbit, mates[next][dst_j])] =
                    (uint8_t)synth_sticker_cell_of(orbit, mates[i][j]);
            }
        }
    }
    return variants;
}

static void synth_path_to_root(const ct_synth_side *side, int node, ct_move *out, int *out_len) {
    ct_move rev[CT_SYNTH_FALLBACK_DEPTH_MAX];
    int len = 0;
    int cur = node;
    while (side->nodes[cur].parent >= 0) {
        rev[len++] = side->nodes[cur].move;
        cur = side->nodes[cur].parent;
    }
    for (int i = 0; i < len; i++) {
        out[i] = rev[len - 1 - i];
    }
    *out_len = len;
}

static bool synth_fallback(int orbit, int cb, int c1, int c2, const ct_move *moves, int move_n,
                           int max_depth, ct_move *out, int *out_len) {
    uint8_t perms[CT_CUBE_FACE_COUNT * (CT_MOVE_QUARTERS - 1)][CT_SYNTH_CELLS];
    uint8_t perm_inv[CT_CUBE_FACE_COUNT * (CT_MOVE_QUARTERS - 1)][CT_SYNTH_CELLS];
    for (int i = 0; i < move_n; i++) {
        ct_cube cube;
        synth_value_identity(&cube);
        ct_apply(&cube, moves[i]);
        synth_read_state(&cube, perms[i]);
        for (int s = 0; s < CT_SYNTH_CELLS; s++) {
            perm_inv[i][perms[i][s]] = (uint8_t)s;
        }
    }
    ct_synth_side fwd;
    ct_synth_side bwd;
    memset(&fwd, 0, sizeof fwd);
    memset(&bwd, 0, sizeof bwd);
    fwd.nodes = malloc((size_t)CT_SYNTH_FALLBACK_NODES * sizeof *fwd.nodes);
    bwd.nodes = malloc((size_t)CT_SYNTH_FALLBACK_NODES * sizeof *bwd.nodes);
    fwd.hash = calloc((size_t)CT_SYNTH_FALLBACK_HASH, sizeof *fwd.hash);
    bwd.hash = calloc((size_t)CT_SYNTH_FALLBACK_HASH, sizeof *bwd.hash);
    bool ok = fwd.nodes != NULL && bwd.nodes != NULL && fwd.hash != NULL && bwd.hash != NULL;
    bool found = false;
    if (ok) {
        uint8_t variants[CT_SYNTH_EIGHT][CT_SYNTH_CELLS];
        int variant_n = synth_variant_states(orbit, cb, c1, c2, variants);
        ok = variant_n > 0;
        if (ok) {
            fwd.count = 0;
            bwd.count = 0;
            for (int q = 0; q < CT_SYNTH_CELLS; q++) {
                fwd.nodes[0].state[q] = (uint8_t)q;
            }
            fwd.nodes[0].parent = -1;
            fwd.nodes[0].move = (ct_move){0, 0, 0, 0};
            fwd.hash[synth_side_slot(&fwd, fwd.nodes[0].state)] = 1;
            fwd.count = 1;
            for (int v = 0; v < variant_n; v++) {
                int slot = synth_side_slot(&bwd, variants[v]);
                if (bwd.hash[slot] != 0) {
                    continue;
                }
                memcpy(bwd.nodes[bwd.count].state, variants[v], (size_t)CT_SYNTH_CELLS);
                bwd.nodes[bwd.count].parent = -1;
                bwd.nodes[bwd.count].move = (ct_move){0, 0, 0, 0};
                bwd.hash[slot] = bwd.count + 1;
                bwd.count++;
            }
            int frontier_cap = CT_SYNTH_FALLBACK_NODES;
            int *fwd_frontier = malloc((size_t)frontier_cap * sizeof *fwd_frontier);
            int *bwd_frontier = malloc((size_t)frontier_cap * sizeof *bwd_frontier);
            int *fwd_next = malloc((size_t)frontier_cap * sizeof *fwd_next);
            int *bwd_next = malloc((size_t)frontier_cap * sizeof *bwd_next);
            ok = fwd_frontier != NULL && bwd_frontier != NULL && fwd_next != NULL &&
                 bwd_next != NULL;
            if (ok) {
                int fwd_len = 1;
                fwd_frontier[0] = 0;
                int bwd_len = 0;
                for (int v = 0; v < bwd.count; v++) {
                    bwd_frontier[bwd_len++] = v;
                }
                for (int level = 0; level < max_depth && !found; level++) {
                    bool expand_fwd = fwd_len <= bwd_len;
                    ct_synth_side *side = expand_fwd ? &fwd : &bwd;
                    ct_synth_side *other = expand_fwd ? &bwd : &fwd;
                    int *frontier = expand_fwd ? fwd_frontier : bwd_frontier;
                    int *next = expand_fwd ? fwd_next : bwd_next;
                    int flen = expand_fwd ? fwd_len : bwd_len;
                    int nlen = 0;
                    for (int fi = 0; fi < flen && !found; fi++) {
                        int node_idx = frontier[fi];
                        for (int mi = 0; mi < move_n; mi++) {
                            if (side->nodes[node_idx].parent >= 0 &&
                                side->nodes[node_idx].move.face == moves[mi].face) {
                                continue;
                            }
                            uint8_t child[CT_SYNTH_CELLS];
                            for (int q = 0; q < CT_SYNTH_CELLS; q++) {
                                child[q] = side->nodes[node_idx].state[perm_inv[mi][q]];
                            }
                            int own_slot = synth_side_slot(side, child);
                            if (side->hash[own_slot] != 0) {
                                continue;
                            }
                            if (side->count >= CT_SYNTH_FALLBACK_NODES) {
                                continue;
                            }
                            memcpy(side->nodes[side->count].state, child, (size_t)CT_SYNTH_CELLS);
                            side->nodes[side->count].parent = node_idx;
                            side->nodes[side->count].move = moves[mi];
                            side->hash[own_slot] = side->count + 1;
                            int other_slot = synth_side_slot(other, child);
                            if (other->hash[other_slot] != 0) {
                                int fwd_node =
                                    expand_fwd ? side->count : other->hash[other_slot] - 1;
                                int bwd_node =
                                    expand_fwd ? other->hash[other_slot] - 1 : side->count;
                                ct_move fseq[CT_SYNTH_FALLBACK_DEPTH_MAX];
                                ct_move bseq[CT_SYNTH_FALLBACK_DEPTH_MAX];
                                ct_move binv[CT_SYNTH_FALLBACK_DEPTH_MAX];
                                int fseq_len = 0;
                                int bseq_len = 0;
                                synth_path_to_root(&fwd, fwd_node, fseq, &fseq_len);
                                synth_path_to_root(&bwd, bwd_node, bseq, &bseq_len);
                                int binv_len = ct_transform_inverse(bseq, bseq_len, binv);
                                if (fseq_len + binv_len <= CT_SYNTH_MAX_MOVES) {
                                    memcpy(out, fseq, (size_t)fseq_len * sizeof *fseq);
                                    memcpy(out + fseq_len, binv, (size_t)binv_len * sizeof *binv);
                                    *out_len = fseq_len + binv_len;
                                    found = true;
                                }
                            }
                            if (nlen < frontier_cap) {
                                next[nlen++] = side->count;
                            }
                            side->count++;
                        }
                    }
                    if (expand_fwd) {
                        fwd_frontier = fwd_next;
                        fwd_next = frontier;
                        fwd_len = nlen;
                    } else {
                        bwd_frontier = bwd_next;
                        bwd_next = frontier;
                        bwd_len = nlen;
                    }
                }
            }
            free(fwd_frontier);
            free(bwd_frontier);
            free(fwd_next);
            free(bwd_next);
        }
    }
    free(fwd.nodes);
    free(bwd.nodes);
    free(fwd.hash);
    free(bwd.hash);
    return found;
}

int ct_synth_target(int orbit, int buffer, int t1, int t2, const ct_config *cfg,
                    ct_synth_result *out) {
    if (out == NULL || cfg == NULL) {
        return -1;
    }
    memset(out, 0, sizeof *out);
    if (!synth_case_valid(orbit, buffer, t1, t2)) {
        return -1;
    }
    if (!synth_built && ct_synth_init() < 0) {
        return -1;
    }
    int cb = synth_sticker_cell_of(orbit, buffer);
    int c1 = synth_sticker_cell_of(orbit, t1);
    int c2 = synth_sticker_cell_of(orbit, t2);
    int cand_cap = cfg->gen.candidate_max;
    if (cand_cap < CT_SYNTH_ONE) {
        cand_cap = CT_SYNTH_ONE;
    }
    if (cand_cap > CT_SYNTH_CAND_MAX) {
        cand_cap = CT_SYNTH_CAND_MAX;
    }
    ct_synth_cand cands[CT_SYNTH_CAND_MAX];
    int cand_n = 0;
    ct_move inner[CT_SYNTH_TPL_MAX];
    int inner_len = 0;
    if (synth_lookup_seq(cb, c1, c2, inner, &inner_len)) {
        ct_move simplified[CT_SYNTH_MAX_MOVES];
        int simplified_len =
            ct_transform_simplify(inner, inner_len, simplified, CT_SYNTH_MAX_MOVES);
        if (simplified_len >= 0 && synth_validate(simplified, simplified_len, orbit, cb, c1, c2)) {
            synth_cand_add(cands, &cand_n, cand_cap, simplified, simplified_len, false);
        }
    }
    ct_move subset[CT_CUBE_FACE_COUNT * (CT_MOVE_QUARTERS - 1)];
    int move_n = synth_subset_moves(cfg->gen.preferred_moves, subset);
    if (move_n > 0 && cand_n < cand_cap && cfg->gen.setup_bfs_depth >= 1) {
        int depth = cfg->gen.setup_bfs_depth;
        if (depth > CT_SYNTH_SETUP_MAX) {
            depth = CT_SYNTH_SETUP_MAX;
        }
        synth_setup_search(orbit, cb, c1, c2, subset, move_n, depth, cands, cand_cap, &cand_n);
    }
    if (cand_n > 0) {
        int best = -1;
        double best_total = 0.0;
        for (int i = 0; i < cand_n; i++) {
            ct_score_parts parts;
            if (ct_score_alg(cands[i].seq, cands[i].len, cfg, orbit, &parts) != CT_SCORE_OK) {
                continue;
            }
            if (best < 0 || parts.total < best_total) {
                best = i;
                best_total = parts.total;
            }
        }
        if (best >= 0 && synth_fill(out, cfg, orbit, cands[best].seq, cands[best].len,
                                    cands[best].setup, false)) {
            return 0;
        }
    }
    if (move_n > 0 && cand_n == 0 && cfg->gen.fallback_depth >= 1) {
        int depth = cfg->gen.fallback_depth;
        if (depth > CT_SYNTH_FALLBACK_DEPTH_MAX) {
            depth = CT_SYNTH_FALLBACK_DEPTH_MAX;
        }
        ct_move seq[CT_SYNTH_MAX_MOVES];
        int seq_len = 0;
        if (synth_fallback(orbit, cb, c1, c2, subset, move_n, depth, seq, &seq_len)) {
            ct_move simplified[CT_SYNTH_MAX_MOVES];
            int simplified_len =
                ct_transform_simplify(seq, seq_len, simplified, CT_SYNTH_MAX_MOVES);
            if (simplified_len >= 0 &&
                synth_validate(simplified, simplified_len, orbit, cb, c1, c2)) {
                synth_fill(out, cfg, orbit, simplified, simplified_len, false, true);
                return 0;
            }
        }
    }
    return 0;
}
