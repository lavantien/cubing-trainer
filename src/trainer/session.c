#include "trainer/session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage/repo_reviews.h"

static const char CT_SQL_SESSION_INSERT[] =
    "INSERT INTO session_log(deck, started_at, ended_at, n_cards, n_again, n_hard, n_good, n_easy)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

enum {
    ct_civ_mar_shift = 3,
    ct_civ_jan_shift = 9,
    ct_civ_arc = ct_civ_jan_shift * (ct_civ_jan_shift + 8),
    ct_civ_five = 5,
    ct_civ_ten = ct_civ_five * 2,
    ct_civ_quad = 4,
    ct_civ_century = ct_civ_ten * ct_civ_ten,
    ct_civ_era_years = ct_civ_century * ct_civ_quad,
    ct_civ_era_years_m1 = ct_civ_era_years - 1,
    ct_civ_year_days = ct_civ_century * ct_civ_mar_shift + ct_civ_ten * 6 + ct_civ_five,
    ct_civ_era_leaps = ct_civ_century - ct_civ_mar_shift,
    ct_civ_era_days = ct_civ_era_years * ct_civ_year_days + ct_civ_era_leaps,
    ct_civ_era_days_m1 = ct_civ_era_days - 1,
    ct_civ_quad_days = ct_civ_quad * ct_civ_year_days,
    ct_civ_century_days = ct_civ_century * ct_civ_year_days + ct_civ_ten * 2 + ct_civ_quad,
    ct_civ_epoch_year = 2 * ct_civ_ten * ct_civ_ten * ct_civ_ten - ct_civ_mar_shift * ct_civ_ten,
    ct_iso_year_digits = 4,
    ct_iso_month_at = ct_iso_year_digits + 1,
    ct_iso_day_at = ct_iso_year_digits + ct_civ_mar_shift + 1,
    ct_iso_date_len = ct_civ_ten,
    ct_civ_year_min = 1,
    ct_civ_year_max = ct_civ_ten * ct_civ_ten * ct_civ_ten * ct_civ_ten - 1,
    ct_civ_month_min = 1,
    ct_civ_month_max = 12,
    ct_civ_day_min = 1,
    ct_civ_day_max = 31,
    ct_queue_first_cap = CT_SESSION_STAMP_MAX
};

static long ct_days_raw(long y, int m, int d) {
    y -= m <= 2;
    long era = (y >= 0 ? y : y - ct_civ_era_years_m1) / ct_civ_era_years;
    long yoe = y - era * ct_civ_era_years;
    long mm = m > 2 ? m - ct_civ_mar_shift : m + ct_civ_jan_shift;
    long doy = (ct_civ_arc * mm + 2) / ct_civ_five + d - 1;
    long doe = yoe * ct_civ_year_days + yoe / ct_civ_quad - yoe / ct_civ_century + doy;
    return era * ct_civ_era_days + doe;
}

static void ct_civil_raw(long z, long *y, int *m, int *d) {
    long era = (z >= 0 ? z : z - ct_civ_era_days_m1) / ct_civ_era_days;
    long doe = z - era * ct_civ_era_days;
    long yoe =
        (doe - doe / ct_civ_quad_days + doe / ct_civ_century_days - doe / ct_civ_era_days_m1) /
        ct_civ_year_days;
    long doy = doe - (yoe * ct_civ_year_days + yoe / ct_civ_quad - yoe / ct_civ_century);
    long mp = (ct_civ_five * doy + 2) / ct_civ_arc;
    int mm = (int)(mp < ct_civ_ten ? mp + ct_civ_mar_shift : mp - ct_civ_jan_shift);
    *m = mm;
    *d = (int)(doy - (ct_civ_arc * mp + 2) / ct_civ_five + 1);
    *y = yoe + era * ct_civ_era_years + (mm <= 2);
}

long ct_session_date_to_day(long y, int m, int d) {
    return ct_days_raw(y, m, d) - ct_days_raw(ct_civ_epoch_year, 1, 1);
}

void ct_session_day_to_date(long day, long *y, int *m, int *d) {
    ct_civil_raw(day + ct_days_raw(ct_civ_epoch_year, 1, 1), y, m, d);
}

static int ct_iso_digit(const char *s, int at) {
    char c = s[at];
    if (c < '0' || c > '9')
        return -1;
    return c - '0';
}

static int ct_iso_scan(const char *iso, long *y, int *m, int *d) {
    if (iso == NULL)
        return CT_SESSION_ERR_ARG;
    long yy = 0;
    for (int i = 0; i < ct_iso_year_digits; i++) {
        int v = ct_iso_digit(iso, i);
        if (v < 0)
            return CT_SESSION_ERR_ARG;
        yy = yy * CT_DECIMAL_BASE + v;
    }
    if (iso[ct_iso_year_digits] != '-')
        return CT_SESSION_ERR_ARG;
    if (iso[ct_iso_day_at - 1] != '-')
        return CT_SESSION_ERR_ARG;
    int m1 = ct_iso_digit(iso, ct_iso_month_at);
    int m2 = ct_iso_digit(iso, ct_iso_month_at + 1);
    int d1 = ct_iso_digit(iso, ct_iso_day_at);
    int d2 = ct_iso_digit(iso, ct_iso_day_at + 1);
    if (m1 < 0 || m2 < 0 || d1 < 0 || d2 < 0)
        return CT_SESSION_ERR_ARG;
    int mm = m1 * CT_DECIMAL_BASE + m2;
    int dd = d1 * CT_DECIMAL_BASE + d2;
    long ry;
    int rm, rd;
    ct_session_day_to_date(ct_session_date_to_day(yy, mm, dd), &ry, &rm, &rd);
    if (ry != yy || rm != mm || rd != dd)
        return CT_SESSION_ERR_ARG;
    if (iso[ct_iso_date_len] != '\0' && iso[ct_iso_date_len] != 'T')
        return CT_SESSION_ERR_ARG;
    *y = yy;
    *m = mm;
    *d = dd;
    return CT_SESSION_OK;
}

int ct_session_iso_add_days(const char *iso, long days, char *out, unsigned cap) {
    long y;
    int m, d;
    int rc = ct_iso_scan(iso, &y, &m, &d);
    if (rc != CT_SESSION_OK)
        return rc;
    if (out == NULL)
        return CT_SESSION_ERR_ARG;
    size_t len = strlen(iso);
    size_t suffix = len < (size_t)ct_iso_date_len ? 0 : len - (size_t)ct_iso_date_len;
    long base = ct_session_date_to_day(y, m, d);
    long min_day = ct_session_date_to_day(ct_civ_year_min, ct_civ_month_min, ct_civ_day_min);
    long max_day = ct_session_date_to_day(ct_civ_year_max, ct_civ_month_max, ct_civ_day_max);
    if (days < min_day - base || days > max_day - base)
        return CT_SESSION_ERR_ARG;
    long ry;
    int rm, rd;
    ct_session_day_to_date(base + days, &ry, &rm, &rd);
    int n = snprintf(out, cap, "%04ld-%02d-%02d", ry, rm, rd);
    if (n < 0)
        return CT_SESSION_ERR_DB;
    size_t date_len = (size_t)n;
    if (date_len + suffix + 1 > cap)
        return CT_SESSION_ERR_ARG;
    memcpy(out + date_len, iso + ct_iso_date_len, suffix);
    out[date_len + suffix] = '\0';
    return CT_SESSION_OK;
}

static int ct_queue_load(ct_session *s, const char *deck, const char *now_iso, int limit) {
    unsigned cap = limit > 0 ? (unsigned)limit : (unsigned)ct_queue_first_cap;
    for (;;) {
        long *grown = realloc(s->queue, (size_t)cap * sizeof *grown);
        if (grown == NULL)
            return CT_SESSION_ERR_DB;
        s->queue = grown;
        int n = ct_repo_due_cards(s->db, deck, now_iso, s->queue, (int)cap);
        if (n < 0)
            return CT_SESSION_ERR_DB;
        if (n < (int)cap || limit > 0) {
            s->queue_count = n;
            return CT_SESSION_OK;
        }
        cap *= 2u;
    }
}

int ct_session_begin(ct_session *s, sqlite3 *db, const ct_fsrs *fsrs, const char *deck,
                     const char *now_iso, long today_day, int limit) {
    if (s == NULL || db == NULL || fsrs == NULL)
        return CT_SESSION_ERR_ARG;
    if (!ct_repo_text_ok(deck, CT_SESSION_DECK_MAX))
        return CT_SESSION_ERR_ARG;
    if (!ct_repo_text_ok(now_iso, CT_SESSION_STAMP_MAX))
        return CT_SESSION_ERR_ARG;
    long y;
    int m, d;
    if (ct_iso_scan(now_iso, &y, &m, &d) != CT_SESSION_OK)
        return CT_SESSION_ERR_ARG;
    free(s->queue);
    s->queue = NULL;
    s->queue_count = 0;
    s->queue_pos = 0;
    s->current_card = 0;
    s->card_open = false;
    s->n_graded = 0;
    s->n_again = 0;
    s->n_hard = 0;
    s->n_good = 0;
    s->n_easy = 0;
    s->db = db;
    s->fsrs = fsrs;
    s->today_day = today_day;
    snprintf(s->deck, sizeof s->deck, "%s", deck);
    snprintf(s->started_at, sizeof s->started_at, "%s", now_iso);
    int rc = ct_queue_load(s, deck, now_iso, limit);
    if (rc != CT_SESSION_OK) {
        free(s->queue);
        s->queue = NULL;
        return rc;
    }
    return s->queue_count;
}

int ct_session_current(ct_session *s, ct_card *out) {
    if (s == NULL || out == NULL || s->db == NULL)
        return CT_SESSION_ERR_ARG;
    if (s->queue == NULL || s->queue_pos >= s->queue_count)
        return CT_SESSION_ERR_STATE;
    ct_card card;
    int rc = ct_repo_card_get(s->db, s->queue[s->queue_pos], &card);
    if (rc != CT_REPO_OK)
        return rc == CT_REPO_MISSING ? CT_SESSION_ERR_STATE : CT_SESSION_ERR_DB;
    *out = card;
    s->current_card = card.id;
    s->card_open = true;
    return CT_SESSION_OK;
}

bool ct_session_has_next(const ct_session *s) {
    return s != NULL && s->queue != NULL && s->queue_pos < s->queue_count;
}

int ct_session_grade(ct_session *s, ct_grade g, const char *now_iso) {
    if (s == NULL || s->db == NULL || s->fsrs == NULL)
        return CT_SESSION_ERR_ARG;
    if (g < CT_AGAIN || g > CT_EASY)
        return CT_SESSION_ERR_ARG;
    long vy;
    int vm, vd;
    if (ct_iso_scan(now_iso, &vy, &vm, &vd) != CT_SESSION_OK)
        return CT_SESSION_ERR_ARG;
    if (s->queue == NULL || s->queue_pos >= s->queue_count)
        return CT_SESSION_ERR_STATE;
    long id = s->queue[s->queue_pos];
    ct_card card;
    int rc = ct_repo_card_get(s->db, id, &card);
    if (rc != CT_REPO_OK)
        return rc == CT_REPO_MISSING ? CT_SESSION_ERR_STATE : CT_SESSION_ERR_DB;

    ct_srs_row row;
    rc = ct_repo_srs_get(s->db, id, &row);
    if (rc != CT_REPO_OK && rc != CT_REPO_MISSING)
        return CT_SESSION_ERR_DB;

    double pre_s = 0.0;
    double pre_d = 0.0;
    double elapsed = 0.0;
    ct_fsrs_card fc = {0};
    if (rc == CT_REPO_OK) {
        pre_s = row.stability;
        pre_d = row.difficulty;
        fc.stability = row.stability;
        fc.difficulty = row.difficulty;
        fc.reps = row.reps;
        fc.lapses = row.lapses;
        fc.state = row.state;
        if (row.last_review[0] != '\0') {
            long ly;
            int lm, ld;
            if (ct_iso_scan(row.last_review, &ly, &lm, &ld) != CT_SESSION_OK) {
                return CT_SESSION_ERR_ARG;
            }
            fc.last_review_day = ct_session_date_to_day(ly, lm, ld);
            elapsed = (double)(s->today_day - fc.last_review_day);
            if (elapsed < 0.0) {
                elapsed = 0.0;
            }
        }
    }
    ct_fsrs_card out = ct_fsrs_review(s->fsrs, &fc, g, elapsed, s->today_day);
    long ivl = out.due_day - s->today_day;
    if (ivl < 0)
        ivl = 0;

    ct_srs_row next;
    next.stability = out.stability;
    next.difficulty = out.difficulty;
    next.reps = out.reps;
    next.lapses = out.lapses;
    next.state = out.state;
    if (ct_session_iso_add_days(now_iso, ivl, next.due, (unsigned)sizeof next.due) !=
        CT_SESSION_OK) {
        return CT_SESSION_ERR_ARG;
    }
    snprintf(next.last_review, sizeof next.last_review, "%s", now_iso);
    if (sqlite3_exec(s->db, "BEGIN IMMEDIATE", NULL, NULL, NULL) != SQLITE_OK)
        return CT_SESSION_ERR_DB;
    rc = ct_repo_srs_put(s->db, id, &next);
    if (rc == CT_REPO_OK)
        rc = ct_repo_review_log(s->db, id, (int)g, now_iso, pre_s, pre_d, elapsed, (double)ivl);
    if (rc != CT_REPO_OK) {
        sqlite3_exec(s->db, "ROLLBACK", NULL, NULL, NULL);
        return CT_SESSION_ERR_DB;
    }
    if (sqlite3_exec(s->db, "COMMIT", NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_exec(s->db, "ROLLBACK", NULL, NULL, NULL);
        return CT_SESSION_ERR_DB;
    }

    s->n_graded++;
    if (g == CT_AGAIN) {
        s->n_again++;
    } else if (g == CT_HARD) {
        s->n_hard++;
    } else if (g == CT_GOOD) {
        s->n_good++;
    } else {
        s->n_easy++;
    }
    s->queue_pos++;
    s->current_card = 0;
    s->card_open = false;
    return CT_SESSION_OK;
}

int ct_session_end(ct_session *s, const char *now_iso) {
    if (s == NULL || s->db == NULL)
        return CT_SESSION_ERR_ARG;
    long y;
    int m, d;
    if (ct_iso_scan(now_iso, &y, &m, &d) != CT_SESSION_OK)
        return CT_SESSION_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(s->db, CT_SQL_SESSION_INSERT, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_SESSION_ERR_DB;
    }
    int ix = 0;
    sqlite3_bind_text(st, ++ix, s->deck, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, ++ix, s->started_at, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, ++ix, now_iso, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, ++ix, s->n_graded);
    sqlite3_bind_int(st, ++ix, s->n_again);
    sqlite3_bind_int(st, ++ix, s->n_hard);
    sqlite3_bind_int(st, ++ix, s->n_good);
    sqlite3_bind_int(st, ++ix, s->n_easy);
    int step = sqlite3_step(st);
    sqlite3_finalize(st);
    return step == SQLITE_DONE ? CT_SESSION_OK : CT_SESSION_ERR_DB;
}

void ct_session_free(ct_session *s) {
    if (s == NULL)
        return;
    free(s->queue);
    s->queue = NULL;
    s->queue_count = 0;
    s->queue_pos = 0;
    s->current_card = 0;
    s->card_open = false;
}
