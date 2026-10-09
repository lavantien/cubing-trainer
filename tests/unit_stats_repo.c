#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/storage/repo_reviews.h"
#include "../src/storage/repo_stats.h"
#include "framework.h"

static sqlite3 *stats_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static long add_card(sqlite3 *db, const char *deck, const char *case_key, int direction) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", case_key);
    c.direction = direction;
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    CT_ASSERT(c.id > 0);
    return c.id;
}

static void put_srs(sqlite3 *db, long id, double stability, double difficulty, const char *due,
                    int state) {
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    row.stability = stability;
    row.difficulty = difficulty;
    snprintf(row.due, sizeof row.due, "%s", due);
    snprintf(row.last_review, sizeof row.last_review, "%s", "2026-05-01T00:00:00Z");
    row.reps = 1;
    row.state = state;
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);
}

static void log_rating(sqlite3 *db, long card_id, int rating) {
    CT_ASSERT_EQ(
        ct_repo_review_log(db, card_id, rating, "2026-05-01T00:00:00Z", 1.0, 5.0, 0.0, 1.0),
        CT_REPO_OK);
}

static ct_session_row session_at(const char *deck, const char *started, const char *ended,
                                 int cards, int again, int hard, int good, int easy) {
    ct_session_row s;
    memset(&s, 0, sizeof s);
    snprintf(s.deck, sizeof s.deck, "%s", deck);
    snprintf(s.started_at, sizeof s.started_at, "%s", started);
    snprintf(s.ended_at, sizeof s.ended_at, "%s", ended);
    s.n_cards = cards;
    s.n_again = again;
    s.n_hard = hard;
    s.n_good = good;
    s.n_easy = easy;
    return s;
}

CT_SUITE(stats_empty) {
    sqlite3 *db = stats_db();
    ct_deck_stats decks[4];
    ct_due_forecast_row fc[6];
    ct_case_accuracy_row acc[4];
    ct_session_row sess[4];

    CT_ASSERT_EQ(ct_repo_deck_stats(db, "2026-06-01T00:00:00Z", decks, 4), 0);
    CT_ASSERT_EQ(ct_repo_due_forecast(db, "2026-06-01", 3, fc, 6), 3);
    CT_ASSERT_STR_EQ(fc[0].day, "2026-06-01");
    CT_ASSERT_STR_EQ(fc[1].day, "2026-06-02");
    CT_ASSERT_STR_EQ(fc[2].day, "2026-06-03");
    CT_ASSERT_EQ(fc[0].due_count, 0);
    CT_ASSERT_EQ(fc[1].due_count, 0);
    CT_ASSERT_EQ(fc[2].due_count, 0);
    CT_ASSERT_EQ(ct_repo_case_accuracy(db, "none", acc, 4), 0);
    CT_ASSERT_EQ(ct_repo_sessions(db, sess, 4), 0);
    ct_db_close(db);
}

CT_SUITE(stats_deck_aggregates) {
    sqlite3 *db = stats_db();
    long a1 = add_card(db, "alpha", "a1", CT_DIR_FR);
    long a2 = add_card(db, "alpha", "a2", CT_DIR_FR);
    long a3 = add_card(db, "alpha", "a3", CT_DIR_FR);
    add_card(db, "alpha", "a4", CT_DIR_FR);
    long b1 = add_card(db, "beta", "b1", CT_DIR_FR);
    add_card(db, "gamma", "g1", CT_DIR_FR);

    put_srs(db, a1, 10.0, 5.0, "2026-05-30T12:00:00Z", 1);
    put_srs(db, a2, 30.0, 7.0, "2026-06-01T00:00:00Z", 2);
    put_srs(db, a3, 1.0, 8.0, "2026-05-01T00:00:00Z", 0);
    put_srs(db, b1, 2.5, 6.5, "2026-07-01T00:00:00Z", 1);

    ct_deck_stats decks[4];
    memset(decks, 0, sizeof decks);
    CT_ASSERT_EQ(ct_repo_deck_stats(db, "2026-06-01T00:00:00Z", decks, 4), 3);
    CT_ASSERT_STR_EQ(decks[0].deck, "alpha");
    CT_ASSERT_EQ(decks[0].total_cards, 4);
    CT_ASSERT_EQ(decks[0].due_now, 3);
    CT_ASSERT_EQ(decks[0].learning, 1);
    CT_ASSERT_EQ(decks[0].review_state, 1);
    CT_ASSERT_DBL_EQ(decks[0].avg_stability, 41.0 / 3.0, 1e-9);
    CT_ASSERT_DBL_EQ(decks[0].avg_difficulty, 20.0 / 3.0, 1e-9);
    CT_ASSERT_STR_EQ(decks[1].deck, "beta");
    CT_ASSERT_EQ(decks[1].total_cards, 1);
    CT_ASSERT_EQ(decks[1].due_now, 0);
    CT_ASSERT_EQ(decks[1].learning, 1);
    CT_ASSERT_EQ(decks[1].review_state, 0);
    CT_ASSERT_DBL_EQ(decks[1].avg_stability, 2.5, 1e-9);
    CT_ASSERT_DBL_EQ(decks[1].avg_difficulty, 6.5, 1e-9);
    CT_ASSERT_STR_EQ(decks[2].deck, "gamma");
    CT_ASSERT_EQ(decks[2].total_cards, 1);
    CT_ASSERT_EQ(decks[2].due_now, 0);
    CT_ASSERT_EQ(decks[2].learning, 0);
    CT_ASSERT_EQ(decks[2].review_state, 0);
    CT_ASSERT_DBL_EQ(decks[2].avg_stability, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(decks[2].avg_difficulty, 0.0, 1e-9);

    memset(decks, 0, sizeof decks);
    CT_ASSERT_EQ(ct_repo_deck_stats(db, "2026-08-01T00:00:00Z", decks, 4), 3);
    CT_ASSERT_EQ(decks[0].due_now, 3);
    CT_ASSERT_EQ(decks[1].due_now, 1);
    CT_ASSERT_EQ(decks[2].due_now, 0);

    memset(decks, 0, sizeof decks);
    CT_ASSERT_EQ(ct_repo_deck_stats(db, "2026-01-01T00:00:00Z", decks, 4), 3);
    CT_ASSERT_EQ(decks[0].due_now, 0);
    CT_ASSERT_EQ(decks[1].due_now, 0);
    CT_ASSERT_EQ(decks[2].due_now, 0);

    memset(decks, 0, sizeof decks);
    CT_ASSERT_EQ(ct_repo_deck_stats(db, "2026-06-01T00:00:00Z", decks, 1), 1);
    CT_ASSERT_STR_EQ(decks[0].deck, "alpha");
    CT_ASSERT_EQ(decks[1].deck[0], '\0');
    CT_ASSERT_EQ(decks[1].total_cards, 0);
    ct_db_close(db);
}

CT_SUITE(stats_forecast) {
    sqlite3 *db = stats_db();
    long c1 = add_card(db, "f", "c1", CT_DIR_FR);
    long c2 = add_card(db, "f", "c2", CT_DIR_FR);
    long c3 = add_card(db, "f", "c3", CT_DIR_FR);
    long c4 = add_card(db, "f", "c4", CT_DIR_FR);
    long c5 = add_card(db, "f", "c5", CT_DIR_FR);

    put_srs(db, c1, 1.0, 5.0, "2026-06-01T10:00:00Z", 1);
    put_srs(db, c2, 1.0, 5.0, "2026-06-01T23:59:59Z", 1);
    put_srs(db, c3, 1.0, 5.0, "2026-06-03T08:00:00Z", 1);
    put_srs(db, c4, 1.0, 5.0, "2026-05-31T12:00:00Z", 1);
    put_srs(db, c5, 1.0, 5.0, "2026-07-01T00:00:00Z", 1);

    ct_due_forecast_row fc[8];
    memset(fc, 0, sizeof fc);
    CT_ASSERT_EQ(ct_repo_due_forecast(db, "2026-06-01", 4, fc, 8), 4);
    CT_ASSERT_STR_EQ(fc[0].day, "2026-06-01");
    CT_ASSERT_EQ(fc[0].due_count, 2);
    CT_ASSERT_STR_EQ(fc[1].day, "2026-06-02");
    CT_ASSERT_EQ(fc[1].due_count, 0);
    CT_ASSERT_STR_EQ(fc[2].day, "2026-06-03");
    CT_ASSERT_EQ(fc[2].due_count, 1);
    CT_ASSERT_STR_EQ(fc[3].day, "2026-06-04");
    CT_ASSERT_EQ(fc[3].due_count, 0);

    memset(fc, 0, sizeof fc);
    CT_ASSERT_EQ(ct_repo_due_forecast(db, "2026-06-30", 3, fc, 8), 3);
    CT_ASSERT_STR_EQ(fc[0].day, "2026-06-30");
    CT_ASSERT_EQ(fc[0].due_count, 0);
    CT_ASSERT_STR_EQ(fc[1].day, "2026-07-01");
    CT_ASSERT_EQ(fc[1].due_count, 1);
    CT_ASSERT_STR_EQ(fc[2].day, "2026-07-02");
    CT_ASSERT_EQ(fc[2].due_count, 0);

    memset(fc, 0, sizeof fc);
    CT_ASSERT_EQ(ct_repo_due_forecast(db, "2026-12-31", 2, fc, 8), 2);
    CT_ASSERT_STR_EQ(fc[0].day, "2026-12-31");
    CT_ASSERT_EQ(fc[0].due_count, 0);
    CT_ASSERT_STR_EQ(fc[1].day, "2027-01-01");
    CT_ASSERT_EQ(fc[1].due_count, 0);

    memset(fc, 0, sizeof fc);
    CT_ASSERT_EQ(ct_repo_due_forecast(db, "2026-06-01", 4, fc, 2), 2);
    CT_ASSERT_STR_EQ(fc[0].day, "2026-06-01");
    CT_ASSERT_EQ(fc[0].due_count, 2);
    CT_ASSERT_STR_EQ(fc[1].day, "2026-06-02");
    CT_ASSERT_EQ(fc[1].due_count, 0);

    CT_ASSERT_EQ(ct_repo_due_forecast(db, "2026-06-01", 0, fc, 8), 0);

    snprintf(fc[0].day, sizeof fc[0].day, "%s", "sentinel");
    fc[0].due_count = 777;
    CT_ASSERT_EQ(ct_repo_due_forecast(db, "2026-06-01", 4, NULL, 0), 4);
    CT_ASSERT_STR_EQ(fc[0].day, "sentinel");
    CT_ASSERT_EQ(fc[0].due_count, 777);
    ct_db_close(db);
}

CT_SUITE(stats_case_accuracy) {
    sqlite3 *db = stats_db();
    long aa = add_card(db, "acc", "aa", CT_DIR_FL);
    long k3 = add_card(db, "acc", "k3", CT_DIR_FR);
    long k1fr = add_card(db, "acc", "k1", CT_DIR_FR);
    long k1fl = add_card(db, "acc", "k1", CT_DIR_FL);
    long k2 = add_card(db, "acc", "k2", CT_DIR_FR);
    add_card(db, "acc", "zz", CT_DIR_FR);
    long other = add_card(db, "other", "k1", CT_DIR_FR);

    log_rating(db, aa, CT_AGAIN);
    log_rating(db, aa, CT_AGAIN);
    log_rating(db, k3, CT_AGAIN);
    log_rating(db, k3, CT_AGAIN);
    log_rating(db, k1fr, CT_AGAIN);
    log_rating(db, k1fr, CT_AGAIN);
    log_rating(db, k1fr, CT_GOOD);
    log_rating(db, k1fl, CT_GOOD);
    log_rating(db, k2, CT_HARD);
    log_rating(db, k2, CT_EASY);
    log_rating(db, other, CT_EASY);

    ct_case_accuracy_row acc[8];
    memset(acc, 0, sizeof acc);
    CT_ASSERT_EQ(ct_repo_case_accuracy(db, "acc", acc, 8), 5);
    CT_ASSERT_STR_EQ(acc[0].case_key, "aa");
    CT_ASSERT_EQ(acc[0].direction, CT_DIR_FL);
    CT_ASSERT_EQ(acc[0].reviews, 2);
    CT_ASSERT_EQ(acc[0].correct, 0);
    CT_ASSERT_DBL_EQ(acc[0].accuracy, 0.0, 1e-9);
    CT_ASSERT_STR_EQ(acc[1].case_key, "k3");
    CT_ASSERT_EQ(acc[1].direction, CT_DIR_FR);
    CT_ASSERT_EQ(acc[1].reviews, 2);
    CT_ASSERT_EQ(acc[1].correct, 0);
    CT_ASSERT_DBL_EQ(acc[1].accuracy, 0.0, 1e-9);
    CT_ASSERT_STR_EQ(acc[2].case_key, "k1");
    CT_ASSERT_EQ(acc[2].direction, CT_DIR_FR);
    CT_ASSERT_EQ(acc[2].reviews, 3);
    CT_ASSERT_EQ(acc[2].correct, 1);
    CT_ASSERT_DBL_EQ(acc[2].accuracy, 1.0 / 3.0, 1e-9);
    CT_ASSERT_STR_EQ(acc[3].case_key, "k1");
    CT_ASSERT_EQ(acc[3].direction, CT_DIR_FL);
    CT_ASSERT_EQ(acc[3].reviews, 1);
    CT_ASSERT_EQ(acc[3].correct, 1);
    CT_ASSERT_DBL_EQ(acc[3].accuracy, 1.0, 1e-9);
    CT_ASSERT_STR_EQ(acc[4].case_key, "k2");
    CT_ASSERT_EQ(acc[4].direction, CT_DIR_FR);
    CT_ASSERT_EQ(acc[4].reviews, 2);
    CT_ASSERT_EQ(acc[4].correct, 2);
    CT_ASSERT_DBL_EQ(acc[4].accuracy, 1.0, 1e-9);

    memset(acc, 0, sizeof acc);
    CT_ASSERT_EQ(ct_repo_case_accuracy(db, "acc", acc, 2), 2);
    CT_ASSERT_STR_EQ(acc[0].case_key, "aa");
    CT_ASSERT_STR_EQ(acc[1].case_key, "k3");
    CT_ASSERT_EQ(acc[2].case_key[0], '\0');

    memset(acc, 0, sizeof acc);
    CT_ASSERT_EQ(ct_repo_case_accuracy(db, "other", acc, 8), 1);
    CT_ASSERT_STR_EQ(acc[0].case_key, "k1");
    CT_ASSERT_EQ(acc[0].direction, CT_DIR_FR);
    CT_ASSERT_EQ(acc[0].reviews, 1);
    CT_ASSERT_EQ(acc[0].correct, 1);
    CT_ASSERT_DBL_EQ(acc[0].accuracy, 1.0, 1e-9);

    CT_ASSERT_EQ(ct_repo_case_accuracy(db, "missing", acc, 8), 0);
    ct_db_close(db);
}

CT_SUITE(stats_sessions) {
    sqlite3 *db = stats_db();
    ct_session_row rows[4];
    CT_ASSERT_EQ(ct_repo_sessions(db, rows, 4), 0);

    ct_session_row s1 = session_at("cfop", "2026-06-01T10:00:00Z", "", 10, 2, 3, 4, 1);
    ct_session_row s2 =
        session_at("3bld", "2026-06-02T09:00:00Z", "2026-06-02T09:05:00Z", 7, 0, 1, 5, 1);
    ct_session_row s3 =
        session_at("cfop", "2026-06-01T10:00:00Z", "2026-06-01T10:12:00Z", 3, 3, 0, 0, 0);
    CT_ASSERT_EQ(ct_repo_session_log(db, &s1), CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_session_log(db, &s2), CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_session_log(db, &s3), CT_REPO_OK);

    memset(rows, 0, sizeof rows);
    CT_ASSERT_EQ(ct_repo_sessions(db, rows, 4), 3);
    CT_ASSERT_EQ(rows[0].id, 2);
    CT_ASSERT_STR_EQ(rows[0].deck, "3bld");
    CT_ASSERT_STR_EQ(rows[0].started_at, "2026-06-02T09:00:00Z");
    CT_ASSERT_STR_EQ(rows[0].ended_at, "2026-06-02T09:05:00Z");
    CT_ASSERT_EQ(rows[0].n_cards, 7);
    CT_ASSERT_EQ(rows[0].n_again, 0);
    CT_ASSERT_EQ(rows[0].n_hard, 1);
    CT_ASSERT_EQ(rows[0].n_good, 5);
    CT_ASSERT_EQ(rows[0].n_easy, 1);
    CT_ASSERT_EQ(rows[1].id, 3);
    CT_ASSERT_STR_EQ(rows[1].deck, "cfop");
    CT_ASSERT_STR_EQ(rows[1].started_at, "2026-06-01T10:00:00Z");
    CT_ASSERT_STR_EQ(rows[1].ended_at, "2026-06-01T10:12:00Z");
    CT_ASSERT_EQ(rows[1].n_cards, 3);
    CT_ASSERT_EQ(rows[1].n_again, 3);
    CT_ASSERT_EQ(rows[1].n_hard, 0);
    CT_ASSERT_EQ(rows[1].n_good, 0);
    CT_ASSERT_EQ(rows[1].n_easy, 0);
    CT_ASSERT_EQ(rows[2].id, 1);
    CT_ASSERT_STR_EQ(rows[2].deck, "cfop");
    CT_ASSERT_STR_EQ(rows[2].started_at, "2026-06-01T10:00:00Z");
    CT_ASSERT_STR_EQ(rows[2].ended_at, "");
    CT_ASSERT_EQ(rows[2].n_cards, 10);
    CT_ASSERT_EQ(rows[2].n_again, 2);
    CT_ASSERT_EQ(rows[2].n_hard, 3);
    CT_ASSERT_EQ(rows[2].n_good, 4);
    CT_ASSERT_EQ(rows[2].n_easy, 1);

    memset(rows, 0, sizeof rows);
    CT_ASSERT_EQ(ct_repo_sessions(db, rows, 2), 2);
    CT_ASSERT_EQ(rows[0].id, 2);
    CT_ASSERT_EQ(rows[1].id, 3);
    CT_ASSERT_EQ(rows[2].id, 0);
    ct_db_close(db);
}

CT_SUITE(stats_malformed) {
    sqlite3 *db = stats_db();
    ct_deck_stats decks[2];
    ct_due_forecast_row fc[2];
    ct_case_accuracy_row acc[2];
    ct_session_row sess[2];
    ct_session_row row = session_at("d", "2026-06-01T00:00:00Z", "", 1, 0, 0, 1, 0);

    CT_ASSERT(ct_repo_deck_stats(NULL, "2026-06-01T00:00:00Z", decks, 2) < 0);
    CT_ASSERT(ct_repo_deck_stats(db, NULL, decks, 2) < 0);
    CT_ASSERT(ct_repo_deck_stats(db, "", decks, 2) < 0);
    char long_now[CT_STATS_STAMP_MAX + 1];
    memset(long_now, 'n', sizeof long_now - 1);
    long_now[sizeof long_now - 1] = '\0';
    CT_ASSERT(ct_repo_deck_stats(db, long_now, decks, 2) < 0);
    CT_ASSERT(ct_repo_deck_stats(db, "2026-06-01T00:00:00Z", decks, -1) < 0);
    CT_ASSERT(ct_repo_deck_stats(db, "2026-06-01T00:00:00Z", NULL, 2) < 0);
    CT_ASSERT_EQ(ct_repo_deck_stats(db, "2026-06-01T00:00:00Z", NULL, 0), 0);

    CT_ASSERT(ct_repo_due_forecast(NULL, "2026-06-01", 3, fc, 2) < 0);
    CT_ASSERT(ct_repo_due_forecast(db, NULL, 3, fc, 2) < 0);
    CT_ASSERT(ct_repo_due_forecast(db, "", 3, fc, 2) < 0);
    char long_day[CT_STATS_DAY_MAX + 1];
    memset(long_day, 'd', sizeof long_day - 1);
    long_day[sizeof long_day - 1] = '\0';
    CT_ASSERT(ct_repo_due_forecast(db, long_day, 3, fc, 2) < 0);
    CT_ASSERT(ct_repo_due_forecast(db, "2026-06-01", -1, fc, 2) < 0);
    CT_ASSERT(ct_repo_due_forecast(db, "2026-06-01", 3, NULL, 2) < 0);
    CT_ASSERT(ct_repo_due_forecast(db, "2026-06-01", 3, fc, -1) < 0);

    CT_ASSERT(ct_repo_case_accuracy(NULL, "d", acc, 2) < 0);
    CT_ASSERT(ct_repo_case_accuracy(db, NULL, acc, 2) < 0);
    CT_ASSERT(ct_repo_case_accuracy(db, "", acc, 2) < 0);
    char big_deck[CT_STATS_DECK_MAX + 1];
    memset(big_deck, 'd', sizeof big_deck - 1);
    big_deck[sizeof big_deck - 1] = '\0';
    CT_ASSERT(ct_repo_case_accuracy(db, big_deck, acc, 2) < 0);
    CT_ASSERT(ct_repo_case_accuracy(db, "d", acc, -1) < 0);
    CT_ASSERT(ct_repo_case_accuracy(db, "d", NULL, 2) < 0);
    CT_ASSERT_EQ(ct_repo_case_accuracy(db, "d", NULL, 0), 0);

    CT_ASSERT(ct_repo_session_log(NULL, &row) < 0);
    CT_ASSERT(ct_repo_session_log(db, NULL) < 0);
    ct_session_row empty_deck = row;
    empty_deck.deck[0] = '\0';
    CT_ASSERT(ct_repo_session_log(db, &empty_deck) < 0);
    ct_session_row empty_start = row;
    empty_start.started_at[0] = '\0';
    CT_ASSERT(ct_repo_session_log(db, &empty_start) < 0);
    struct {
        ct_session_row r;
        char pad[8];
    } over;
    memset(&over, 0, sizeof over);
    memset(over.r.deck, 'd', CT_STATS_DECK_MAX + 4);
    snprintf(over.r.started_at, sizeof over.r.started_at, "%s", "2026-06-01T00:00:00Z");
    CT_ASSERT(ct_repo_session_log(db, &over.r) < 0);
    CT_ASSERT(ct_repo_sessions(NULL, sess, 2) < 0);
    CT_ASSERT(ct_repo_sessions(db, sess, -1) < 0);
    CT_ASSERT(ct_repo_sessions(db, NULL, 2) < 0);
    CT_ASSERT_EQ(ct_repo_sessions(db, NULL, 0), 0);
    ct_db_close(db);
}
