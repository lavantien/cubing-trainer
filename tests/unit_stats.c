#include <stdio.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/srs/fsrs.h"
#include "../src/stats/stats.h"
#include "framework.h"

enum { CURVE_ROWS = 7, CURVE_BUCKETS = 10, FINE_BUCKETS = 20 };

static ct_review_row review_at(double stability, double elapsed, int rating) {
    ct_review_row row;
    memset(&row, 0, sizeof row);
    row.stability = stability;
    row.elapsed_days = elapsed;
    row.rating = rating;
    return row;
}

static void fill_curve(ct_review_row *rows) {
    rows[0] = review_at(1.0, 1.0, CT_GOOD);
    rows[1] = review_at(1.0, 0.0, CT_EASY);
    rows[2] = review_at(19.0, 243.0, CT_AGAIN);
    rows[3] = review_at(19.0, 243.0, CT_HARD);
    rows[4] = review_at(19.0, 63.0, CT_GOOD);
    rows[5] = review_at(19.0, 1215.0, CT_AGAIN);
    rows[6] = review_at(19.0, 32319.0, CT_AGAIN);
}

static ct_case_accuracy_row acc_row(const char *key, int dir, double acc) {
    ct_case_accuracy_row row;
    memset(&row, 0, sizeof row);
    snprintf(row.case_key, sizeof row.case_key, "%s", key);
    row.direction = dir;
    row.reviews = 2;
    row.correct = 1;
    row.accuracy = acc;
    return row;
}

CT_SUITE(stats_agg_retention) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);

    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(1.0, 1.0), 0.9, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(1.0, 0.0), 1.0, 1e-15);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(19.0, 243.0), 0.5, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(19.0, 63.0), 0.75, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(19.0, 1215.0), 0.25, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(19.0, 32319.0), 0.05, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(0.0, 5.0), 0.0, 1e-15);

    ct_review_row rows[CURVE_ROWS];
    fill_curve(rows);

    long exp_reviews[CURVE_BUCKETS] = {0};
    long exp_actual[CURVE_BUCKETS] = {0};
    double exp_sum[CURVE_BUCKETS] = {0.0};
    for (int i = 0; i < CURVE_ROWS; i++) {
        double r = ct_fsrs_retrievability(rows[i].stability, rows[i].elapsed_days);
        int idx = (int)(r * CURVE_BUCKETS);
        if (idx >= CURVE_BUCKETS) {
            idx = CURVE_BUCKETS - 1;
        }
        exp_reviews[idx] += 1;
        exp_actual[idx] += rows[i].rating > CT_AGAIN ? 1 : 0;
        exp_sum[idx] += r;
    }

    ct_retention_bucket out[CURVE_BUCKETS];
    CT_ASSERT_EQ(ct_stats_retention(rows, CURVE_ROWS, &f, out, CURVE_BUCKETS), CURVE_BUCKETS);
    long total_rev = 0;
    long total_act = 0;
    for (int i = 0; i < CURVE_BUCKETS; i++) {
        CT_ASSERT_DBL_EQ(out[i].bucket_mid, (i + 0.5) / CURVE_BUCKETS, 1e-12);
        CT_ASSERT_EQ(out[i].reviews, exp_reviews[i]);
        CT_ASSERT_EQ(out[i].actual_recall, exp_actual[i]);
        if (exp_reviews[i] > 0) {
            CT_ASSERT_DBL_EQ(out[i].predicted_recall, exp_sum[i] / (double)exp_reviews[i], 1e-12);
        } else {
            CT_ASSERT_DBL_EQ(out[i].predicted_recall, 0.0, 1e-15);
        }
        total_rev += out[i].reviews;
        total_act += out[i].actual_recall;
    }
    CT_ASSERT_EQ(total_rev, 7);
    CT_ASSERT_EQ(total_act, 4);

    CT_ASSERT_EQ(out[0].reviews, 1);
    CT_ASSERT_EQ(out[0].actual_recall, 0);
    CT_ASSERT_DBL_EQ(out[0].predicted_recall, 0.05, 1e-12);
    CT_ASSERT_EQ(out[1].reviews, 0);
    CT_ASSERT_EQ(out[1].actual_recall, 0);
    CT_ASSERT_DBL_EQ(out[1].predicted_recall, 0.0, 1e-15);
    CT_ASSERT_EQ(out[2].reviews, 1);
    CT_ASSERT_EQ(out[2].actual_recall, 0);
    CT_ASSERT_DBL_EQ(out[2].predicted_recall, 0.25, 1e-12);
    CT_ASSERT_EQ(out[3].reviews, 0);
    CT_ASSERT_EQ(out[4].reviews, 0);
    CT_ASSERT_EQ(out[5].reviews, 2);
    CT_ASSERT_EQ(out[5].actual_recall, 1);
    CT_ASSERT_DBL_EQ(out[5].predicted_recall, 0.5, 1e-12);
    CT_ASSERT_EQ(out[6].reviews, 0);
    CT_ASSERT_EQ(out[7].reviews, 1);
    CT_ASSERT_EQ(out[7].actual_recall, 1);
    CT_ASSERT_DBL_EQ(out[7].predicted_recall, 0.75, 1e-12);
    CT_ASSERT_EQ(out[8].reviews, 0);
    CT_ASSERT_EQ(out[9].reviews, 2);
    CT_ASSERT_EQ(out[9].actual_recall, 2);
    CT_ASSERT_DBL_EQ(out[9].predicted_recall, 0.95, 1e-12);
}

CT_SUITE(stats_agg_retention_granularity) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);

    ct_review_row rows[CURVE_ROWS];
    fill_curve(rows);

    ct_retention_bucket one[1];
    CT_ASSERT_EQ(ct_stats_retention(rows, CURVE_ROWS, &f, one, 1), 1);
    CT_ASSERT_DBL_EQ(one[0].bucket_mid, 0.5, 1e-12);
    CT_ASSERT_EQ(one[0].reviews, 7);
    CT_ASSERT_EQ(one[0].actual_recall, 4);
    CT_ASSERT_DBL_EQ(one[0].predicted_recall, 3.95 / 7.0, 1e-12);

    ct_retention_bucket fine[FINE_BUCKETS];
    CT_ASSERT_EQ(ct_stats_retention(rows, CURVE_ROWS, &f, fine, FINE_BUCKETS), FINE_BUCKETS);
    static const int want[CURVE_ROWS] = {18, 19, 10, 10, 15, 5, 1};
    for (int i = 0; i < CURVE_ROWS; i++) {
        double r = ct_fsrs_retrievability(rows[i].stability, rows[i].elapsed_days);
        int idx = (int)(r * FINE_BUCKETS);
        if (idx >= FINE_BUCKETS) {
            idx = FINE_BUCKETS - 1;
        }
        CT_ASSERT_EQ(idx, want[i]);
    }
    CT_ASSERT_EQ(fine[0].reviews, 0);
    CT_ASSERT_DBL_EQ(fine[0].predicted_recall, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(fine[1].bucket_mid, 0.075, 1e-12);
    CT_ASSERT_EQ(fine[1].reviews, 1);
    CT_ASSERT_EQ(fine[1].actual_recall, 0);
    CT_ASSERT_DBL_EQ(fine[1].predicted_recall, 0.05, 1e-12);
    CT_ASSERT_DBL_EQ(fine[5].bucket_mid, 0.275, 1e-12);
    CT_ASSERT_EQ(fine[5].reviews, 1);
    CT_ASSERT_DBL_EQ(fine[5].predicted_recall, 0.25, 1e-12);
    CT_ASSERT_DBL_EQ(fine[10].bucket_mid, 0.525, 1e-12);
    CT_ASSERT_EQ(fine[10].reviews, 2);
    CT_ASSERT_EQ(fine[10].actual_recall, 1);
    CT_ASSERT_DBL_EQ(fine[10].predicted_recall, 0.5, 1e-12);
    CT_ASSERT_EQ(fine[15].reviews, 1);
    CT_ASSERT_EQ(fine[15].actual_recall, 1);
    CT_ASSERT_DBL_EQ(fine[15].predicted_recall, 0.75, 1e-12);
    CT_ASSERT_EQ(fine[18].reviews, 1);
    CT_ASSERT_EQ(fine[18].actual_recall, 1);
    CT_ASSERT_DBL_EQ(fine[18].predicted_recall, 0.9, 1e-12);
    CT_ASSERT_DBL_EQ(fine[19].bucket_mid, 0.975, 1e-12);
    CT_ASSERT_EQ(fine[19].reviews, 1);
    CT_ASSERT_EQ(fine[19].actual_recall, 1);
    CT_ASSERT_DBL_EQ(fine[19].predicted_recall, 1.0, 1e-15);

    ct_review_row edges[3];
    edges[0] = review_at(0.0, 5.0, CT_AGAIN);
    edges[1] = review_at(0.0, 0.0, CT_GOOD);
    edges[2] = review_at(1.0, 0.0, CT_GOOD);
    ct_retention_bucket eo[CURVE_BUCKETS];
    CT_ASSERT_EQ(ct_stats_retention(edges, 3, &f, eo, CURVE_BUCKETS), CURVE_BUCKETS);
    CT_ASSERT_EQ(eo[0].reviews, 2);
    CT_ASSERT_EQ(eo[0].actual_recall, 1);
    CT_ASSERT_DBL_EQ(eo[0].predicted_recall, 0.0, 1e-15);
    CT_ASSERT_EQ(eo[9].reviews, 1);
    CT_ASSERT_EQ(eo[9].actual_recall, 1);
    CT_ASSERT_DBL_EQ(eo[9].predicted_recall, 1.0, 1e-15);
}

CT_SUITE(stats_agg_summary) {
    ct_retention_bucket hand[2] = {{0.25, 3, 1, 0.4}, {0.75, 1, 1, 0.9}};
    ct_retention_summary s = ct_stats_retention_summary(hand, 2);
    CT_ASSERT_DBL_EQ(s.total_reviews, 4.0, 1e-12);
    CT_ASSERT_DBL_EQ(s.total_correct, 2.0, 1e-12);
    CT_ASSERT_DBL_EQ(s.weighted_error, 3.0 / 40.0, 1e-9);
    CT_ASSERT_DBL_EQ(s.max_abs_error, 0.1, 1e-9);
    CT_ASSERT_EQ(s.worst_bucket, 1);

    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    ct_review_row rows[CURVE_ROWS];
    fill_curve(rows);
    ct_retention_bucket out[CURVE_BUCKETS];
    CT_ASSERT_EQ(ct_stats_retention(rows, CURVE_ROWS, &f, out, CURVE_BUCKETS), CURVE_BUCKETS);
    ct_retention_summary c = ct_stats_retention_summary(out, CURVE_BUCKETS);
    CT_ASSERT_DBL_EQ(c.total_reviews, 7.0, 1e-12);
    CT_ASSERT_DBL_EQ(c.total_correct, 4.0, 1e-12);
    CT_ASSERT_DBL_EQ(c.weighted_error, 0.65 / 7.0, 1e-9);
    CT_ASSERT_DBL_EQ(c.max_abs_error, 0.25, 1e-9);
    CT_ASSERT_EQ(c.worst_bucket, 2);

    ct_retention_bucket empty[3];
    memset(empty, 0, sizeof empty);
    ct_retention_summary e = ct_stats_retention_summary(empty, 3);
    CT_ASSERT_DBL_EQ(e.total_reviews, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(e.total_correct, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(e.weighted_error, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(e.max_abs_error, 0.0, 1e-15);
    CT_ASSERT_EQ(e.worst_bucket, -1);

    ct_retention_summary n = ct_stats_retention_summary(NULL, 3);
    CT_ASSERT_EQ(n.worst_bucket, -1);
    ct_retention_summary z = ct_stats_retention_summary(hand, 0);
    CT_ASSERT_EQ(z.worst_bucket, -1);
    ct_retention_summary g = ct_stats_retention_summary(hand, -2);
    CT_ASSERT_EQ(g.worst_bucket, -1);
}

CT_SUITE(stats_agg_correlation) {
    double xs[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double lin[4] = {1.0, 2.0, 3.0, 4.0};
    double up[4] = {2.0, 4.0, 6.0, 8.0};
    double down[4] = {8.0, 6.0, 4.0, 2.0};
    double mixed[5] = {2.0, 1.0, 4.0, 3.0, 5.0};
    double flat[4] = {5.0, 5.0, 5.0, 5.0};

    CT_ASSERT_DBL_EQ(ct_stats_correlation(lin, up, 4), 1.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(lin, down, 4), -1.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(flat, lin, 4), 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(lin, flat, 4), 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(xs, mixed, 5), 0.8, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(xs, mixed, 1), 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(xs, mixed, 0), 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(xs, mixed, -3), 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(NULL, mixed, 5), 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_stats_correlation(xs, NULL, 5), 0.0, 1e-12);
}

CT_SUITE(stats_agg_forecast) {
    ct_due_forecast_row fc[6];
    memset(fc, 0, sizeof fc);
    fc[0].due_count = 5;
    fc[2].due_count = 3;
    fc[3].due_count = 2;

    CT_ASSERT_EQ(ct_stats_forecast_total(fc, 4), 10);
    CT_ASSERT_DBL_EQ(ct_stats_forecast_peak_day_ratio(fc, 4), 2.0, 1e-12);

    memset(fc, 0, sizeof fc);
    fc[2].due_count = 9;
    CT_ASSERT_EQ(ct_stats_forecast_total(fc, 4), 9);
    CT_ASSERT_DBL_EQ(ct_stats_forecast_peak_day_ratio(fc, 4), 4.0, 1e-12);

    memset(fc, 0, sizeof fc);
    fc[0].due_count = 7;
    CT_ASSERT_EQ(ct_stats_forecast_total(fc, 1), 7);
    CT_ASSERT_DBL_EQ(ct_stats_forecast_peak_day_ratio(fc, 1), 1.0, 1e-12);

    memset(fc, 0, sizeof fc);
    CT_ASSERT_EQ(ct_stats_forecast_total(fc, 3), 0);
    CT_ASSERT_DBL_EQ(ct_stats_forecast_peak_day_ratio(fc, 3), 0.0, 1e-15);

    CT_ASSERT_EQ(ct_stats_forecast_total(fc, 0), 0);
    CT_ASSERT_DBL_EQ(ct_stats_forecast_peak_day_ratio(fc, 0), 0.0, 1e-15);
    CT_ASSERT_EQ(ct_stats_forecast_total(NULL, 4), 0);
    CT_ASSERT_DBL_EQ(ct_stats_forecast_peak_day_ratio(NULL, 4), 0.0, 1e-15);
    CT_ASSERT_EQ(ct_stats_forecast_total(fc, -1), 0);
    CT_ASSERT_DBL_EQ(ct_stats_forecast_peak_day_ratio(fc, -1), 0.0, 1e-15);
}

CT_SUITE(stats_agg_heatmap) {
    ct_case_accuracy_row rows[7];
    rows[0] = acc_row("mb", CT_DIR_FR, 0.9);
    rows[1] = acc_row("aa", CT_DIR_FR, 0.5);
    rows[2] = acc_row("ab", CT_DIR_FR, 0.5);
    rows[3] = acc_row("zz", CT_DIR_FR, 0.1);
    rows[4] = acc_row("ma", CT_DIR_FR, 0.9);
    rows[5] = acc_row("k1", CT_DIR_FL, 0.5);
    rows[6] = acc_row("k1", CT_DIR_FR, 0.5);

    int order[7];
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 7, order, 7), 7);
    static const int want[7] = {3, 1, 2, 5, 6, 4, 0};
    for (int i = 0; i < 7; i++) {
        CT_ASSERT_EQ(order[i], want[i]);
    }

    memset(order, 0, sizeof order);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 7, order, 3), 3);
    CT_ASSERT_EQ(order[0], 3);
    CT_ASSERT_EQ(order[1], 1);
    CT_ASSERT_EQ(order[2], 2);

    order[3] = 777;
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 7, order + 3, 0), 0);
    CT_ASSERT_EQ(order[3], 777);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 7, order + 3, -1), 0);
    CT_ASSERT_EQ(order[3], 777);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 7, NULL, -1), 0);

    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 7, NULL, 5), -1);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(NULL, 7, order, 7), -1);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, -1, order, 7), -1);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 0, order, 7), 0);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(NULL, 0, order, 7), 0);
    CT_ASSERT_EQ(ct_stats_heatmap_rank(rows, 7, NULL, 0), 0);

    ct_case_accuracy_row sorted[3];
    sorted[0] = acc_row("a", CT_DIR_FR, 0.1);
    sorted[1] = acc_row("b", CT_DIR_FR, 0.5);
    sorted[2] = acc_row("c", CT_DIR_FR, 0.9);
    int sorder[3];
    CT_ASSERT_EQ(ct_stats_heatmap_rank(sorted, 3, sorder, 3), 3);
    CT_ASSERT_EQ(sorder[0], 0);
    CT_ASSERT_EQ(sorder[1], 1);
    CT_ASSERT_EQ(sorder[2], 2);
}

CT_SUITE(stats_agg_malformed) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    ct_review_row rows[2];
    rows[0] = review_at(1.0, 1.0, CT_GOOD);
    rows[1] = review_at(1.0, 0.0, CT_AGAIN);
    ct_retention_bucket out[4];

    CT_ASSERT_EQ(ct_stats_retention(NULL, 2, &f, out, 4), -1);
    CT_ASSERT_EQ(ct_stats_retention(rows, -1, &f, out, 4), -1);
    CT_ASSERT_EQ(ct_stats_retention(rows, 2, &f, NULL, 4), -1);
    CT_ASSERT_EQ(ct_stats_retention(rows, 2, &f, out, -1), -1);
    CT_ASSERT_EQ(ct_stats_retention(rows, 2, &f, NULL, 0), 0);
    CT_ASSERT_EQ(ct_stats_retention(NULL, 0, &f, out, 4), 4);
    CT_ASSERT_DBL_EQ(out[0].bucket_mid, 0.125, 1e-12);
    CT_ASSERT_EQ(out[0].reviews, 0);
    CT_ASSERT_EQ(out[0].actual_recall, 0);
    CT_ASSERT_DBL_EQ(out[0].predicted_recall, 0.0, 1e-15);
    CT_ASSERT_EQ(ct_stats_retention(rows, 0, &f, out, 4), 4);
    out[0].reviews = 777;
    CT_ASSERT_EQ(ct_stats_retention(rows, 2, &f, out, 0), 0);
    CT_ASSERT_EQ(out[0].reviews, 777);
}
