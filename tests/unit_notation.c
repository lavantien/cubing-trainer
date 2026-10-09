#include "engine/cube.h"
#include "engine/notation.h"
#include "framework.h"

#include <string.h>

typedef struct {
    const char *token;
    int n;
    int face;
    int depth_from;
    int depth_to;
    int quarter;
} ct_notation_golden;

static const ct_notation_golden ct_goldens[] = {
    {"R", 3, CT_FACE_R, 1, 1, 1},  {"R'", 3, CT_FACE_R, 1, 1, 3},  {"R2", 3, CT_FACE_R, 1, 1, 2},
    {"2R", 3, CT_FACE_R, 2, 2, 1}, {"Rw", 3, CT_FACE_R, 1, 2, 1},  {"r", 3, CT_FACE_R, 1, 2, 1},
    {"3R", 3, CT_FACE_R, 3, 3, 1}, {"2Rw", 3, CT_FACE_R, 1, 2, 1}, {"R'2", 3, CT_FACE_R, 1, 1, 2},
    {"M", 3, CT_FACE_L, 2, 2, 1},  {"M'", 3, CT_FACE_L, 2, 2, 3},  {"E2", 3, CT_FACE_D, 2, 2, 2},
    {"S", 3, CT_FACE_F, 2, 2, 1},  {"x", 3, CT_FACE_R, 1, 2, 1},   {"x'", 3, CT_FACE_R, 1, 2, 3},
    {"y2", 3, CT_FACE_U, 1, 2, 2}, {"z", 3, CT_FACE_F, 1, 2, 1},   {"R", 7, CT_FACE_R, 1, 1, 1},
    {"2R", 7, CT_FACE_R, 2, 2, 1}, {"3R", 7, CT_FACE_R, 3, 3, 1},  {"3Rw", 7, CT_FACE_R, 1, 3, 1},
    {"Rw", 7, CT_FACE_R, 1, 2, 1}, {"r", 7, CT_FACE_R, 1, 2, 1},   {"M", 7, CT_FACE_L, 4, 4, 1},
    {"M'", 7, CT_FACE_L, 4, 4, 3}, {"E2", 7, CT_FACE_D, 4, 4, 2},  {"S", 7, CT_FACE_F, 4, 4, 1},
    {"x", 7, CT_FACE_R, 1, 4, 1},  {"x'", 7, CT_FACE_R, 1, 4, 3},  {"y2", 7, CT_FACE_U, 1, 4, 2},
    {"z", 7, CT_FACE_F, 1, 4, 1},  {"M", 4, CT_FACE_L, 2, 2, 1},   {"x", 4, CT_FACE_R, 1, 2, 1},
    {"R", 1, CT_FACE_R, 1, 1, 1},
};

typedef struct {
    const char *token;
    int n;
    const char *canonical;
} ct_notation_canonical;

static const ct_notation_canonical ct_canonicals[] = {
    {"R", 3, "R"},   {"R'", 3, "R'"}, {"R2", 3, "R2"}, {"R'2", 3, "R2"},  {"2R", 3, "2R"},
    {"3R", 3, "3R"}, {"Rw", 3, "x"},  {"r", 3, "x"},   {"2Rw", 3, "x"},   {"M", 3, "M"},
    {"M'", 3, "M'"}, {"E2", 3, "E2"}, {"S", 3, "S"},   {"2L", 3, "M"},    {"x", 3, "x"},
    {"x'", 3, "x'"}, {"y2", 3, "y2"}, {"z", 3, "z"},   {"R", 7, "R"},     {"2R", 7, "2R"},
    {"3R", 7, "3R"}, {"Rw", 7, "Rw"}, {"r", 7, "Rw"},  {"2Rw", 7, "Rw"},  {"3Rw", 7, "3Rw"},
    {"4Rw", 7, "x"}, {"4L", 7, "M"},  {"4F", 7, "S"},  {"4D", 7, "E"},    {"M", 7, "M"},
    {"E2", 7, "E2"}, {"S", 7, "S"},   {"x", 7, "x"},   {"x'", 7, "x'"},   {"y2", 7, "y2"},
    {"z", 7, "z"},   {"Rw", 4, "x"},  {"2L", 4, "M"},  {"3Rw", 4, "3Rw"}, {"3Rw", 5, "x"},
    {"3L", 5, "M"},  {"3F", 5, "S"},  {"Rw", 5, "Rw"}, {"R", 1, "x"},     {"S", 1, "z"},
};

static bool ct_move_eq(ct_move a, ct_move b) {
    return a.face == b.face && a.depth_from == b.depth_from && a.depth_to == b.depth_to &&
           a.quarter == b.quarter;
}

CT_SUITE(notation_goldens) {
    for (size_t k = 0; k < sizeof ct_goldens / sizeof ct_goldens[0]; k++) {
        const ct_notation_golden *g = &ct_goldens[k];
        ct_move m = {0, 0, 0, 0};
        int r = ct_notation_parse_one(g->token, g->n, &m);
        CT_ASSERT_EQ(r, 0);
        CT_ASSERT_EQ(m.face, g->face);
        CT_ASSERT_EQ(m.depth_from, g->depth_from);
        CT_ASSERT_EQ(m.depth_to, g->depth_to);
        CT_ASSERT_EQ(m.quarter, g->quarter);
    }
}

CT_SUITE(notation_canonical) {
    for (size_t k = 0; k < sizeof ct_canonicals / sizeof ct_canonicals[0]; k++) {
        const ct_notation_canonical *c = &ct_canonicals[k];
        ct_move m = {0, 0, 0, 0};
        int r = ct_notation_parse_one(c->token, c->n, &m);
        CT_ASSERT_EQ(r, 0);
        char buf[CT_NOTATION_TOKEN_MAX];
        int len = ct_notation_format_one(&m, c->n, buf, sizeof buf);
        CT_ASSERT_EQ(len, (int)strlen(c->canonical));
        CT_ASSERT_STR_EQ(buf, c->canonical);
        ct_move back = {0, 0, 0, 0};
        r = ct_notation_parse_one(c->canonical, c->n, &back);
        CT_ASSERT_EQ(r, 0);
        CT_ASSERT(ct_move_eq(m, back));
    }
}

CT_SUITE(notation_roundtrip) {
    const int sizes[] = {3, 4, 5, 7};
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        int n = sizes[s];
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
            for (int d = 1; d <= n; d++) {
                for (int quarter = 1; quarter <= 3; quarter++) {
                    ct_move m = {face, d, d, quarter};
                    char buf[CT_NOTATION_TOKEN_MAX];
                    int len = ct_notation_format_one(&m, n, buf, sizeof buf);
                    CT_ASSERT_EQ(len >= 0, true);
                    ct_move back = {0, 0, 0, 0};
                    int r = ct_notation_parse_one(buf, n, &back);
                    CT_ASSERT_EQ(r, 0);
                    CT_ASSERT(ct_move_eq(m, back));
                }
            }
            for (int d = 2; d <= n; d++) {
                for (int quarter = 1; quarter <= 3; quarter++) {
                    ct_move m = {face, 1, d, quarter};
                    char buf[CT_NOTATION_TOKEN_MAX];
                    int len = ct_notation_format_one(&m, n, buf, sizeof buf);
                    CT_ASSERT_EQ(len >= 0, true);
                    ct_move back = {0, 0, 0, 0};
                    int r = ct_notation_parse_one(buf, n, &back);
                    CT_ASSERT_EQ(r, 0);
                    CT_ASSERT(ct_move_eq(m, back));
                }
            }
        }
    }
}

CT_SUITE(notation_alg) {
    ct_move moves[8];
    int count = ct_notation_parse("  R U R'  U' ", 3, moves, 8);
    CT_ASSERT_EQ(count, 4);
    CT_ASSERT(ct_move_eq(moves[0], (ct_move){CT_FACE_R, 1, 1, 1}));
    CT_ASSERT(ct_move_eq(moves[1], (ct_move){CT_FACE_U, 1, 1, 1}));
    CT_ASSERT(ct_move_eq(moves[2], (ct_move){CT_FACE_R, 1, 1, 3}));
    CT_ASSERT(ct_move_eq(moves[3], (ct_move){CT_FACE_U, 1, 1, 3}));

    char buf[64];
    int len = ct_notation_format(moves, count, 3, buf, sizeof buf);
    CT_ASSERT_EQ(len, 9);
    CT_ASSERT_STR_EQ(buf, "R U R' U'");

    count = ct_notation_parse("", 3, moves, 8);
    CT_ASSERT_EQ(count, 0);
    count = ct_notation_parse("   ", 3, moves, 8);
    CT_ASSERT_EQ(count, 0);
    len = ct_notation_format(moves, 0, 3, buf, sizeof buf);
    CT_ASSERT_EQ(len, 0);
    CT_ASSERT_STR_EQ(buf, "");

    CT_ASSERT_EQ(ct_notation_parse("R U R' U'", 3, moves, 2), CT_NOTATION_E_CAP);

    ct_move m = {CT_FACE_R, 1, 1, 3};
    CT_ASSERT_EQ(ct_notation_format_one(&m, 3, buf, 2), CT_NOTATION_E_CAP);
    ct_move pair[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 1}};
    CT_ASSERT_EQ(ct_notation_format(pair, 2, 3, buf, 3), CT_NOTATION_E_CAP);
    CT_ASSERT_EQ(ct_notation_format(pair, 2, 3, buf, 4), 3);
    CT_ASSERT_STR_EQ(buf, "R U");

    ct_move bad_span = {CT_FACE_U, 2, 3, 1};
    CT_ASSERT_EQ(ct_notation_format_one(&bad_span, 3, buf, sizeof buf), CT_NOTATION_E_MOVE);
    ct_move bad_quarter = {CT_FACE_R, 1, 1, 4};
    CT_ASSERT_EQ(ct_notation_format_one(&bad_quarter, 3, buf, sizeof buf), CT_NOTATION_E_MOVE);
    ct_move bad_depth = {CT_FACE_R, 1, 8, 1};
    CT_ASSERT_EQ(ct_notation_format_one(&bad_depth, 7, buf, sizeof buf), CT_NOTATION_E_MOVE);
    ct_move bad_face = {CT_CUBE_FACE_COUNT, 1, 1, 1};
    CT_ASSERT_EQ(ct_notation_format_one(&bad_face, 3, buf, sizeof buf), CT_NOTATION_E_MOVE);
    CT_ASSERT_EQ(ct_notation_format(&bad_span, 1, 3, buf, sizeof buf), CT_NOTATION_E_MOVE);

    CT_ASSERT_EQ(ct_notation_parse(NULL, 3, moves, 8), CT_NOTATION_E_INPUT);
    CT_ASSERT_EQ(ct_notation_parse("R", 0, moves, 8), CT_NOTATION_E_INPUT);
    CT_ASSERT_EQ(ct_notation_parse("R", CT_CUBE_MAX_N + 1, moves, 8), CT_NOTATION_E_INPUT);
    CT_ASSERT_EQ(ct_notation_parse_one(NULL, 3, &m), CT_NOTATION_E_INPUT);
    CT_ASSERT_EQ(ct_notation_format_one(NULL, 3, buf, sizeof buf), CT_NOTATION_E_INPUT);
}

CT_SUITE(notation_errors) {
    ct_move m;
    CT_ASSERT_EQ(ct_notation_parse_one("1R", 3, &m), CT_NOTATION_E_DIGIT);
    CT_ASSERT_EQ(ct_notation_parse_one("8R", 3, &m), CT_NOTATION_E_DIGIT);
    CT_ASSERT_EQ(ct_notation_parse_one("0R", 3, &m), CT_NOTATION_E_DIGIT);
    CT_ASSERT_EQ(ct_notation_parse_one("9Rw", 3, &m), CT_NOTATION_E_DIGIT);
    CT_ASSERT_EQ(ct_notation_parse_one("5R", 3, &m), CT_NOTATION_E_DEPTH);
    CT_ASSERT_EQ(ct_notation_parse_one("3Rw", 3, &m), 0);
    CT_ASSERT(ct_move_eq(m, (ct_move){CT_FACE_R, 1, 3, 1}));
    CT_ASSERT_EQ(ct_notation_parse_one("4Rw", 3, &m), CT_NOTATION_E_DEPTH);
    CT_ASSERT_EQ(ct_notation_parse_one("Rw", 1, &m), CT_NOTATION_E_DEPTH);
    CT_ASSERT_EQ(ct_notation_parse_one("5R", 5, &m), 0);
    CT_ASSERT(ct_move_eq(m, (ct_move){CT_FACE_R, 5, 5, 1}));
    CT_ASSERT_EQ(ct_notation_parse_one("2Rw", 3, &m), 0);
    CT_ASSERT(ct_move_eq(m, (ct_move){CT_FACE_R, 1, 2, 1}));

    CT_ASSERT_EQ(ct_notation_parse_one("Rr", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("Rx", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("R'q", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("xw", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("Mw", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("rw", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("R2'", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("R''", 3, &m), CT_NOTATION_E_SUFFIX);
    CT_ASSERT_EQ(ct_notation_parse_one("R'2'", 3, &m), CT_NOTATION_E_SUFFIX);

    CT_ASSERT_EQ(ct_notation_parse_one("2x", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("2M", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("m", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("e", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("s", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("X", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("Y", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("Z", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("2", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("w", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("'", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse_one("22R", 3, &m), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse("R q", 3, &m, 4), CT_NOTATION_E_TOKEN);
    CT_ASSERT_EQ(ct_notation_parse("R R'q", 3, &m, 4), CT_NOTATION_E_SUFFIX);
}

CT_SUITE(notation_fuzz) {
    static const char alphabet[] = "UDLRFBudlrfbMESxyzxyzw'2 013456789";
    ct_sfc64 rng;
    ct_sfc64_init(&rng, 20261010);
    for (int iter = 0; iter < 4000; iter++) {
        char token[12];
        unsigned len = (unsigned)ct_sfc64_range(&rng, sizeof token);
        for (unsigned k = 0; k < len; k++)
            token[k] = alphabet[ct_sfc64_range(&rng, sizeof alphabet - 1)];
        token[len] = '\0';
        int n = 3 + (int)ct_sfc64_range(&rng, 5);
        ct_move m;
        int r = ct_notation_parse_one(token, n, &m);
        if (r != 0) {
            CT_ASSERT(r < 0);
            continue;
        }
        CT_ASSERT(m.face >= 0 && m.face < CT_CUBE_FACE_COUNT);
        CT_ASSERT(m.quarter >= 1 && m.quarter <= 3);
        CT_ASSERT(m.depth_from >= 1 && m.depth_from <= m.depth_to && m.depth_to <= n);
        char buf[CT_NOTATION_TOKEN_MAX];
        int flen = ct_notation_format_one(&m, n, buf, sizeof buf);
        CT_ASSERT_EQ(flen >= 0, true);
        ct_move back;
        CT_ASSERT_EQ(ct_notation_parse_one(buf, n, &back), 0);
        CT_ASSERT(ct_move_eq(m, back));

        char alg[48];
        unsigned alen = (unsigned)ct_sfc64_range(&rng, sizeof alg);
        for (unsigned k = 0; k < alen; k++)
            alg[k] = alphabet[ct_sfc64_range(&rng, sizeof alphabet - 1)];
        alg[alen] = '\0';
        ct_move moves[16];
        int count = ct_notation_parse(alg, n, moves, 16);
        if (count >= 0) {
            CT_ASSERT(count <= 16);
            char out[128];
            int olen = ct_notation_format(moves, count, n, out, sizeof out);
            CT_ASSERT_EQ(olen >= 0, true);
            if (olen > 0) {
                ct_move reparsed[16];
                CT_ASSERT_EQ(ct_notation_parse(out, n, reparsed, 16), count);
                for (int k = 0; k < count; k++)
                    CT_ASSERT(ct_move_eq(moves[k], reparsed[k]));
            }
        }
    }
}

CT_SUITE(notation_engine) {
    const char *tokens[] = {"R",  "L",  "U", "D",  "F",  "B", "R'", "U2",
                            "2R", "Rw", "M", "E2", "S'", "x", "y'", "z2"};
    for (size_t k = 0; k < sizeof tokens / sizeof tokens[0]; k++) {
        ct_move m;
        int r = ct_notation_parse_one(tokens[k], 3, &m);
        CT_ASSERT_EQ(r, 0);
        ct_cube c;
        ct_cube_init(&c, 3);
        ct_apply(&c, m);
        CT_ASSERT_EQ(ct_cube_solved(&c), false);
    }
    const char *big[] = {"3R", "x", "M", "2L", "3Fw", "5R'"};
    for (size_t k = 0; k < sizeof big / sizeof big[0]; k++) {
        ct_move m;
        int r = ct_notation_parse_one(big[k], 5, &m);
        CT_ASSERT_EQ(r, 0);
        ct_cube c;
        ct_cube_init(&c, 5);
        ct_apply(&c, m);
        CT_ASSERT_EQ(ct_cube_solved(&c), false);
    }

    ct_cube scrambled;
    ct_cube_init(&scrambled, 3);
    const char *scramble = "R U F2 M";
    ct_move sm[8];
    int sc = ct_notation_parse(scramble, 3, sm, 8);
    CT_ASSERT_EQ(sc, 4);
    for (int k = 0; k < sc; k++)
        ct_apply(&scrambled, sm[k]);
    CT_ASSERT_EQ(ct_cube_solved(&scrambled), false);

    ct_cube rotated = scrambled;
    ct_move xm[8];
    int xc = ct_notation_parse("x x x x", 3, xm, 8);
    CT_ASSERT_EQ(xc, 4);
    for (int k = 0; k < xc; k++) {
        CT_ASSERT_EQ(xm[k].quarter, 1);
        CT_ASSERT(ct_move_eq(xm[k], xm[0]));
        ct_apply(&rotated, xm[k]);
    }
    CT_ASSERT(ct_cube_equal(&scrambled, &rotated));
}
