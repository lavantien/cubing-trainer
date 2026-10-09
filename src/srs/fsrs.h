#ifndef CT_FSRS_H
#define CT_FSRS_H

#include "config/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double stability;
    double difficulty;
    long due_day;
    long last_review_day;
    int reps;
    int lapses;
    int state;
} ct_fsrs_card;

typedef struct {
    double w[CT_FSRS_WEIGHT_COUNT];
    double request_retention;
    double max_ivl_days;
} ct_fsrs;

void ct_fsrs_init(ct_fsrs *f, const ct_config *cfg);

double ct_fsrs_retrievability(double stability, double elapsed_days);

long ct_fsrs_next_interval_days(const ct_fsrs *f, double stability);

ct_fsrs_card ct_fsrs_review(const ct_fsrs *f, const ct_fsrs_card *card, ct_grade g,
                            double elapsed_days, long today_day);

#ifdef __cplusplus
}
#endif

#endif
