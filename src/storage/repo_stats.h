#ifndef CT_STORAGE_REPO_STATS_H
#define CT_STORAGE_REPO_STATS_H

#include <sqlite3.h>

#include "repo_cards.h"

enum {
    CT_STATS_DECK_MAX = 64,
    CT_STATS_CASE_KEY_MAX = 64,
    CT_STATS_DAY_MAX = 11,
    CT_STATS_STAMP_MAX = 32
};

typedef struct {
    char deck[CT_STATS_DECK_MAX];
    long total_cards;
    long due_now;
    long learning;
    long review_state;
    double avg_stability;
    double avg_difficulty;
} ct_deck_stats;

typedef struct {
    char day[CT_STATS_DAY_MAX];
    long due_count;
} ct_due_forecast_row;

typedef struct {
    char case_key[CT_STATS_CASE_KEY_MAX];
    int direction;
    long reviews;
    long correct;
    double accuracy;
} ct_case_accuracy_row;

typedef struct {
    long id;
    char deck[CT_STATS_DECK_MAX];
    char started_at[CT_STATS_STAMP_MAX];
    char ended_at[CT_STATS_STAMP_MAX];
    int n_cards, n_again, n_hard, n_good, n_easy;
} ct_session_row;

int ct_repo_deck_stats(sqlite3 *db, const char *now_iso, ct_deck_stats *out, int cap);
int ct_repo_due_forecast(sqlite3 *db, const char *from_day, int days, ct_due_forecast_row *out, int cap);
int ct_repo_case_accuracy(sqlite3 *db, const char *deck, ct_case_accuracy_row *out, int cap);
int ct_repo_session_log(sqlite3 *db, const ct_session_row *row);
int ct_repo_sessions(sqlite3 *db, ct_session_row *out, int cap);

#endif
