#include <stdio.h>
#include <string.h>

#include "app/script_internal.h"
#include "storage/repo_reviews.h"

static const char *const CT_SCRIPT_TABLES[] = {
    "settings",   "letter_scheme", "letter_map",  "flashcard",  "srs_state",
    "review_log", "generated_alg", "session_log", "event_pack",
};

static const char *ct_script_card_field(const ct_card *c, const char *name) {
    if (strcmp(name, "alg3") == 0) {
        return c->alg_3x3;
    }
    if (strcmp(name, "algbig") == 0) {
        return c->alg_big;
    }
    if (strcmp(name, "algoh") == 0) {
        return c->alg_oh;
    }
    if (strcmp(name, "front") == 0) {
        return c->front_json;
    }
    if (strcmp(name, "notes") == 0) {
        return c->notes;
    }
    if (strcmp(name, "deck") == 0) {
        return c->deck;
    }
    if (strcmp(name, "key") == 0) {
        return c->case_key;
    }
    return NULL;
}

static int ct_script_count(sqlite3 *db, const char *table, long *out) {
    for (unsigned i = 0; i < sizeof CT_SCRIPT_TABLES / sizeof CT_SCRIPT_TABLES[0]; i++) {
        if (strcmp(CT_SCRIPT_TABLES[i], table) != 0) {
            continue;
        }
        char sql[CT_SCRIPT_ERR_MAX];
        snprintf(sql, sizeof sql, "SELECT COUNT(*) FROM %s", table);
        sqlite3_stmt *st = NULL;
        if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) {
            sqlite3_finalize(st);
            return -1;
        }
        int rc = sqlite3_step(st);
        *out = rc == SQLITE_ROW ? (long)sqlite3_column_int64(st, 0) : -1;
        sqlite3_finalize(st);
        return rc == SQLITE_ROW ? 0 : -1;
    }
    return -1;
}

static int ct_script_expect_count(ct_app *app, const ct_script *s, const ct_script_row *c,
                                  char *err, unsigned cap) {
    int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_1));
    long want;
    if (op < 0 || ct_script_long(ct_script_arg(s, c, CT_SCRIPT_ARGC_2), &want) != 0) {
        return ct_script_fail(err, cap, c, "bad count operator or value", CT_SCRIPT_EXIT_EXPECT);
    }
    long got;
    if (ct_script_count(app->db, ct_script_arg(s, c, 0), &got) != 0) {
        return ct_script_fail(err, cap, c, "unknown table", CT_SCRIPT_EXIT_EXPECT);
    }
    if (!ct_script_long_ok(op, got, want)) {
        snprintf(err, cap, "line %d: count %s is %ld, expected %s %ld", c->line,
                 ct_script_arg(s, c, 0), got, ct_script_arg(s, c, 1), want);
        return CT_SCRIPT_EXIT_EXPECT;
    }
    return 0;
}

static int ct_script_card_lookup(ct_app *app, const ct_script *s, const ct_script_row *c,
                                 int deck_at, int key_at, ct_card *card, char *err, unsigned cap) {
    int rc = ct_repo_card_get_by_key(app->db, ct_script_arg(s, c, deck_at),
                                     ct_script_arg(s, c, key_at), CT_DIR_FR, card);
    if (rc != CT_REPO_OK) {
        return ct_script_fail(err, cap, c, "card not found", CT_SCRIPT_EXIT_EXPECT);
    }
    return 0;
}

static int ct_script_expect_card(ct_app *app, const ct_script *s, const ct_script_row *c, char *err,
                                 unsigned cap) {
    ct_card card;
    int rc = ct_script_card_lookup(app, s, c, 0, 1, &card, err, cap);
    if (rc != 0) {
        return rc;
    }
    const char *got = ct_script_card_field(&card, ct_script_arg(s, c, CT_SCRIPT_ARGC_2));
    if (got == NULL) {
        return ct_script_fail(err, cap, c, "unknown card field", CT_SCRIPT_EXIT_EXPECT);
    }
    bool equal = strcmp(got, ct_script_arg(s, c, CT_SCRIPT_ARGC_4)) == 0;
    bool want_eq = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_3)) == 0;
    if (equal != want_eq) {
        snprintf(err, cap, "line %d: card %s is [%s], expected %s [%s]", c->line,
                 ct_script_arg(s, c, CT_SCRIPT_ARGC_2), got, ct_script_arg(s, c, CT_SCRIPT_ARGC_3),
                 ct_script_arg(s, c, CT_SCRIPT_ARGC_4));
        return CT_SCRIPT_EXIT_EXPECT;
    }
    return 0;
}

static int ct_script_expect_srs(ct_app *app, const ct_script *s, const ct_script_row *c, char *err,
                                unsigned cap) {
    ct_card card;
    int rc = ct_script_card_lookup(app, s, c, 0, 1, &card, err, cap);
    if (rc != 0) {
        return rc;
    }
    ct_srs_row row;
    if (ct_repo_srs_get(app->db, card.id, &row) != CT_REPO_OK) {
        return ct_script_fail(err, cap, c, "srs state missing", CT_SCRIPT_EXIT_EXPECT);
    }
    const char *field = ct_script_arg(s, c, 2);
    const char *value = ct_script_arg(s, c, CT_SCRIPT_ARGC_4);
    if (strcmp(field, "due") == 0) {
        char want[CT_SESSION_STAMP_MAX];
        if (strcmp(value, "today") == 0) {
            long day = 0;
            if (ct_script_now(want, (unsigned)sizeof want, &day) != 0) {
                return ct_script_fail(err, cap, c, "cannot read the clock", CT_SCRIPT_EXIT_EXPECT);
            }
        } else if (ct_script_iso_day(value, &(long){0}) != 0) {
            return ct_script_fail(err, cap, c, "due value is not a date", CT_SCRIPT_EXIT_EXPECT);
        } else {
            snprintf(want, sizeof want, "%s", value);
        }
        if (strncmp(row.due, want, CT_ISO_DATE_LEN) != 0) {
            snprintf(err, cap, "line %d: due is [%s], expected [%s]", c->line, row.due, want);
            return CT_SCRIPT_EXIT_EXPECT;
        }
        return 0;
    }
    long got;
    if (strcmp(field, "reps") == 0) {
        got = row.reps;
    } else if (strcmp(field, "lapses") == 0) {
        got = row.lapses;
    } else if (strcmp(field, "state") == 0) {
        got = row.state;
    } else {
        return ct_script_fail(err, cap, c, "unknown srs field", CT_SCRIPT_EXIT_EXPECT);
    }
    int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_3));
    long want;
    if (op < 0 || ct_script_long(value, &want) != 0) {
        return ct_script_fail(err, cap, c, "bad srs operator or value", CT_SCRIPT_EXIT_EXPECT);
    }
    if (!ct_script_long_ok(op, got, want)) {
        snprintf(err, cap, "line %d: srs %s is %ld, expected %s %ld", c->line, field, got,
                 ct_script_arg(s, c, CT_SCRIPT_ARGC_3), want);
        return CT_SCRIPT_EXIT_EXPECT;
    }
    return 0;
}

static int ct_script_expect_reviews(ct_app *app, const ct_script *s, const ct_script_row *c,
                                    char *err, unsigned cap) {
    ct_card card;
    int rc = ct_script_card_lookup(app, s, c, 0, 1, &card, err, cap);
    if (rc != 0) {
        return rc;
    }
    ct_review_row rows[CT_STATS_REVIEW_ROWS_MAX];
    int n = ct_repo_reviews_for_card(app->db, card.id, rows, CT_STATS_REVIEW_ROWS_MAX);
    if (n < 0) {
        return ct_script_fail(err, cap, c, "review query failed", CT_SCRIPT_EXIT_EXPECT);
    }
    char got[CT_SCRIPT_ERR_MAX];
    unsigned at = 0;
    for (int i = 0; i < n; i++) {
        if (i > 0 && at + 1 < sizeof got) {
            got[at++] = ',';
        }
        at += (unsigned)snprintf(got + at, sizeof got - at, "%d", rows[i].rating);
    }
    int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_2));
    if (op != 0 && op != 3) {
        return ct_script_fail(err, cap, c, "reviews supports == or !=", CT_SCRIPT_EXIT_EXPECT);
    }
    bool equal = strcmp(got, ct_script_arg(s, c, CT_SCRIPT_ARGC_3)) == 0;
    if (equal != (op == 0)) {
        snprintf(err, cap, "line %d: ratings are [%s], expected %s [%s]", c->line, got,
                 ct_script_arg(s, c, 2), ct_script_arg(s, c, CT_SCRIPT_ARGC_3));
        return CT_SCRIPT_EXIT_EXPECT;
    }
    return 0;
}

static int ct_script_expect_setting(ct_app *app, const ct_script *s, const ct_script_row *c,
                                    char *err, unsigned cap) {
    char got[CT_SETTINGS_TEXT_MAX];
    if (ct_repo_settings_get(app->db, ct_script_arg(s, c, 0), got, (unsigned)sizeof got) != 0) {
        return ct_script_fail(err, cap, c, "setting not found", CT_SCRIPT_EXIT_EXPECT);
    }
    bool equal = strcmp(got, ct_script_arg(s, c, 2)) == 0;
    bool want_eq = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_1)) == 0;
    if (equal != want_eq) {
        snprintf(err, cap, "line %d: setting is [%s], expected %s [%s]", c->line, got,
                 ct_script_arg(s, c, 1), ct_script_arg(s, c, CT_SCRIPT_ARGC_2));
        return CT_SCRIPT_EXIT_EXPECT;
    }
    return 0;
}

static int ct_script_moves_mask(const char *letters, unsigned *mask) {
    unsigned m = 0;
    for (const char *p = letters; *p != '\0'; p++) {
        unsigned bit = ct_move_face_from_char(*p);
        if (bit == 0 || (bit & CT_MOVE_ROTATION_MASK) != 0) {
            return -1;
        }
        m |= bit;
    }
    *mask = m;
    return 0;
}

static int ct_script_expect_cfg(ct_app *app, const ct_script *s, const ct_script_row *c, char *err,
                                unsigned cap) {
    const char *key = ct_script_arg(s, c, 0);
    const char *value = ct_script_arg(s, c, CT_SCRIPT_ARGC_2);
    int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_1));
    if (strcmp(key, "srs.request_retention") == 0 || strcmp(key, "anim.move_seconds") == 0) {
        double want;
        if (ct_script_double(value, &want) != 0) {
            return ct_script_fail(err, cap, c, "cfg value is not a number", CT_SCRIPT_EXIT_EXPECT);
        }
        double got = strcmp(key, "srs.request_retention") == 0 ? app->cfg.fsrs.request_retention
                                                               : app->cfg.anim.move_seconds;
        if (!ct_script_double_ok(op, got, want)) {
            snprintf(err, cap, "line %d: cfg %s is %g, expected %s %g", c->line, key, got,
                     ct_script_arg(s, c, 1), want);
            return CT_SCRIPT_EXIT_EXPECT;
        }
        return 0;
    }
    if (strcmp(key, "gen.preferred_moves") == 0) {
        unsigned want;
        if (ct_script_moves_mask(value, &want) != 0) {
            return ct_script_fail(err, cap, c, "cfg value is not a move set",
                                  CT_SCRIPT_EXIT_EXPECT);
        }
        if (app->cfg.gen.preferred_moves != want) {
            snprintf(err, cap, "line %d: cfg preferred moves is %u, expected %u", c->line,
                     app->cfg.gen.preferred_moves, want);
            return CT_SCRIPT_EXIT_EXPECT;
        }
        return 0;
    }
    return ct_script_fail(err, cap, c, "unknown cfg key", CT_SCRIPT_EXIT_EXPECT);
}

static int ct_script_expect_algcount(ct_app *app, const ct_script_rt *rt, const ct_script *s,
                                     const ct_script_row *c, char *err, unsigned cap) {
    if (!rt->generated) {
        return ct_script_fail(err, cap, c, "no generated params hash in scope",
                              CT_SCRIPT_EXIT_EXPECT);
    }
    int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_0));
    long want;
    if (op < 0 || ct_script_long(ct_script_arg(s, c, CT_SCRIPT_ARGC_1), &want) != 0) {
        return ct_script_fail(err, cap, c, "bad algcount operator or value", CT_SCRIPT_EXIT_EXPECT);
    }
    long got = ct_repo_alg_count(app->db, rt->params_hash);
    if (!ct_script_long_ok(op, got, want)) {
        snprintf(err, cap, "line %d: algcount is %ld, expected %s %ld", c->line, got,
                 ct_script_arg(s, c, 0), want);
        return CT_SCRIPT_EXIT_EXPECT;
    }
    return 0;
}

static int ct_script_expect_genalg(ct_app *app, const ct_script_rt *rt, const ct_script *s,
                                   const ct_script_row *c, char *err, unsigned cap) {
    if (!rt->generated) {
        return ct_script_fail(err, cap, c, "no generated params hash in scope",
                              CT_SCRIPT_EXIT_EXPECT);
    }
    ct_generated_alg a;
    if (ct_repo_alg_get(app->db, rt->orbit, ct_script_arg(s, c, 0), CT_BUFFER_FIXED,
                        CT_ALG_CATEGORY_CHART, rt->params_hash, &a) != CT_REPO_OK) {
        return ct_script_fail(err, cap, c, "generated alg not found", CT_SCRIPT_EXIT_EXPECT);
    }
    const char *field = ct_script_arg(s, c, 1);
    const char *value = ct_script_arg(s, c, CT_SCRIPT_ARGC_3);
    if (strcmp(field, "sequence") == 0 || strcmp(field, "setup") == 0 ||
        strcmp(field, "interchange") == 0 || strcmp(field, "insertion") == 0) {
        const char *got = strcmp(field, "sequence") == 0      ? a.sequence
                          : strcmp(field, "setup") == 0       ? a.setup
                          : strcmp(field, "interchange") == 0 ? a.interchange
                                                              : a.insertion;
        if (strcmp(got, value) != 0) {
            snprintf(err, cap, "line %d: genalg %s is [%s], expected [%s]", c->line, field, got,
                     value);
            return CT_SCRIPT_EXIT_EXPECT;
        }
        return 0;
    }
    if (strcmp(field, "classification") == 0) {
        if (strcmp(ct_classification_label(a.classification), value) != 0) {
            snprintf(err, cap, "line %d: genalg classification is [%s], expected [%s]", c->line,
                     ct_classification_label(a.classification), value);
            return CT_SCRIPT_EXIT_EXPECT;
        }
        return 0;
    }
    if (strcmp(field, "qtm") == 0) {
        int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_2));
        long want;
        if (op < 0 || ct_script_long(value, &want) != 0) {
            return ct_script_fail(err, cap, c, "bad qtm operator or value", CT_SCRIPT_EXIT_EXPECT);
        }
        if (!ct_script_long_ok(op, a.qtm, want)) {
            snprintf(err, cap, "line %d: genalg qtm is %d, expected %s %ld", c->line, a.qtm,
                     ct_script_arg(s, c, 2), want);
            return CT_SCRIPT_EXIT_EXPECT;
        }
        return 0;
    }
    if (strcmp(field, "score") == 0) {
        int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_2));
        double want;
        if (op < 0 || ct_script_double(value, &want) != 0) {
            return ct_script_fail(err, cap, c, "bad score operator or value",
                                  CT_SCRIPT_EXIT_EXPECT);
        }
        if (!ct_script_double_ok(op, a.score, want)) {
            snprintf(err, cap, "line %d: genalg score is %g, expected %s %g", c->line, a.score,
                     ct_script_arg(s, c, 2), want);
            return CT_SCRIPT_EXIT_EXPECT;
        }
        return 0;
    }
    return ct_script_fail(err, cap, c, "unknown genalg field", CT_SCRIPT_EXIT_EXPECT);
}

static int ct_script_expect_memo(ct_app *app, const ct_script *s, const ct_script_row *c,
                                 char *err, unsigned cap) {
    int orbit = ct_script_orbit_parse(ct_script_arg(s, c, 0));
    if (orbit < 0) {
        return ct_script_fail(err, cap, c, "unknown orbit", CT_SCRIPT_EXIT_EXPECT);
    }
    ct_memo_row row;
    if (ct_memo_get(app->db, orbit, ct_script_arg(s, c, 1), &row) != CT_REPO_OK) {
        return ct_script_fail(err, cap, c, "memo row missing", CT_SCRIPT_EXIT_EXPECT);
    }
    const char *field = ct_script_arg(s, c, 2);
    const char *got;
    if (strcmp(field, "phoneme") == 0) {
        got = row.phoneme;
    } else if (strcmp(field, "image") == 0) {
        got = row.image;
    } else {
        return ct_script_fail(err, cap, c, "unknown memo field", CT_SCRIPT_EXIT_EXPECT);
    }
    int op = ct_script_op(ct_script_arg(s, c, CT_SCRIPT_ARGC_3));
    if (op != CT_SCRIPT_EQ && op != CT_SCRIPT_NE) {
        return ct_script_fail(err, cap, c, "memo supports == or !=", CT_SCRIPT_EXIT_EXPECT);
    }
    bool equal = strcmp(got, ct_script_arg(s, c, CT_SCRIPT_ARGC_4)) == 0;
    if (equal != (op == CT_SCRIPT_EQ)) {
        snprintf(err, cap, "line %d: memo %s is [%s], expected %s [%s]", c->line, field, got,
                 ct_script_arg(s, c, CT_SCRIPT_ARGC_3), ct_script_arg(s, c, CT_SCRIPT_ARGC_4));
        return CT_SCRIPT_EXIT_EXPECT;
    }
    return 0;
}

int ct_script_expect(ct_app *app, ct_script_rt *rt, const ct_script *s, const ct_script_row *c,
                     char *err, unsigned cap) {
    switch (c->kind) {
    case CT_SCRIPT_EXPECT_COUNT:
        return ct_script_expect_count(app, s, c, err, cap);
    case CT_SCRIPT_EXPECT_CARD:
        return ct_script_expect_card(app, s, c, err, cap);
    case CT_SCRIPT_EXPECT_SRS:
        return ct_script_expect_srs(app, s, c, err, cap);
    case CT_SCRIPT_EXPECT_REVIEWS:
        return ct_script_expect_reviews(app, s, c, err, cap);
    case CT_SCRIPT_EXPECT_SETTING:
        return ct_script_expect_setting(app, s, c, err, cap);
    case CT_SCRIPT_EXPECT_CFG:
        return ct_script_expect_cfg(app, s, c, err, cap);
    case CT_SCRIPT_EXPECT_ALGCOUNT:
        return ct_script_expect_algcount(app, rt, s, c, err, cap);
    case CT_SCRIPT_EXPECT_MEMO:
        return ct_script_expect_memo(app, s, c, err, cap);
    default:
        return ct_script_expect_genalg(app, rt, s, c, err, cap);
    }
}
