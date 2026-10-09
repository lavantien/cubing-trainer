#ifndef CT_STATS_H
#define CT_STATS_H

#include "srs/fsrs.h"
#include "storage/repo_reviews.h"
#include "storage/repo_stats.h"

typedef struct {
    double bucket_mid;
    long reviews;
    long actual_recall;
    double predicted_recall;
} ct_retention_bucket;

typedef struct {
    double total_reviews;
    double total_correct;
    double weighted_error;
    double max_abs_error;
    long worst_bucket;
} ct_retention_summary;

int ct_stats_retention(const ct_review_row *rows, int count, const ct_fsrs *f,
                       ct_retention_bucket *out, int buckets);
ct_retention_summary ct_stats_retention_summary(const ct_retention_bucket *buckets, int count);
double ct_stats_correlation(const double *xs, const double *ys, int count);
int ct_stats_forecast_total(const ct_due_forecast_row *rows, int count);
double ct_stats_forecast_peak_day_ratio(const ct_due_forecast_row *rows, int count);
int ct_stats_heatmap_rank(const ct_case_accuracy_row *rows, int count, int *order_out, int cap);

#endif
