#include "config/config.h"
#include "framework.h"

#include <stdio.h>
#include <string.h>

int ct_fuzz_iterations(void);

enum {
    CT_CK_RETENTION,
    CT_CK_MAXIVL,
    CT_CK_WLEN,
    CT_CK_WSUBSET,
    CT_CK_WREGRIP,
    CT_CK_WFINGER,
    CT_CK_BFS,
    CT_CK_FALLBACK,
    CT_CK_STRAGGLER,
    CT_CK_SECONDS,
    CT_CK_WIDTH,
    CT_CK_HEIGHT,
    CT_CK_FPS,
    CT_CK_FUZZIT,
    CT_CK_FILENAME,
    CT_CK_WBASE = 100,
    CT_CK_PENBASE = 200
};

enum {
    CT_CKV_DBL_CLAMP = 0,
    CT_CKV_DBL_LO,
    CT_CKV_DBL_FREE,
    CT_CKV_LONG_FREE,
    CT_CKV_LONG_LO,
    CT_CKV_FILENAME
};

enum {
    CT_CK_KEY_MAX = 48,
    CT_CK_FILENAME_MAX = 200,
    CT_CK_SOUP_MAX = 24,
    CT_CK_W_INDEX_SPAN = 26,
    CT_CK_INVALID_FACES = 6
};

#define CT_FZ(cond, seed)                                                                          \
    do {                                                                                           \
        if (cond) {                                                                                \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_eq(__FILE__, __LINE__, #cond, "seed", (unsigned long long)(seed), 0);          \
        }                                                                                          \
    } while (0)

typedef struct {
    const char *name;
    int slot;
    int kind;
} ct_ck_entry;

static const ct_ck_entry ct_ck_table[] = {
    {"srs.request_retention", CT_CK_RETENTION, CT_CKV_DBL_CLAMP},
    {"srs.max_ivl_days", CT_CK_MAXIVL, CT_CKV_DBL_LO},
    {"gen.weight_len", CT_CK_WLEN, CT_CKV_DBL_FREE},
    {"gen.weight_subset", CT_CK_WSUBSET, CT_CKV_DBL_FREE},
    {"gen.weight_regrip", CT_CK_WREGRIP, CT_CKV_DBL_FREE},
    {"gen.weight_finger", CT_CK_WFINGER, CT_CKV_DBL_FREE},
    {"gen.setup_bfs_depth", CT_CK_BFS, CT_CKV_LONG_LO},
    {"gen.fallback_depth", CT_CK_FALLBACK, CT_CKV_LONG_LO},
    {"gen.straggler_depth", CT_CK_STRAGGLER, CT_CKV_LONG_LO},
    {"anim.move_seconds", CT_CK_SECONDS, CT_CKV_DBL_LO},
    {"window.width", CT_CK_WIDTH, CT_CKV_LONG_FREE},
    {"window.height", CT_CK_HEIGHT, CT_CKV_LONG_FREE},
    {"window.target_fps", CT_CK_FPS, CT_CKV_LONG_LO},
    {"fuzz.iterations", CT_CK_FUZZIT, CT_CKV_LONG_FREE},
    {"db.filename", CT_CK_FILENAME, CT_CKV_FILENAME},
};

typedef struct {
    const char *text;
    int ok;
    double value;
} ct_ck_dbl;

static const ct_ck_dbl ct_ck_dbls[] = {
    {"0", 1, 0.0},    {"42", 1, 42.0},  {"-17", 1, -17.0}, {"3.5", 1, 3.5},   {"-3.5", 1, -3.5},
    {" 8.5", 1, 8.5}, {"+2.5", 1, 2.5}, {"1e2", 1, 100.0}, {"1e999", 0, 0.0}, {"-1e999", 0, 0.0},
    {"", 0, 0.0},     {"abc", 0, 0.0},  {"1x", 0, 0.0},    {"1.2.3", 0, 0.0}, {"90 ", 0, 0.0},
    {"nan", 0, 0.0},  {"inf", 0, 0.0},  {"-inf", 0, 0.0},  {".", 0, 0.0},     {"1e", 0, 0.0},
};

typedef struct {
    const char *text;
    int ok;
    long value;
} ct_ck_long;

static const ct_ck_long ct_ck_longs[] = {
    {"0", 1, 0},
    {"-5", 1, -5},
    {"2147483647", 1, 2147483647},
    {"-2147483648", 1, -2147483648},
    {" 90", 1, 90},
    {"+7", 1, 7},
    {"3000000000", 0, 0},
    {"-3000000000", 0, 0},
    {"99999999999999999999", 0, 0},
    {"60.5", 0, 0},
    {"", 0, 0},
    {"x", 0, 0},
    {"12a", 0, 0},
    {"+", 0, 0},
};

typedef struct {
    char c;
    unsigned bit;
} ct_ck_face_bit;

static const ct_ck_face_bit ct_ck_face_bits[] = {
    {'U', CT_MOVE_BIT_U}, {'u', CT_MOVE_BIT_U}, {'D', CT_MOVE_BIT_D}, {'d', CT_MOVE_BIT_D},
    {'L', CT_MOVE_BIT_L}, {'l', CT_MOVE_BIT_L}, {'R', CT_MOVE_BIT_R}, {'r', CT_MOVE_BIT_R},
    {'F', CT_MOVE_BIT_F}, {'f', CT_MOVE_BIT_F}, {'B', CT_MOVE_BIT_B}, {'b', CT_MOVE_BIT_B},
    {'M', CT_MOVE_BIT_M}, {'m', CT_MOVE_BIT_M}, {'E', CT_MOVE_BIT_E}, {'e', CT_MOVE_BIT_E},
    {'S', CT_MOVE_BIT_S}, {'s', CT_MOVE_BIT_S}, {'x', CT_MOVE_BIT_X}, {'X', CT_MOVE_BIT_X},
    {'y', CT_MOVE_BIT_Y}, {'Y', CT_MOVE_BIT_Y}, {'z', CT_MOVE_BIT_Z}, {'Z', CT_MOVE_BIT_Z},
};

static int ct_ck_face_index(char c) {
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        if (ct_move_face_char(1u << i) == c) {
            return i;
        }
    }
    return -1;
}

static double *ct_ck_dbl_field(ct_config *c, int slot) {
    if (slot >= CT_CK_WBASE && slot < CT_CK_WBASE + CT_FSRS_WEIGHT_COUNT) {
        return &c->fsrs.w[slot - CT_CK_WBASE];
    }
    if (slot >= CT_CK_PENBASE && slot < CT_CK_PENBASE + CT_FACE_COUNT) {
        return &c->gen.penalty[slot - CT_CK_PENBASE];
    }
    switch (slot) {
    case CT_CK_RETENTION:
        return &c->fsrs.request_retention;
    case CT_CK_MAXIVL:
        return &c->fsrs.max_ivl_days;
    case CT_CK_WLEN:
        return &c->gen.weight_len;
    case CT_CK_WSUBSET:
        return &c->gen.weight_subset;
    case CT_CK_WREGRIP:
        return &c->gen.weight_regrip;
    case CT_CK_WFINGER:
        return &c->gen.weight_finger;
    case CT_CK_SECONDS:
        return &c->anim.move_seconds;
    default:
        return NULL;
    }
}

static int *ct_ck_int_field(ct_config *c, int slot) {
    switch (slot) {
    case CT_CK_BFS:
        return &c->gen.setup_bfs_depth;
    case CT_CK_FALLBACK:
        return &c->gen.fallback_depth;
    case CT_CK_STRAGGLER:
        return &c->gen.straggler_depth;
    case CT_CK_WIDTH:
        return &c->window.width;
    case CT_CK_HEIGHT:
        return &c->window.height;
    case CT_CK_FPS:
        return &c->window.target_fps;
    case CT_CK_FUZZIT:
        return &c->fuzz.iterations;
    default:
        return NULL;
    }
}

static void ct_ck_expect_double(ct_config *expected, int slot, int kind, double value) {
    double *field = ct_ck_dbl_field(expected, slot);
    if (kind == CT_CKV_DBL_CLAMP) {
        *field = value < CT_FSRS_RETENTION_MIN   ? CT_FSRS_RETENTION_MIN
                 : value > CT_FSRS_RETENTION_MAX ? CT_FSRS_RETENTION_MAX
                                                 : value;
    } else if (kind == CT_CKV_DBL_LO) {
        double lo = slot == CT_CK_SECONDS ? CT_ANIM_MIN_MOVE_SECONDS : CT_FSRS_MIN_STABILITY;
        *field = value < lo ? lo : value;
    } else {
        *field = value;
    }
}

static void ct_ck_expect_long(ct_config *expected, int slot, int kind, long value) {
    int *field = ct_ck_int_field(expected, slot);
    long lo = slot == CT_CK_FPS ? CT_MIN_FPS : CT_MIN_DEPTH;
    if (kind == CT_CKV_LONG_LO && value < lo) {
        *field = (int)lo;
    } else {
        *field = (int)value;
    }
}

CT_SUITE(fuzz_config_keys) {
    static const char valid_faces[] = "UDLRFBMESxyz";
    static const char invalid_faces[CT_CK_INVALID_FACES] = {'q', 'r', 'T', 'A', 'X', 'W'};
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0xC0F16000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        ct_config defaults = ct_config_defaults();
        ct_config cfg = defaults;
        char key[CT_CK_KEY_MAX];
        int slot = 0;
        int kind = CT_CKV_DBL_FREE;
        bool known = false;
        unsigned family = (unsigned)ct_sfc64_range(&s, 3);
        if (family == 0) {
            const ct_ck_entry *e =
                &ct_ck_table[ct_sfc64_range(&s, sizeof ct_ck_table / sizeof ct_ck_table[0])];
            snprintf(key, sizeof key, "%s", e->name);
            slot = e->slot;
            kind = e->kind;
            known = true;
        } else if (family == 1) {
            unsigned idx = (unsigned)ct_sfc64_range(&s, CT_CK_W_INDEX_SPAN);
            snprintf(key, sizeof key, "srs.w%u", idx);
            if (idx < CT_FSRS_WEIGHT_COUNT) {
                slot = CT_CK_WBASE + (int)idx;
                known = true;
            }
        } else {
            unsigned span = (unsigned)(sizeof valid_faces - 1) + CT_CK_INVALID_FACES;
            unsigned pick = (unsigned)ct_sfc64_range(&s, span);
            if (pick < sizeof valid_faces - 1) {
                snprintf(key, sizeof key, "gen.penalty_%c", valid_faces[pick]);
                slot = CT_CK_PENBASE + ct_ck_face_index(valid_faces[pick]);
                known = true;
            } else {
                snprintf(key, sizeof key, "gen.penalty_%c",
                         invalid_faces[pick - (sizeof valid_faces - 1)]);
            }
        }
        if (!known) {
            CT_FZ(ct_config_set(&cfg, key, "1.25") == CT_CFG_UNKNOWN_KEY, seed);
            CT_FZ(memcmp(&cfg, &defaults, sizeof cfg) == 0, seed);
            continue;
        }
        if (kind == CT_CKV_FILENAME) {
            char value[CT_CK_FILENAME_MAX];
            unsigned mode = (unsigned)ct_sfc64_range(&s, 3);
            unsigned len = mode == 0   ? 0
                           : mode == 1 ? 1 + (unsigned)ct_sfc64_range(&s, CT_DB_NAME_MAX - 1)
                                       : CT_DB_NAME_MAX + (unsigned)ct_sfc64_range(&s, 40);
            for (unsigned k = 0; k < len && k + 1 < sizeof value; k++) {
                value[k] = (char)('a' + ct_sfc64_range(&s, 26));
            }
            value[len < sizeof value ? len : sizeof value - 1] = '\0';
            ct_config expected = defaults;
            bool want_ok = value[0] != '\0' && strlen(value) < sizeof expected.db.filename;
            if (want_ok) {
                snprintf(expected.db.filename, sizeof expected.db.filename, "%s", value);
            }
            int rc = ct_config_set(&cfg, key, value);
            if (want_ok) {
                CT_FZ(rc == CT_CFG_OK, seed);
                CT_FZ(memcmp(&cfg, &expected, sizeof cfg) == 0, seed);
            } else {
                CT_FZ(rc == CT_CFG_BAD_VALUE, seed);
                CT_FZ(memcmp(&cfg, &defaults, sizeof cfg) == 0, seed);
            }
            continue;
        }
        if (kind == CT_CKV_LONG_FREE || kind == CT_CKV_LONG_LO) {
            const ct_ck_long *pat =
                &ct_ck_longs[ct_sfc64_range(&s, sizeof ct_ck_longs / sizeof ct_ck_longs[0])];
            ct_config expected = defaults;
            if (pat->ok) {
                ct_ck_expect_long(&expected, slot, kind, pat->value);
            }
            int rc = ct_config_set(&cfg, key, pat->text);
            if (pat->ok) {
                CT_FZ(rc == CT_CFG_OK, seed);
                CT_FZ(memcmp(&cfg, &expected, sizeof cfg) == 0, seed);
            } else {
                CT_FZ(rc == CT_CFG_BAD_VALUE, seed);
                CT_FZ(memcmp(&cfg, &defaults, sizeof cfg) == 0, seed);
            }
        } else {
            const ct_ck_dbl *pat =
                &ct_ck_dbls[ct_sfc64_range(&s, sizeof ct_ck_dbls / sizeof ct_ck_dbls[0])];
            ct_config expected = defaults;
            if (pat->ok) {
                ct_ck_expect_double(&expected, slot, kind, pat->value);
            }
            int rc = ct_config_set(&cfg, key, pat->text);
            if (pat->ok) {
                CT_FZ(rc == CT_CFG_OK, seed);
                CT_FZ(memcmp(&cfg, &expected, sizeof cfg) == 0, seed);
            } else {
                CT_FZ(rc == CT_CFG_BAD_VALUE, seed);
                CT_FZ(memcmp(&cfg, &defaults, sizeof cfg) == 0, seed);
            }
        }
        char mutated[CT_CK_KEY_MAX + 8];
        unsigned mutation = (unsigned)ct_sfc64_range(&s, 4);
        if (family == 1 && mutation == 2) {
            mutation = 0;
        }
        if (mutation == 0) {
            snprintf(mutated, sizeof mutated, "%sq", key);
        } else if (mutation == 1) {
            snprintf(mutated, sizeof mutated, "zz.%s", key);
        } else if (mutation == 2) {
            snprintf(mutated, sizeof mutated, "%.*s", (int)(strlen(key) - 1), key);
        } else {
            snprintf(mutated, sizeof mutated, "%s-bad", key);
        }
        ct_config before = cfg;
        CT_FZ(ct_config_set(&cfg, mutated, "1.25") == CT_CFG_UNKNOWN_KEY, seed);
        CT_FZ(memcmp(&cfg, &before, sizeof cfg) == 0, seed);
    }
}

CT_SUITE(fuzz_config_moves) {
    static const char invalid[] = "qQwW2!#";
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0xA0F00000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        ct_config defaults = ct_config_defaults();
        ct_config cfg = defaults;
        char soup[CT_CK_SOUP_MAX + 1];
        unsigned len = (unsigned)ct_sfc64_range(&s, CT_CK_SOUP_MAX + 1);
        for (unsigned k = 0; k < len; k++) {
            if (ct_sfc64_range(&s, 3) == 0) {
                soup[k] = invalid[ct_sfc64_range(&s, sizeof invalid - 1)];
            } else {
                soup[k] = ct_ck_face_bits[ct_sfc64_range(&s, sizeof ct_ck_face_bits /
                                                                 sizeof ct_ck_face_bits[0])]
                              .c;
            }
        }
        soup[len] = '\0';
        unsigned mask = 0;
        bool valid = true;
        for (unsigned k = 0; k < len; k++) {
            int hit = -1;
            for (unsigned f = 0; f < sizeof ct_ck_face_bits / sizeof ct_ck_face_bits[0]; f++) {
                if (ct_ck_face_bits[f].c == soup[k]) {
                    hit = (int)f;
                    break;
                }
            }
            if (hit < 0) {
                valid = false;
                break;
            }
            mask |= ct_ck_face_bits[hit].bit;
        }
        unsigned sentinel = (unsigned)ct_sfc64_range(&s, 1u << CT_FACE_COUNT);
        cfg.gen.preferred_moves = sentinel;
        ct_config reject_expected = cfg;
        ct_config accept_expected = defaults;
        accept_expected.gen.preferred_moves = mask;
        int rc = ct_config_set(&cfg, "gen.preferred_moves", soup);
        if (valid) {
            CT_FZ(rc == CT_CFG_OK, seed);
            CT_FZ(memcmp(&cfg, &accept_expected, sizeof cfg) == 0, seed);
        } else {
            CT_FZ(rc == CT_CFG_BAD_VALUE, seed);
            CT_FZ(memcmp(&cfg, &reject_expected, sizeof cfg) == 0, seed);
        }
    }
}
