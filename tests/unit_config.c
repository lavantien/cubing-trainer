#include "framework.h"

#include "../src/config/config.h"

#include <stdio.h>
#include <string.h>

static const double spec_weights[CT_FSRS_WEIGHT_COUNT] = {
    0.40255, 1.18385, 3.173,   15.69105, 7.1949,  0.5345,  1.4604, 0.0046,
    1.54575, 0.1192,  1.01925, 1.9395,   0.11,    0.29605, 2.2698, 0.2315,
    2.9898,  0.51655, 0.6621
};

static const unsigned spec_default_mask = (1u << 0) | (1u << 1) | (1u << 3);

static int face_index(char c) {
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        if (ct_move_face_char(1u << i) == c) {
            return i;
        }
    }
    return -1;
}

CT_SUITE(config_constants)
{
    CT_ASSERT_EQ(CT_FACE_COUNT, 12);
    CT_ASSERT_EQ(CT_FSRS_WEIGHT_COUNT, 19);
    CT_ASSERT_DBL_EQ(CT_FSRS_REQUEST_RETENTION, 0.9, 1e-12);
    CT_ASSERT_DBL_EQ(CT_FSRS_MAX_IVL_DAYS, 36500.0, 1e-12);
    CT_ASSERT_DBL_EQ(CT_FSRS_MIN_STABILITY, 0.1, 1e-12);
    CT_ASSERT_DBL_EQ(CT_FSRS_MAX_DIFFICULTY, 10.0, 1e-12);
    CT_ASSERT_DBL_EQ(CT_FSRS_MIN_DIFFICULTY, 1.0, 1e-12);
    CT_ASSERT_DBL_EQ(CT_FSRS_FACTOR, 0.2345679012345679, 1e-12);
    CT_ASSERT_DBL_EQ(CT_FSRS_DECAY, -0.5, 1e-12);
    CT_ASSERT_EQ(CT_FACE_U, 1);
    CT_ASSERT_EQ(CT_FACE_D, 2);
    CT_ASSERT_EQ(CT_FACE_L, 4);
    CT_ASSERT_EQ(CT_FACE_R, 8);
    CT_ASSERT_EQ(CT_FACE_F, 16);
    CT_ASSERT_EQ(CT_FACE_B, 32);
    CT_ASSERT_EQ(CT_FACE_M, 64);
    CT_ASSERT_EQ(CT_FACE_E, 128);
    CT_ASSERT_EQ(CT_FACE_S, 256);
    CT_ASSERT_EQ(CT_FACE_X, 512);
    CT_ASSERT_EQ(CT_FACE_Y, 1024);
    CT_ASSERT_EQ(CT_FACE_Z, 2048);
    CT_ASSERT_EQ(CT_GEN_DEFAULT_PREFERRED_MOVES, 11);
    CT_ASSERT_DBL_EQ(CT_GEN_PENALTY_PREFERRED, 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(CT_GEN_PENALTY_OTHER, 2.0, 1e-12);
    CT_ASSERT_DBL_EQ(CT_GEN_WEIGHT_LEN, 1.0, 1e-12);
    CT_ASSERT_DBL_EQ(CT_GEN_WEIGHT_SUBSET, 2.0, 1e-12);
    CT_ASSERT_DBL_EQ(CT_GEN_WEIGHT_REGRIP, 1.5, 1e-12);
    CT_ASSERT_DBL_EQ(CT_GEN_WEIGHT_FINGER, 0.5, 1e-12);
    CT_ASSERT_EQ(CT_GEN_SETUP_BFS_DEPTH, 4);
    CT_ASSERT_EQ(CT_GEN_FALLBACK_DEPTH, 5);
    CT_ASSERT_EQ(CT_GEN_STRAGGLER_DEPTH, 6);
    CT_ASSERT_DBL_EQ(CT_ANIM_MOVE_SECONDS, 0.25, 1e-12);
    CT_ASSERT_EQ(CT_WINDOW_WIDTH, 1280);
    CT_ASSERT_EQ(CT_WINDOW_HEIGHT, 720);
    CT_ASSERT_EQ(CT_WINDOW_TARGET_FPS, 60);
    CT_ASSERT_STR_EQ(CT_DB_DIR_NAME, "cubing-trainer");
    CT_ASSERT_STR_EQ(CT_DB_FILENAME, "trainer.db");
    CT_ASSERT_EQ(CT_FUZZ_ITERATIONS, 10000);
}

CT_SUITE(config_enums)
{
    CT_ASSERT_EQ(CT_AGAIN, 1);
    CT_ASSERT_EQ(CT_HARD, 2);
    CT_ASSERT_EQ(CT_GOOD, 3);
    CT_ASSERT_EQ(CT_EASY, 4);
    CT_ASSERT_EQ(CT_ORBIT_CORNER, 0);
    CT_ASSERT_EQ(CT_ORBIT_EDGE, 1);
    CT_ASSERT_EQ(CT_ORBIT_CENTER, 2);
    CT_ASSERT_EQ(CT_ORBIT_WING, 3);
    CT_ASSERT_EQ(CT_BUFFER_FIXED, 0);
    CT_ASSERT_EQ(CT_BUFFER_FLOATING, 1);
    CT_ASSERT_EQ(CT_CLS_PURE, 0);
    CT_ASSERT_EQ(CT_CLS_A9, 1);
    CT_ASSERT_EQ(CT_CLS_B9, 2);
    CT_ASSERT_EQ(CT_CLS_ORTHOGONAL, 3);
    CT_ASSERT_EQ(CT_CLS_CYCLIC_SHIFT, 4);
    CT_ASSERT_EQ(CT_CLS_COLUMN, 5);
    CT_ASSERT_EQ(CT_CLS_PER_SPECIAL, 6);
    CT_ASSERT_EQ(CT_CLS_HALF_SLICE_PLANE, 7);
    CT_ASSERT_EQ(CT_CLS_SP5, 8);
    CT_ASSERT_EQ(CT_CLS_SP7, 9);
    CT_ASSERT_EQ(CT_CLS_SP9, 10);
    CT_ASSERT_EQ(CT_CLS_PARITY, 11);
    CT_ASSERT_EQ(CT_CLS_FLIP, 12);
    CT_ASSERT_EQ(CT_CLS_TWIST, 13);
    CT_ASSERT_EQ(CT_EVENT_3X3, 0);
    CT_ASSERT_EQ(CT_EVENT_2X2, 1);
    CT_ASSERT_EQ(CT_EVENT_3BLD, 2);
    CT_ASSERT_EQ(CT_EVENT_4BLD, 3);
    CT_ASSERT_EQ(CT_EVENT_5BLD, 4);
    CT_ASSERT_EQ(CT_EVENT_MULTI_BLD, 5);
    CT_ASSERT_EQ(CT_EVENT_FMC, 6);
    CT_ASSERT_EQ(CT_EVENT_OH, 7);
    CT_ASSERT_EQ(CT_EVENT_4X4, 8);
    CT_ASSERT_EQ(CT_EVENT_5X5, 9);
    CT_ASSERT_EQ(CT_EVENT_6X6, 10);
    CT_ASSERT_EQ(CT_EVENT_7X7, 11);
    CT_ASSERT_EQ(CT_EVENT_CLOCK, 12);
    CT_ASSERT_EQ(CT_EVENT_MEGAMINX, 13);
    CT_ASSERT_EQ(CT_EVENT_PYRAMINX, 14);
    CT_ASSERT_EQ(CT_EVENT_SKEWB, 15);
    CT_ASSERT_EQ(CT_EVENT_SQUARE1, 16);
    CT_ASSERT_EQ(CT_EVENT_FTO, 17);
    CT_ASSERT_EQ(CT_METHOD_CFOP, 0);
    CT_ASSERT_EQ(CT_METHOD_3STYLE, 1);
    CT_ASSERT_EQ(CT_METHOD_OLD_POCHMANN, 2);
    CT_ASSERT_EQ(CT_METHOD_M2, 3);
    CT_ASSERT_EQ(CT_METHOD_U2, 4);
    CT_ASSERT_EQ(CT_METHOD_R2, 5);
    CT_ASSERT_EQ(CT_METHOD_LBL, 6);
    CT_ASSERT_EQ(CT_METHOD_L4E, 7);
    CT_ASSERT_EQ(CT_METHOD_4LLL, 8);
    CT_ASSERT_EQ(CT_METHOD_SARAH_INTERMEDIATE, 9);
    CT_ASSERT_EQ(CT_METHOD_YAU, 10);
    CT_ASSERT_EQ(CT_METHOD_REDUCTION, 11);
    CT_ASSERT_EQ(CT_METHOD_CLL, 12);
    CT_ASSERT_EQ(CT_METHOD_L4T_REDUX, 13);
    CT_ASSERT_EQ(CT_METHOD_CONCEPT, 14);
    CT_ASSERT_EQ(CT_STAGE_PLL, 0);
    CT_ASSERT_EQ(CT_STAGE_OLL, 1);
    CT_ASSERT_EQ(CT_STAGE_F2L, 2);
    CT_ASSERT_EQ(CT_STAGE_CROSS, 3);
    CT_ASSERT_EQ(CT_STAGE_CLL, 4);
    CT_ASSERT_EQ(CT_STAGE_CUBESHAPE, 5);
    CT_ASSERT_EQ(CT_STAGE_CO, 6);
    CT_ASSERT_EQ(CT_STAGE_EO, 7);
    CT_ASSERT_EQ(CT_STAGE_CP, 8);
    CT_ASSERT_EQ(CT_STAGE_EP, 9);
    CT_ASSERT_EQ(CT_STAGE_L4E, 10);
    CT_ASSERT_EQ(CT_STAGE_4LLL, 11);
    CT_ASSERT_EQ(CT_STAGE_SARAH, 12);
    CT_ASSERT_EQ(CT_STAGE_L4T, 13);
    CT_ASSERT_EQ(CT_STAGE_PARITY, 14);
    CT_ASSERT_EQ(CT_STAGE_CONCEPT, 15);
    CT_ASSERT_EQ(CT_DIR_FR, 0);
    CT_ASSERT_EQ(CT_DIR_FL, 1);
    CT_ASSERT_EQ(CT_DIR_BR, 2);
    CT_ASSERT_EQ(CT_DIR_BL, 3);
    CT_ASSERT_EQ(CT_SCREEN_MENU, 0);
    CT_ASSERT_EQ(CT_SCREEN_TRAINER, 1);
    CT_ASSERT_EQ(CT_SCREEN_CARDS, 2);
    CT_ASSERT_EQ(CT_SCREEN_REVIEW, 3);
    CT_ASSERT_EQ(CT_SCREEN_SETTINGS, 4);
    CT_ASSERT_EQ(CT_SCREEN_STATS, 5);
}

CT_SUITE(config_face_helpers)
{
    CT_ASSERT_EQ(ct_move_face_from_char('U'), CT_FACE_U);
    CT_ASSERT_EQ(ct_move_face_from_char('D'), CT_FACE_D);
    CT_ASSERT_EQ(ct_move_face_from_char('L'), CT_FACE_L);
    CT_ASSERT_EQ(ct_move_face_from_char('R'), CT_FACE_R);
    CT_ASSERT_EQ(ct_move_face_from_char('F'), CT_FACE_F);
    CT_ASSERT_EQ(ct_move_face_from_char('B'), CT_FACE_B);
    CT_ASSERT_EQ(ct_move_face_from_char('M'), CT_FACE_M);
    CT_ASSERT_EQ(ct_move_face_from_char('E'), CT_FACE_E);
    CT_ASSERT_EQ(ct_move_face_from_char('S'), CT_FACE_S);
    CT_ASSERT_EQ(ct_move_face_from_char('x'), CT_FACE_X);
    CT_ASSERT_EQ(ct_move_face_from_char('y'), CT_FACE_Y);
    CT_ASSERT_EQ(ct_move_face_from_char('z'), CT_FACE_Z);
    CT_ASSERT_EQ(ct_move_face_from_char('Q'), 0);
    CT_ASSERT_EQ(ct_move_face_from_char('u'), 0);
    CT_ASSERT_EQ(ct_move_face_from_char(' '), 0);
    CT_ASSERT_EQ(ct_move_face_from_char('\0'), 0);
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_U), 'U');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_D), 'D');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_L), 'L');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_R), 'R');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_F), 'F');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_B), 'B');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_M), 'M');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_E), 'E');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_S), 'S');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_X), 'x');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_Y), 'y');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_Z), 'z');
    CT_ASSERT_EQ(ct_move_face_char(0), '\0');
    CT_ASSERT_EQ(ct_move_face_char(CT_FACE_U | CT_FACE_D), '\0');
    CT_ASSERT_EQ(ct_move_face_char(1u << 12), '\0');
}

CT_SUITE(config_defaults)
{
    ct_config c = ct_config_defaults();
    for (int i = 0; i < CT_FSRS_WEIGHT_COUNT; i++) {
        CT_ASSERT_DBL_EQ(c.fsrs.w[i], spec_weights[i], 1e-12);
    }
    CT_ASSERT_DBL_EQ(c.fsrs.request_retention, CT_FSRS_REQUEST_RETENTION, 1e-12);
    CT_ASSERT_DBL_EQ(c.fsrs.max_ivl_days, CT_FSRS_MAX_IVL_DAYS, 1e-12);
    CT_ASSERT_DBL_EQ(c.fsrs.min_stability, CT_FSRS_MIN_STABILITY, 1e-12);
    CT_ASSERT_DBL_EQ(c.fsrs.max_difficulty, CT_FSRS_MAX_DIFFICULTY, 1e-12);
    CT_ASSERT_DBL_EQ(c.fsrs.min_difficulty, CT_FSRS_MIN_DIFFICULTY, 1e-12);
    CT_ASSERT_DBL_EQ(c.fsrs.factor, CT_FSRS_FACTOR, 1e-15);
    CT_ASSERT_DBL_EQ(c.fsrs.decay, CT_FSRS_DECAY, 1e-12);
    CT_ASSERT_EQ(c.gen.preferred_moves, spec_default_mask);
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        double want = ((1u << i) & spec_default_mask) ? CT_GEN_PENALTY_PREFERRED : CT_GEN_PENALTY_OTHER;
        CT_ASSERT_DBL_EQ(c.gen.penalty[i], want, 1e-12);
    }
    CT_ASSERT_DBL_EQ(c.gen.weight_len, CT_GEN_WEIGHT_LEN, 1e-12);
    CT_ASSERT_DBL_EQ(c.gen.weight_subset, CT_GEN_WEIGHT_SUBSET, 1e-12);
    CT_ASSERT_DBL_EQ(c.gen.weight_regrip, CT_GEN_WEIGHT_REGRIP, 1e-12);
    CT_ASSERT_DBL_EQ(c.gen.weight_finger, CT_GEN_WEIGHT_FINGER, 1e-12);
    CT_ASSERT_EQ(c.gen.setup_bfs_depth, CT_GEN_SETUP_BFS_DEPTH);
    CT_ASSERT_EQ(c.gen.fallback_depth, CT_GEN_FALLBACK_DEPTH);
    CT_ASSERT_EQ(c.gen.straggler_depth, CT_GEN_STRAGGLER_DEPTH);
    CT_ASSERT_DBL_EQ(c.anim.move_seconds, CT_ANIM_MOVE_SECONDS, 1e-12);
    CT_ASSERT_EQ(c.anim.ease, CT_EASE_CUBIC_IN_OUT);
    CT_ASSERT_EQ(c.window.width, CT_WINDOW_WIDTH);
    CT_ASSERT_EQ(c.window.height, CT_WINDOW_HEIGHT);
    CT_ASSERT_EQ(c.window.target_fps, CT_WINDOW_TARGET_FPS);
    CT_ASSERT_STR_EQ(c.db.dir_name, CT_DB_DIR_NAME);
    CT_ASSERT_STR_EQ(c.db.filename, CT_DB_FILENAME);
    CT_ASSERT_EQ(c.fuzz.iterations, CT_FUZZ_ITERATIONS);
}

CT_SUITE(config_set_roundtrip)
{
    ct_config c = ct_config_defaults();

    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "0.85"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.request_retention, 0.85, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", "1000.5"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.max_ivl_days, 1000.5, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "LF"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, CT_FACE_L | CT_FACE_F);

    CT_ASSERT_EQ(ct_config_set(&c, "gen.weight_len", "0.5"), 0);
    CT_ASSERT_DBL_EQ(c.gen.weight_len, 0.5, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.weight_subset", "0.75"), 0);
    CT_ASSERT_DBL_EQ(c.gen.weight_subset, 0.75, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.weight_regrip", "2.25"), 0);
    CT_ASSERT_DBL_EQ(c.gen.weight_regrip, 2.25, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.weight_finger", "1.125"), 0);
    CT_ASSERT_DBL_EQ(c.gen.weight_finger, 1.125, 1e-12);

    int m = face_index('M');
    CT_ASSERT(m >= 0);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.penalty_M", "1.25"), 0);
    CT_ASSERT_DBL_EQ(c.gen.penalty[m], 1.25, 1e-12);

    int x = face_index('x');
    CT_ASSERT(x >= 0);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.penalty_x", "0.75"), 0);
    CT_ASSERT_DBL_EQ(c.gen.penalty[x], 0.75, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "gen.setup_bfs_depth", "7"), 0);
    CT_ASSERT_EQ(c.gen.setup_bfs_depth, 7);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.fallback_depth", "3"), 0);
    CT_ASSERT_EQ(c.gen.fallback_depth, 3);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.straggler_depth", "8"), 0);
    CT_ASSERT_EQ(c.gen.straggler_depth, 8);

    CT_ASSERT_EQ(ct_config_set(&c, "anim.move_seconds", "0.4"), 0);
    CT_ASSERT_DBL_EQ(c.anim.move_seconds, 0.4, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "window.width", "640"), 0);
    CT_ASSERT_EQ(c.window.width, 640);
    CT_ASSERT_EQ(ct_config_set(&c, "window.height", "480"), 0);
    CT_ASSERT_EQ(c.window.height, 480);
    CT_ASSERT_EQ(ct_config_set(&c, "window.target_fps", "144"), 0);
    CT_ASSERT_EQ(c.window.target_fps, 144);

    CT_ASSERT_EQ(ct_config_set(&c, "db.filename", "other.db"), 0);
    CT_ASSERT_STR_EQ(c.db.filename, "other.db");

    CT_ASSERT_EQ(ct_config_set(&c, "fuzz.iterations", "500"), 0);
    CT_ASSERT_EQ(c.fuzz.iterations, 500);

    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", "1e3"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.max_ivl_days, 1000.0, 1e-9);
}

CT_SUITE(config_set_srs_weights)
{
    ct_config c = ct_config_defaults();
    for (int i = 0; i < CT_FSRS_WEIGHT_COUNT; i++) {
        char key[16];
        char val[32];
        snprintf(key, sizeof key, "srs.w%d", i);
        snprintf(val, sizeof val, "%d.5", i);
        CT_ASSERT_EQ(ct_config_set(&c, key, val), 0);
    }
    for (int i = 0; i < CT_FSRS_WEIGHT_COUNT; i++) {
        CT_ASSERT_DBL_EQ(c.fsrs.w[i], i + 0.5, 1e-12);
    }
}

CT_SUITE(config_set_errors)
{
    ct_config c = ct_config_defaults();

    CT_ASSERT_EQ(ct_config_set(&c, "nope.key", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.unknown", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.w19", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.wabc", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.w-1", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.w", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.penalty_Q", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.penalty_r", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.penalty_rr", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.penalty_", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "db.dir_name", "x"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "window.depth", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "", "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, NULL, "1"), -1);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", NULL), -1);
    CT_ASSERT_EQ(ct_config_set(NULL, "srs.request_retention", "0.9"), -1);

    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "abc"), -2);
    CT_ASSERT_DBL_EQ(c.fsrs.request_retention, CT_FSRS_REQUEST_RETENTION, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", ""), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "0.9x"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "nan"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", "inf"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", "90 "), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", " 90"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.max_ivl_days, 90.0, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.w3", "junk"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.weight_len", "1,5"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.penalty_R", "bad"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "window.width", "12a"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "window.width", ""), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "window.target_fps", "60.5"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.setup_bfs_depth", "4.5"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.setup_bfs_depth", "+"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "fuzz.iterations", "x"), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "anim.move_seconds", "-"), -2);

    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "RUQ"), -2);
    CT_ASSERT_EQ(c.gen.preferred_moves, spec_default_mask);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "R "), -2);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "9"), -2);

    CT_ASSERT_EQ(ct_config_set(&c, "db.filename", ""), -2);
    CT_ASSERT_STR_EQ(c.db.filename, CT_DB_FILENAME);
    char long_name[CT_DB_NAME_MAX + 2];
    memset(long_name, 'a', sizeof long_name - 1);
    long_name[sizeof long_name - 1] = '\0';
    CT_ASSERT_EQ(ct_config_set(&c, "db.filename", long_name), -2);
    CT_ASSERT_STR_EQ(c.db.filename, CT_DB_FILENAME);
}

CT_SUITE(config_preferred_moves)
{
    ct_config c = ct_config_defaults();
    unsigned rud = CT_FACE_R | CT_FACE_U | CT_FACE_D;

    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "RUD"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, rud);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "rud"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, rud);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "dUr"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, rud);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "RRUUDD"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, rud);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "rUd"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, rud);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "drruU"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, rud);

    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "xyz"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, CT_FACE_X | CT_FACE_Y | CT_FACE_Z);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "XYZ"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, CT_FACE_X | CT_FACE_Y | CT_FACE_Z);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "MES"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, CT_FACE_M | CT_FACE_E | CT_FACE_S);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "UDLRFBMESxyz"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, 0xFFFu);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "Rr"), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, CT_FACE_R);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", ""), 0);
    CT_ASSERT_EQ(c.gen.preferred_moves, 0u);
}

CT_SUITE(config_clamping)
{
    ct_config c = ct_config_defaults();

    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "0.5"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.request_retention, 0.7, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "1.5"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.request_retention, 0.99, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "0.99"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.request_retention, 0.99, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.request_retention", "0.7"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.request_retention, 0.7, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", "0"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.max_ivl_days, CT_FSRS_MIN_STABILITY, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", "-3"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.max_ivl_days, CT_FSRS_MIN_STABILITY, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "srs.max_ivl_days", "365"), 0);
    CT_ASSERT_DBL_EQ(c.fsrs.max_ivl_days, 365.0, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "gen.setup_bfs_depth", "0"), 0);
    CT_ASSERT_EQ(c.gen.setup_bfs_depth, 1);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.fallback_depth", "-2"), 0);
    CT_ASSERT_EQ(c.gen.fallback_depth, 1);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.straggler_depth", "1"), 0);
    CT_ASSERT_EQ(c.gen.straggler_depth, 1);

    CT_ASSERT_EQ(ct_config_set(&c, "anim.move_seconds", "0.001"), 0);
    CT_ASSERT_DBL_EQ(c.anim.move_seconds, 0.01, 1e-12);
    CT_ASSERT_EQ(ct_config_set(&c, "anim.move_seconds", "-1"), 0);
    CT_ASSERT_DBL_EQ(c.anim.move_seconds, 0.01, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "window.target_fps", "0"), 0);
    CT_ASSERT_EQ(c.window.target_fps, 1);
    CT_ASSERT_EQ(ct_config_set(&c, "window.target_fps", "-9"), 0);
    CT_ASSERT_EQ(c.window.target_fps, 1);
}
