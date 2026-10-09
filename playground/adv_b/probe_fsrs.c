#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "srs/fsrs.h"

static int failures = 0;

static void check(int cond, const char *what) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    }
}

static unsigned long long s_state = 0xADDB5EED5EED0001ULL;

static unsigned long long s_next(void) {
    unsigned long long s = s_state;
    s ^= s << 12;
    s ^= s >> 25;
    s ^= s << 27;
    s_state = s ^ 0x9E3779B97F4A7C15ULL;
    return s_state;
}

static double s_uniform(double lo, double hi) {
    return lo + (hi - lo) * ((double)(s_next() >> 11) / 9007199254740992.0);
}

static double wiki_d0(const double *w, int g) {
    return w[4] - exp(w[5] * (g - 1)) + 1;
}

static double wiki_r(double s, double t) {
    return pow(1 + (19.0 / 81.0) * t / s, -0.5);
}

static double wiki_interval(double s, double rr) {
    return (s / (19.0 / 81.0)) * (pow(rr, 1 / -0.5) - 1);
}

static void goldens(const ct_fsrs *f) {
    const double *w = f->w;
    ct_fsrs_card c = {0};
    for (int g = 1; g <= 4; g++) {
        ct_fsrs_card out = ct_fsrs_review(f, &c, (ct_grade)g, 0, 100);
        check(fabs(out.stability - w[g - 1]) < 1e-12, "S0 = w[g-1]");
        check(fabs(out.difficulty - (wiki_d0(w, g) > 10 ? 10 : (wiki_d0(w, g) < 1 ? 1 : wiki_d0(w, g)))) < 1e-12,
              "D0 = w4 - e^(w5(g-1)) + 1 clamped");
        printf("S0(%d)=%.6f D0(%d)=%.9f\n", g, out.stability, g, out.difficulty);
    }
    double r = ct_fsrs_retrievability(10.0, 10.0);
    check(fabs(r - wiki_r(10, 10)) < 1e-15, "R formula matches wiki rederivation");
    printf("R(10,10)=%.12f wiki=%.12f\n", r, wiki_r(10, 10));
    double s_in = 123.4;
    long ivl = ct_fsrs_next_interval_days(f, s_in);
    double raw = wiki_interval(s_in, 0.9);
    printf("I(0.9,123.4)=%ld raw=%.9f\n", ivl, raw);
    check(ivl == (long)round(raw), "interval equals wiki formula rounded");
}

static void monotone_retrievability(void) {
    static const double ss[8] = {0.1, 0.5, 1, 5, 25, 100, 1000, 36500};
    long violations = 0;
    for (size_t k = 0; k < 8; k++) {
        double prev = ct_fsrs_retrievability(ss[k], 0.0);
        for (double t = 0.25; t <= 40000; t += 0.25) {
            double r = ct_fsrs_retrievability(ss[k], t);
            if (isnan(r) || r > prev + 1e-15) {
                violations++;
            }
            prev = r;
        }
    }
    printf("R monotone violations=%ld\n", violations);
    check(violations == 0, "R monotone decreasing for t >= 0");
}

static void monotone_interval(const ct_fsrs *f) {
    long prev = ct_fsrs_next_interval_days(f, 0.1);
    long violations = 0;
    long count = 1;
    for (double s = 0.1 + 0.373; s <= 36500; s += 0.373) {
        long ivl = ct_fsrs_next_interval_days(f, s);
        if (ivl < prev || ivl < 1 || ivl > 36500) {
            violations++;
            if (violations < 5) {
                printf("  interval violation at s=%.3f ivl=%ld prev=%ld\n", s, ivl, prev);
            }
        }
        prev = ivl;
        count++;
    }
    printf("interval monotone violations=%ld over %ld points\n", violations, count);
    check(violations == 0, "interval monotone in stability and clamped to [1, max]");
}

static void random_review_sweep(const ct_fsrs *f) {
    long oob_d = 0;
    long oob_s = 0;
    long recall_shrink = 0;
    long forget_above_recall = 0;
    long d_changed_same_day = 0;
    for (int i = 0; i < 300000; i++) {
        ct_fsrs_card c = {0};
        c.state = s_next() % 2 ? 1 : 2;
        c.stability = s_uniform(0.1, 36500);
        c.difficulty = s_uniform(1, 10);
        c.last_review_day = 1000;
        ct_grade g = (ct_grade)(1 + s_next() % 4);
        double elapsed = s_uniform(0, 200);
        if (elapsed < 0) {
            elapsed = 0;
        }
        ct_fsrs_card out = ct_fsrs_review(f, &c, g, elapsed, 1100);
        if (out.difficulty < 1 || out.difficulty > 10) {
            oob_d++;
        }
        if (out.stability < 0.1 || out.stability > 36500) {
            oob_s++;
        }
        if (g >= CT_HARD && elapsed > 0 && out.stability < c.stability) {
            recall_shrink++;
        }
        if (g == CT_AGAIN && elapsed > 0) {
            ct_fsrs_card good = ct_fsrs_review(f, &c, CT_GOOD, elapsed, 1100);
            if (!(out.stability < good.stability)) {
                forget_above_recall++;
            }
        }
        ct_fsrs_card sd = ct_fsrs_review(f, &c, g, 0, 1000);
        if (sd.difficulty != c.difficulty) {
            d_changed_same_day++;
        }
        if (sd.stability < 0.1 || sd.stability > 36500) {
            oob_s++;
        }
    }
    printf("random sweep: oob difficulty=%ld oob stability=%ld recall-shrink=%ld "
           "forget>=recall=%ld same-day-delta-d=%ld\n",
           oob_d, oob_s, recall_shrink, forget_above_recall, d_changed_same_day);
    check(oob_d == 0, "difficulty bounded");
    check(oob_s == 0, "stability bounded for elapsed >= 0");
    check(recall_shrink == 0, "recall grades never shrink stability for elapsed > 0");
    check(forget_above_recall == 0, "forget stability below recall stability");
    check(d_changed_same_day == 0, "same-day leaves difficulty untouched");
}

static void negative_elapsed(const ct_fsrs *f) {
    static const double ss[6] = {0.1, 0.5, 1, 5, 25, 100};
    static const double tt[10] = {-0.2, -0.5, -1, -2, -3, -4, -4.2, -4.3, -5, -10};
    for (size_t k = 0; k < 6; k++) {
        for (size_t j = 0; j < 10; j++) {
            double r = ct_fsrs_retrievability(ss[k], tt[j]);
            ct_fsrs_card c = {0};
            c.state = 1;
            c.stability = ss[k];
            c.difficulty = 5;
            c.last_review_day = 1000;
            ct_fsrs_card out = ct_fsrs_review(f, &c, CT_GOOD, tt[j], 1000);
            long ivl = out.due_day - 1000;
            printf("S=%.1f t=%.1f R=%f out.S=%f ivl=%ld due=%ld nan_r=%d nan_s=%d\n", ss[k], tt[j],
                   r, out.stability, ivl, out.due_day, isnan(r), isnan(out.stability));
        }
    }
}

static void same_day_grade_boundaries(const ct_fsrs *f) {
    for (int i = 0; i < 50000; i++) {
        ct_fsrs_card c = {0};
        c.state = 1;
        c.stability = s_uniform(0.1, 36500);
        c.difficulty = i % 2 ? 1.0 : 10.0;
        c.last_review_day = 500;
        ct_grade g = (ct_grade)(1 + s_next() % 4);
        ct_fsrs_card out = ct_fsrs_review(f, &c, g, 0, 500);
        check(out.difficulty == c.difficulty, "boundary same-day D unchanged");
        check(out.stability >= 0.1 && out.stability <= 36500, "boundary same-day S in bounds");
        check(out.due_day == 500, "same-day due is today");
        ct_fsrs_card cross = ct_fsrs_review(f, &c, g, 0, 501);
        check(cross.difficulty >= 1 && cross.difficulty <= 10, "elapsed 0 next day D in bounds");
    }
    printf("same-day boundary sweep done\n");
}

int main(int argc, char **argv) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    if (argc > 1 && strcmp(argv[1], "nan") == 0) {
        negative_elapsed(&f);
        printf("nan section complete failures=%d\n", failures);
        return 0;
    }
    goldens(&f);
    monotone_retrievability();
    monotone_interval(&f);
    random_review_sweep(&f);
    same_day_grade_boundaries(&f);
    negative_elapsed(&f);
    printf("%s failures=%d\n", failures == 0 ? "ALL-OK" : "HAD-FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
