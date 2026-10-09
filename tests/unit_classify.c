#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "engine/transform.h"
#include "framework.h"
#include "gen/classify.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    TEST_CELLS = 54,
    TEST_ROW_MAX = 32,
    TEST_FIELD_MAX = 256,
    TEST_LINE_MAX = 512,
    TEST_ALG_MAX = CT_CLASSIFY_MOVES_MAX,
    TEST_TPL_MAX = CT_CLASSIFY_MOVES_MAX + CT_CLASSIFY_SETUP_MAX * 2 + CT_CLASSIFY_PART_MAX * 2,
    TEST_FIXTURE_ROWS = 21,
    TEST_SYNTH_SAMPLE_STEP = 16,
    TEST_TARGET_SAMPLES = 20,
    TEST_STRESS_ROUNDS = 400,
    TEST_MOVE_KINDS = 6,
    TEST_CORNER_PIECES = 8,
    TEST_EDGE_PIECES = 12,
};

static int test_raw(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int test_compact(int face, int row, int col) {
    return face * CT_CLASSIFY_N * CT_CLASSIFY_N + row * CT_CLASSIFY_N + col;
}

static int test_sticker_compact(int orbit, int sticker) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return test_compact(face, row, col);
}

static void test_identity(ct_cube *c) {
    memset(c, 0, sizeof *c);
    c->n = CT_CLASSIFY_N;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_CLASSIFY_N; row++) {
            for (int col = 0; col < CT_CLASSIFY_N; col++) {
                c->facelet[test_raw(face, row, col)] = (uint8_t)test_compact(face, row, col);
            }
        }
    }
}

static void test_simulate(const ct_move *seq, int len, uint8_t state[TEST_CELLS]) {
    ct_cube c;
    test_identity(&c);
    for (int i = 0; i < len; i++) {
        ct_apply(&c, seq[i]);
    }
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < CT_CLASSIFY_N; row++) {
            for (int col = 0; col < CT_CLASSIFY_N; col++) {
                state[test_compact(face, row, col)] = c.facelet[test_raw(face, row, col)];
            }
        }
    }
}

static int test_parse(const char *alg, ct_move *out) {
    return ct_notation_parse(alg, CT_CLASSIFY_N, out, TEST_ALG_MAX);
}

static int test_piece_of(int orbit, int sticker) {
    return orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(sticker) : ct_edge_piece_of(sticker);
}

static void test_dest_map(const uint8_t state[TEST_CELLS], int dest[TEST_CELLS]) {
    for (int q = 0; q < TEST_CELLS; q++) {
        dest[state[q]] = q;
    }
}

static bool test_centers_fixed(const uint8_t state[TEST_CELLS]) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        int mid = test_compact(face, 1, 1);
        if (state[mid] != (uint8_t)mid) {
            return false;
        }
    }
    return true;
}

static int test_cell_sticker(int orbit, int cell) {
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        if (test_sticker_compact(orbit, s) == cell) {
            return s;
        }
    }
    return CT_TARGET_NONE;
}

static bool test_orbit_transposition(const uint8_t state[TEST_CELLS], int orbit, int pieces,
                                     int *swaps) {
    int dest[TEST_CELLS];
    test_dest_map(state, dest);
    bool seen[TEST_EDGE_PIECES] = {false};
    int count = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int piece = test_piece_of(orbit, s);
        if (seen[piece]) {
            continue;
        }
        seen[piece] = true;
        int cell = test_sticker_compact(orbit, s);
        int t = test_cell_sticker(orbit, dest[cell]);
        if (t == CT_TARGET_NONE) {
            return false;
        }
        int target = test_piece_of(orbit, t);
        if (target != piece) {
            if (seen[target]) {
                return false;
            }
            seen[target] = true;
            count++;
        }
    }
    *swaps = count;
    return count == 1 && pieces > 0;
}

static bool test_orbit_home(const uint8_t state[TEST_CELLS], int orbit) {
    int dest[TEST_CELLS];
    test_dest_map(state, dest);
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int cell = test_sticker_compact(orbit, s);
        int t = test_cell_sticker(orbit, dest[cell]);
        if (t == CT_TARGET_NONE) {
            return false;
        }
        if (test_piece_of(orbit, t) != test_piece_of(orbit, s)) {
            return false;
        }
    }
    return true;
}

static bool test_is_parity(const uint8_t state[TEST_CELLS]) {
    if (!test_centers_fixed(state)) {
        return false;
    }
    int corner_swaps = 0;
    int edge_swaps = 0;
    if (!test_orbit_transposition(state, CT_ORBIT_CORNER, TEST_CORNER_PIECES, &corner_swaps)) {
        return false;
    }
    if (!test_orbit_transposition(state, CT_ORBIT_EDGE, TEST_EDGE_PIECES, &edge_swaps)) {
        return false;
    }
    return corner_swaps == 1 && edge_swaps == 1;
}

static bool test_is_flip(const uint8_t state[TEST_CELLS]) {
    if (!test_centers_fixed(state)) {
        return false;
    }
    if (!test_orbit_home(state, CT_ORBIT_CORNER) || !test_orbit_home(state, CT_ORBIT_EDGE)) {
        return false;
    }
    int dest[TEST_CELLS];
    test_dest_map(state, dest);
    int flipped = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        if (dest[test_sticker_compact(CT_ORBIT_EDGE, s)] !=
            test_sticker_compact(CT_ORBIT_EDGE, s)) {
            flipped++;
        }
        if (dest[test_sticker_compact(CT_ORBIT_CORNER, s)] !=
            test_sticker_compact(CT_ORBIT_CORNER, s)) {
            return false;
        }
    }
    return flipped >= CT_TARGET_EDGE_STICKERS_PER_PIECE;
}

static bool test_is_twist(const uint8_t state[TEST_CELLS]) {
    if (!test_centers_fixed(state)) {
        return false;
    }
    if (!test_orbit_home(state, CT_ORBIT_CORNER) || !test_orbit_home(state, CT_ORBIT_EDGE)) {
        return false;
    }
    int dest[TEST_CELLS];
    test_dest_map(state, dest);
    int twisted = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        if (dest[test_sticker_compact(CT_ORBIT_CORNER, s)] !=
            test_sticker_compact(CT_ORBIT_CORNER, s)) {
            twisted++;
        }
        if (dest[test_sticker_compact(CT_ORBIT_EDGE, s)] !=
            test_sticker_compact(CT_ORBIT_EDGE, s)) {
            return false;
        }
    }
    return twisted >= CT_TARGET_CORNER_STICKERS_PER_PIECE;
}

static bool test_rebuild_equal(const ct_classify_result *r, const ct_move *seq, int len) {
    if (r->cls == CT_CLS_UNKNOWN || r->cls == CT_CLS_PARITY || r->cls == CT_CLS_FLIP ||
        r->cls == CT_CLS_TWIST) {
        return true;
    }
    ct_move tpl[TEST_TPL_MAX];
    int n = r->setup_len;
    if (n > CT_CLASSIFY_SETUP_MAX) {
        return false;
    }
    memcpy(tpl, r->setup, (size_t)n * sizeof *tpl);
    if (r->cyclic) {
        ct_move a[CT_CLASSIFY_PART_MAX];
        int alen = r->insertion_len;
        if (alen > CT_CLASSIFY_PART_MAX || r->interchange_len != 1) {
            return false;
        }
        memcpy(a, r->insertion, (size_t)alen * sizeof *a);
        ct_move ainv[CT_CLASSIFY_PART_MAX];
        int ainv_len = ct_transform_inverse(a, alen, ainv);
        memcpy(tpl + n, a, (size_t)alen * sizeof *a);
        n += alen;
        tpl[n++] = r->interchange[0];
        memcpy(tpl + n, ainv, (size_t)ainv_len * sizeof *ainv);
        n += ainv_len;
        for (int i = alen - 1; i >= 0; i--) {
            tpl[n++] = a[i];
        }
        tpl[n++] = r->interchange[0];
        ct_move c[CT_CLASSIFY_PART_MAX];
        int clen = 0;
        for (int i = alen - 1; i >= 0; i--) {
            c[clen++] = a[i];
        }
        ct_move cinv[CT_CLASSIFY_PART_MAX];
        int cinv_len = ct_transform_inverse(c, clen, cinv);
        memcpy(tpl + n, cinv, (size_t)cinv_len * sizeof *cinv);
        n += cinv_len;
    } else {
        const ct_move *first = r->insert_first ? r->insertion : r->interchange;
        const ct_move *second = r->insert_first ? r->interchange : r->insertion;
        int flen = r->insert_first ? r->insertion_len : r->interchange_len;
        int slen = r->insert_first ? r->interchange_len : r->insertion_len;
        if (flen > CT_CLASSIFY_PART_MAX || slen > CT_CLASSIFY_PART_MAX) {
            return false;
        }
        memcpy(tpl + n, first, (size_t)flen * sizeof *first);
        n += flen;
        memcpy(tpl + n, second, (size_t)slen * sizeof *second);
        n += slen;
        ct_move finv[CT_CLASSIFY_PART_MAX];
        ct_move sinv[CT_CLASSIFY_PART_MAX];
        int finv_len = ct_transform_inverse(first, flen, finv);
        int sinv_len = ct_transform_inverse(second, slen, sinv);
        memcpy(tpl + n, finv, (size_t)finv_len * sizeof *finv);
        n += finv_len;
        memcpy(tpl + n, sinv, (size_t)sinv_len * sizeof *sinv);
        n += sinv_len;
    }
    ct_move setup_inv[CT_CLASSIFY_SETUP_MAX];
    int setup_inv_len = ct_transform_inverse(r->setup, r->setup_len, setup_inv);
    memcpy(tpl + n, setup_inv, (size_t)setup_inv_len * sizeof *setup_inv);
    n += setup_inv_len;
    uint8_t a[TEST_CELLS];
    uint8_t b[TEST_CELLS];
    test_simulate(tpl, n, a);
    test_simulate(seq, len, b);
    return memcmp(a, b, (size_t)TEST_CELLS) == 0;
}

static int test_trim_field(char *text) {
    int len = (int)strlen(text);
    int start = 0;
    while (start < len && (text[start] == ' ' || text[start] == '\t')) {
        start++;
    }
    int end = len;
    while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\n' ||
                           text[end - 1] == '\r')) {
        end--;
    }
    text[end] = '\0';
    return start;
}

static int test_load_fixture(char orbits[TEST_ROW_MAX][8], char classes[TEST_ROW_MAX][24],
                             char algs[TEST_ROW_MAX][TEST_FIELD_MAX]) {
    FILE *file = fopen("tests/fixtures/bh_classification.md", "r");
    if (file == NULL) {
        return -1;
    }
    char line[TEST_LINE_MAX];
    int rows = 0;
    bool in_rows = false;
    while (fgets(line, sizeof line, file) != NULL) {
        if (strncmp(line, "## Example rows", 15) == 0) {
            in_rows = true;
            continue;
        }
        if (!in_rows || line[0] != '|' || strncmp(line, "| orbit", 6) == 0 ||
            strncmp(line, "| ---", 5) == 0) {
            continue;
        }
        char orbit[8] = {0};
        char cls[24] = {0};
        char cycle[32] = {0};
        char alg[TEST_FIELD_MAX] = {0};
        if (sscanf(line, "| %7[^|]| %23[^|]| %31[^|]| %255[^|]|", orbit, cls, cycle, alg) != 4) {
            continue;
        }
        if (rows >= TEST_ROW_MAX) {
            fclose(file);
            return -2;
        }
        snprintf(orbits[rows], sizeof orbits[rows], "%s", orbit + test_trim_field(orbit));
        snprintf(classes[rows], sizeof classes[rows], "%s", cls + test_trim_field(cls));
        snprintf(algs[rows], sizeof algs[rows], "%s", alg + test_trim_field(alg));
        rows++;
    }
    fclose(file);
    return rows;
}

CT_SUITE(unit_classify) {
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_PURE), "pure");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_A9), "A9");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_B9), "B9");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_ORTHOGONAL), "orthogonal");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_CYCLIC_SHIFT), "cyclic shift");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_COLUMN), "column");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_PER_SPECIAL), "per special");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_HALF_SLICE_PLANE), "half slice-plane");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_SP5), "SP5");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_SP7), "SP7");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_SP9), "SP9");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_PARITY), "parity");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_FLIP), "flip");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_TWIST), "twist");
    CT_ASSERT_STR_EQ(ct_classification_label(CT_CLS_UNKNOWN), "unknown");
    CT_ASSERT_STR_EQ(ct_classification_alias(CT_CLS_ORTHOGONAL), "perpendicular");
    CT_ASSERT_STR_EQ(ct_classification_alias(CT_CLS_CYCLIC_SHIFT), "collision");
    CT_ASSERT_STR_EQ(ct_classification_alias(CT_CLS_COLUMN), "simultaneous");
    CT_ASSERT_STR_EQ(ct_classification_alias(CT_CLS_PURE), "pure");
    CT_ASSERT_STR_EQ(ct_classification_alias(CT_CLS_UNKNOWN), "unknown");
    static const ct_classification all[] = {
        CT_CLS_PURE,         CT_CLS_A9,     CT_CLS_B9,          CT_CLS_ORTHOGONAL,
        CT_CLS_CYCLIC_SHIFT, CT_CLS_COLUMN, CT_CLS_PER_SPECIAL, CT_CLS_HALF_SLICE_PLANE,
        CT_CLS_SP5,          CT_CLS_SP7,    CT_CLS_SP9,         CT_CLS_PARITY,
        CT_CLS_FLIP,         CT_CLS_TWIST,  CT_CLS_UNKNOWN,
    };
    for (size_t i = 0; i < sizeof all / sizeof all[0]; i++) {
        CT_ASSERT_EQ(ct_classification_from_label(ct_classification_label(all[i])), all[i]);
    }
    CT_ASSERT_EQ(ct_classification_from_label("no such class"), CT_CLS_UNKNOWN);
    CT_ASSERT_EQ(ct_classification_from_label(NULL), CT_CLS_UNKNOWN);

    ct_classify_result r;
    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify(NULL, 0, &r), CT_CLASSIFY_E_ARG);
    CT_ASSERT(ct_classify(NULL, 1, &r) != CT_CLASSIFY_OK);
    ct_move seq[TEST_ALG_MAX];
    CT_ASSERT(ct_classify(seq, -1, &r) != CT_CLASSIFY_OK);
    CT_ASSERT(ct_classify(seq, 1, NULL) != CT_CLASSIFY_OK);
    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify_alg("", &r), CT_CLASSIFY_OK);
    CT_ASSERT_EQ(r.cls, CT_CLS_UNKNOWN);
    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify_alg("R", &r), CT_CLASSIFY_OK);
    CT_ASSERT_EQ(r.cls, CT_CLS_UNKNOWN);
    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify_alg("R L", &r), CT_CLASSIFY_OK);
    CT_ASSERT_EQ(r.cls, CT_CLS_UNKNOWN);
    CT_ASSERT(ct_classify_alg("Q7", &r) != CT_CLASSIFY_OK);
    CT_ASSERT(ct_classify_alg(NULL, &r) != CT_CLASSIFY_OK);
    CT_ASSERT(ct_classify_alg("R U", NULL) != CT_CLASSIFY_OK);

    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify_alg("R U R' U'", &r), CT_CLASSIFY_OK);
    CT_ASSERT_EQ(r.cls, CT_CLS_HALF_SLICE_PLANE);
    int len = test_parse("R U R' U'", seq);
    CT_ASSERT(len > 0);
    CT_ASSERT(test_rebuild_equal(&r, seq, len));

    memset(&r, 0, sizeof r);
    const char *rot_alg = "y' M' U2 M U2 M' U2 M U2 y";
    CT_ASSERT_EQ(ct_classify_alg(rot_alg, &r), CT_CLASSIFY_OK);
    len = test_parse(rot_alg, seq);
    CT_ASSERT(len > 0);
    if (r.cls != CT_CLS_UNKNOWN) {
        CT_ASSERT(test_rebuild_equal(&r, seq, len));
    }

    memset(&r, 0, sizeof r);
    ct_move big[CT_CLASSIFY_MOVES_MAX + 8];
    for (int i = 0; i < CT_CLASSIFY_MOVES_MAX + 4; i++) {
        big[i].face = CT_FACE_R;
        big[i].depth_from = 1;
        big[i].depth_to = 1;
        big[i].quarter = 1;
    }
    CT_ASSERT_EQ(ct_classify(big, CT_CLASSIFY_MOVES_MAX + 4, &r), CT_CLASSIFY_OK);
    CT_ASSERT_EQ(r.cls, CT_CLS_UNKNOWN);
}

CT_SUITE(unit_classify_stress) {
    ct_move seq[TEST_ALG_MAX];
    ct_sfc64 rng;
    ct_sfc64_init(&rng, 0x5EED5);
    int unknown = 0;
    int special = 0;
    int commutator = 0;
    for (int round = 0; round < TEST_STRESS_ROUNDS; round++) {
        int len_r = 4 + (int)ct_sfc64_range(&rng, 9);
        for (int i = 0; i < len_r; i++) {
            seq[i].face = (int)ct_sfc64_range(&rng, TEST_MOVE_KINDS);
            seq[i].depth_from = 1;
            seq[i].depth_to = 1;
            seq[i].quarter = 1 + (int)ct_sfc64_range(&rng, CT_MOVE_QUARTERS - 1);
        }
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        CT_ASSERT_EQ(ct_classify(seq, len_r, &r), CT_CLASSIFY_OK);
        if (r.cls == CT_CLS_UNKNOWN) {
            unknown++;
        } else if (r.cls == CT_CLS_PARITY || r.cls == CT_CLS_FLIP || r.cls == CT_CLS_TWIST) {
            special++;
            uint8_t state[TEST_CELLS];
            test_simulate(seq, len_r, state);
            CT_ASSERT(r.cls == CT_CLS_PARITY ? test_is_parity(state)
                      : r.cls == CT_CLS_FLIP ? test_is_flip(state)
                                             : test_is_twist(state));
        } else {
            commutator++;
            if (!test_rebuild_equal(&r, seq, len_r)) {
                printf("random round %d misclassified as %s\n", round,
                       ct_classification_label(r.cls));
            }
            CT_ASSERT(test_rebuild_equal(&r, seq, len_r));
        }
    }
    printf("unit_classify_stress: unknown=%d special=%d commutator=%d\n", unknown, special,
           commutator);
    CT_ASSERT(unknown > TEST_STRESS_ROUNDS / 2);
}

CT_SUITE(unit_classify_fixture) {
    static char orbits[TEST_ROW_MAX][8];
    static char classes[TEST_ROW_MAX][24];
    static char algs[TEST_ROW_MAX][TEST_FIELD_MAX];
    int rows = test_load_fixture(orbits, classes, algs);
    CT_ASSERT_EQ(rows, TEST_FIXTURE_ROWS);
    for (int i = 0; i < rows; i++) {
        ct_move seq[TEST_ALG_MAX];
        int len = test_parse(algs[i], seq);
        CT_ASSERT(len > 0);
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        int rc = ct_classify_alg(algs[i], &r);
        if (rc != CT_CLASSIFY_OK || r.cls != ct_classification_from_label(classes[i])) {
            printf("fixture row %d: alg %s want %s got rc=%d cls=%s\n", i, algs[i], classes[i], rc,
                   ct_classification_label(r.cls));
        }
        CT_ASSERT_EQ(rc, CT_CLASSIFY_OK);
        CT_ASSERT(r.cls == ct_classification_from_label(classes[i]));
        CT_ASSERT_STR_EQ(r.label, classes[i]);
        CT_ASSERT(r.rationale[0] != '\0');
        CT_ASSERT(test_rebuild_equal(&r, seq, len));
        CT_ASSERT(r.moves > 0);
        CT_ASSERT(r.qtm > 0);
    }
}

CT_SUITE(unit_classify_synth_table) {
    CT_ASSERT(ct_synth_init() > 0);
    int size = ct_synth_table_size();
    CT_ASSERT(size > 0);
    int counts[CT_CLS_UNKNOWN + 1] = {0};
    int unknowns = 0;
    for (int i = 0; i < size; i++) {
        int key[CT_SYNTH_KEY_FIELDS];
        ct_move seq[CT_SYNTH_TPL_MAX];
        int len = ct_synth_table_entry(i, key, seq, CT_SYNTH_TPL_MAX);
        CT_ASSERT(len > 0);
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        CT_ASSERT_EQ(ct_classify(seq, len, &r), CT_CLASSIFY_OK);
        counts[r.cls]++;
        if (r.cls == CT_CLS_UNKNOWN) {
            unknowns++;
            printf("synth table entry %d unclassifiable\n", i);
            continue;
        }
        if (i % TEST_SYNTH_SAMPLE_STEP == 0) {
            CT_ASSERT(test_rebuild_equal(&r, seq, len));
        }
    }
    printf("unit_classify_synth_table: entries=%d unknowns=%d pure=%d a9=%d b9=%d orth=%d "
           "cyclic=%d column=%d perspecial=%d hsp=%d sp5=%d sp7=%d sp9=%d\n",
           size, unknowns, counts[CT_CLS_PURE], counts[CT_CLS_A9], counts[CT_CLS_B9],
           counts[CT_CLS_ORTHOGONAL], counts[CT_CLS_CYCLIC_SHIFT], counts[CT_CLS_COLUMN],
           counts[CT_CLS_PER_SPECIAL], counts[CT_CLS_HALF_SLICE_PLANE], counts[CT_CLS_SP5],
           counts[CT_CLS_SP7], counts[CT_CLS_SP9]);
    CT_ASSERT_EQ(unknowns, 0);
    CT_ASSERT(counts[CT_CLS_PURE] > 0);
}

CT_SUITE(unit_classify_synth_results) {
    ct_config cfg = ct_config_defaults();
    CT_ASSERT(ct_synth_init() > 0);
    static ct_target_pair corners[378];
    static ct_target_pair edges[440];
    CT_ASSERT_EQ(ct_targets_corner(0, corners, 378), 378);
    CT_ASSERT_EQ(ct_targets_edge(2, edges, 440), 440);
    int found = 0;
    int direct = 0;
    int setup = 0;
    int fallback = 0;
    int classified = 0;
    for (int i = 0; i < TEST_TARGET_SAMPLES; i++) {
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_CORNER, 0, corners[i].t1, corners[i].t2, &cfg, &res),
                     0);
        CT_ASSERT(res.found);
        found++;
        direct += !res.used_setup && !res.used_fallback;
        setup += res.used_setup;
        fallback += res.used_fallback;
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        CT_ASSERT_EQ(ct_classify(res.seq, res.len, &r), CT_CLASSIFY_OK);
        if (r.cls != CT_CLS_UNKNOWN) {
            classified++;
            CT_ASSERT(test_rebuild_equal(&r, res.seq, res.len));
        }
        if (!res.used_fallback) {
            if (r.cls == CT_CLS_UNKNOWN) {
                printf("synth result %d corner t%d/%d unclassifiable\n", i, corners[i].t1,
                       corners[i].t2);
            }
            CT_ASSERT(r.cls != CT_CLS_UNKNOWN);
        }
    }
    for (int i = 0; i < TEST_TARGET_SAMPLES; i++) {
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        CT_ASSERT_EQ(ct_synth_target(CT_ORBIT_EDGE, 2, edges[i].t1, edges[i].t2, &cfg, &res), 0);
        CT_ASSERT(res.found);
        found++;
        direct += !res.used_setup && !res.used_fallback;
        setup += res.used_setup;
        fallback += res.used_fallback;
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        CT_ASSERT_EQ(ct_classify(res.seq, res.len, &r), CT_CLASSIFY_OK);
        if (r.cls != CT_CLS_UNKNOWN) {
            classified++;
            CT_ASSERT(test_rebuild_equal(&r, res.seq, res.len));
        }
        if (!res.used_fallback) {
            if (r.cls == CT_CLS_UNKNOWN) {
                printf("synth result %d edge t%d/%d unclassifiable\n", i, edges[i].t1, edges[i].t2);
            }
            CT_ASSERT(r.cls != CT_CLS_UNKNOWN);
        }
    }
    printf("unit_classify_synth_results: found=%d direct=%d setup=%d fallback=%d "
           "classified=%d\n",
           found, direct, setup, fallback, classified);
    CT_ASSERT(direct > 0);
    CT_ASSERT(setup > 0);
}

CT_SUITE(unit_classify_special) {
    static const char *parity_alg = "R U R' U' R' F R2 U' R' U' R U R' F'";
    static const char *flip_alg = "M' U M' U M' U2 M U M U M U2";
    static const char *twist_alg = "R D' R' D R D' R' D U D' R D R' D' R D R' U'";

    ct_move seq[TEST_ALG_MAX];
    int len = test_parse(parity_alg, seq);
    CT_ASSERT(len > 0);
    uint8_t state[TEST_CELLS];
    test_simulate(seq, len, state);
    CT_ASSERT(test_is_parity(state));
    ct_classify_result r;
    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify(seq, len, &r), CT_CLASSIFY_OK);
    if (r.cls != CT_CLS_PARITY) {
        printf("parity alg classified %s\n", ct_classification_label(r.cls));
    }
    CT_ASSERT_EQ(r.cls, CT_CLS_PARITY);
    CT_ASSERT_STR_EQ(r.label, "parity");
    CT_ASSERT(r.rationale[0] != '\0');

    len = test_parse(flip_alg, seq);
    CT_ASSERT(len > 0);
    test_simulate(seq, len, state);
    CT_ASSERT(test_is_flip(state));
    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify(seq, len, &r), CT_CLASSIFY_OK);
    if (r.cls != CT_CLS_FLIP) {
        printf("flip alg classified %s\n", ct_classification_label(r.cls));
    }
    CT_ASSERT_EQ(r.cls, CT_CLS_FLIP);

    len = test_parse(twist_alg, seq);
    CT_ASSERT(len > 0);
    test_simulate(seq, len, state);
    CT_ASSERT(test_is_twist(state));
    memset(&r, 0, sizeof r);
    CT_ASSERT_EQ(ct_classify(seq, len, &r), CT_CLASSIFY_OK);
    if (r.cls != CT_CLS_TWIST) {
        printf("twist alg classified %s\n", ct_classification_label(r.cls));
    }
    CT_ASSERT_EQ(r.cls, CT_CLS_TWIST);
}
