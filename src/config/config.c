#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const double fsrs_default_weights[CT_FSRS_WEIGHT_COUNT] = {
    0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046,  1.54575, 0.1192,
    1.01925, 1.9395,  0.11,  0.29605,  2.2698, 0.2315, 2.9898, 0.51655, 0.6621};

const double ct_finger_defaults[CT_FINGER_FACE_COUNT][CT_FINGER_DIR_COUNT] = {
    {CT_FINGER_COST_CHEAP, CT_FINGER_COST_CHEAP},
    {CT_FINGER_COST_CHEAP, CT_FINGER_COST_CHEAP},
    {CT_FINGER_COST_MEDIUM, CT_FINGER_COST_MEDIUM},
    {CT_FINGER_COST_MEDIUM, CT_FINGER_COST_MEDIUM},
    {CT_FINGER_COST_EXPENSIVE, CT_FINGER_COST_EXPENSIVE},
    {CT_FINGER_COST_EXPENSIVE, CT_FINGER_COST_EXPENSIVE},
};

static int parse_double(const char *text, double *out) {
    if (text == NULL || text[0] == '\0') {
        return CT_CFG_BAD_VALUE;
    }
    errno = 0;
    char *end = NULL;
    double value = strtod(text, &end);
    if (end == text || end[0] != '\0' || errno == ERANGE || !isfinite(value)) {
        return CT_CFG_BAD_VALUE;
    }
    *out = value;
    return CT_CFG_OK;
}

static int parse_long(const char *text, long *out) {
    if (text == NULL || text[0] == '\0') {
        return CT_CFG_BAD_VALUE;
    }
    errno = 0;
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || end[0] != '\0' || errno == ERANGE || value < INT_MIN || value > INT_MAX) {
        return CT_CFG_BAD_VALUE;
    }
    *out = value;
    return CT_CFG_OK;
}

static int assign_double(double *field, const char *value) {
    double parsed = 0.0;
    int rc = parse_double(value, &parsed);
    if (rc != CT_CFG_OK) {
        return rc;
    }
    *field = parsed;
    return CT_CFG_OK;
}

static int assign_double_lo(double *field, const char *value, double lo) {
    int rc = assign_double(field, value);
    if (rc != CT_CFG_OK) {
        return rc;
    }
    if (*field < lo) {
        *field = lo;
    }
    return CT_CFG_OK;
}

static int assign_double_range(double *field, const char *value, double lo, double hi) {
    int rc = assign_double(field, value);
    if (rc != CT_CFG_OK) {
        return rc;
    }
    if (*field < lo) {
        *field = lo;
    }
    if (*field > hi) {
        *field = hi;
    }
    return CT_CFG_OK;
}

static int assign_long_lo(int *field, const char *value, long lo) {
    long parsed = 0;
    int rc = parse_long(value, &parsed);
    if (rc != CT_CFG_OK) {
        return rc;
    }
    *field = parsed < lo ? (int)lo : (int)parsed;
    return CT_CFG_OK;
}

static int assign_long_min(int *field, const char *value, long min) {
    long parsed = 0;
    int rc = parse_long(value, &parsed);
    if (rc != CT_CFG_OK) {
        return rc;
    }
    if (parsed < min) {
        return CT_CFG_BAD_VALUE;
    }
    *field = (int)parsed;
    return CT_CFG_OK;
}

static unsigned face_from_char_ci(char c) {
    unsigned face = ct_move_face_from_char(c);
    if (face != 0) {
        return face;
    }
    face = ct_move_face_from_char((char)tolower((unsigned char)c));
    if (face != 0) {
        return face;
    }
    return ct_move_face_from_char((char)toupper((unsigned char)c));
}

static int face_bit_index(unsigned face) {
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        if ((1u << i) == face) {
            return i;
        }
    }
    return -1;
}

ct_config ct_config_defaults(void) {
    ct_config c;
    memset(&c, 0, sizeof c);
    memcpy(c.fsrs.w, fsrs_default_weights, sizeof c.fsrs.w);
    c.fsrs.request_retention = CT_FSRS_REQUEST_RETENTION;
    c.fsrs.max_ivl_days = CT_FSRS_MAX_IVL_DAYS;
    c.fsrs.min_stability = CT_FSRS_MIN_STABILITY;
    c.fsrs.max_difficulty = CT_FSRS_MAX_DIFFICULTY;
    c.fsrs.min_difficulty = CT_FSRS_MIN_DIFFICULTY;
    c.fsrs.factor = CT_FSRS_FACTOR;
    c.fsrs.decay = CT_FSRS_DECAY;
    c.gen.preferred_moves = CT_GEN_DEFAULT_PREFERRED_MOVES;
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        if ((1u << i) & c.gen.preferred_moves) {
            c.gen.penalty[i] = CT_GEN_PENALTY_PREFERRED;
        } else {
            c.gen.penalty[i] = CT_GEN_PENALTY_OTHER;
        }
    }
    c.gen.weight_len = CT_GEN_WEIGHT_LEN;
    c.gen.weight_subset = CT_GEN_WEIGHT_SUBSET;
    c.gen.weight_regrip = CT_GEN_WEIGHT_REGRIP;
    c.gen.weight_finger = CT_GEN_WEIGHT_FINGER;
    memcpy(c.gen.finger_cost, ct_finger_defaults, sizeof c.gen.finger_cost);
    c.gen.setup_bfs_depth = CT_GEN_SETUP_BFS_DEPTH;
    c.gen.fallback_depth = CT_GEN_FALLBACK_DEPTH;
    c.gen.straggler_depth = CT_GEN_STRAGGLER_DEPTH;
    c.anim.move_seconds = CT_ANIM_MOVE_SECONDS;
    c.anim.ease = CT_EASE_CUBIC_IN_OUT;
    c.window.width = CT_WINDOW_WIDTH;
    c.window.height = CT_WINDOW_HEIGHT;
    c.window.target_fps = CT_WINDOW_TARGET_FPS;
    snprintf(c.db.dir_name, sizeof c.db.dir_name, "%s", CT_DB_DIR_NAME);
    snprintf(c.db.filename, sizeof c.db.filename, "%s", CT_DB_FILENAME);
    c.fuzz.iterations = CT_FUZZ_ITERATIONS;
    return c;
}

int ct_config_set(ct_config *cfg, const char *key, const char *value) {
    if (cfg == NULL || key == NULL || value == NULL) {
        return CT_CFG_UNKNOWN_KEY;
    }
    if (strcmp(key, "srs.request_retention") == 0) {
        return assign_double_range(&cfg->fsrs.request_retention, value, CT_FSRS_RETENTION_MIN,
                                   CT_FSRS_RETENTION_MAX);
    }
    if (strcmp(key, "srs.max_ivl_days") == 0) {
        return assign_double_lo(&cfg->fsrs.max_ivl_days, value, CT_FSRS_MIN_STABILITY);
    }
    if (strncmp(key, "srs.w", sizeof "srs.w" - 1) == 0) {
        const char *num = key + sizeof "srs.w" - 1;
        if (num[0] == '\0') {
            return CT_CFG_UNKNOWN_KEY;
        }
        for (const char *p = num; *p != '\0'; p++) {
            if (*p < '0' || *p > '9') {
                return CT_CFG_UNKNOWN_KEY;
            }
        }
        if (num[0] == '0' && num[1] != '\0') {
            return CT_CFG_UNKNOWN_KEY;
        }
        errno = 0;
        char *end = NULL;
        long idx = strtol(num, &end, 10);
        if (end[0] != '\0' || errno == ERANGE || idx >= CT_FSRS_WEIGHT_COUNT) {
            return CT_CFG_UNKNOWN_KEY;
        }
        return assign_double(&cfg->fsrs.w[idx], value);
    }
    if (strcmp(key, "gen.preferred_moves") == 0) {
        unsigned mask = 0;
        for (const char *p = value; *p != '\0'; p++) {
            unsigned face = face_from_char_ci(*p);
            if (face == 0 || (face & CT_MOVE_ROTATION_MASK) != 0) {
                return CT_CFG_BAD_VALUE;
            }
            mask |= face;
        }
        if (mask == 0) {
            return CT_CFG_BAD_VALUE;
        }
        cfg->gen.preferred_moves = mask;
        return CT_CFG_OK;
    }
    if (strcmp(key, "gen.weight_len") == 0) {
        return assign_double(&cfg->gen.weight_len, value);
    }
    if (strcmp(key, "gen.weight_subset") == 0) {
        return assign_double(&cfg->gen.weight_subset, value);
    }
    if (strcmp(key, "gen.weight_regrip") == 0) {
        return assign_double(&cfg->gen.weight_regrip, value);
    }
    if (strcmp(key, "gen.weight_finger") == 0) {
        return assign_double(&cfg->gen.weight_finger, value);
    }
    if (strncmp(key, "gen.penalty_", sizeof "gen.penalty_" - 1) == 0) {
        const char *face = key + sizeof "gen.penalty_" - 1;
        if (face[0] == '\0' || face[1] != '\0') {
            return CT_CFG_UNKNOWN_KEY;
        }
        unsigned token = ct_move_face_from_char(face[0]);
        int idx = token == 0 ? -1 : face_bit_index(token);
        if (idx < 0) {
            return CT_CFG_UNKNOWN_KEY;
        }
        return assign_double(&cfg->gen.penalty[idx], value);
    }
    if (strncmp(key, "gen.finger_cost_", sizeof "gen.finger_cost_" - 1) == 0) {
        const char *face = key + sizeof "gen.finger_cost_" - 1;
        unsigned token = ct_move_face_from_char(face[0]);
        int idx = token == 0 ? -1 : face_bit_index(token);
        if (idx < 0 || idx >= CT_FINGER_FACE_COUNT) {
            return CT_CFG_UNKNOWN_KEY;
        }
        int dir;
        if (strcmp(face + 1, "_cw") == 0) {
            dir = CT_FINGER_DIR_CW;
        } else if (strcmp(face + 1, "_ccw") == 0) {
            dir = CT_FINGER_DIR_CCW;
        } else {
            return CT_CFG_UNKNOWN_KEY;
        }
        return assign_double(&cfg->gen.finger_cost[idx][dir], value);
    }
    if (strcmp(key, "gen.setup_bfs_depth") == 0) {
        return assign_long_lo(&cfg->gen.setup_bfs_depth, value, CT_MIN_DEPTH);
    }
    if (strcmp(key, "gen.fallback_depth") == 0) {
        return assign_long_lo(&cfg->gen.fallback_depth, value, CT_MIN_DEPTH);
    }
    if (strcmp(key, "gen.straggler_depth") == 0) {
        return assign_long_lo(&cfg->gen.straggler_depth, value, CT_MIN_DEPTH);
    }
    if (strcmp(key, "anim.move_seconds") == 0) {
        return assign_double_lo(&cfg->anim.move_seconds, value, CT_ANIM_MIN_MOVE_SECONDS);
    }
    if (strcmp(key, "window.width") == 0) {
        return assign_long_min(&cfg->window.width, value, CT_WINDOW_MIN_DIMENSION);
    }
    if (strcmp(key, "window.height") == 0) {
        return assign_long_min(&cfg->window.height, value, CT_WINDOW_MIN_DIMENSION);
    }
    if (strcmp(key, "window.target_fps") == 0) {
        return assign_long_lo(&cfg->window.target_fps, value, CT_MIN_FPS);
    }
    if (strcmp(key, "db.filename") == 0) {
        size_t len = strlen(value);
        if (len == 0 || len >= sizeof cfg->db.filename || strchr(value, '/') != NULL ||
            strchr(value, '\\') != NULL) {
            return CT_CFG_BAD_VALUE;
        }
        memcpy(cfg->db.filename, value, len + 1);
        return CT_CFG_OK;
    }
    if (strcmp(key, "fuzz.iterations") == 0) {
        return assign_long_min(&cfg->fuzz.iterations, value, CT_FUZZ_MIN_ITERATIONS);
    }
    return CT_CFG_UNKNOWN_KEY;
}
