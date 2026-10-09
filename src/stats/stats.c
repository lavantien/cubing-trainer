#include <math.h>
#include <string.h>

#include "stats/stats.h"

static int heat_before(const ct_case_accuracy_row *rows, int a, int b) {
    if (rows[a].accuracy != rows[b].accuracy) {
        return rows[a].accuracy < rows[b].accuracy;
    }
    return strcmp(rows[a].case_key, rows[b].case_key) < 0;
}

int ct_stats_retention(const ct_review_row *rows, int count, const ct_fsrs *f,
                       ct_retention_bucket *out, int buckets) {
    (void)f;
    if (count < 0 || buckets < 0 || (count > 0 && rows == NULL) || (buckets > 0 && out == NULL)) {
        return -1;
    }
    if (buckets == 0) {
        return 0;
    }
    for (int i = 0; i < buckets; i++) {
        out[i].bucket_mid = (2.0 * i + 1.0) / (2.0 * buckets);
        out[i].reviews = 0;
        out[i].actual_recall = 0;
        out[i].predicted_recall = 0.0;
    }
    for (int i = 0; i < count; i++) {
        double r = ct_fsrs_retrievability(rows[i].stability, rows[i].elapsed_days);
        if (!(r > 0.0)) {
            r = 0.0;
        }
        if (r > 1.0) {
            r = 1.0;
        }
        int idx = (int)(r * buckets);
        if (idx >= buckets) {
            idx = buckets - 1;
        }
        out[idx].reviews++;
        if (rows[i].rating > (int)CT_AGAIN) {
            out[idx].actual_recall++;
        }
        out[idx].predicted_recall += r;
    }
    for (int i = 0; i < buckets; i++) {
        if (out[i].reviews > 0) {
            out[i].predicted_recall /= (double)out[i].reviews;
        }
    }
    return buckets;
}

ct_retention_summary ct_stats_retention_summary(const ct_retention_bucket *buckets, int count) {
    ct_retention_summary s = {0.0, 0.0, 0.0, 0.0, -1};
    if (count <= 0 || buckets == NULL) {
        return s;
    }
    long total = 0;
    long correct = 0;
    for (int i = 0; i < count; i++) {
        total += buckets[i].reviews;
        correct += buckets[i].actual_recall;
    }
    s.total_reviews = (double)total;
    s.total_correct = (double)correct;
    if (total == 0) {
        return s;
    }
    double weighted = 0.0;
    for (int i = 0; i < count; i++) {
        if (buckets[i].reviews == 0) {
            continue;
        }
        double actual = (double)buckets[i].actual_recall / (double)buckets[i].reviews;
        double err = fabs(buckets[i].predicted_recall - actual);
        weighted += (double)buckets[i].reviews * err;
        if (err > s.max_abs_error) {
            s.max_abs_error = err;
            s.worst_bucket = i;
        }
    }
    s.weighted_error = weighted / (double)total;
    return s;
}

double ct_stats_correlation(const double *xs, const double *ys, int count) {
    if (count < 2 || xs == NULL || ys == NULL) {
        return 0.0;
    }
    double mx = 0.0;
    double my = 0.0;
    for (int i = 0; i < count; i++) {
        mx += xs[i];
        my += ys[i];
    }
    mx /= (double)count;
    my /= (double)count;
    double cov = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    for (int i = 0; i < count; i++) {
        double dx = xs[i] - mx;
        double dy = ys[i] - my;
        cov += dx * dy;
        vx += dx * dx;
        vy += dy * dy;
    }
    if (vx <= 0.0 || vy <= 0.0) {
        return 0.0;
    }
    return cov / sqrt(vx * vy);
}

int ct_stats_forecast_total(const ct_due_forecast_row *rows, int count) {
    if (count <= 0 || rows == NULL) {
        return 0;
    }
    long total = 0;
    for (int i = 0; i < count; i++) {
        total += rows[i].due_count;
    }
    return (int)total;
}

double ct_stats_forecast_peak_day_ratio(const ct_due_forecast_row *rows, int count) {
    if (count <= 0 || rows == NULL) {
        return 0.0;
    }
    long total = 0;
    long peak = 0;
    for (int i = 0; i < count; i++) {
        total += rows[i].due_count;
        if (rows[i].due_count > peak) {
            peak = rows[i].due_count;
        }
    }
    if (total == 0) {
        return 0.0;
    }
    return (double)peak / ((double)total / (double)count);
}

int ct_stats_heatmap_rank(const ct_case_accuracy_row *rows, int count, int *order_out, int cap) {
    if (count < 0 || (count > 0 && rows == NULL) || (cap > 0 && order_out == NULL)) {
        return -1;
    }
    int n = count < cap ? count : cap;
    if (n <= 0) {
        return 0;
    }
    for (int i = 0; i < count; i++) {
        if (i >= n && !heat_before(rows, i, order_out[n - 1])) {
            continue;
        }
        int j = i < n ? i : n - 1;
        order_out[j] = i;
        while (j > 0 && heat_before(rows, order_out[j], order_out[j - 1])) {
            int tmp = order_out[j];
            order_out[j] = order_out[j - 1];
            order_out[j - 1] = tmp;
            j--;
        }
    }
    return n;
}
