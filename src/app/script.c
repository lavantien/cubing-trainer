#include "app/script.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/script_internal.h"
#include "engine/notation.h"
#include "gen/synth.h"
#include "storage/repo_settings.h"
#include "trainer/cfop.h"
#include "trainer/selector.h"
#include "trainer/session.h"

typedef struct {
    const char *word;
    ct_script_cmd kind;
    int argc;
} ct_script_word;

static const ct_script_word CT_SCRIPT_WORDS[] = {
    {"restart", CT_SCRIPT_RESTART, 0},
    {"cards_open", CT_SCRIPT_CARDS_OPEN, 1},
    {"cards_new", CT_SCRIPT_CARDS_NEW, 1},
    {"cards_select", CT_SCRIPT_CARDS_SELECT, 1},
    {"cards_edit", CT_SCRIPT_CARDS_EDIT, 0},
    {"cards_field", CT_SCRIPT_CARDS_FIELD, CT_SCRIPT_ARGC_2},
    {"cards_save", CT_SCRIPT_CARDS_SAVE, 0},
    {"cards_delete", CT_SCRIPT_CARDS_DELETE, 0},
    {"cfop_populate", CT_SCRIPT_CFOP_POPULATE, 1},
    {"due", CT_SCRIPT_DUE, CT_SCRIPT_ARGC_3},
    {"review_begin", CT_SCRIPT_REVIEW_BEGIN, 0},
    {"review_reveal", CT_SCRIPT_REVIEW_REVEAL, 0},
    {"review_grade", CT_SCRIPT_REVIEW_GRADE, 1},
    {"review_end", CT_SCRIPT_REVIEW_END, 0},
    {"wait", CT_SCRIPT_WAIT, 1},
    {"settings_open", CT_SCRIPT_SETTINGS_OPEN, 0},
    {"settings_retention", CT_SCRIPT_SETTINGS_RETENTION, 1},
    {"settings_move", CT_SCRIPT_SETTINGS_MOVE, CT_SCRIPT_ARGC_2},
    {"settings_save", CT_SCRIPT_SETTINGS_SAVE, 0},
    {"trainer_list", CT_SCRIPT_TRAINER_LIST, CT_SCRIPT_ARGC_3},
    {"trainer_case", CT_SCRIPT_TRAINER_CASE, 1},
    {"trainer_generate", CT_SCRIPT_TRAINER_GENERATE, 0},
    {"trainer_save", CT_SCRIPT_TRAINER_SAVE, 1},
};

static const ct_script_word CT_SCRIPT_EXPECTS[] = {
    {"count", CT_SCRIPT_EXPECT_COUNT, CT_SCRIPT_ARGC_3},
    {"card", CT_SCRIPT_EXPECT_CARD, CT_SCRIPT_ARGC_5},
    {"srs", CT_SCRIPT_EXPECT_SRS, CT_SCRIPT_ARGC_5},
    {"reviews", CT_SCRIPT_EXPECT_REVIEWS, CT_SCRIPT_ARGC_4},
    {"setting", CT_SCRIPT_EXPECT_SETTING, CT_SCRIPT_ARGC_3},
    {"cfg", CT_SCRIPT_EXPECT_CFG, CT_SCRIPT_ARGC_3},
    {"algcount", CT_SCRIPT_EXPECT_ALGCOUNT, 2},
    {"genalg", CT_SCRIPT_EXPECT_GENALG, CT_SCRIPT_ARGC_4},
};

static const struct {
    const char *word;
    ct_grade grade;
} CT_SCRIPT_GRADES[] = {
    {"again", CT_AGAIN},
    {"hard", CT_HARD},
    {"good", CT_GOOD},
    {"easy", CT_EASY},
};

static const struct {
    const char *word;
    ct_event event;
} CT_SCRIPT_EVENTS[] = {
    {"3x3", CT_EVENT_3X3},           {"2x2", CT_EVENT_2X2},
    {"3bld", CT_EVENT_3BLD},         {"4bld", CT_EVENT_4BLD},
    {"5bld", CT_EVENT_5BLD},         {"mbld", CT_EVENT_MULTI_BLD},
    {"fmc", CT_EVENT_FMC},           {"oh", CT_EVENT_OH},
    {"4x4", CT_EVENT_4X4},           {"5x5", CT_EVENT_5X5},
    {"6x6", CT_EVENT_6X6},           {"7x7", CT_EVENT_7X7},
    {"clock", CT_EVENT_CLOCK},       {"megaminx", CT_EVENT_MEGAMINX},
    {"pyraminx", CT_EVENT_PYRAMINX}, {"skewb", CT_EVENT_SKEWB},
    {"sq1", CT_EVENT_SQUARE1},       {"fto", CT_EVENT_FTO},
};

static const struct {
    const char *word;
    ct_method method;
} CT_SCRIPT_METHODS[] = {
    {"cfop", CT_METHOD_CFOP},       {"3style", CT_METHOD_3STYLE},
    {"op", CT_METHOD_OLD_POCHMANN}, {"m2", CT_METHOD_M2},
    {"u2", CT_METHOD_U2},           {"r2", CT_METHOD_R2},
    {"lbl", CT_METHOD_LBL},         {"l4e", CT_METHOD_L4E},
    {"4lll", CT_METHOD_4LLL},       {"sarah", CT_METHOD_SARAH_INTERMEDIATE},
    {"yau", CT_METHOD_YAU},         {"reduction", CT_METHOD_REDUCTION},
    {"cll", CT_METHOD_CLL},         {"l4t", CT_METHOD_L4T_REDUX},
    {"concept", CT_METHOD_CONCEPT},
};

enum {
    CT_SCRIPT_KEYWORD = 0,
    CT_SCRIPT_SUB = 1,
    CT_SCRIPT_EQ = 0,
    CT_SCRIPT_GE = 1,
    CT_SCRIPT_LE = 2,
    CT_SCRIPT_YEAR_DIGITS = 4,
    CT_SCRIPT_MONTH_AT = 5,
    CT_SCRIPT_DAY_AT = 8,
    CT_SCRIPT_LIST_CAP = 8
};

static int ct_script_sep(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

static const ct_script_word *ct_script_lookup(const ct_script_word *words, unsigned count,
                                              const char *w) {
    for (unsigned i = 0; i < count; i++) {
        if (strcmp(words[i].word, w) == 0) {
            return &words[i];
        }
    }
    return NULL;
}

int ct_script_parse(const char *src, ct_script *out, char *err, unsigned cap) {
    if (src == NULL || out == NULL || err == NULL || cap == 0) {
        return CT_SCRIPT_EXIT_PARSE;
    }
    err[0] = '\0';
    size_t len = strlen(src);
    if (len >= sizeof out->text) {
        snprintf(err, cap, "script text exceeds %u bytes", (unsigned)sizeof out->text);
        return CT_SCRIPT_EXIT_PARSE;
    }
    memcpy(out->text, src, len + 1);
    out->count = 0;
    char *p = out->text;
    char *end = out->text + len;
    int line = 1;
    while (p < end) {
        if (*p == '\n') {
            line++;
            p++;
            continue;
        }
        if (ct_script_sep(*p)) {
            p++;
            continue;
        }
        if (*p == '#') {
            while (p < end && *p != '\n') {
                p++;
            }
            continue;
        }
        if (out->count == CT_SCRIPT_CMDS_MAX) {
            snprintf(err, cap, "line %d: too many commands", line);
            return CT_SCRIPT_EXIT_PARSE;
        }
        ct_script_row *cmd = &out->cmds[out->count];
        memset(cmd, 0, sizeof *cmd);
        cmd->line = line;
        int at = line;
        char *line_end = p;
        while (line_end < end && *line_end != '\n') {
            line_end++;
        }
        bool has_nl = line_end < end;
        int argc = 0;
        while (p < line_end) {
            while (p < line_end && ct_script_sep(*p)) {
                p++;
            }
            if (p >= line_end || *p == '#') {
                break;
            }
            if (argc == CT_SCRIPT_ARGS_MAX) {
                snprintf(err, cap, "line %d: too many arguments", at);
                return CT_SCRIPT_EXIT_PARSE;
            }
            char *tok;
            if (*p == '"') {
                tok = p + 1;
                char *close = tok;
                while (close < line_end && *close != '"') {
                    close++;
                }
                if (close >= line_end) {
                    snprintf(err, cap, "line %d: unterminated quote", at);
                    return CT_SCRIPT_EXIT_PARSE;
                }
                *close = '\0';
                p = close + 1;
            } else {
                tok = p;
                while (p < line_end && !ct_script_sep(*p) && *p != '#') {
                    p++;
                }
                char delim = p < line_end ? *p : '\0';
                *p = '\0';
                p++;
                if (delim == '#') {
                    break;
                }
            }
            cmd->off[argc] = (unsigned)(tok - out->text);
            cmd->len[argc] = (unsigned)strlen(tok);
            argc++;
        }
        p = has_nl ? line_end + 1 : line_end;
        if (has_nl) {
            line++;
        }
        if (argc == 0) {
            continue;
        }
        const char *word = out->text + cmd->off[CT_SCRIPT_KEYWORD];
        const ct_script_word *spec;
        int drop;
        if (strcmp(word, "expect") == 0) {
            if (argc < CT_SCRIPT_SUB + 1) {
                snprintf(err, cap, "line %d: expect needs a kind", at);
                return CT_SCRIPT_EXIT_PARSE;
            }
            spec = ct_script_lookup(CT_SCRIPT_EXPECTS,
                                    sizeof CT_SCRIPT_EXPECTS / sizeof CT_SCRIPT_EXPECTS[0],
                                    out->text + cmd->off[CT_SCRIPT_SUB]);
            drop = CT_SCRIPT_SUB + 1;
        } else {
            spec = ct_script_lookup(CT_SCRIPT_WORDS,
                                    sizeof CT_SCRIPT_WORDS / sizeof CT_SCRIPT_WORDS[0], word);
            drop = CT_SCRIPT_KEYWORD + 1;
        }
        if (spec == NULL) {
            snprintf(err, cap, "line %d: unknown command %s", at, word);
            return CT_SCRIPT_EXIT_PARSE;
        }
        if (argc - drop != spec->argc) {
            snprintf(err, cap, "line %d: bad arity for %s, expects %d arguments", at, spec->word,
                     spec->argc);
            return CT_SCRIPT_EXIT_PARSE;
        }
        static const int CT_SCRIPT_OP_AT[CT_SCRIPT_ARGS_MAX] = {
            [CT_SCRIPT_EXPECT_COUNT - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_1,
            [CT_SCRIPT_EXPECT_CARD - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_3,
            [CT_SCRIPT_EXPECT_SRS - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_3,
            [CT_SCRIPT_EXPECT_REVIEWS - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_2,
            [CT_SCRIPT_EXPECT_SETTING - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_1,
            [CT_SCRIPT_EXPECT_CFG - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_1,
            [CT_SCRIPT_EXPECT_ALGCOUNT - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_0,
            [CT_SCRIPT_EXPECT_GENALG - CT_SCRIPT_EXPECT_COUNT] = CT_SCRIPT_ARGC_2,
        };
        for (int i = 0; i < spec->argc; i++) {
            cmd->off[i] = cmd->off[drop + i];
            cmd->len[i] = cmd->len[drop + i];
        }
        if (spec->kind >= CT_SCRIPT_EXPECT_COUNT) {
            int op_at = CT_SCRIPT_OP_AT[spec->kind - CT_SCRIPT_EXPECT_COUNT];
            if (ct_script_op(out->text + cmd->off[op_at]) < 0) {
                snprintf(err, cap, "line %d: bad operator for expect %s", at, spec->word);
                return CT_SCRIPT_EXIT_PARSE;
            }
        }
        cmd->kind = spec->kind;
        cmd->argc = spec->argc;
        out->count++;
    }
    if (out->count == 0) {
        snprintf(err, cap, "empty script");
        return CT_SCRIPT_EXIT_PARSE;
    }
    return 0;
}

const char *ct_script_arg(const ct_script *s, const ct_script_row *c, int i) {
    if (s == NULL || c == NULL || i < 0 || i >= c->argc || i >= CT_SCRIPT_ARGS_MAX) {
        return NULL;
    }
    return s->text + c->off[i];
}

int ct_script_fail(char *err, unsigned cap, const ct_script_row *c, const char *what, int code) {
    snprintf(err, cap, "line %d: %s", c->line, what);
    return code;
}

int ct_script_long(const char *s, long *out) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, CT_DECIMAL_BASE);
    if (end == s || end[0] != '\0' || errno == ERANGE) {
        return -1;
    }
    *out = v;
    return 0;
}

int ct_script_double(const char *s, double *out) {
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || end[0] != '\0' || errno == ERANGE || !isfinite(v)) {
        return -1;
    }
    *out = v;
    return 0;
}

int ct_script_op(const char *s) {
    if (strcmp(s, "==") == 0) {
        return CT_SCRIPT_EQ;
    }
    if (strcmp(s, ">=") == 0) {
        return CT_SCRIPT_GE;
    }
    if (strcmp(s, "<=") == 0) {
        return CT_SCRIPT_LE;
    }
    if (strcmp(s, "!=") == 0) {
        return CT_SCRIPT_NE;
    }
    return -1;
}

int ct_script_long_ok(int op, long a, long b) {
    switch (op) {
    case CT_SCRIPT_EQ:
        return a == b;
    case CT_SCRIPT_GE:
        return a >= b;
    case CT_SCRIPT_LE:
        return a <= b;
    default:
        return a != b;
    }
}

int ct_script_double_ok(int op, double a, double b) {
    double d = a - b;
    if (d < 0.0) {
        d = -d;
    }
    switch (op) {
    case CT_SCRIPT_EQ:
        return d <= CT_SCRIPT_DBL_EPS;
    case CT_SCRIPT_GE:
        return a >= b;
    case CT_SCRIPT_LE:
        return a <= b;
    default:
        return d > CT_SCRIPT_DBL_EPS;
    }
}

int ct_script_iso_day(const char *iso, long *day) {
    long y = 0;
    for (int i = 0; i < CT_SCRIPT_YEAR_DIGITS; i++) {
        if (iso[i] < '0' || iso[i] > '9') {
            return -1;
        }
        y = y * CT_DECIMAL_BASE + (iso[i] - '0');
    }
    if (iso[CT_SCRIPT_YEAR_DIGITS] != '-') {
        return -1;
    }
    int m = 0;
    for (int i = CT_SCRIPT_MONTH_AT; i < CT_SCRIPT_MONTH_AT + 2; i++) {
        if (iso[i] < '0' || iso[i] > '9') {
            return -1;
        }
        m = m * CT_DECIMAL_BASE + (iso[i] - '0');
    }
    if (iso[CT_SCRIPT_MONTH_AT + 2] != '-') {
        return -1;
    }
    int d = 0;
    for (int i = CT_SCRIPT_DAY_AT; i < CT_SCRIPT_DAY_AT + 2; i++) {
        if (iso[i] < '0' || iso[i] > '9') {
            return -1;
        }
        d = d * CT_DECIMAL_BASE + (iso[i] - '0');
    }
    *day = ct_session_date_to_day(y, m, d);
    return 0;
}

int ct_script_now(char *now, unsigned cap, long *day) {
    ct_now_utc_iso(now, cap);
    if (now[0] == '\0') {
        return -1;
    }
    return ct_script_iso_day(now, day);
}

static char *ct_script_card_field_mut(ct_card *c, const char *name, unsigned *cap) {
    if (strcmp(name, "alg3") == 0) {
        *cap = sizeof c->alg_3x3;
        return c->alg_3x3;
    }
    if (strcmp(name, "algbig") == 0) {
        *cap = sizeof c->alg_big;
        return c->alg_big;
    }
    if (strcmp(name, "algoh") == 0) {
        *cap = sizeof c->alg_oh;
        return c->alg_oh;
    }
    if (strcmp(name, "front") == 0) {
        *cap = sizeof c->front_json;
        return c->front_json;
    }
    if (strcmp(name, "notes") == 0) {
        *cap = sizeof c->notes;
        return c->notes;
    }
    return NULL;
}

static int ct_script_step_cards(ct_app *app, ct_script_rt *rt, const ct_script *s,
                                const ct_script_row *c, char *err, unsigned cap) {
    ct_cards_form *f = &rt->cards;
    switch (c->kind) {
    case CT_SCRIPT_CARDS_OPEN:
        ct_cards_form_open(f, app->db, ct_script_arg(s, c, 0));
        return f->status == CT_REPO_ERR_DB
                   ? ct_script_fail(err, cap, c, "cards query failed", CT_SCRIPT_EXIT_RUN)
                   : 0;
    case CT_SCRIPT_CARDS_NEW: {
        int rc = ct_cards_form_create(f, app->db, ct_script_arg(s, c, 0));
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "cards create failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_CARDS_SELECT: {
        long row;
        if (ct_script_long(ct_script_arg(s, c, 0), &row) != 0) {
            return ct_script_fail(err, cap, c, "row is not an integer", CT_SCRIPT_EXIT_RUN);
        }
        int rc = ct_cards_form_select(f, (int)row);
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "cards select failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_CARDS_EDIT: {
        int rc = ct_cards_form_begin_edit(f);
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "cards edit failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_CARDS_FIELD: {
        unsigned field_cap = 0;
        char *buf = ct_script_card_field_mut(&f->card, ct_script_arg(s, c, 0), &field_cap);
        if (buf == NULL) {
            return ct_script_fail(err, cap, c, "unknown card field", CT_SCRIPT_EXIT_RUN);
        }
        const char *value = ct_script_arg(s, c, 1);
        if (strlen(value) >= field_cap) {
            return ct_script_fail(err, cap, c, "card field value too long", CT_SCRIPT_EXIT_RUN);
        }
        snprintf(buf, field_cap, "%s", value);
        f->dirty = true;
        return 0;
    }
    case CT_SCRIPT_CARDS_SAVE: {
        int rc = ct_cards_form_save(f, app->db);
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "cards save failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_CARDS_DELETE: {
        int rc = ct_cards_form_delete(f, app->db);
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "cards delete failed", CT_SCRIPT_EXIT_RUN);
    }
    default:
        return 0;
    }
}

static int ct_script_step_review(ct_app *app, ct_script_rt *rt, const ct_script *s,
                                 const ct_script_row *c, char *err, unsigned cap) {
    if (c->kind == CT_SCRIPT_REVIEW_BEGIN) {
        char now[CT_SESSION_STAMP_MAX];
        long day = 0;
        if (ct_script_now(now, (unsigned)sizeof now, &day) != 0) {
            return ct_script_fail(err, cap, c, "cannot read the clock", CT_SCRIPT_EXIT_RUN);
        }
        int rc = ct_review_form_begin(&rt->review, app->db, CT_UI_DEFAULT_DECK, now, day, 0);
        if (rc < 0) {
            return ct_script_fail(err, cap, c, "review begin failed", CT_SCRIPT_EXIT_RUN);
        }
        rt->review_open = true;
        return 0;
    }
    if (!rt->review_open) {
        return ct_script_fail(err, cap, c, "review session not open", CT_SCRIPT_EXIT_RUN);
    }
    switch (c->kind) {
    case CT_SCRIPT_REVIEW_REVEAL:
        ct_review_form_reveal(&rt->review);
        return 0;
    case CT_SCRIPT_REVIEW_GRADE: {
        const char *word = ct_script_arg(s, c, 0);
        bool found = false;
        ct_grade g = CT_AGAIN;
        for (unsigned i = 0; i < sizeof CT_SCRIPT_GRADES / sizeof CT_SCRIPT_GRADES[0]; i++) {
            if (strcmp(CT_SCRIPT_GRADES[i].word, word) == 0) {
                g = CT_SCRIPT_GRADES[i].grade;
                found = true;
                break;
            }
        }
        if (!found) {
            return ct_script_fail(err, cap, c, "unknown grade", CT_SCRIPT_EXIT_RUN);
        }
        char now[CT_SESSION_STAMP_MAX];
        ct_now_utc_iso(now, (unsigned)sizeof now);
        if (now[0] == '\0') {
            return ct_script_fail(err, cap, c, "cannot read the clock", CT_SCRIPT_EXIT_RUN);
        }
        int rc = ct_review_form_grade(&rt->review, g, now);
        return rc == CT_SESSION_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "review grade failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_REVIEW_END: {
        char now[CT_SESSION_STAMP_MAX];
        ct_now_utc_iso(now, (unsigned)sizeof now);
        if (now[0] == '\0') {
            return ct_script_fail(err, cap, c, "cannot read the clock", CT_SCRIPT_EXIT_RUN);
        }
        ct_review_form_end(&rt->review, now);
        return rt->review.status == CT_SESSION_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "review end failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_WAIT: {
        long ticks;
        if (ct_script_long(ct_script_arg(s, c, 0), &ticks) != 0 || ticks < 1 ||
            ticks > CT_SCRIPT_REVIEW_WAIT_MAX) {
            return ct_script_fail(err, cap, c, "wait out of range", CT_SCRIPT_EXIT_RUN);
        }
        ct_review_form *f = &rt->review;
        for (long i = 0; i < ticks; i++) {
            ct_anim_step_result r =
                ct_anim_update(&f->anim, app->fixed_dt, app->cfg.anim.move_seconds);
            if (r.just_completed) {
                ct_apply(&f->cube, r.move);
                f->scrub_index = f->move_count - f->anim.count;
                if (!ct_anim_active(&f->anim)) {
                    ct_anim_pause(&f->anim);
                    f->playing = false;
                }
            }
        }
        return 0;
    }
    default:
        return 0;
    }
}

static int ct_script_step_settings(ct_app *app, ct_script_rt *rt, const ct_script *s,
                                   const ct_script_row *c, char *err, unsigned cap) {
    ct_settings_form *f = &rt->settings;
    switch (c->kind) {
    case CT_SCRIPT_SETTINGS_OPEN:
        ct_settings_form_open(f, app->db);
        return f->status == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "settings open failed", CT_SCRIPT_EXIT_RUN);
    case CT_SCRIPT_SETTINGS_RETENTION: {
        double v;
        if (ct_script_double(ct_script_arg(s, c, 0), &v) != 0) {
            return ct_script_fail(err, cap, c, "retention is not a number", CT_SCRIPT_EXIT_RUN);
        }
        int rc = ct_settings_form_stage_retention(f, v);
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "retention stage failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_SETTINGS_MOVE: {
        const char *face = ct_script_arg(s, c, 0);
        const char *want = ct_script_arg(s, c, 1);
        if (face[0] == '\0' || face[1] != '\0') {
            return ct_script_fail(err, cap, c, "face must be one letter", CT_SCRIPT_EXIT_RUN);
        }
        unsigned bit = ct_move_face_from_char(face[0]);
        bool on;
        if (strcmp(want, "on") == 0) {
            on = true;
        } else if (strcmp(want, "off") == 0) {
            on = false;
        } else {
            return ct_script_fail(err, cap, c, "move state must be on or off", CT_SCRIPT_EXIT_RUN);
        }
        if (on == ((f->move_mask & bit) != 0)) {
            return 0;
        }
        int rc = ct_settings_form_stage_move(f, bit);
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "move stage failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_SETTINGS_SAVE: {
        int rc = ct_settings_form_save(f);
        return rc == CT_REPO_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "settings save failed", CT_SCRIPT_EXIT_RUN);
    }
    default:
        return 0;
    }
}

static int ct_script_stage_parse(const char *word) {
    if (strcmp(word, "pll") == 0) {
        return CT_STAGE_PLL;
    }
    if (strcmp(word, "oll") == 0) {
        return CT_STAGE_OLL;
    }
    if (strcmp(word, "f2l") == 0) {
        return CT_STAGE_F2L;
    }
    return -1;
}

static int ct_script_step_trainer(ct_app *app, ct_script_rt *rt, const ct_script *s,
                                  const ct_script_row *c, char *err, unsigned cap) {
    switch (c->kind) {
    case CT_SCRIPT_CFOP_POPULATE: {
        int stage = ct_script_stage_parse(ct_script_arg(s, c, 0));
        if (stage < 0) {
            return ct_script_fail(err, cap, c, "unknown cfop stage", CT_SCRIPT_EXIT_RUN);
        }
        int made = ct_cfop_populate(app->db, (ct_stage)stage);
        return made >= 0 ? 0
                         : ct_script_fail(err, cap, c, "cfop populate failed", CT_SCRIPT_EXIT_RUN);
    }
    case CT_SCRIPT_TRAINER_LIST: {
        const char *ev_word = ct_script_arg(s, c, 0);
        const char *m_word = ct_script_arg(s, c, 1);
        long list_id = 0;
        if (ct_script_long(ct_script_arg(s, c, 2), &list_id) != 0) {
            return ct_script_fail(err, cap, c, "list id is not an integer", CT_SCRIPT_EXIT_RUN);
        }
        ct_event ev = CT_EVENT_3X3;
        bool ev_ok = false;
        for (unsigned i = 0; i < sizeof CT_SCRIPT_EVENTS / sizeof CT_SCRIPT_EVENTS[0]; i++) {
            if (strcmp(CT_SCRIPT_EVENTS[i].word, ev_word) == 0) {
                ev = CT_SCRIPT_EVENTS[i].event;
                ev_ok = true;
                break;
            }
        }
        ct_method m = CT_METHOD_CFOP;
        bool m_ok = false;
        for (unsigned i = 0; i < sizeof CT_SCRIPT_METHODS / sizeof CT_SCRIPT_METHODS[0]; i++) {
            if (strcmp(CT_SCRIPT_METHODS[i].word, m_word) == 0) {
                m = CT_SCRIPT_METHODS[i].method;
                m_ok = true;
                break;
            }
        }
        if (!ev_ok || !m_ok) {
            return ct_script_fail(err, cap, c, "unknown event or method", CT_SCRIPT_EXIT_RUN);
        }
        if (ct_selector_scheme_load(app->db, &rt->scheme) != 0) {
            return ct_script_fail(err, cap, c, "letter scheme unavailable", CT_SCRIPT_EXIT_RUN);
        }
        ct_selector_list lists[CT_SCRIPT_LIST_CAP];
        int found = ct_selector_lists(ev, m, lists, CT_SCRIPT_LIST_CAP);
        if (found <= 0 || list_id < 0 || list_id >= found || list_id >= CT_SCRIPT_LIST_CAP) {
            return ct_script_fail(err, cap, c, "list id out of range", CT_SCRIPT_EXIT_RUN);
        }
        rt->event = ev;
        rt->method = m;
        rt->list_id = (int)list_id;
        rt->orbit = lists[list_id].orbit;
        rt->target_key[0] = '\0';
        rt->generated = false;
        return 0;
    }
    case CT_SCRIPT_TRAINER_CASE: {
        long index;
        if (ct_script_long(ct_script_arg(s, c, 0), &index) != 0) {
            return ct_script_fail(err, cap, c, "case index is not an integer", CT_SCRIPT_EXIT_RUN);
        }
        int count =
            ct_selector_case_count(app->db, &rt->scheme, rt->event, rt->method, rt->list_id);
        if (count <= 0 || index < 0 || index >= count) {
            return ct_script_fail(err, cap, c, "case index out of range", CT_SCRIPT_EXIT_RUN);
        }
        char key[CT_CARD_CASE_KEY_MAX];
        if (ct_selector_case_key(app->db, &rt->scheme, rt->event, rt->method, rt->list_id,
                                 (int)index, key, (unsigned)sizeof key) != 0) {
            return ct_script_fail(err, cap, c, "case key unavailable", CT_SCRIPT_EXIT_RUN);
        }
        snprintf(rt->target_key, sizeof rt->target_key, "%s", key);
        rt->case_index = (int)index;
        rt->generated = false;
        return 0;
    }
    case CT_SCRIPT_TRAINER_GENERATE: {
        if (rt->target_key[0] == '\0') {
            return ct_script_fail(err, cap, c, "no target pair in scope", CT_SCRIPT_EXIT_RUN);
        }
        if (!rt->synth_ready) {
            ct_synth_init();
            rt->synth_ready = true;
        }
        int t1 = 0;
        int t2 = 0;
        if (ct_selector_case_pair(&rt->scheme, rt->event, rt->method, rt->list_id, rt->case_index,
                                  &t1, &t2) != 0) {
            return ct_script_fail(err, cap, c, "target is not a chart pair", CT_SCRIPT_EXIT_RUN);
        }
        int buffer = rt->orbit == CT_ORBIT_EDGE ? rt->scheme.buffer_edge : rt->scheme.buffer_corner;
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(rt->orbit, buffer, t1, t2, &app->cfg, &res);
        if (rc != 0 || !res.found) {
            return ct_script_fail(err, cap, c, "synthesis found nothing", CT_SCRIPT_EXIT_RUN);
        }
        if (ct_notation_format(res.seq, res.len, CT_SCRIPT_CUBE_N, rt->alg_text,
                               (unsigned)sizeof rt->alg_text) < 0) {
            return ct_script_fail(err, cap, c, "sequence formatting failed", CT_SCRIPT_EXIT_RUN);
        }
        memset(&rt->cls, 0, sizeof rt->cls);
        if (ct_classify(res.seq, res.len, &rt->cls) != CT_CLASSIFY_OK) {
            return ct_script_fail(err, cap, c, "classification failed", CT_SCRIPT_EXIT_RUN);
        }
        memset(&rt->parts, 0, sizeof rt->parts);
        if (ct_score_alg(res.seq, res.len, &app->cfg, &rt->parts) != CT_SCORE_OK) {
            return ct_script_fail(err, cap, c, "scoring failed", CT_SCRIPT_EXIT_RUN);
        }
        int scheme = ct_letter_scheme_default_id(app->db);
        if (scheme <= 0) {
            if (ct_letter_seed_speffz(app->db) != 0) {
                return ct_script_fail(err, cap, c, "letter scheme unavailable", CT_SCRIPT_EXIT_RUN);
            }
            scheme = ct_letter_scheme_default_id(app->db);
        }
        if (scheme <= 0) {
            return ct_script_fail(err, cap, c, "letter scheme unavailable", CT_SCRIPT_EXIT_RUN);
        }
        ct_params_hash(&app->cfg, scheme, CT_BUFFER_FIXED, rt->scheme.buffer_corner,
                       rt->scheme.buffer_edge, &rt->scheme.map, rt->params_hash);
        ct_generated_alg a;
        memset(&a, 0, sizeof a);
        a.orbit = rt->orbit;
        snprintf(a.target_pair, sizeof a.target_pair, "%s", rt->target_key);
        a.buffer_mode = CT_BUFFER_FIXED;
        a.category = rt->list_id;
        a.classification = rt->cls.cls;
        snprintf(a.sequence, sizeof a.sequence, "%s", rt->alg_text);
        ct_notation_format(rt->cls.setup, rt->cls.setup_len, CT_SCRIPT_CUBE_N, a.setup,
                           (unsigned)sizeof a.setup);
        ct_notation_format(rt->cls.interchange, rt->cls.interchange_len, CT_SCRIPT_CUBE_N,
                           a.interchange, (unsigned)sizeof a.interchange);
        ct_notation_format(rt->cls.insertion, rt->cls.insertion_len, CT_SCRIPT_CUBE_N, a.insertion,
                           (unsigned)sizeof a.insertion);
        a.qtm = rt->parts.qtm;
        a.score = rt->parts.total;
        ct_score_parts_json(&rt->parts, a.score_parts_json, (unsigned)sizeof a.score_parts_json);
        if (ct_repo_alg_put(app->db, &a, rt->params_hash) != CT_REPO_OK) {
            return ct_script_fail(err, cap, c, "generated alg store failed", CT_SCRIPT_EXIT_RUN);
        }
        rt->generated = true;
        return 0;
    }
    case CT_SCRIPT_TRAINER_SAVE: {
        if (!rt->generated) {
            return ct_script_fail(err, cap, c, "no generated alg in scope", CT_SCRIPT_EXIT_RUN);
        }
        ct_card card;
        memset(&card, 0, sizeof card);
        snprintf(card.deck, sizeof card.deck, "%s", ct_script_arg(s, c, 0));
        snprintf(card.case_key, sizeof card.case_key, "%s", rt->target_key);
        card.direction = CT_DIR_FR;
        snprintf(card.alg_3x3, sizeof card.alg_3x3, "%s", rt->alg_text);
        if (ct_repo_card_insert(app->db, &card) != CT_REPO_OK) {
            return ct_script_fail(err, cap, c, "card insert failed", CT_SCRIPT_EXIT_RUN);
        }
        return 0;
    }
    default:
        return 0;
    }
}

static int ct_script_step_due(ct_app *app, const ct_script *s, const ct_script_row *c, char *err,
                              unsigned cap) {
    ct_card card;
    int rc = ct_repo_card_get_by_key(app->db, ct_script_arg(s, c, 0), ct_script_arg(s, c, 1),
                                     CT_DIR_FR, &card);
    if (rc != CT_REPO_OK) {
        return ct_script_fail(err, cap, c, "card not found", CT_SCRIPT_EXIT_RUN);
    }
    const char *date = ct_script_arg(s, c, 2);
    if (ct_script_iso_day(date, &(long){0}) != 0) {
        return ct_script_fail(err, cap, c, "due date is not a date", CT_SCRIPT_EXIT_RUN);
    }
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    snprintf(row.due, sizeof row.due, "%s", date);
    rc = ct_repo_srs_put(app->db, card.id, &row);
    return rc == CT_REPO_OK ? 0
                            : ct_script_fail(err, cap, c, "srs seed failed", CT_SCRIPT_EXIT_RUN);
}

static void ct_script_rt_reset(ct_script_rt *rt) {
    if (rt->review_open) {
        ct_session_free(&rt->review.session);
    }
    memset(rt, 0, sizeof *rt);
}

static int ct_script_step(ct_app *app, ct_script_rt *rt, const ct_script *s, const ct_script_row *c,
                          char *err, unsigned cap) {
    switch (c->kind) {
    case CT_SCRIPT_RESTART:
        ct_script_rt_reset(rt);
        ct_app_shutdown(app);
        ct_app_init(app);
        if (!app->running) {
            return ct_script_fail(err, cap, c, "restart failed", CT_SCRIPT_EXIT_RUN);
        }
        return 0;
    case CT_SCRIPT_DUE:
        return ct_script_step_due(app, s, c, err, cap);
    case CT_SCRIPT_CARDS_OPEN:
    case CT_SCRIPT_CARDS_NEW:
    case CT_SCRIPT_CARDS_SELECT:
    case CT_SCRIPT_CARDS_EDIT:
    case CT_SCRIPT_CARDS_FIELD:
    case CT_SCRIPT_CARDS_SAVE:
    case CT_SCRIPT_CARDS_DELETE:
        return ct_script_step_cards(app, rt, s, c, err, cap);
    case CT_SCRIPT_REVIEW_BEGIN:
    case CT_SCRIPT_REVIEW_REVEAL:
    case CT_SCRIPT_REVIEW_GRADE:
    case CT_SCRIPT_REVIEW_END:
    case CT_SCRIPT_WAIT:
        return ct_script_step_review(app, rt, s, c, err, cap);
    case CT_SCRIPT_SETTINGS_OPEN:
    case CT_SCRIPT_SETTINGS_RETENTION:
    case CT_SCRIPT_SETTINGS_MOVE:
    case CT_SCRIPT_SETTINGS_SAVE:
        return ct_script_step_settings(app, rt, s, c, err, cap);
    case CT_SCRIPT_CFOP_POPULATE:
    case CT_SCRIPT_TRAINER_LIST:
    case CT_SCRIPT_TRAINER_CASE:
    case CT_SCRIPT_TRAINER_GENERATE:
    case CT_SCRIPT_TRAINER_SAVE:
        return ct_script_step_trainer(app, rt, s, c, err, cap);
    default:
        return ct_script_expect(app, rt, s, c, err, cap);
    }
}

int ct_script_run(ct_app *app, ct_script_rt *rt, const ct_script *s, char *err, unsigned cap) {
    if (app == NULL || rt == NULL || s == NULL || err == NULL || cap == 0) {
        return CT_SCRIPT_EXIT_RUN;
    }
    for (int i = 0; i < s->count; i++) {
        int rc = ct_script_step(app, rt, s, &s->cmds[i], err, cap);
        if (rc != 0) {
            return rc;
        }
    }
    if (rt->review_open) {
        ct_session_free(&rt->review.session);
        rt->review_open = false;
    }
    return 0;
}

int ct_app_run_scripted(ct_app *app, const char *path) {
    if (app == NULL || path == NULL || path[0] == '\0') {
        fprintf(stderr, "cubing-trainer: missing script path\n");
        return CT_SCRIPT_EXIT_USAGE;
    }
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "cubing-trainer: cannot open script %s\n", path);
        return CT_SCRIPT_EXIT_READ;
    }
    char *buf = malloc(CT_SCRIPT_TEXT_MAX + 1);
    if (buf == NULL) {
        fclose(f);
        fprintf(stderr, "cubing-trainer: out of memory\n");
        return CT_SCRIPT_EXIT_READ;
    }
    size_t got = fread(buf, 1, CT_SCRIPT_TEXT_MAX + 1, f);
    fclose(f);
    if (got > CT_SCRIPT_TEXT_MAX) {
        free(buf);
        fprintf(stderr, "cubing-trainer: script %s exceeds %u bytes\n", path,
                (unsigned)CT_SCRIPT_TEXT_MAX);
        return CT_SCRIPT_EXIT_READ;
    }
    buf[got] = '\0';
    ct_script script;
    char err[CT_SCRIPT_ERR_MAX];
    int rc = ct_script_parse(buf, &script, err, (unsigned)sizeof err);
    free(buf);
    if (rc != 0) {
        fprintf(stderr, "cubing-trainer: script %s: %s\n", path, err);
        return rc;
    }
    ct_script_rt rt;
    memset(&rt, 0, sizeof rt);
    rc = ct_script_run(app, &rt, &script, err, (unsigned)sizeof err);
    if (rc != 0) {
        fprintf(stderr, "cubing-trainer: script %s failed: %s\n", path, err);
    }
    return rc;
}
