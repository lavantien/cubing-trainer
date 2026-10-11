#include "app/script_internal.h"

#include <stdio.h>
#include <string.h>

#include "app/shots.h"
#include "storage/repo_cards.h"
#include "storage/repo_reviews.h"
#include "trainer/session.h"

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

ct_event ct_script_event_parse(const char *word, bool *ok) {
    *ok = false;
    for (unsigned i = 0; i < sizeof CT_SCRIPT_EVENTS / sizeof CT_SCRIPT_EVENTS[0]; i++) {
        if (strcmp(CT_SCRIPT_EVENTS[i].word, word) == 0) {
            *ok = true;
            return CT_SCRIPT_EVENTS[i].event;
        }
    }
    return CT_EVENT_3X3;
}

ct_method ct_script_method_parse(const char *word, bool *ok) {
    *ok = false;
    for (unsigned i = 0; i < sizeof CT_SCRIPT_METHODS / sizeof CT_SCRIPT_METHODS[0]; i++) {
        if (strcmp(CT_SCRIPT_METHODS[i].word, word) == 0) {
            *ok = true;
            return CT_SCRIPT_METHODS[i].method;
        }
    }
    return CT_METHOD_CFOP;
}

ct_grade ct_script_grade_parse(const char *word, bool *ok) {
    *ok = false;
    for (unsigned i = 0; i < sizeof CT_SCRIPT_GRADES / sizeof CT_SCRIPT_GRADES[0]; i++) {
        if (strcmp(CT_SCRIPT_GRADES[i].word, word) == 0) {
            *ok = true;
            return CT_SCRIPT_GRADES[i].grade;
        }
    }
    return CT_AGAIN;
}

int ct_script_step_reviewed(ct_app *app, const ct_script *s, const ct_script_row *c, char *err,
                            unsigned cap) {
    ct_card card;
    int rc = ct_repo_card_get_by_key(app->db, ct_script_arg(s, c, 0), ct_script_arg(s, c, 1),
                                     CT_DIR_FR, &card);
    if (rc != CT_REPO_OK) {
        return ct_script_fail(err, cap, c, "card not found", CT_SCRIPT_EXIT_RUN);
    }
    bool found = false;
    ct_grade g = ct_script_grade_parse(ct_script_arg(s, c, 2), &found);
    if (!found) {
        return ct_script_fail(err, cap, c, "unknown grade", CT_SCRIPT_EXIT_RUN);
    }
    const char *iso = ct_script_arg(s, c, 3);
    if (ct_script_iso_day(iso, &(long){0}) != 0) {
        return ct_script_fail(err, cap, c, "reviewed date is not a date", CT_SCRIPT_EXIT_RUN);
    }
    double stability = 0.0;
    double elapsed = 0.0;
    if (ct_script_double(ct_script_arg(s, c, 4), &stability) != 0) {
        return ct_script_fail(err, cap, c, "stability is not a number", CT_SCRIPT_EXIT_RUN);
    }
    if (ct_script_double(ct_script_arg(s, c, 5), &elapsed) != 0) {
        return ct_script_fail(err, cap, c, "elapsed is not a number", CT_SCRIPT_EXIT_RUN);
    }
    rc = ct_repo_review_log(app->db, card.id, (int)g, iso, stability, 0.0, elapsed, 0.0);
    return rc == CT_REPO_OK
               ? 0
               : ct_script_fail(err, cap, c, "review log seed failed", CT_SCRIPT_EXIT_RUN);
}

int ct_script_step_due_in(ct_app *app, const ct_script *s, const ct_script_row *c, char *err,
                          unsigned cap) {
    ct_card card;
    int rc = ct_repo_card_get_by_key(app->db, ct_script_arg(s, c, 0), ct_script_arg(s, c, 1),
                                     CT_DIR_FR, &card);
    if (rc != CT_REPO_OK) {
        return ct_script_fail(err, cap, c, "card not found", CT_SCRIPT_EXIT_RUN);
    }
    long days = 0;
    if (ct_script_long(ct_script_arg(s, c, 2), &days) != 0) {
        return ct_script_fail(err, cap, c, "days is not an integer", CT_SCRIPT_EXIT_RUN);
    }
    char now[CT_SESSION_STAMP_MAX];
    long today = 0;
    if (ct_script_now(now, (unsigned)sizeof now, &today) != 0) {
        return ct_script_fail(err, cap, c, "cannot read the clock", CT_SCRIPT_EXIT_RUN);
    }
    char date[CT_SESSION_STAMP_MAX];
    if (ct_session_iso_add_days(now, days, date, (unsigned)sizeof date) != CT_SESSION_OK) {
        return ct_script_fail(err, cap, c, "days out of range", CT_SCRIPT_EXIT_RUN);
    }
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    snprintf(row.due, sizeof row.due, "%s", date);
    rc = ct_repo_srs_put(app->db, card.id, &row);
    return rc == CT_REPO_OK ? 0
                            : ct_script_fail(err, cap, c, "srs seed failed", CT_SCRIPT_EXIT_RUN);
}

int ct_script_step_shot(ct_app *app, const ct_script *s, const ct_script_row *c, char *err,
                        unsigned cap) {
    int rc;
    switch (c->kind) {
    case CT_SCRIPT_SHOT_MENU:
        rc = ct_shots_menu(app, ct_script_arg(s, c, 0));
        return rc == CT_SHOTS_OK
                   ? 0
                   : ct_script_fail(err, cap, c, "screenshot capture failed", CT_SCRIPT_EXIT_RUN);
    case CT_SCRIPT_SHOT_TRAINER:
    case CT_SCRIPT_SHOT_CHART: {
        bool ev_ok = false;
        bool m_ok = false;
        ct_event ev = ct_script_event_parse(ct_script_arg(s, c, 0), &ev_ok);
        ct_method m = ct_script_method_parse(ct_script_arg(s, c, 1), &m_ok);
        if (!ev_ok || !m_ok) {
            return ct_script_fail(err, cap, c, "unknown event or method", CT_SCRIPT_EXIT_RUN);
        }
        long list_id = 0;
        long pick = 0;
        if (ct_script_long(ct_script_arg(s, c, 2), &list_id) != 0) {
            return ct_script_fail(err, cap, c, "list id is not an integer", CT_SCRIPT_EXIT_RUN);
        }
        if (ct_script_long(ct_script_arg(s, c, 3), &pick) != 0) {
            return ct_script_fail(err, cap, c, "pick index is not an integer", CT_SCRIPT_EXIT_RUN);
        }
        if (c->kind == CT_SCRIPT_SHOT_TRAINER) {
            rc = ct_shots_trainer(app, ev, m, (int)list_id, (int)pick,
                                  ct_script_arg(s, c, CT_SCRIPT_ARG_D));
        } else {
            rc = ct_shots_chart(app, ev, m, (int)list_id, (int)pick,
                                ct_script_arg(s, c, CT_SCRIPT_ARG_D));
        }
        break;
    }
    case CT_SCRIPT_SHOT_REVIEW: {
        const char *mode = ct_script_arg(s, c, 0);
        bool revealed;
        if (strcmp(mode, "hidden") == 0) {
            revealed = false;
        } else if (strcmp(mode, "revealed") == 0) {
            revealed = true;
        } else {
            return ct_script_fail(err, cap, c, "review state must be hidden or revealed",
                                  CT_SCRIPT_EXIT_RUN);
        }
        rc = ct_shots_review(app, revealed, ct_script_arg(s, c, 1));
        break;
    }
    case CT_SCRIPT_SHOT_CARDS: {
        const char *mode = ct_script_arg(s, c, 0);
        bool editing;
        if (strcmp(mode, "browse") == 0) {
            editing = false;
        } else if (strcmp(mode, "edit") == 0) {
            editing = true;
        } else {
            return ct_script_fail(err, cap, c, "cards state must be browse or edit",
                                  CT_SCRIPT_EXIT_RUN);
        }
        rc = ct_shots_cards(app, ct_script_arg(s, c, 1), editing, ct_script_arg(s, c, 2));
        break;
    }
    case CT_SCRIPT_SHOT_SETTINGS: {
        const char *mode = ct_script_arg(s, c, 0);
        int tab;
        if (strcmp(mode, "general") == 0) {
            tab = CT_SETTINGS_TAB_GENERAL;
        } else if (strcmp(mode, "letters") == 0) {
            tab = CT_SETTINGS_TAB_LETTERS;
        } else if (strcmp(mode, "bld") == 0) {
            tab = CT_SETTINGS_TAB_BLD;
        } else {
            return ct_script_fail(err, cap, c, "unknown settings tab", CT_SCRIPT_EXIT_RUN);
        }
        rc = ct_shots_settings(app, tab, ct_script_arg(s, c, 1));
        break;
    }
    case CT_SCRIPT_SHOT_STATS:
        rc = ct_shots_stats(app, ct_script_arg(s, c, 0));
        break;
    default:
        return 0;
    }
    if (rc == CT_SHOTS_E_WRITE) {
        return ct_script_fail(err, cap, c, "screenshot write failed", CT_SCRIPT_EXIT_RUN);
    }
    return rc == CT_SHOTS_OK
               ? 0
               : ct_script_fail(err, cap, c, "screenshot capture failed", CT_SCRIPT_EXIT_RUN);
}
