#include "engine/cube.h"
#include "engine/geometry.h"
#include "engine/notation.h"
#include "framework.h"
#include "gen/interchange.h"

#include <string.h>

enum {
    PERM_BYTES = CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE,
    APPLY_STEP = 7,
    PERM_STEP = 4
};

typedef struct {
    int n;
    int count;
} ct_geometry_count_row;

static const ct_geometry_count_row ct_count_rows[] = {
    {2, 18}, {3, 54}, {4, 54}, {5, 108}, {6, 108}, {7, 180},
};

typedef struct {
    const char *token;
    int n;
} ct_geometry_token_row;

static const ct_geometry_token_row ct_token_ok[] = {
    {"R", 3},   {"R'", 3},   {"R2", 3}, {"U", 3},  {"F2", 3},  {"Lw", 3},   {"Bw'", 3}, {"Dw2", 3},
    {"M", 3},   {"E2", 3},   {"S'", 3}, {"2R", 3}, {"R", 2},   {"R2", 4},   {"Rw", 5},  {"3R", 5},
    {"3Rw", 5}, {"2Bw'", 5}, {"M", 7},  {"4R", 7}, {"4F'", 7}, {"3Lw2", 7},
};

static const ct_geometry_token_row ct_token_bad[] = {
    {"x", 3}, {"x'", 7}, {"y2", 5}, {"z", 2},  {"3R", 3},  {"3Rw", 3}, {"5R", 5}, {"7Rw", 7},
    {"R", 1}, {"R", 8},  {"Q", 3},  {"2M", 3}, {"R R", 3}, {"", 3},    {"Rq", 3}, {"5F'", 7},
};

static bool move_eq(ct_move a, ct_move b) {
    return a.face == b.face && a.depth_from == b.depth_from && a.depth_to == b.depth_to &&
           a.quarter == b.quarter;
}

static const ct_move *candidate_table(int n, int *count) {
    static ct_move table[CT_INTERCHANGE_MAX_CANDIDATES];
    *count = ct_candidate_moves(n, table, CT_INTERCHANGE_MAX_CANDIDATES);
    return table;
}

static int candidate_index(const ct_move *cands, int count, ct_move m) {
    for (int i = 0; i < count; i++) {
        if (move_eq(cands[i], m)) {
            return i;
        }
    }
    return -1;
}

static void fill_identity(ct_cube *c, int n) {
    memset(c, 0, sizeof *c);
    c->n = n;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                c->facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col] =
                    (uint8_t)(face * n * n + row * n + col);
            }
        }
    }
}

CT_SUITE(geometry_identity) {
    const ct_geometry *g = ct_geometry_nxn();
    CT_ASSERT(g != NULL);
    CT_ASSERT_STR_EQ(g->name, "nxn");
    for (int n = CT_GEOMETRY_MIN_N; n <= CT_GEOMETRY_MAX_N; n++) {
        int half = (n + 1) / 2;
        int closed = CT_CUBE_FACE_COUNT * (CT_MOVE_QUARTERS - 1) * (half * (half + 1) / 2);
        int oracle = 0;
        candidate_table(n, &oracle);
        CT_ASSERT_EQ(oracle, closed);
        CT_ASSERT_EQ(g->move_count(n), closed);
        CT_ASSERT_EQ(g->sticker_count(n), CT_CUBE_FACE_COUNT * n * n);
    }
    for (size_t k = 0; k < sizeof ct_count_rows / sizeof ct_count_rows[0]; k++) {
        CT_ASSERT_EQ(g->move_count(ct_count_rows[k].n), ct_count_rows[k].count);
    }
}

CT_SUITE(geometry_ordering) {
    const ct_geometry *g = ct_geometry_nxn();
    const int sizes[] = {3, 5};
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        int n = sizes[s];
        int count = 0;
        const ct_move *cands = candidate_table(n, &count);
        CT_ASSERT_EQ(g->move_count(n), count);
        for (int id = 0; id < count; id++) {
            char token[CT_NOTATION_TOKEN_MAX];
            if (ct_notation_format_one(&cands[id], n, token, sizeof token) < 0) {
                continue;
            }
            ct_move m = {0, 0, 0, 0};
            int got = g->move_from_token(g, token, n, &m);
            CT_ASSERT_EQ(got, id);
            CT_ASSERT(move_eq(m, cands[id]));
        }
    }
}

CT_SUITE(geometry_perm) {
    const ct_geometry *g = ct_geometry_nxn();
    const int sizes[] = {2, 3, 4};
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        int n = sizes[s];
        int count = 0;
        const ct_move *cands = candidate_table(n, &count);
        for (int id = 0; id < count; id++) {
            uint8_t got[PERM_BYTES];
            uint8_t want[PERM_BYTES];
            memset(got, 0xAA, sizeof got);
            CT_ASSERT_EQ(g->move_facelets(g, id, n, got), 0);
            CT_ASSERT_EQ(ct_sticker_perm(cands[id], n, want), 0);
            CT_ASSERT_EQ(memcmp(got, want, (size_t)(CT_CUBE_FACE_COUNT * n * n)), 0);
            for (int k = CT_CUBE_FACE_COUNT * n * n; k < PERM_BYTES; k++) {
                CT_ASSERT_EQ(got[k], 0);
            }
        }
    }
    int count = 0;
    const ct_move *cands = candidate_table(5, &count);
    for (int id = 0; id < count; id += PERM_STEP) {
        uint8_t got[PERM_BYTES];
        uint8_t want[PERM_BYTES];
        CT_ASSERT_EQ(g->move_facelets(g, id, 5, got), 0);
        CT_ASSERT_EQ(ct_sticker_perm(cands[id], 5, want), 0);
        CT_ASSERT_EQ(memcmp(got, want, (size_t)(CT_CUBE_FACE_COUNT * 5 * 5)), 0);
    }
    uint8_t got[PERM_BYTES];
    uint8_t want[PERM_BYTES];
    CT_ASSERT_EQ(g->move_facelets(g, count - 1, 5, got), 0);
    CT_ASSERT_EQ(ct_sticker_perm(cands[count - 1], 5, want), 0);
    CT_ASSERT_EQ(memcmp(got, want, (size_t)(CT_CUBE_FACE_COUNT * 5 * 5)), 0);
}

CT_SUITE(geometry_stickers) {
    const ct_geometry *g = ct_geometry_nxn();
    const int sizes[] = {3, 7};
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        int n = sizes[s];
        int rows[] = {0, n / 2, n - 1};
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
            for (size_t r = 0; r < sizeof rows / sizeof rows[0]; r++) {
                for (size_t c = 0; c < sizeof rows / sizeof rows[0]; c++) {
                    int sticker = face * n * n + rows[r] * n + (int)c;
                    ct_v3 pos = {0, 0, 0};
                    ct_v3 normal = {0, 0, 0};
                    CT_ASSERT_EQ(g->sticker_geometry(g, sticker, n, &pos, &normal), 0);
                    ct_v3 want_pos = {0, 0, 0};
                    ct_v3 want_normal = {0, 0, 0};
                    ct_sticker_world(n, face, rows[r], (int)c, &want_pos, &want_normal);
                    CT_ASSERT_EQ(pos.x, want_pos.x);
                    CT_ASSERT_EQ(pos.y, want_pos.y);
                    CT_ASSERT_EQ(pos.z, want_pos.z);
                    CT_ASSERT_EQ(normal.x, want_normal.x);
                    CT_ASSERT_EQ(normal.y, want_normal.y);
                    CT_ASSERT_EQ(normal.z, want_normal.z);
                }
            }
        }
        ct_v3 pos = {0, 0, 0};
        ct_v3 normal = {0, 0, 0};
        CT_ASSERT(g->sticker_geometry(g, CT_CUBE_FACE_COUNT * n * n, n, &pos, &normal) < 0);
        CT_ASSERT(g->sticker_geometry(g, -1, n, &pos, &normal) < 0);
    }
}

CT_SUITE(geometry_axis) {
    const ct_geometry *g = ct_geometry_nxn();
    for (int n = CT_GEOMETRY_MIN_N; n <= CT_GEOMETRY_MAX_N; n++) {
        int count = 0;
        const ct_move *cands = candidate_table(n, &count);
        for (int id = 0; id < count; id++) {
            ct_v3 axis = {0, 0, 0};
            CT_ASSERT_EQ(g->turn_axis(g, id, n, &axis), 0);
            ct_v3 center = {0, 0, 0};
            ct_v3 normal = {0, 0, 0};
            ct_sticker_world(n, cands[id].face, 0, 0, &center, &normal);
            CT_ASSERT_EQ(axis.x, normal.x);
            CT_ASSERT_EQ(axis.y, normal.y);
            CT_ASSERT_EQ(axis.z, normal.z);
            CT_ASSERT_EQ(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z, 1);
        }
    }
}

CT_SUITE(geometry_token) {
    const ct_geometry *g = ct_geometry_nxn();
    for (size_t k = 0; k < sizeof ct_token_ok / sizeof ct_token_ok[0]; k++) {
        int n = ct_token_ok[k].n;
        int count = 0;
        const ct_move *cands = candidate_table(n, &count);
        ct_move parsed = {0, 0, 0, 0};
        CT_ASSERT_EQ(ct_notation_parse_one(ct_token_ok[k].token, n, &parsed), 0);
        int want = candidate_index(cands, count, parsed);
        CT_ASSERT(want >= 0);
        ct_move m = {0, 0, 0, 0};
        int got = g->move_from_token(g, ct_token_ok[k].token, n, &m);
        CT_ASSERT(got >= 0);
        CT_ASSERT_EQ(got, want);
        CT_ASSERT(move_eq(m, parsed));
    }
    for (size_t k = 0; k < sizeof ct_token_bad / sizeof ct_token_bad[0]; k++) {
        ct_move m = {CT_FACE_U, 1, 1, 2};
        CT_ASSERT(g->move_from_token(g, ct_token_bad[k].token, ct_token_bad[k].n, &m) < 0);
        CT_ASSERT(move_eq(m, (ct_move){CT_FACE_U, 1, 1, 2}));
    }
}

CT_SUITE(geometry_apply) {
    const ct_geometry *g = ct_geometry_nxn();
    const int sizes[] = {2, 3, 5};
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        int n = sizes[s];
        int count = 0;
        const ct_move *cands = candidate_table(n, &count);
        int step = n == 3 ? 1 : APPLY_STEP;
        for (int id = 0; id < count; id += step) {
            ct_cube c;
            fill_identity(&c, n);
            uint8_t arr[PERM_BYTES];
            memcpy(arr, c.facelet, sizeof arr);
            CT_ASSERT_EQ(ct_geometry_apply(g, id, n, arr), 0);
            ct_apply(&c, cands[id]);
            CT_ASSERT_EQ(memcmp(arr, c.facelet, sizeof arr), 0);
        }
    }
    int count = 0;
    const ct_move *cands = candidate_table(3, &count);
    for (int id = 0; id < count; id += APPLY_STEP) {
        ct_cube c;
        fill_identity(&c, 3);
        uint8_t arr[PERM_BYTES];
        memcpy(arr, c.facelet, sizeof arr);
        int inv = candidate_index(cands, count, ct_move_inverse(cands[id]));
        CT_ASSERT(inv >= 0);
        CT_ASSERT_EQ(ct_geometry_apply(g, id, 3, arr), 0);
        CT_ASSERT_EQ(ct_geometry_apply(g, inv, 3, arr), 0);
        CT_ASSERT_EQ(memcmp(arr, c.facelet, sizeof arr), 0);
    }
}

CT_SUITE(geometry_fuzz) {
    const ct_geometry *g = ct_geometry_nxn();
    static const char alphabet[] = "UDLRFBudlrfbMESxyzw'2 0123456789";
    ct_sfc64 rng;
    ct_sfc64_init(&rng, 20261010);
    for (int iter = 0; iter < 4000; iter++) {
        int n = CT_GEOMETRY_MIN_N + (int)ct_sfc64_range(&rng, CT_GEOMETRY_MAX_N - 1);
        char token[12];
        unsigned len = (unsigned)ct_sfc64_range(&rng, sizeof token);
        for (unsigned k = 0; k < len; k++)
            token[k] = alphabet[ct_sfc64_range(&rng, sizeof alphabet - 1)];
        token[len] = '\0';
        ct_move m = {0, 0, 0, 0};
        int id = g->move_from_token(g, token, n, &m);
        if (id < 0) {
            continue;
        }
        CT_ASSERT(id < g->move_count(n));
        CT_ASSERT(m.face >= 0 && m.face < CT_CUBE_FACE_COUNT);
        CT_ASSERT(m.depth_from >= 1 && m.depth_from <= m.depth_to);
        CT_ASSERT(m.depth_to <= (n + 1) / 2);
        CT_ASSERT(m.quarter >= 1 && m.quarter <= CT_MOVE_QUARTERS - 1);
        int count = 0;
        const ct_move *cands = candidate_table(n, &count);
        CT_ASSERT_EQ(candidate_index(cands, count, m), id);
        char canonical[CT_NOTATION_TOKEN_MAX];
        if (ct_notation_format_one(&m, n, canonical, sizeof canonical) >= 0) {
            ct_move back = {0, 0, 0, 0};
            CT_ASSERT_EQ(g->move_from_token(g, canonical, n, &back), id);
        }

        int sticker = (int)ct_sfc64_range(&rng, (uint64_t)(CT_CUBE_FACE_COUNT * n * n));
        ct_v3 pos = {0, 0, 0};
        ct_v3 normal = {0, 0, 0};
        CT_ASSERT_EQ(g->sticker_geometry(g, sticker, n, &pos, &normal), 0);
        int face = sticker / (n * n);
        int cell = sticker % (n * n);
        ct_v3 want_pos = {0, 0, 0};
        ct_v3 want_normal = {0, 0, 0};
        ct_sticker_world(n, face, cell / n, cell % n, &want_pos, &want_normal);
        CT_ASSERT_EQ(pos.x, want_pos.x);
        CT_ASSERT_EQ(pos.y, want_pos.y);
        CT_ASSERT_EQ(pos.z, want_pos.z);
        CT_ASSERT_EQ(normal.x, want_normal.x);
        CT_ASSERT_EQ(normal.y, want_normal.y);
        CT_ASSERT_EQ(normal.z, want_normal.z);

        int random_id = (int)ct_sfc64_range(&rng, (uint64_t)(count + 2));
        uint8_t perm[PERM_BYTES];
        if (random_id >= count || n > CT_GEOMETRY_PERM_MAX_N) {
            if (n <= CT_GEOMETRY_PERM_MAX_N) {
                CT_ASSERT(g->move_facelets(g, random_id, n, perm) < 0);
            }
            continue;
        }
        CT_ASSERT_EQ(g->move_facelets(g, random_id, n, perm), 0);
        uint8_t want[PERM_BYTES];
        CT_ASSERT_EQ(ct_sticker_perm(cands[random_id], n, want), 0);
        CT_ASSERT_EQ(memcmp(perm, want, (size_t)(CT_CUBE_FACE_COUNT * n * n)), 0);
    }
}

CT_SUITE(geometry_guards) {
    const ct_geometry *g = ct_geometry_nxn();
    uint8_t perm[PERM_BYTES];
    ct_v3 pos = {0, 0, 0};
    ct_v3 axis = {0, 0, 0};
    ct_move m = {0, 0, 0, 0};
    CT_ASSERT(g->sticker_count(CT_GEOMETRY_MIN_N - 1) < 0);
    CT_ASSERT(g->sticker_count(CT_GEOMETRY_MAX_N + 1) < 0);
    CT_ASSERT(g->move_count(CT_GEOMETRY_MIN_N - 1) < 0);
    CT_ASSERT(g->move_count(CT_GEOMETRY_MAX_N + 1) < 0);
    CT_ASSERT(g->move_facelets(g, -1, 3, perm) < 0);
    CT_ASSERT(g->move_facelets(g, g->move_count(3), 3, perm) < 0);
    CT_ASSERT(g->move_facelets(g, 0, CT_GEOMETRY_MIN_N - 1, perm) < 0);
    CT_ASSERT(g->move_facelets(g, 0, CT_GEOMETRY_PERM_MAX_N + 1, perm) < 0);
    CT_ASSERT(g->move_facelets(g, 0, 3, NULL) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, CT_GEOMETRY_MIN_N - 1, &pos, &pos) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, 3, NULL, &pos) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, 3, &pos, NULL) < 0);
    CT_ASSERT(g->turn_axis(g, -1, 3, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, g->move_count(3), 3, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, 0, CT_GEOMETRY_MIN_N - 1, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, 0, 3, NULL) < 0);
    CT_ASSERT(g->move_from_token(g, NULL, 3, &m) < 0);
    CT_ASSERT(g->move_from_token(g, "R", 3, NULL) < 0);
    CT_ASSERT(ct_geometry_apply(NULL, 0, 3, perm) < 0);
    CT_ASSERT(ct_geometry_apply(g, -1, 3, perm) < 0);
    CT_ASSERT(ct_geometry_apply(g, 0, CT_GEOMETRY_MIN_N - 1, perm) < 0);
    CT_ASSERT(ct_geometry_apply(g, 0, CT_GEOMETRY_MAX_N + 1, perm) < 0);
    CT_ASSERT(ct_geometry_apply(g, 0, 3, NULL) < 0);
    CT_ASSERT(ct_geometry_apply(g, g->move_count(3), 3, perm) < 0);
}
