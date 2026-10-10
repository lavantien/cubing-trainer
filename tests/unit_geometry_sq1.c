#include "engine/geometry.h"
#include "engine/geometry_sq1.h"
#include "framework.h"

#include <string.h>

enum { PERM_BYTES = CT_SQ1_STICKERS, WALK_SEED = 20261010 };

typedef struct {
    const char *token;
    int id;
    int face;
    int x;
    int y;
} ct_sq1_token_row;

static const ct_sq1_token_row ct_sq1_ok[] = {
    {"(-5,-5)", 0, CT_SQ1_MOVE_TWIST, -5, -5},  {"(-5,6)", 11, CT_SQ1_MOVE_TWIST, -5, 6},
    {"(-1,-1)", 52, CT_SQ1_MOVE_TWIST, -1, -1}, {"(0,-5)", 60, CT_SQ1_MOVE_TWIST, 0, -5},
    {"(0,1)", 65, CT_SQ1_MOVE_TWIST, 0, 1},     {"(1,0)", 76, CT_SQ1_MOVE_TWIST, 1, 0},
    {"(3,6)", 106, CT_SQ1_MOVE_TWIST, 3, 6},    {"(6,6)", 142, CT_SQ1_MOVE_TWIST, 6, 6},
    {"(6,-5)", 131, CT_SQ1_MOVE_TWIST, 6, -5},  {"/", 143, CT_SQ1_MOVE_SLASH, 0, 0},
};

static const char *const ct_sq1_bad[] = {
    "(0,0)",
    "(7,0)",
    "(0,7)",
    "(-6,0)",
    "(0,-6)",
    "(01,0)",
    "(1,01)",
    "(+1,0)",
    "( 1,0)",
    "(1, 0)",
    "(1)",
    "(1,0",
    "1,0",
    "(1,0)x",
    "()",
    "(,)",
    "(1,2,3)",
    "//",
    "/x",
    "",
    "(U)",
    "(U')",
    "(D)",
    "3,3",
    "(1,0))",
    "(1,-0)",
    "(-0,1)",
    "(-0,5)",
    "(-0,-0)",
    "(1000000000000,0)",
    "(1,1000000000000)",
    "(2147483648,0)",
    "(4294967296,0)",
    "(99999999999999999999,0)",
    "(-99999999999,0)",
    "(2147483647,0)",
};

static int side_idx(int layer, int unit) {
    return layer * CT_SQ1_UNITS + unit;
}

static int face_idx(int layer, int unit) {
    return CT_SQ1_LAYERS * CT_SQ1_UNITS + layer * CT_SQ1_UNITS + unit;
}

static const uint8_t *state_layer(const ct_sq1_state *s, int layer) {
    return s->units + layer * CT_SQ1_UNITS;
}

static const char *next_token(const char *cursor, char *out, unsigned cap) {
    while (*cursor == ' ') {
        cursor++;
    }
    if (*cursor == '\0') {
        return NULL;
    }
    unsigned len = 0;
    if (cursor[0] != '/') {
        while (cursor[len] != '\0' && cursor[len] != ' ' && cursor[len] != '/') {
            len++;
        }
    } else {
        len = 1;
    }
    CT_ASSERT(len < cap);
    memcpy(out, cursor, len);
    out[len] = '\0';
    return cursor + len;
}

static int inv_turn(int turn) {
    int wrapped = ((-turn) % CT_SQ1_UNITS + CT_SQ1_UNITS) % CT_SQ1_UNITS;
    return wrapped > CT_SQ1_TURN_MAX ? wrapped - CT_SQ1_UNITS : wrapped;
}

static void apply_alg(const char *alg, ct_sq1_state *s) {
    char tok[64];
    const char *cursor = alg;
    while ((cursor = next_token(cursor, tok, sizeof tok)) != NULL) {
        ct_move m;
        int id =
            ct_geometry_sq1()->move_from_token(ct_geometry_sq1(), tok, CT_GEOMETRY_SINGLE_N, &m);
        CT_ASSERT(id >= 0);
        CT_ASSERT_EQ(ct_sq1_apply(s, id), 0);
    }
}

static int count_tokens(const char *alg, int *parsed) {
    char tok[64];
    const char *cursor = alg;
    int ok = 0;
    while ((cursor = next_token(cursor, tok, sizeof tok)) != NULL) {
        ct_move m;
        if (ct_geometry_sq1()->move_from_token(ct_geometry_sq1(), tok, CT_GEOMETRY_SINGLE_N, &m) >=
            0) {
            ok++;
        }
    }
    *parsed = ok;
    return 0;
}

CT_SUITE(sq1_identity) {
    const ct_geometry *g = ct_geometry_sq1();
    CT_ASSERT(g != NULL);
    CT_ASSERT_STR_EQ(g->name, "sq1");
    CT_ASSERT_EQ(g->sticker_count(CT_GEOMETRY_SINGLE_N), CT_SQ1_STICKERS);
    CT_ASSERT_EQ(g->sticker_count(CT_GEOMETRY_SINGLE_N), 48);
    CT_ASSERT_EQ(g->move_count(CT_GEOMETRY_SINGLE_N), CT_SQ1_MOVES);
    CT_ASSERT_EQ(g->move_count(CT_GEOMETRY_SINGLE_N), 144);
    CT_ASSERT(g->sticker_count(0) < 0);
    CT_ASSERT(g->sticker_count(2) < 0);
    CT_ASSERT(g->move_count(0) < 0);
    CT_ASSERT(g->move_count(CT_GEOMETRY_MAX_N) < 0);
    CT_ASSERT(g->move_facelets_state != NULL);
    CT_ASSERT(ct_geometry_nxn()->move_facelets_state == NULL);
}

CT_SUITE(sq1_token) {
    const ct_geometry *g = ct_geometry_sq1();
    for (size_t k = 0; k < sizeof ct_sq1_ok / sizeof ct_sq1_ok[0]; k++) {
        ct_move m = {9, 9, 9, 9};
        int got = g->move_from_token(g, ct_sq1_ok[k].token, CT_GEOMETRY_SINGLE_N, &m);
        CT_ASSERT_EQ(got, ct_sq1_ok[k].id);
        CT_ASSERT_EQ(m.face, ct_sq1_ok[k].face);
        CT_ASSERT_EQ(m.depth_from, ct_sq1_ok[k].x);
        CT_ASSERT_EQ(m.depth_to, ct_sq1_ok[k].y);
        CT_ASSERT_EQ(m.quarter, 0);
        CT_ASSERT(got < g->move_count(CT_GEOMETRY_SINGLE_N));
    }
    for (size_t k = 0; k < sizeof ct_sq1_bad / sizeof ct_sq1_bad[0]; k++) {
        ct_move m = {5, 5, 5, 5};
        CT_ASSERT(g->move_from_token(g, ct_sq1_bad[k], CT_GEOMETRY_SINGLE_N, &m) < 0);
        CT_ASSERT_EQ(m.face, 5);
        CT_ASSERT_EQ(m.depth_from, 5);
        CT_ASSERT_EQ(m.depth_to, 5);
        CT_ASSERT_EQ(m.quarter, 5);
    }
    ct_move m = {0, 0, 0, 0};
    CT_ASSERT(g->move_from_token(g, "(1,0)", 2, &m) < 0);
    CT_ASSERT(g->move_from_token(g, NULL, CT_GEOMETRY_SINGLE_N, &m) < 0);
    CT_ASSERT(g->move_from_token(g, "(1,0)", CT_GEOMETRY_SINGLE_N, NULL) < 0);
    CT_ASSERT(g->move_from_token(NULL, "(1,0)", CT_GEOMETRY_SINGLE_N, &m) < 0);
}

CT_SUITE(sq1_move_ids) {
    int seen[CT_SQ1_MOVES] = {0};
    for (int x = CT_SQ1_TURN_MIN; x <= CT_SQ1_TURN_MAX; x++) {
        for (int y = CT_SQ1_TURN_MIN; y <= CT_SQ1_TURN_MAX; y++) {
            int id = ct_sq1_move_id(x, y);
            if (x == 0 && y == 0) {
                CT_ASSERT(id < 0);
                continue;
            }
            CT_ASSERT(id >= 0 && id < CT_SQ1_TWISTS);
            seen[id]++;
            int bx = 0;
            int by = 0;
            CT_ASSERT_EQ(ct_sq1_move_pair(id, &bx, &by), 0);
            CT_ASSERT_EQ(bx, x);
            CT_ASSERT_EQ(by, y);
        }
    }
    for (int id = 0; id < CT_SQ1_TWISTS; id++) {
        CT_ASSERT_EQ(seen[id], 1);
    }
    CT_ASSERT(ct_sq1_move_id(7, 0) < 0);
    CT_ASSERT(ct_sq1_move_id(0, -6) < 0);
    CT_ASSERT(ct_sq1_move_id(0, 0) < 0);
    int x = 0;
    int y = 0;
    CT_ASSERT(ct_sq1_move_pair(-1, &x, &y) < 0);
    CT_ASSERT(ct_sq1_move_pair(CT_SQ1_SLASH, &x, &y) < 0);
    CT_ASSERT(ct_sq1_move_pair(CT_SQ1_MOVES, &x, &y) < 0);
}

CT_SUITE(sq1_state) {
    static const uint8_t solved_top[CT_SQ1_UNITS] = {0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7};
    static const uint8_t solved_bottom[CT_SQ1_UNITS] = {8,  9,  9,  10, 11, 11,
                                                        12, 13, 13, 14, 15, 15};
    static const uint8_t twist_top[CT_SQ1_UNITS] = {7, 0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 6};
    static const uint8_t twist_bottom[CT_SQ1_UNITS] = {15, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15};
    static const uint8_t slash_top[CT_SQ1_UNITS] = {0, 0, 1, 2, 2, 3, 12, 13, 13, 14, 15, 15};
    static const uint8_t slash_bottom[CT_SQ1_UNITS] = {8, 9, 9, 10, 11, 11, 4, 4, 5, 6, 6, 7};

    ct_sq1_state s;
    ct_sq1_state_solved(&s);
    CT_ASSERT(s.slice_flipped == false);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 0), solved_top, CT_SQ1_UNITS), 0);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 1), solved_bottom, CT_SQ1_UNITS), 0);

    ct_sq1_apply(&s, ct_sq1_move_id(1, 0));
    CT_ASSERT_EQ(memcmp(state_layer(&s, 0), twist_top, CT_SQ1_UNITS), 0);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 1), solved_bottom, CT_SQ1_UNITS), 0);
    ct_sq1_apply(&s, ct_sq1_move_id(-1, 0));
    CT_ASSERT_EQ(memcmp(state_layer(&s, 0), solved_top, CT_SQ1_UNITS), 0);

    ct_sq1_state_solved(&s);
    ct_sq1_apply(&s, ct_sq1_move_id(0, 1));
    CT_ASSERT_EQ(memcmp(state_layer(&s, 0), solved_top, CT_SQ1_UNITS), 0);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 1), twist_bottom, CT_SQ1_UNITS), 0);

    ct_sq1_state_solved(&s);
    ct_sq1_apply(&s, CT_SQ1_SLASH);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 0), slash_top, CT_SQ1_UNITS), 0);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 1), slash_bottom, CT_SQ1_UNITS), 0);
    CT_ASSERT(s.slice_flipped == true);
    ct_sq1_apply(&s, CT_SQ1_SLASH);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 0), solved_top, CT_SQ1_UNITS), 0);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 1), solved_bottom, CT_SQ1_UNITS), 0);
    CT_ASSERT(s.slice_flipped == false);

    const int pairs[][2] = {{1, 0}, {0, 1}, {3, -4}, {6, 6}, {-5, 2}, {2, 2}};
    for (size_t k = 0; k < sizeof pairs / sizeof pairs[0]; k++) {
        ct_sq1_state_solved(&s);
        ct_sq1_apply(&s, ct_sq1_move_id(pairs[k][0], pairs[k][1]));
        ct_sq1_apply(&s, ct_sq1_move_id(inv_turn(pairs[k][0]), inv_turn(pairs[k][1])));
        ct_sq1_state want;
        ct_sq1_state_solved(&want);
        CT_ASSERT_EQ(memcmp(&s, &want, sizeof want), 0);
    }

    ct_sq1_state_solved(&s);
    ct_sq1_apply(&s, ct_sq1_move_id(6, 6));
    ct_sq1_apply(&s, ct_sq1_move_id(6, 6));
    CT_ASSERT_EQ(memcmp(state_layer(&s, 0), solved_top, CT_SQ1_UNITS), 0);
    CT_ASSERT_EQ(memcmp(state_layer(&s, 1), solved_bottom, CT_SQ1_UNITS), 0);
    CT_ASSERT(s.slice_flipped == false);
}

CT_SUITE(sq1_slash_legality) {
    ct_sq1_state s;
    ct_sq1_state_solved(&s);
    CT_ASSERT_EQ(ct_sq1_can_slash(&s), 1);
    ct_sq1_apply(&s, ct_sq1_move_id(1, 0));
    CT_ASSERT_EQ(ct_sq1_can_slash(&s), 1);
    ct_sq1_state_solved(&s);
    ct_sq1_apply(&s, ct_sq1_move_id(2, 0));
    CT_ASSERT_EQ(ct_sq1_can_slash(&s), 0);
    ct_sq1_state before = s;
    CT_ASSERT(ct_sq1_apply(&s, CT_SQ1_SLASH) < 0);
    CT_ASSERT_EQ(memcmp(&s, &before, sizeof before), 0);
    CT_ASSERT(ct_sq1_can_slash(NULL) < 0);
    CT_ASSERT(ct_sq1_apply(NULL, CT_SQ1_SLASH) < 0);
    ct_sq1_state_solved(&s);
    CT_ASSERT(ct_sq1_apply(&s, CT_SQ1_MOVES) < 0);
    CT_ASSERT(ct_sq1_apply(&s, -1) < 0);
}

CT_SUITE(sq1_perm) {
    const ct_geometry *g = ct_geometry_sq1();
    uint8_t perm[PERM_BYTES];

    CT_ASSERT_EQ(g->move_facelets(g, ct_sq1_move_id(1, 0), CT_GEOMETRY_SINGLE_N, perm), 0);
    CT_ASSERT_EQ(perm[side_idx(0, 0)], side_idx(0, 11));
    CT_ASSERT_EQ(perm[side_idx(0, 11)], side_idx(0, 10));
    CT_ASSERT_EQ(perm[face_idx(0, 0)], face_idx(0, 11));
    CT_ASSERT_EQ(perm[face_idx(0, 11)], face_idx(0, 10));
    CT_ASSERT_EQ(perm[side_idx(1, 5)], side_idx(1, 5));
    CT_ASSERT_EQ(perm[face_idx(1, 7)], face_idx(1, 7));

    CT_ASSERT_EQ(g->move_facelets(g, ct_sq1_move_id(0, 1), CT_GEOMETRY_SINGLE_N, perm), 0);
    CT_ASSERT_EQ(perm[side_idx(1, 0)], side_idx(1, 11));
    CT_ASSERT_EQ(perm[side_idx(1, 11)], side_idx(1, 10));
    CT_ASSERT_EQ(perm[side_idx(0, 4)], side_idx(0, 4));

    CT_ASSERT_EQ(g->move_facelets(g, ct_sq1_move_id(-1, -1), CT_GEOMETRY_SINGLE_N, perm), 0);
    CT_ASSERT_EQ(perm[side_idx(0, 0)], side_idx(0, 1));
    CT_ASSERT_EQ(perm[face_idx(1, 0)], face_idx(1, 1));

    CT_ASSERT(g->move_facelets(g, CT_SQ1_SLASH, CT_GEOMETRY_SINGLE_N, perm) == CT_GEOMETRY_E_STATE);
    CT_ASSERT(g->move_facelets(g, -1, CT_GEOMETRY_SINGLE_N, perm) < 0);
    CT_ASSERT(g->move_facelets(g, CT_SQ1_MOVES, CT_GEOMETRY_SINGLE_N, perm) < 0);
    CT_ASSERT(g->move_facelets(g, 0, 2, perm) < 0);
    CT_ASSERT(g->move_facelets(g, 0, CT_GEOMETRY_SINGLE_N, NULL) < 0);
    CT_ASSERT(g->move_facelets(NULL, 0, CT_GEOMETRY_SINGLE_N, perm) < 0);

    for (int id = 0; id < CT_SQ1_TWISTS; id++) {
        CT_ASSERT_EQ(g->move_facelets(g, id, CT_GEOMETRY_SINGLE_N, perm), 0);
        bool seen[PERM_BYTES] = {false};
        for (int i = 0; i < PERM_BYTES; i++) {
            CT_ASSERT(perm[i] < PERM_BYTES);
            CT_ASSERT(!seen[perm[i]]);
            seen[perm[i]] = true;
        }
        uint8_t inv[PERM_BYTES];
        int x = 0;
        int y = 0;
        CT_ASSERT_EQ(ct_sq1_move_pair(id, &x, &y), 0);
        CT_ASSERT_EQ(g->move_facelets(g, ct_sq1_move_id(inv_turn(x), inv_turn(y)),
                                      CT_GEOMETRY_SINGLE_N, inv),
                     0);
        for (int i = 0; i < PERM_BYTES; i++) {
            CT_ASSERT_EQ(inv[perm[i]], i);
        }
    }
}

static void state_from_positions(ct_sq1_state *s) {
    for (int i = 0; i < CT_SQ1_LAYERS * CT_SQ1_UNITS; i++) {
        s->units[i] = (uint8_t)i;
    }
    s->slice_flipped = false;
}

CT_SUITE(sq1_perm_convention) {
    const ct_geometry *g = ct_geometry_sq1();
    uint8_t perm[PERM_BYTES];
    for (int id = 0; id < CT_SQ1_MOVES; id++) {
        ct_sq1_state s;
        state_from_positions(&s);
        if (id == CT_SQ1_SLASH) {
            CT_ASSERT_EQ(ct_sq1_can_slash(&s), 1);
            CT_ASSERT_EQ(g->move_facelets_state(g, id, CT_GEOMETRY_SINGLE_N, &s, perm), 0);
        } else {
            CT_ASSERT_EQ(g->move_facelets(g, id, CT_GEOMETRY_SINGLE_N, perm), 0);
        }
        CT_ASSERT_EQ(ct_sq1_apply(&s, id), 0);
        for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
            for (int u = 0; u < CT_SQ1_UNITS; u++) {
                int origin = s.units[layer * CT_SQ1_UNITS + u];
                int origin_layer = origin / CT_SQ1_UNITS;
                int origin_unit = origin % CT_SQ1_UNITS;
                CT_ASSERT_EQ(perm[side_idx(layer, u)], side_idx(origin_layer, origin_unit));
                CT_ASSERT_EQ(perm[face_idx(layer, u)], face_idx(origin_layer, origin_unit));
            }
        }
    }
}

CT_SUITE(sq1_perm_state) {
    const ct_geometry *g = ct_geometry_sq1();
    uint8_t perm[PERM_BYTES];
    uint8_t pure[PERM_BYTES];
    ct_sq1_state s;

    CT_ASSERT(g->move_facelets_state(g, CT_SQ1_SLASH, CT_GEOMETRY_SINGLE_N, NULL, perm) < 0);
    ct_sq1_state_solved(&s);
    CT_ASSERT_EQ(g->move_facelets_state(g, CT_SQ1_SLASH, CT_GEOMETRY_SINGLE_N, &s, perm), 0);
    CT_ASSERT_EQ(perm[side_idx(0, 6)], side_idx(1, 6));
    CT_ASSERT_EQ(perm[side_idx(1, 11)], side_idx(0, 11));
    CT_ASSERT_EQ(perm[side_idx(0, 0)], side_idx(0, 0));
    CT_ASSERT_EQ(perm[side_idx(1, 5)], side_idx(1, 5));
    CT_ASSERT_EQ(perm[face_idx(0, 6)], face_idx(1, 6));
    CT_ASSERT_EQ(perm[face_idx(1, 8)], face_idx(0, 8));
    CT_ASSERT_EQ(perm[face_idx(0, 2)], face_idx(0, 2));
    bool seen[PERM_BYTES] = {false};
    for (int i = 0; i < PERM_BYTES; i++) {
        CT_ASSERT(perm[i] < PERM_BYTES);
        CT_ASSERT(!seen[perm[i]]);
        seen[perm[i]] = true;
    }

    ct_sq1_state_solved(&s);
    ct_sq1_apply(&s, ct_sq1_move_id(2, 0));
    CT_ASSERT(g->move_facelets_state(g, CT_SQ1_SLASH, CT_GEOMETRY_SINGLE_N, &s, perm) < 0);

    for (int id = 0; id < CT_SQ1_TWISTS; id += 7) {
        CT_ASSERT_EQ(g->move_facelets(g, id, CT_GEOMETRY_SINGLE_N, pure), 0);
        CT_ASSERT_EQ(g->move_facelets_state(g, id, CT_GEOMETRY_SINGLE_N, NULL, perm), 0);
        CT_ASSERT_EQ(memcmp(perm, pure, PERM_BYTES), 0);
    }
    CT_ASSERT(g->move_facelets_state(g, -1, CT_GEOMETRY_SINGLE_N, NULL, perm) < 0);
    CT_ASSERT(g->move_facelets_state(g, CT_SQ1_MOVES, CT_GEOMETRY_SINGLE_N, NULL, perm) < 0);
    CT_ASSERT(g->move_facelets_state(g, 0, 2, NULL, perm) < 0);
    CT_ASSERT(g->move_facelets_state(g, 0, CT_GEOMETRY_SINGLE_N, NULL, NULL) < 0);
    CT_ASSERT(g->move_facelets_state(NULL, 0, CT_GEOMETRY_SINGLE_N, NULL, perm) < 0);
}

CT_SUITE(sq1_axis) {
    const ct_geometry *g = ct_geometry_sq1();
    ct_v3 axis = {9, 9, 9};
    for (int id = 0; id < CT_SQ1_TWISTS; id += 13) {
        CT_ASSERT_EQ(g->turn_axis(g, id, CT_GEOMETRY_SINGLE_N, &axis), 0);
        CT_ASSERT_EQ(axis.x, 0);
        CT_ASSERT_EQ(axis.y, 1);
        CT_ASSERT_EQ(axis.z, 0);
    }
    CT_ASSERT_EQ(g->turn_axis(g, CT_SQ1_SLASH, CT_GEOMETRY_SINGLE_N, &axis), 0);
    CT_ASSERT_EQ(axis.x, 0);
    CT_ASSERT_EQ(axis.y, 0);
    CT_ASSERT_EQ(axis.z, 1);
    CT_ASSERT(g->turn_axis(g, -1, CT_GEOMETRY_SINGLE_N, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, CT_SQ1_MOVES, CT_GEOMETRY_SINGLE_N, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, 0, 2, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, 0, CT_GEOMETRY_SINGLE_N, NULL) < 0);
    CT_ASSERT(g->turn_axis(NULL, 0, CT_GEOMETRY_SINGLE_N, &axis) < 0);
}

CT_SUITE(sq1_stickers) {
    const ct_geometry *g = ct_geometry_sq1();
    ct_v3 pos = {9, 9, 9};
    ct_v3 normal = {9, 9, 9};
    CT_ASSERT_EQ(g->sticker_geometry(g, 0, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 1);
    CT_ASSERT_EQ(pos.y, 1);
    CT_ASSERT_EQ(pos.z, 2);
    CT_ASSERT_EQ(normal.x, 1);
    CT_ASSERT_EQ(normal.y, 0);
    CT_ASSERT_EQ(normal.z, 2);
    CT_ASSERT_EQ(g->sticker_geometry(g, 3, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 2);
    CT_ASSERT_EQ(pos.z, -1);
    CT_ASSERT_EQ(g->sticker_geometry(g, 12, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 1);
    CT_ASSERT_EQ(pos.y, -1);
    CT_ASSERT_EQ(pos.z, -2);
    CT_ASSERT_EQ(normal.z, -2);
    CT_ASSERT_EQ(g->sticker_geometry(g, 23, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, -1);
    CT_ASSERT_EQ(pos.z, -2);
    CT_ASSERT_EQ(g->sticker_geometry(g, 24, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 1);
    CT_ASSERT_EQ(pos.y, 2);
    CT_ASSERT_EQ(pos.z, 2);
    CT_ASSERT_EQ(normal.x, 0);
    CT_ASSERT_EQ(normal.y, 1);
    CT_ASSERT_EQ(normal.z, 0);
    CT_ASSERT_EQ(g->sticker_geometry(g, 42, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, -1);
    CT_ASSERT_EQ(pos.y, -2);
    CT_ASSERT_EQ(pos.z, 2);
    CT_ASSERT_EQ(normal.y, -1);
    CT_ASSERT_EQ(g->sticker_geometry(g, 47, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, -1);
    CT_ASSERT_EQ(pos.z, -2);

    ct_v3 positions[PERM_BYTES];
    for (int i = 0; i < PERM_BYTES; i++) {
        ct_v3 n = {0, 0, 0};
        CT_ASSERT_EQ(g->sticker_geometry(g, i, CT_GEOMETRY_SINGLE_N, &positions[i], &n), 0);
    }
    for (int i = 0; i < PERM_BYTES; i++) {
        for (int j = i + 1; j < PERM_BYTES; j++) {
            CT_ASSERT(positions[i].x != positions[j].x || positions[i].y != positions[j].y ||
                      positions[i].z != positions[j].z);
        }
    }
    CT_ASSERT(g->sticker_geometry(g, -1, CT_GEOMETRY_SINGLE_N, &pos, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, PERM_BYTES, CT_GEOMETRY_SINGLE_N, &pos, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, 2, &pos, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, CT_GEOMETRY_SINGLE_N, NULL, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, CT_GEOMETRY_SINGLE_N, &pos, NULL) < 0);
    CT_ASSERT(g->sticker_geometry(NULL, 0, CT_GEOMETRY_SINGLE_N, &pos, &normal) < 0);
}

CT_SUITE(sq1_seed_snapshot) {
    const char *const algs[] = {
        "(1,0)/(-1,0)",
        "/ (3,0) / (-3,-3) / (0,3) /",
        "(U) / (3,3) / (-1,0) / (2,-4) / (4,-2) / (0,-2) / (-4,2) / (1,-5) / (3,0) / (3,3) / (3,0)",
        "/ 3,3 /",
    };
    const int want[] = {3, 7, 20, 2};
    for (size_t k = 0; k < sizeof algs / sizeof algs[0]; k++) {
        int parsed = 0;
        CT_ASSERT_EQ(count_tokens(algs[k], &parsed), 0);
        CT_ASSERT_EQ(parsed, want[k]);
    }

    ct_sq1_state s;
    ct_sq1_state_solved(&s);
    apply_alg("/ (3,0) / (-3,-3) / (0,3) /", &s);
    ct_sq1_state_solved(&s);
    apply_alg("(1,0)/(3,0)/(3,0)/(-1,-1)/(-2,1)/(-3,0)/", &s);
}

CT_SUITE(sq1_perm_state_coherence) {
    const ct_geometry *g = ct_geometry_sq1();
    static const uint8_t solved[CT_SQ1_LAYERS * CT_SQ1_UNITS] = {
        0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 15,
    };
    ct_sfc64 rng;
    ct_sfc64_init(&rng, WALK_SEED);
    for (int iter = 0; iter < 300; iter++) {
        ct_sq1_state s;
        ct_sq1_state_solved(&s);
        uint8_t facelet[PERM_BYTES];
        for (int i = 0; i < PERM_BYTES; i++) {
            facelet[i] = (uint8_t)i;
        }
        int steps = 1 + (int)ct_sfc64_range(&rng, 12);
        for (int k = 0; k < steps; k++) {
            int id;
            if (ct_sq1_can_slash(&s) && ct_sfc64_range(&rng, 4) == 0) {
                id = CT_SQ1_SLASH;
            } else {
                int x = (int)ct_sfc64_range(&rng, 12) - 5;
                int y = (int)ct_sfc64_range(&rng, 12) - 5;
                if (x == 0 && y == 0) {
                    y = 1;
                }
                id = ct_sq1_move_id(x, y);
            }
            uint8_t perm[PERM_BYTES];
            uint8_t next[PERM_BYTES];
            CT_ASSERT_EQ(g->move_facelets_state(g, id, CT_GEOMETRY_SINGLE_N, &s, perm), 0);
            for (int i = 0; i < PERM_BYTES; i++) {
                next[i] = facelet[perm[i]];
            }
            memcpy(facelet, next, sizeof facelet);
            CT_ASSERT_EQ(ct_sq1_apply(&s, id), 0);
            for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
                for (int u = 0; u < CT_SQ1_UNITS; u++) {
                    int origin = facelet[side_idx(layer, u)];
                    CT_ASSERT_EQ(s.units[layer * CT_SQ1_UNITS + u], solved[origin]);
                }
            }
        }
    }
}

CT_SUITE(sq1_fuzz) {
    const ct_geometry *g = ct_geometry_sq1();
    static const char alphabet[] = "0123456789(),/-+ Uux";
    ct_sfc64 rng;
    ct_sfc64_init(&rng, WALK_SEED + 1);
    for (int iter = 0; iter < 4000; iter++) {
        char token[12];
        unsigned len = (unsigned)ct_sfc64_range(&rng, sizeof token);
        for (unsigned k = 0; k < len; k++) {
            token[k] = alphabet[ct_sfc64_range(&rng, sizeof alphabet - 1)];
        }
        token[len] = '\0';
        ct_move m = {7, 7, 7, 7};
        int id = g->move_from_token(g, token, CT_GEOMETRY_SINGLE_N, &m);
        if (id < 0) {
            CT_ASSERT_EQ(m.face, 7);
            continue;
        }
        CT_ASSERT(id >= 0 && id < CT_SQ1_MOVES);
        if (id == CT_SQ1_SLASH) {
            CT_ASSERT_EQ(m.face, CT_SQ1_MOVE_SLASH);
        } else {
            CT_ASSERT_EQ(m.face, CT_SQ1_MOVE_TWIST);
            CT_ASSERT(m.depth_from >= CT_SQ1_TURN_MIN && m.depth_from <= CT_SQ1_TURN_MAX);
            CT_ASSERT(m.depth_to >= CT_SQ1_TURN_MIN && m.depth_to <= CT_SQ1_TURN_MAX);
            CT_ASSERT(m.depth_from != 0 || m.depth_to != 0);
            CT_ASSERT_EQ(ct_sq1_move_id(m.depth_from, m.depth_to), id);
        }
        uint8_t perm[PERM_BYTES];
        if (id != CT_SQ1_SLASH) {
            CT_ASSERT_EQ(g->move_facelets(g, id, CT_GEOMETRY_SINGLE_N, perm), 0);
        }
        ct_v3 axis = {0, 0, 0};
        CT_ASSERT_EQ(g->turn_axis(g, id, CT_GEOMETRY_SINGLE_N, &axis), 0);
        if (id == CT_SQ1_SLASH) {
            CT_ASSERT_EQ(axis.z, 1);
        } else {
            CT_ASSERT_EQ(axis.y, 1);
        }
    }
}

CT_SUITE(sq1_fuzz_long) {
    const ct_geometry *g = ct_geometry_sq1();
    ct_sfc64 rng;
    ct_sfc64_init(&rng, WALK_SEED + 2);
    int accepted = 0;
    for (int iter = 0; iter < 20000; iter++) {
        char token[48];
        unsigned at = 0;
        bool short_form = iter % 100 == 0;
        token[at++] = '(';
        for (int part = 0; part < 2; part++) {
            if (part == 1) {
                token[at++] = ',';
            }
            if (!short_form && ct_sfc64_range(&rng, 6) == 0) {
                token[at++] = '-';
            }
            unsigned run = 1 + (unsigned)ct_sfc64_range(&rng, short_form ? 2 : 20);
            CT_ASSERT(at + run < sizeof token);
            for (unsigned k = 0; k < run; k++) {
                token[at++] = (char)('0' + ct_sfc64_range(&rng, 10));
            }
        }
        token[at++] = ')';
        token[at] = '\0';
        ct_move m = {7, 7, 7, 7};
        int id = g->move_from_token(g, token, CT_GEOMETRY_SINGLE_N, &m);
        if (id < 0) {
            CT_ASSERT_EQ(m.face, 7);
            continue;
        }
        accepted++;
        CT_ASSERT(id < CT_SQ1_MOVES);
        CT_ASSERT(m.depth_from >= CT_SQ1_TURN_MIN && m.depth_from <= CT_SQ1_TURN_MAX);
        CT_ASSERT(m.depth_to >= CT_SQ1_TURN_MIN && m.depth_to <= CT_SQ1_TURN_MAX);
        CT_ASSERT_EQ(ct_sq1_move_id(m.depth_from, m.depth_to), id);
    }
    CT_ASSERT(accepted > 0);
}
