#include "engine/geometry.h"
#include "engine/geometry_clock.h"
#include "framework.h"

#include <string.h>

enum { PERM_BYTES = CT_CLOCK_CLOCKS, FUZZ_SEED = 20261011 };

typedef struct {
    const char *token;
    int id;
    int name;
    int amount;
} ct_clock_token_row;

static const ct_clock_token_row ct_clock_ok[] = {
    {"UR0+", 0, CT_CLOCK_UR, 0},      {"UR5-", 7, CT_CLOCK_UR, -5},
    {"UR1-", 11, CT_CLOCK_UR, -1},    {"DR0+", 12, CT_CLOCK_DR, 0},
    {"DL6+", 30, CT_CLOCK_DL, 6},     {"UL3-", 45, CT_CLOCK_UL, -3},
    {"U2+", 50, CT_CLOCK_U, 2},       {"R4-", 68, CT_CLOCK_R, -4},
    {"D1-", 83, CT_CLOCK_D, -1},      {"L0+", 84, CT_CLOCK_L, 0},
    {"ALL6+", 102, CT_CLOCK_ALL, 6},  {"ALL5-", 103, CT_CLOCK_ALL, -5},
    {"y2", 108, CT_CLOCK_MOVE_Y2, 0},
};

static const char *const ct_clock_bad[] = {
    "UR6-", "UR0-",  "UR7+", "UR05+", "UR5++", "UR-",   "UR2", "UR2x", "y2 ",  "y",
    "Y2",   "all1-", "A2+",  "URD2+", "0+",    "ALL6-", "",    "U2*",  "UR2 ",
};

static void expected_vector(int name, int8_t *vec) {
    static const int corner_sets[CT_CLOCK_NAMES][5] = {
        [CT_CLOCK_UR] = {1, 2},   [CT_CLOCK_DR] = {1, 8},   [CT_CLOCK_DL] = {1, 6},
        [CT_CLOCK_UL] = {1, 0},   [CT_CLOCK_U] = {2, 0, 2}, [CT_CLOCK_R] = {2, 2, 8},
        [CT_CLOCK_D] = {2, 6, 8}, [CT_CLOCK_L] = {2, 0, 6}, [CT_CLOCK_ALL] = {4, 0, 2, 6, 8},
    };
    memset(vec, 0, CT_CLOCK_CLOCKS);
    for (int k = 1; k <= corner_sets[name][0]; k++) {
        int corner = corner_sets[name][k];
        int r = corner / 3;
        int c = corner % 3;
        vec[3 * r + c] = 1;
        vec[3 * r + 1] = 1;
        vec[3 + c] = 1;
        vec[4] = 1;
        vec[CT_CLOCK_FACE_CLOCKS + 3 * r + (2 - c)] = -1;
    }
}

CT_SUITE(clock_identity) {
    const ct_geometry *g = ct_geometry_clock();
    CT_ASSERT(g != NULL);
    CT_ASSERT_STR_EQ(g->name, "clock");
    CT_ASSERT_EQ(g->sticker_count(CT_GEOMETRY_SINGLE_N), CT_CLOCK_CLOCKS);
    CT_ASSERT_EQ(g->sticker_count(CT_GEOMETRY_SINGLE_N), 18);
    CT_ASSERT_EQ(g->move_count(CT_GEOMETRY_SINGLE_N), CT_CLOCK_MOVES);
    CT_ASSERT_EQ(g->move_count(CT_GEOMETRY_SINGLE_N), 109);
    CT_ASSERT(g->sticker_count(0) < 0);
    CT_ASSERT(g->sticker_count(3) < 0);
    CT_ASSERT(g->move_count(0) < 0);
    CT_ASSERT(g->move_count(CT_GEOMETRY_MAX_N) < 0);
    CT_ASSERT(g->move_facelets_state == NULL);
}

CT_SUITE(clock_token) {
    const ct_geometry *g = ct_geometry_clock();
    for (size_t k = 0; k < sizeof ct_clock_ok / sizeof ct_clock_ok[0]; k++) {
        ct_move m = {9, 9, 9, 9};
        int got = g->move_from_token(g, ct_clock_ok[k].token, CT_GEOMETRY_SINGLE_N, &m);
        CT_ASSERT_EQ(got, ct_clock_ok[k].id);
        CT_ASSERT_EQ(m.face, ct_clock_ok[k].name);
        CT_ASSERT_EQ(m.depth_from, ct_clock_ok[k].amount);
        CT_ASSERT_EQ(m.depth_to, 0);
        CT_ASSERT_EQ(m.quarter, 0);
        CT_ASSERT(got < g->move_count(CT_GEOMETRY_SINGLE_N));
    }
    for (size_t k = 0; k < sizeof ct_clock_bad / sizeof ct_clock_bad[0]; k++) {
        ct_move m = {5, 5, 5, 5};
        CT_ASSERT(g->move_from_token(g, ct_clock_bad[k], CT_GEOMETRY_SINGLE_N, &m) < 0);
        CT_ASSERT_EQ(m.face, 5);
        CT_ASSERT_EQ(m.depth_from, 5);
    }
    ct_move m = {0, 0, 0, 0};
    CT_ASSERT(g->move_from_token(g, "U2+", 2, &m) < 0);
    CT_ASSERT(g->move_from_token(g, NULL, CT_GEOMETRY_SINGLE_N, &m) < 0);
    CT_ASSERT(g->move_from_token(g, "U2+", CT_GEOMETRY_SINGLE_N, NULL) < 0);
    CT_ASSERT(g->move_from_token(NULL, "U2+", CT_GEOMETRY_SINGLE_N, &m) < 0);
}

CT_SUITE(clock_move_ids) {
    int seen[CT_CLOCK_MOVES] = {0};
    for (int name = 0; name < CT_CLOCK_NAMES; name++) {
        for (int amount = 0; amount <= CT_CLOCK_AMOUNT_MAX; amount++) {
            int id = ct_clock_move_id(name, amount);
            CT_ASSERT(id >= 0 && id < CT_CLOCK_Y2);
            CT_ASSERT_EQ(id, name * CT_CLOCK_FORMS + amount);
            seen[id]++;
        }
        for (int amount = 1; amount < CT_CLOCK_AMOUNT_MAX; amount++) {
            int neg_id = ct_clock_move_id(name, -amount);
            CT_ASSERT_EQ(neg_id, name * CT_CLOCK_FORMS + CT_CLOCK_FORMS - amount);
            seen[neg_id]++;
        }
        CT_ASSERT(ct_clock_move_id(name, -CT_CLOCK_AMOUNT_MAX) < 0);
    }
    CT_ASSERT(ct_clock_move_id(CT_CLOCK_NAMES, 0) < 0);
    CT_ASSERT(ct_clock_move_id(0, 7) < 0);
    CT_ASSERT(ct_clock_move_id(0, -7) < 0);
    CT_ASSERT(ct_clock_move_id(-1, 1) < 0);
    for (int id = 0; id < CT_CLOCK_Y2; id++) {
        CT_ASSERT_EQ(seen[id], 1);
    }
}

CT_SUITE(clock_state_vectors) {
    const int amounts[] = {1, 2, 5};
    for (int name = 0; name < CT_CLOCK_NAMES; name++) {
        for (size_t a = 0; a < sizeof amounts / sizeof amounts[0]; a++) {
            int amount = amounts[a];
            int8_t vec[CT_CLOCK_CLOCKS];
            expected_vector(name, vec);
            ct_clock_state s;
            ct_clock_state_solved(&s);
            CT_ASSERT(s.flipped == false);
            CT_ASSERT_EQ(ct_clock_apply(&s, ct_clock_move_id(name, amount)), 0);
            for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
                int want = ((vec[i] * amount) % 12 + 12) % 12;
                CT_ASSERT_EQ(s.clocks[i], want);
            }
        }
    }
}

CT_SUITE(clock_state) {
    ct_clock_state s;
    ct_clock_state_solved(&s);
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_U, 2));
    for (int i = 0; i < 6; i++) {
        CT_ASSERT_EQ(s.clocks[i], 2);
    }
    for (int i = 6; i < 9; i++) {
        CT_ASSERT_EQ(s.clocks[i], 0);
    }
    CT_ASSERT_EQ(s.clocks[9], 10);
    CT_ASSERT_EQ(s.clocks[10], 0);
    CT_ASSERT_EQ(s.clocks[11], 10);
    for (int i = 12; i < 18; i++) {
        CT_ASSERT_EQ(s.clocks[i], 0);
    }

    ct_clock_state_solved(&s);
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_ALL, -1));
    for (int i = 0; i < 9; i++) {
        CT_ASSERT_EQ(s.clocks[i], 11);
    }
    CT_ASSERT_EQ(s.clocks[9], 1);
    CT_ASSERT_EQ(s.clocks[11], 1);
    CT_ASSERT_EQ(s.clocks[15], 1);
    CT_ASSERT_EQ(s.clocks[17], 1);
    CT_ASSERT_EQ(s.clocks[10], 0);
    CT_ASSERT_EQ(s.clocks[13], 0);

    ct_clock_state_solved(&s);
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_UR, 3));
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_UR, -3));
    ct_clock_state want;
    ct_clock_state_solved(&want);
    CT_ASSERT_EQ(memcmp(&s, &want, sizeof want), 0);

    ct_clock_state_solved(&s);
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_D, 6));
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_D, 6));
    CT_ASSERT_EQ(memcmp(&s, &want, sizeof want), 0);

    ct_clock_state_solved(&s);
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_L, 0));
    CT_ASSERT_EQ(memcmp(&s, &want, sizeof want), 0);

    ct_clock_state_solved(&s);
    ct_clock_apply(&s, CT_CLOCK_Y2);
    CT_ASSERT(s.flipped == true);
    CT_ASSERT_EQ(memcmp(s.clocks, want.clocks, CT_CLOCK_CLOCKS), 0);
    ct_clock_apply(&s, CT_CLOCK_Y2);
    CT_ASSERT(s.flipped == false);

    ct_clock_state_solved(&s);
    ct_clock_apply(&s, ct_clock_move_id(CT_CLOCK_R, 4));
    ct_clock_state t = s;
    ct_clock_apply(&s, CT_CLOCK_Y2);
    for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
        CT_ASSERT_EQ(s.clocks[i], t.clocks[(i + CT_CLOCK_FACE_CLOCKS) % CT_CLOCK_CLOCKS]);
    }

    ct_clock_state_solved(&s);
    CT_ASSERT(ct_clock_apply(&s, CT_CLOCK_MOVES) < 0);
    CT_ASSERT(ct_clock_apply(&s, -1) < 0);
    CT_ASSERT(ct_clock_apply(NULL, 0) < 0);
}

CT_SUITE(clock_perm) {
    const ct_geometry *g = ct_geometry_clock();
    uint8_t perm[PERM_BYTES];
    for (int id = 0; id < CT_CLOCK_Y2; id += 11) {
        CT_ASSERT(g->move_facelets(g, id, CT_GEOMETRY_SINGLE_N, perm) == CT_GEOMETRY_E_PERM);
    }
    CT_ASSERT(g->move_facelets(g, CT_CLOCK_Y2 - 1, CT_GEOMETRY_SINGLE_N, perm) ==
              CT_GEOMETRY_E_PERM);
    CT_ASSERT_EQ(g->move_facelets(g, CT_CLOCK_Y2, CT_GEOMETRY_SINGLE_N, perm), 0);
    for (int i = 0; i < CT_CLOCK_FACE_CLOCKS; i++) {
        CT_ASSERT_EQ(perm[i], CT_CLOCK_FACE_CLOCKS + i);
        CT_ASSERT_EQ(perm[CT_CLOCK_FACE_CLOCKS + i], i);
    }
    CT_ASSERT(g->move_facelets(g, -1, CT_GEOMETRY_SINGLE_N, perm) < 0);
    CT_ASSERT(g->move_facelets(g, CT_CLOCK_MOVES, CT_GEOMETRY_SINGLE_N, perm) < 0);
    CT_ASSERT(g->move_facelets(g, 0, 3, perm) < 0);
    CT_ASSERT(g->move_facelets(g, 0, CT_GEOMETRY_SINGLE_N, NULL) < 0);
    CT_ASSERT(g->move_facelets(NULL, 0, CT_GEOMETRY_SINGLE_N, perm) < 0);
}

CT_SUITE(clock_axis) {
    const ct_geometry *g = ct_geometry_clock();
    ct_v3 axis = {9, 9, 9};
    for (int id = 0; id < CT_CLOCK_Y2; id += 7) {
        CT_ASSERT_EQ(g->turn_axis(g, id, CT_GEOMETRY_SINGLE_N, &axis), 0);
        CT_ASSERT_EQ(axis.x, 0);
        CT_ASSERT_EQ(axis.y, 0);
        CT_ASSERT_EQ(axis.z, 1);
    }
    CT_ASSERT_EQ(g->turn_axis(g, CT_CLOCK_Y2, CT_GEOMETRY_SINGLE_N, &axis), 0);
    CT_ASSERT_EQ(axis.x, 0);
    CT_ASSERT_EQ(axis.y, 1);
    CT_ASSERT_EQ(axis.z, 0);
    CT_ASSERT(g->turn_axis(g, -1, CT_GEOMETRY_SINGLE_N, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, CT_CLOCK_MOVES, CT_GEOMETRY_SINGLE_N, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, 0, 2, &axis) < 0);
    CT_ASSERT(g->turn_axis(g, 0, CT_GEOMETRY_SINGLE_N, NULL) < 0);
    CT_ASSERT(g->turn_axis(NULL, 0, CT_GEOMETRY_SINGLE_N, &axis) < 0);
}

CT_SUITE(clock_stickers) {
    const ct_geometry *g = ct_geometry_clock();
    ct_v3 pos = {9, 9, 9};
    ct_v3 normal = {9, 9, 9};
    CT_ASSERT_EQ(g->sticker_geometry(g, 0, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, -2);
    CT_ASSERT_EQ(pos.y, 2);
    CT_ASSERT_EQ(pos.z, 2);
    CT_ASSERT_EQ(normal.x, 0);
    CT_ASSERT_EQ(normal.y, 0);
    CT_ASSERT_EQ(normal.z, 1);
    CT_ASSERT_EQ(g->sticker_geometry(g, 4, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 0);
    CT_ASSERT_EQ(pos.y, 0);
    CT_ASSERT_EQ(pos.z, 2);
    CT_ASSERT_EQ(g->sticker_geometry(g, 8, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 2);
    CT_ASSERT_EQ(pos.y, -2);
    CT_ASSERT_EQ(g->sticker_geometry(g, 9, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 2);
    CT_ASSERT_EQ(pos.y, 2);
    CT_ASSERT_EQ(pos.z, -2);
    CT_ASSERT_EQ(normal.z, -1);
    CT_ASSERT_EQ(g->sticker_geometry(g, 13, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, 0);
    CT_ASSERT_EQ(pos.y, 0);
    CT_ASSERT_EQ(pos.z, -2);
    CT_ASSERT_EQ(g->sticker_geometry(g, 17, CT_GEOMETRY_SINGLE_N, &pos, &normal), 0);
    CT_ASSERT_EQ(pos.x, -2);
    CT_ASSERT_EQ(pos.y, -2);

    ct_v3 positions[CT_CLOCK_CLOCKS];
    for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
        ct_v3 n = {0, 0, 0};
        CT_ASSERT_EQ(g->sticker_geometry(g, i, CT_GEOMETRY_SINGLE_N, &positions[i], &n), 0);
    }
    for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
        for (int j = i + 1; j < CT_CLOCK_CLOCKS; j++) {
            CT_ASSERT(positions[i].x != positions[j].x || positions[i].y != positions[j].y ||
                      positions[i].z != positions[j].z);
        }
    }
    CT_ASSERT(g->sticker_geometry(g, -1, CT_GEOMETRY_SINGLE_N, &pos, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, CT_CLOCK_CLOCKS, CT_GEOMETRY_SINGLE_N, &pos, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, 2, &pos, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, CT_GEOMETRY_SINGLE_N, NULL, &normal) < 0);
    CT_ASSERT(g->sticker_geometry(g, 0, CT_GEOMETRY_SINGLE_N, &pos, NULL) < 0);
    CT_ASSERT(g->sticker_geometry(NULL, 0, CT_GEOMETRY_SINGLE_N, &pos, &normal) < 0);
}

CT_SUITE(clock_scramble_form) {
    static const char scramble[] =
        "UR2+ DR1- DL4+ UL0+ U3+ R5- D2+ L1- ALL2+ y2 U1+ R3- D2- L4- ALL4-";
    const char *cursor = scramble;
    int count = 0;
    while (*cursor != '\0') {
        while (*cursor == ' ') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }
        char tok[8];
        unsigned len = 0;
        while (cursor[len] != '\0' && cursor[len] != ' ') {
            len++;
        }
        CT_ASSERT(len < sizeof tok);
        memcpy(tok, cursor, len);
        tok[len] = '\0';
        cursor += len;
        ct_move m;
        CT_ASSERT(ct_geometry_clock()->move_from_token(ct_geometry_clock(), tok,
                                                       CT_GEOMETRY_SINGLE_N, &m) >= 0);
        count++;
    }
    CT_ASSERT_EQ(count, 15);
}

CT_SUITE(clock_fuzz) {
    const ct_geometry *g = ct_geometry_clock();
    static const char alphabet[] = "URDLAE0123456789+-y2 ";
    ct_sfc64 rng;
    ct_sfc64_init(&rng, FUZZ_SEED);
    for (int iter = 0; iter < 4000; iter++) {
        char token[10];
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
        CT_ASSERT(id >= 0 && id < CT_CLOCK_MOVES);
        if (id == CT_CLOCK_Y2) {
            CT_ASSERT_EQ(m.face, CT_CLOCK_MOVE_Y2);
        } else {
            CT_ASSERT(m.face >= 0 && m.face < CT_CLOCK_NAMES);
            CT_ASSERT(m.depth_from >= -CT_CLOCK_AMOUNT_MAX && m.depth_from <= CT_CLOCK_AMOUNT_MAX);
            CT_ASSERT_EQ(ct_clock_move_id(m.face, m.depth_from), id);
        }
    }
}
