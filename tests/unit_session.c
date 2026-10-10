#include <limits.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "framework.h"
#include "srs/fsrs.h"
#include "storage/db.h"
#include "storage/repo_cards.h"
#include "storage/repo_reviews.h"
#include "trainer/session.h"

static const char now_stamp[] = "2026-10-09T00:00:00Z";
static const long today_num = 20735;
static const char last4_stamp[] = "2026-10-05T00:00:00Z";

static sqlite3 *session_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static ct_fsrs session_fsrs(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    return f;
}

static long seed_card(sqlite3 *db, const char *deck, const char *key) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", key);
    c.direction = CT_DIR_FR;
    snprintf(c.front_json, sizeof c.front_json, "%s", "{\"scramble\":\"R U R'\"}");
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    return c.id;
}

static void seed_learning(sqlite3 *db, long id, const char *due) {
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    row.stability = 10.0;
    row.difficulty = 5.0;
    snprintf(row.due, sizeof row.due, "%s", due);
    snprintf(row.last_review, sizeof row.last_review, "%s", last4_stamp);
    row.reps = 1;
    row.lapses = 0;
    row.state = 1;
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);
}

static void seed_new(sqlite3 *db, long id) {
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    snprintf(row.due, sizeof row.due, "%s", "2026-10-01T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);
}

static bool read_last_session(sqlite3 *db, char *deck, char *started, char *ended, long *counts) {
    static const char sql[] =
        "SELECT deck, started_at, ended_at, n_cards, n_again, n_hard, n_good, n_easy"
        " FROM session_log ORDER BY id DESC LIMIT 1";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return false;
    }
    if (sqlite3_step(st) != SQLITE_ROW) {
        sqlite3_finalize(st);
        return false;
    }
    snprintf(deck, CT_SESSION_DECK_MAX, "%s", (const char *)sqlite3_column_text(st, 0));
    snprintf(started, CT_SESSION_STAMP_MAX, "%s", (const char *)sqlite3_column_text(st, 1));
    snprintf(ended, CT_SESSION_STAMP_MAX, "%s", (const char *)sqlite3_column_text(st, 2));
    for (int i = 0; i < 5; i++) {
        counts[i] = (long)sqlite3_column_int64(st, 3 + i);
    }
    sqlite3_finalize(st);
    return true;
}

CT_SUITE(session_day_numbers) {
    CT_ASSERT_EQ(ct_session_date_to_day(1970, 1, 1), 0);
    CT_ASSERT_EQ(ct_session_date_to_day(1969, 12, 31), -1);
    CT_ASSERT_EQ(ct_session_date_to_day(2000, 1, 1), 10957);
    CT_ASSERT_EQ(ct_session_date_to_day(2024, 1, 1), 19723);
    CT_ASSERT_EQ(ct_session_date_to_day(2024, 2, 29), 19782);
    CT_ASSERT_EQ(ct_session_date_to_day(2026, 10, 9), 20735);

    static const long gold_day[6] = {0, -1, 10957, 19723, 19782, 20735};
    static const long gold_y[6] = {1970, 1969, 2000, 2024, 2024, 2026};
    static const int gold_m[6] = {1, 12, 1, 1, 2, 10};
    static const int gold_d[6] = {1, 31, 1, 1, 29, 9};
    for (int i = 0; i < 6; i++) {
        long y = 0;
        int m = 0;
        int d = 0;
        ct_session_day_to_date(gold_day[i], &y, &m, &d);
        CT_ASSERT_EQ(y, gold_y[i]);
        CT_ASSERT_EQ(m, gold_m[i]);
        CT_ASSERT_EQ(d, gold_d[i]);
        CT_ASSERT_EQ(ct_session_date_to_day(y, m, d), gold_day[i]);
    }

    bool round_trip_ok = true;
    for (long day = -400; day < 25000; day += 37) {
        long y = 0;
        int m = 0;
        int d = 0;
        ct_session_day_to_date(day, &y, &m, &d);
        if (ct_session_date_to_day(y, m, d) != day)
            round_trip_ok = false;
        if (m < 1 || m > 12)
            round_trip_ok = false;
        if (d < 1 || d > 31)
            round_trip_ok = false;
    }
    CT_ASSERT(round_trip_ok);
}

CT_SUITE(session_iso_add_days) {
    static const char *input[] = {"2026-02-28", "2027-02-28", "2028-02-28", "2028-02-28",
                                  "2026-12-31", "2026-01-01", "2028-01-01", "1900-02-28",
                                  "2000-02-28", "2100-02-28", "2026-03-01"};
    static const long add[] = {1, 1, 1, 2, 1, 365, 366, 1, 1, 1, -1};
    static const char *want[] = {"2026-03-01", "2027-03-01", "2028-02-29", "2028-03-01",
                                 "2027-01-01", "2027-01-01", "2029-01-01", "1900-03-01",
                                 "2000-02-29", "2100-03-01", "2026-02-28"};
    char out[CT_SESSION_STAMP_MAX];
    for (int i = 0; i < 11; i++) {
        CT_ASSERT_EQ(ct_session_iso_add_days(input[i], add[i], out, (unsigned)sizeof out),
                     CT_SESSION_OK);
        CT_ASSERT_STR_EQ(out, want[i]);
    }

    CT_ASSERT_EQ(ct_session_iso_add_days(now_stamp, 20, out, (unsigned)sizeof out), CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "2026-10-29T00:00:00Z");
    CT_ASSERT_EQ(ct_session_iso_add_days("2026-10-09T12:34:56Z", 20, out, (unsigned)sizeof out),
                 CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "2026-10-29T12:34:56Z");
    CT_ASSERT_EQ(ct_session_iso_add_days(now_stamp, 0, out, (unsigned)sizeof out), CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, now_stamp);
    CT_ASSERT_EQ(ct_session_iso_add_days("2026-02-28", 1, out, 11), CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "2026-03-01");
    CT_ASSERT(ct_session_iso_add_days("2026-02-28", 1, out, 10) < 0);
    CT_ASSERT_EQ(ct_session_iso_add_days(now_stamp, 20, out, 21), CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "2026-10-29T00:00:00Z");
    CT_ASSERT(ct_session_iso_add_days(now_stamp, 20, out, 20) < 0);

    CT_ASSERT(ct_session_iso_add_days(NULL, 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("202a-02-28", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026/02/28", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-2-28", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-13-01", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-00-10", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-01-32", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-02-30", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-02-29", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-01-01", 1, NULL, (unsigned)sizeof out) < 0);
}

CT_SUITE(session_iso_add_days_domain) {
    char out[CT_SESSION_STAMP_MAX];
    CT_ASSERT(ct_session_iso_add_days("2026-10-09", LONG_MAX, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-10-09", LONG_MIN, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("9999-12-31", LONG_MAX, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("0001-01-01", LONG_MIN, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("0001-01-01", LONG_MAX, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("9999-12-31", LONG_MIN, out, (unsigned)sizeof out) < 0);

    long min_day = ct_session_date_to_day(1, 1, 1);
    long max_day = ct_session_date_to_day(9999, 12, 31);
    CT_ASSERT_EQ(min_day, -719162);
    CT_ASSERT_EQ(max_day, 2932896);
    long y = 0;
    int m = 0;
    int d = 0;
    ct_session_day_to_date(max_day, &y, &m, &d);
    CT_ASSERT_EQ(y, 9999);
    CT_ASSERT_EQ(m, 12);
    CT_ASSERT_EQ(d, 31);
    ct_session_day_to_date(min_day, &y, &m, &d);
    CT_ASSERT_EQ(y, 1);
    CT_ASSERT_EQ(m, 1);
    CT_ASSERT_EQ(d, 1);

    long span_hi = max_day - 20735;
    long span_lo = min_day - 20735;
    CT_ASSERT_EQ(ct_session_iso_add_days("2026-10-09", span_hi, out, (unsigned)sizeof out),
                 CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "9999-12-31");
    CT_ASSERT(ct_session_iso_add_days("2026-10-09", span_hi + 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT_EQ(ct_session_iso_add_days("2026-10-09", span_lo, out, (unsigned)sizeof out),
                 CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "0001-01-01");
    CT_ASSERT(ct_session_iso_add_days("2026-10-09", span_lo - 1, out, (unsigned)sizeof out) < 0);
}

CT_SUITE(session_stamp_boundaries) {
    char out[CT_SESSION_STAMP_MAX];
    CT_ASSERT(ct_session_iso_add_days("2026-01-01XYZ9876", 5, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-01-01x00:00:00Z", 5, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-01-01 00:00:00Z", 5, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("2026-01-01T00:00:00Z", 5, out, (unsigned)sizeof out) ==
              CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "2026-01-06T00:00:00Z");
    CT_ASSERT(ct_session_iso_add_days("2026-10-09", 0, out, (unsigned)sizeof out) == CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "2026-10-09");

    CT_ASSERT(ct_session_iso_add_days("9999-12-31", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("9999-12-31T23:59:59Z", 1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("9999-12-31", 0, out, (unsigned)sizeof out) == CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "9999-12-31");
    CT_ASSERT(ct_session_iso_add_days("9998-12-31", 365, out, (unsigned)sizeof out) ==
              CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "9999-12-31");
    CT_ASSERT(ct_session_iso_add_days("0001-01-01", -1, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("0001-01-01", -366, out, (unsigned)sizeof out) < 0);
    CT_ASSERT(ct_session_iso_add_days("0001-01-01", 0, out, (unsigned)sizeof out) == CT_SESSION_OK);
    CT_ASSERT_STR_EQ(out, "0001-01-01");

    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT(ct_session_begin(&s, db, &f, "d", "10000-01-01T00:00:00Z", 0, 0) != CT_SESSION_OK);
    CT_ASSERT(ct_session_begin(&s, db, &f, "d", "2026-01-01XYZ9876", 0, 0) != CT_SESSION_OK);
    ct_db_close(db);
}

CT_SUITE(session_stale_last_review) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long id = seed_card(db, "review", "a");
    seed_learning(db, id, "2026-10-01T00:00:00Z");
    ct_srs_row ahead;
    memset(&ahead, 0, sizeof ahead);
    ahead.stability = 10.0;
    ahead.difficulty = 5.0;
    snprintf(ahead.due, sizeof ahead.due, "%s", "2026-10-01T00:00:00Z");
    snprintf(ahead.last_review, sizeof ahead.last_review, "%s", "2026-12-01T00:00:00Z");
    ahead.reps = 1;
    ahead.state = 1;
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &ahead), CT_REPO_OK);

    ct_session s;
    CT_ASSERT(ct_session_begin(&s, db, &f, "review", now_stamp, today_num, 0) > 0);
    CT_ASSERT_EQ(ct_session_current(&s, &(ct_card){0}), CT_SESSION_OK);
    CT_ASSERT_EQ(ct_session_grade(&s, CT_GOOD, now_stamp), CT_SESSION_OK);

    ct_srs_row row;
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &row), CT_REPO_OK);
    CT_ASSERT_STR_EQ(row.last_review, now_stamp);
    CT_ASSERT(row.stability >= CT_FSRS_MIN_STABILITY);

    ct_review_row logs[4];
    int n = ct_repo_reviews_for_card(db, id, logs, 4);
    CT_ASSERT_EQ(n, 1);
    CT_ASSERT_DBL_EQ(logs[0].elapsed_days, 0.0, 0.0);
    ct_session_end(&s, now_stamp);
    ct_db_close(db);
}

CT_SUITE(session_flow_golden) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long a = seed_card(db, "review", "a");
    long b = seed_card(db, "review", "b");
    long c = seed_card(db, "review", "c");
    seed_learning(db, a, "2026-10-01T00:00:00Z");
    seed_learning(db, b, "2026-10-02T00:00:00Z");
    seed_learning(db, c, "2026-10-03T00:00:00Z");

    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "review", now_stamp, today_num, 0), 3);
    CT_ASSERT_STR_EQ(s.deck, "review");
    CT_ASSERT_STR_EQ(s.started_at, now_stamp);
    CT_ASSERT_EQ(s.today_day, today_num);
    CT_ASSERT_EQ(s.queue_count, 3);
    CT_ASSERT_EQ(s.queue_pos, 0);
    CT_ASSERT(s.queue != NULL);
    CT_ASSERT(ct_session_has_next(&s));

    ct_card cur;
    memset(&cur, 0, sizeof cur);
    CT_ASSERT_EQ(ct_session_current(&s, &cur), CT_SESSION_OK);
    CT_ASSERT_EQ(cur.id, a);
    CT_ASSERT_STR_EQ(cur.deck, "review");
    CT_ASSERT_STR_EQ(cur.case_key, "a");
    CT_ASSERT(s.card_open);
    CT_ASSERT_EQ(s.current_card, a);
    memset(&cur, 0, sizeof cur);
    CT_ASSERT_EQ(ct_session_current(&s, &cur), CT_SESSION_OK);
    CT_ASSERT_EQ(cur.id, a);
    CT_ASSERT_EQ(s.queue_pos, 0);

    CT_ASSERT_EQ(ct_session_grade(&s, CT_GOOD, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(s.n_graded, 1);
    CT_ASSERT_EQ(s.n_good, 1);
    CT_ASSERT_EQ(s.n_again, 0);
    CT_ASSERT_EQ(s.n_hard, 0);
    CT_ASSERT_EQ(s.n_easy, 0);
    CT_ASSERT(!s.card_open);
    CT_ASSERT_EQ(s.queue_pos, 1);

    ct_srs_row row;
    memset(&row, 0, sizeof row);
    CT_ASSERT_EQ(ct_repo_srs_get(db, a, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 19.791582142057077, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 4.991832707311108, 1e-9);
    CT_ASSERT_STR_EQ(row.due, "2026-10-29T00:00:00Z");
    CT_ASSERT_STR_EQ(row.last_review, now_stamp);
    CT_ASSERT_EQ(row.reps, 2);
    CT_ASSERT_EQ(row.lapses, 0);
    CT_ASSERT_EQ(row.state, 2);

    ct_review_row revs[4];
    memset(revs, 0, sizeof revs);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, a, revs, 4), 1);
    CT_ASSERT_EQ(revs[0].card_id, a);
    CT_ASSERT_EQ(revs[0].rating, CT_GOOD);
    CT_ASSERT_STR_EQ(revs[0].reviewed_at, now_stamp);
    CT_ASSERT_DBL_EQ(revs[0].stability, 10.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].difficulty, 5.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].elapsed_days, 4.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].scheduled_days, 20.0, 1e-9);

    memset(&cur, 0, sizeof cur);
    CT_ASSERT_EQ(ct_session_current(&s, &cur), CT_SESSION_OK);
    CT_ASSERT_EQ(cur.id, b);
    CT_ASSERT_EQ(ct_session_grade(&s, CT_HARD, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(ct_repo_srs_get(db, b, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 11.962060841460307, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 5.7994339073111085, 1e-9);
    CT_ASSERT_STR_EQ(row.due, now_stamp);
    CT_ASSERT_EQ(row.state, 1);
    CT_ASSERT_EQ(s.n_hard, 1);

    memset(&cur, 0, sizeof cur);
    CT_ASSERT_EQ(ct_session_current(&s, &cur), CT_SESSION_OK);
    CT_ASSERT_EQ(cur.id, c);
    CT_ASSERT_EQ(ct_session_grade(&s, CT_EASY, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(ct_repo_srs_get(db, c, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 43.20991950624265, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 4.184231507311108, 1e-9);
    CT_ASSERT_STR_EQ(row.due, "2026-11-21T00:00:00Z");
    CT_ASSERT_EQ(row.state, 2);
    CT_ASSERT_EQ(s.n_easy, 1);
    CT_ASSERT_EQ(s.n_graded, 3);
    CT_ASSERT(!ct_session_has_next(&s));

    CT_ASSERT(ct_session_grade(&s, CT_GOOD, now_stamp) < 0);
    CT_ASSERT_EQ(s.n_graded, 3);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, a, revs, 4), 1);
    CT_ASSERT(ct_session_current(&s, &cur) < 0);

    CT_ASSERT_EQ(ct_session_end(&s, now_stamp), CT_SESSION_OK);
    char deck[CT_SESSION_DECK_MAX];
    char started[CT_SESSION_STAMP_MAX];
    char ended[CT_SESSION_STAMP_MAX];
    long counts[5];
    CT_ASSERT(read_last_session(db, deck, started, ended, counts));
    CT_ASSERT_STR_EQ(deck, "review");
    CT_ASSERT_STR_EQ(started, now_stamp);
    CT_ASSERT_STR_EQ(ended, now_stamp);
    CT_ASSERT_EQ(counts[0], 3);
    CT_ASSERT_EQ(counts[1], 0);
    CT_ASSERT_EQ(counts[2], 1);
    CT_ASSERT_EQ(counts[3], 1);
    CT_ASSERT_EQ(counts[4], 1);

    ct_session_free(&s);
    CT_ASSERT(s.queue == NULL);
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_new_card) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long good_id = seed_card(db, "fresh", "n1");
    long easy_id = seed_card(db, "fresh", "n2");
    seed_new(db, good_id);
    seed_new(db, easy_id);

    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "fresh", now_stamp, today_num, 0), 2);

    ct_card cur;
    memset(&cur, 0, sizeof cur);
    CT_ASSERT_EQ(ct_session_current(&s, &cur), CT_SESSION_OK);
    CT_ASSERT_EQ(cur.id, good_id);
    CT_ASSERT_EQ(ct_session_grade(&s, CT_GOOD, now_stamp), CT_SESSION_OK);

    ct_srs_row row;
    memset(&row, 0, sizeof row);
    CT_ASSERT_EQ(ct_repo_srs_get(db, good_id, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 3.173, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 5.282434422319005, 1e-9);
    CT_ASSERT_STR_EQ(row.due, now_stamp);
    CT_ASSERT_STR_EQ(row.last_review, now_stamp);
    CT_ASSERT_EQ(row.reps, 1);
    CT_ASSERT_EQ(row.lapses, 0);
    CT_ASSERT_EQ(row.state, 1);

    ct_review_row revs[2];
    memset(revs, 0, sizeof revs);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, good_id, revs, 2), 1);
    CT_ASSERT_EQ(revs[0].rating, CT_GOOD);
    CT_ASSERT_DBL_EQ(revs[0].stability, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].difficulty, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].elapsed_days, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].scheduled_days, 0.0, 1e-9);

    CT_ASSERT_EQ(ct_session_grade(&s, CT_EASY, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(ct_repo_srs_get(db, easy_id, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 15.69105, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 3.2245015893713678, 1e-9);
    CT_ASSERT_STR_EQ(row.due, now_stamp);
    CT_ASSERT_EQ(row.reps, 1);
    CT_ASSERT_EQ(row.lapses, 0);
    CT_ASSERT_EQ(row.state, 1);
    CT_ASSERT_EQ(s.n_graded, 2);
    CT_ASSERT_EQ(s.n_good, 1);
    CT_ASSERT_EQ(s.n_easy, 1);
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_absent_row) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long id = seed_card(db, "orphan", "x");

    ct_session s;
    memset(&s, 0, sizeof s);
    s.db = db;
    s.fsrs = &f;
    s.queue = &id;
    s.queue_count = 1;
    s.queue_pos = 0;
    s.today_day = today_num;
    snprintf(s.started_at, sizeof s.started_at, "%s", now_stamp);

    ct_srs_row row;
    memset(&row, 0xAA, sizeof row);
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &row), CT_REPO_MISSING);

    CT_ASSERT_EQ(ct_session_grade(&s, CT_GOOD, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 3.173, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 5.282434422319005, 1e-9);
    CT_ASSERT_STR_EQ(row.last_review, now_stamp);
    CT_ASSERT_EQ(row.state, 1);

    ct_review_row revs[2];
    memset(revs, 0, sizeof revs);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, id, revs, 2), 1);
    CT_ASSERT_DBL_EQ(revs[0].elapsed_days, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].scheduled_days, 0.0, 1e-9);
    CT_ASSERT_EQ(s.n_graded, 1);
    s.queue = NULL;
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_lapse) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long id = seed_card(db, "lapse", "l1");
    seed_learning(db, id, "2026-10-01T00:00:00Z");

    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "lapse", now_stamp, today_num, 0), 1);
    CT_ASSERT_EQ(ct_session_grade(&s, CT_AGAIN, now_stamp), CT_SESSION_OK);
    CT_ASSERT_EQ(s.n_again, 1);
    CT_ASSERT_EQ(s.n_good, 0);
    CT_ASSERT_EQ(s.n_graded, 1);

    ct_srs_row row;
    memset(&row, 0, sizeof row);
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 1.7994687509165919, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 6.607035107311108, 1e-9);
    CT_ASSERT_STR_EQ(row.due, now_stamp);
    CT_ASSERT_EQ(row.state, 1);
    CT_ASSERT_EQ(row.lapses, 0);
    CT_ASSERT_EQ(row.reps, 2);

    ct_review_row revs[2];
    memset(revs, 0, sizeof revs);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, id, revs, 2), 1);
    CT_ASSERT_EQ(revs[0].rating, CT_AGAIN);
    CT_ASSERT_DBL_EQ(revs[0].elapsed_days, 4.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].scheduled_days, 0.0, 1e-9);
    CT_ASSERT(!ct_session_has_next(&s));
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_same_day) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long id = seed_card(db, "sameday", "s1");
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    row.stability = 10.0;
    row.difficulty = 5.0;
    snprintf(row.due, sizeof row.due, "%s", now_stamp);
    snprintf(row.last_review, sizeof row.last_review, "%s", now_stamp);
    row.reps = 2;
    row.lapses = 0;
    row.state = 1;
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);

    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "sameday", now_stamp, today_num, 0), 1);
    CT_ASSERT_EQ(ct_session_grade(&s, CT_GOOD, now_stamp), CT_SESSION_OK);

    memset(&row, 0, sizeof row);
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &row), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(row.stability, 14.077712147375411, 1e-9);
    CT_ASSERT_DBL_EQ(row.difficulty, 5.0, 1e-9);
    CT_ASSERT_STR_EQ(row.due, now_stamp);
    CT_ASSERT_EQ(row.reps, 3);
    CT_ASSERT_EQ(row.lapses, 0);
    CT_ASSERT_EQ(row.state, 1);

    ct_review_row revs[2];
    memset(revs, 0, sizeof revs);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, id, revs, 2), 1);
    CT_ASSERT_DBL_EQ(revs[0].elapsed_days, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].scheduled_days, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].stability, 10.0, 1e-9);
    CT_ASSERT_DBL_EQ(revs[0].difficulty, 5.0, 1e-9);
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_empty_deck) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    seed_card(db, "other", "x");
    long id = seed_card(db, "later", "y");
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    row.stability = 10.0;
    row.difficulty = 5.0;
    snprintf(row.due, sizeof row.due, "%s", "2026-12-01T00:00:00Z");
    snprintf(row.last_review, sizeof row.last_review, "%s", now_stamp);
    row.state = 1;
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);

    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "empty", now_stamp, today_num, 0), 0);
    CT_ASSERT(!ct_session_has_next(&s));
    ct_card cur;
    CT_ASSERT(ct_session_current(&s, &cur) < 0);
    CT_ASSERT(ct_session_grade(&s, CT_GOOD, now_stamp) < 0);
    CT_ASSERT_EQ(s.n_graded, 0);
    CT_ASSERT_EQ(ct_session_end(&s, now_stamp), CT_SESSION_OK);

    char deck[CT_SESSION_DECK_MAX];
    char started[CT_SESSION_STAMP_MAX];
    char ended[CT_SESSION_STAMP_MAX];
    long counts[5];
    CT_ASSERT(read_last_session(db, deck, started, ended, counts));
    CT_ASSERT_STR_EQ(deck, "empty");
    CT_ASSERT_STR_EQ(started, now_stamp);
    CT_ASSERT_STR_EQ(ended, now_stamp);
    for (int i = 0; i < 5; i++) {
        CT_ASSERT_EQ(counts[i], 0);
    }
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_limits) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long a = seed_card(db, "lim", "a");
    long b = seed_card(db, "lim", "b");
    long c = seed_card(db, "lim", "c");
    seed_learning(db, a, "2026-10-01T00:00:00Z");
    seed_learning(db, b, "2026-10-02T00:00:00Z");
    seed_learning(db, c, "2026-10-03T00:00:00Z");

    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "lim", now_stamp, today_num, 2), 2);
    CT_ASSERT_EQ(s.queue_count, 2);
    CT_ASSERT_EQ(s.queue[0], a);
    CT_ASSERT_EQ(s.queue[1], b);
    CT_ASSERT_EQ(ct_session_grade(&s, CT_GOOD, now_stamp), CT_SESSION_OK);
    CT_ASSERT(ct_session_has_next(&s));

    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "lim", now_stamp, today_num, 1), 1);
    CT_ASSERT_EQ(s.queue_count, 1);
    CT_ASSERT_EQ(s.queue[0], b);
    CT_ASSERT_EQ(s.n_graded, 0);

    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "lim", now_stamp, today_num, 0), 2);
    CT_ASSERT_EQ(s.queue_count, 2);
    CT_ASSERT_EQ(s.queue[0], b);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "lim", now_stamp, today_num, -7), 2);
    CT_ASSERT_EQ(s.queue_count, 2);
    CT_ASSERT_EQ(s.queue[1], c);
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_card_missing) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long a = seed_card(db, "gone", "a");
    seed_learning(db, a, "2026-10-01T00:00:00Z");

    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "gone", now_stamp, today_num, 0), 1);
    CT_ASSERT_EQ(ct_repo_card_delete(db, a), CT_REPO_OK);
    ct_card cur;
    CT_ASSERT(ct_session_current(&s, &cur) < 0);
    CT_ASSERT(ct_session_grade(&s, CT_GOOD, now_stamp) < 0);
    CT_ASSERT_EQ(s.n_graded, 0);
    CT_ASSERT_EQ(s.queue_pos, 0);
    ct_review_row revs[2];
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, a, revs, 2), 0);
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_bad_args) {
    sqlite3 *db = session_db();
    ct_fsrs f = session_fsrs();
    long id = seed_card(db, "args", "a");
    seed_learning(db, id, "2026-10-01T00:00:00Z");

    CT_ASSERT(ct_session_begin(NULL, db, &f, "args", now_stamp, today_num, 0) < 0);
    ct_session s;
    memset(&s, 0, sizeof s);
    CT_ASSERT(ct_session_begin(&s, NULL, &f, "args", now_stamp, today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, NULL, "args", now_stamp, today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, &f, NULL, now_stamp, today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, &f, "", now_stamp, today_num, 0) < 0);
    char big_deck[CT_SESSION_DECK_MAX + 1];
    memset(big_deck, 'd', sizeof big_deck - 1);
    big_deck[sizeof big_deck - 1] = '\0';
    CT_ASSERT(ct_session_begin(&s, db, &f, big_deck, now_stamp, today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, &f, "args", NULL, today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, &f, "args", "", today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, &f, "args", "2026", today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, &f, "args", "2026-13-01T00:00:00Z", today_num, 0) < 0);
    CT_ASSERT(ct_session_begin(&s, db, &f, "args", "2026-02-30T00:00:00Z", today_num, 0) < 0);

    CT_ASSERT(!ct_session_has_next(NULL));
    CT_ASSERT(ct_session_current(NULL, NULL) < 0);
    ct_session zero;
    memset(&zero, 0, sizeof zero);
    ct_card cur;
    CT_ASSERT(ct_session_current(&zero, &cur) < 0);
    CT_ASSERT(ct_session_current(&zero, NULL) < 0);
    CT_ASSERT(ct_session_grade(&zero, CT_GOOD, now_stamp) < 0);
    CT_ASSERT(ct_session_end(&zero, now_stamp) < 0);
    CT_ASSERT(ct_session_end(&zero, NULL) < 0);
    CT_ASSERT(ct_session_end(NULL, now_stamp) < 0);

    memset(&s, 0, sizeof s);
    CT_ASSERT_EQ(ct_session_begin(&s, db, &f, "args", now_stamp, today_num, 0), 1);
    CT_ASSERT(ct_session_grade(&s, (ct_grade)0, now_stamp) < 0);
    CT_ASSERT(ct_session_grade(&s, (ct_grade)5, now_stamp) < 0);
    CT_ASSERT(ct_session_grade(&s, CT_GOOD, NULL) < 0);
    CT_ASSERT(ct_session_grade(&s, CT_GOOD, "2026") < 0);
    CT_ASSERT_EQ(s.n_graded, 0);
    CT_ASSERT_EQ(s.queue_pos, 0);
    ct_review_row revs[2];
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, id, revs, 2), 0);
    CT_ASSERT_EQ(ct_session_end(&s, NULL) < 0, true);
    ct_session_free(&s);
    ct_db_close(db);
}

CT_SUITE(session_free_safety) {
    ct_session s;
    memset(&s, 0, sizeof s);
    ct_session_free(&s);
    ct_session_free(&s);
    CT_ASSERT(s.queue == NULL);
    CT_ASSERT_EQ(s.queue_count, 0);
    CT_ASSERT_EQ(s.queue_pos, 0);
    CT_ASSERT(!s.card_open);
    ct_session_free(NULL);
}
