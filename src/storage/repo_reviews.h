#ifndef CT_STORAGE_REPO_REVIEWS_H
#define CT_STORAGE_REPO_REVIEWS_H

#include "repo_cards.h"

typedef struct ct_review_row {
    long id;
    long card_id;
    int rating;
    char reviewed_at[CT_CARD_STAMP_MAX];
    double stability;
    double difficulty;
    double elapsed_days;
    double scheduled_days;
} ct_review_row;

int ct_repo_review_log(sqlite3 *db, long card_id, int rating, const char *reviewed_at,
                       double stability, double difficulty, double elapsed_days,
                       double scheduled_days);
int ct_repo_reviews_for_card(sqlite3 *db, long card_id, ct_review_row *out, int cap);
int ct_repo_due_cards(sqlite3 *db, const char *deck, const char *now_iso, long *ids, int cap);

#endif
