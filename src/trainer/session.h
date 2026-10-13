#ifndef CT_TRAINER_SESSION_H
#define CT_TRAINER_SESSION_H

#include <sqlite3.h>

#include <stdbool.h>

#include "config/config.h"
#include "srs/fsrs.h"
#include "storage/repo_cards.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CT_SESSION_OK = 0,
    CT_SESSION_ERR_ARG = -1,
    CT_SESSION_ERR_DB = -2,
    CT_SESSION_ERR_STATE = -3,
    CT_SESSION_DECK_MAX = CT_CARD_DECK_MAX,
    CT_SESSION_STAMP_MAX = CT_CARD_STAMP_MAX
};

typedef struct {
    sqlite3 *db;
    const ct_fsrs *fsrs;
    char deck[CT_SESSION_DECK_MAX];
    long *queue;
    int queue_count;
    int queue_pos;
    long current_card;
    bool card_open;
    int n_graded, n_again, n_hard, n_good, n_easy;
    char started_at[CT_SESSION_STAMP_MAX];
    long today_day;
} ct_session;

int ct_session_begin(ct_session *s, sqlite3 *db, const ct_fsrs *fsrs, const char *deck,
                     const char *now_iso, long today_day, int limit);
int ct_session_current(ct_session *s, ct_card *out);
bool ct_session_has_next(const ct_session *s);
int ct_session_grade(ct_session *s, ct_grade g, const char *now_iso);
int ct_session_end(ct_session *s, const char *now_iso);
void ct_session_free(ct_session *s);

long ct_session_date_to_day(long y, int m, int d);
void ct_session_day_to_date(long day, long *y, int *m, int *d);
int ct_session_iso_day(const char *iso, long *day);
int ct_session_iso_add_days(const char *iso, long days, char *out, unsigned cap);

#ifdef __cplusplus
}
#endif

#endif
