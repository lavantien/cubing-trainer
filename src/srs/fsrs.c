#include "srs/fsrs.h"

#include <math.h>

enum fsrs_weight {
    fw_s_init, fw_s_init_hard, fw_s_init_good, fw_s_init_easy,
    fw_d_init, fw_d_slope, fw_d_delta, fw_d_revert,
    fw_s_gain, fw_s_tension, fw_s_press,
    fw_f_init, fw_f_damp, fw_f_grow, fw_f_fuzz,
    fw_hard_penalty, fw_easy_bonus,
    fw_sd_gain, fw_sd_bias
};

static double clamp_lo_hi(double x, double lo, double hi) {
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

static double clamp_stability(double s) {
    return clamp_lo_hi(s, CT_FSRS_MIN_STABILITY, CT_FSRS_MAX_IVL_DAYS);
}

static double clamp_difficulty(double d) {
    return clamp_lo_hi(d, CT_FSRS_MIN_DIFFICULTY, CT_FSRS_MAX_DIFFICULTY);
}

static double difficulty_first(const double *w, ct_grade g) {
    return clamp_difficulty(w[fw_d_init] - exp(w[fw_d_slope] * ((int)g - 1)) + 1);
}

static double difficulty_easy_anchor(const double *w) {
    return clamp_difficulty(w[fw_d_init] - exp(w[fw_d_slope] * (CT_EASY - 1)) + 1);
}

static double difficulty_next(const double *w, double d, ct_grade g) {
    double delta = -w[fw_d_delta] * ((int)g - CT_GOOD);
    double shifted = d + delta * (CT_FSRS_MAX_DIFFICULTY - d) /
                             (CT_FSRS_MAX_DIFFICULTY - CT_FSRS_MIN_DIFFICULTY);
    return clamp_difficulty(w[fw_d_revert] * difficulty_easy_anchor(w) +
                            (1 - w[fw_d_revert]) * shifted);
}

static double stability_recall(const double *w, double s, double d, double r, ct_grade g) {
    double hard = (g == CT_HARD) ? w[fw_hard_penalty] : 1;
    double easy = (g == CT_EASY) ? w[fw_easy_bonus] : 1;
    double inc = exp(w[fw_s_gain]) * (CT_FSRS_MAX_DIFFICULTY + 1 - d) *
                 pow(s, -w[fw_s_tension]) * (exp(w[fw_s_press] * (1 - r)) - 1) * hard * easy;
    return s * (1 + inc);
}

static double stability_forget(const double *w, double s, double d, double r) {
    return w[fw_f_init] * pow(d, -w[fw_f_damp]) * (pow(s + 1, w[fw_f_grow]) - 1) *
           exp(w[fw_f_fuzz] * (1 - r));
}

static double stability_same_day(const double *w, double s, ct_grade g) {
    return s * exp(w[fw_sd_gain] * ((int)g - CT_GOOD + w[fw_sd_bias]));
}

void ct_fsrs_init(ct_fsrs *f, const ct_config *cfg) {
    for (int i = 0; i < CT_FSRS_WEIGHT_COUNT; i++) {
        f->w[i] = cfg->fsrs.w[i];
    }
    f->request_retention = cfg->fsrs.request_retention;
    f->max_ivl_days = cfg->fsrs.max_ivl_days;
}

double ct_fsrs_retrievability(double stability, double elapsed_days) {
    return pow(1 + CT_FSRS_FACTOR * elapsed_days / stability, CT_FSRS_DECAY);
}

long ct_fsrs_next_interval_days(const ct_fsrs *f, double stability) {
    double raw = (stability / CT_FSRS_FACTOR) *
                 (pow(f->request_retention, 1 / CT_FSRS_DECAY) - 1);
    if (raw < 1) {
        return 1;
    }
    if (raw > f->max_ivl_days) {
        return (long)f->max_ivl_days;
    }
    return (long)round(raw);
}

ct_fsrs_card ct_fsrs_review(const ct_fsrs *f, const ct_fsrs_card *card, ct_grade g,
                            double elapsed_days, long today_day) {
    ct_fsrs_card out = *card;
    out.reps = card->reps + 1;
    out.last_review_day = today_day;
    if (card->state == 0) {
        out.stability = clamp_stability(f->w[(int)g - 1]);
        out.difficulty = difficulty_first(f->w, g);
        out.state = 1;
        out.due_day = today_day;
        return out;
    }
    if (elapsed_days == 0 && card->last_review_day == today_day) {
        out.stability = clamp_stability(stability_same_day(f->w, card->stability, g));
        out.due_day = today_day;
        return out;
    }
    double r = ct_fsrs_retrievability(card->stability, elapsed_days);
    out.difficulty = difficulty_next(f->w, card->difficulty, g);
    if (g >= CT_HARD) {
        out.stability =
            clamp_stability(stability_recall(f->w, card->stability, out.difficulty, r, g));
    } else {
        out.stability =
            clamp_stability(stability_forget(f->w, card->stability, out.difficulty, r));
        out.lapses = card->lapses + 1;
    }
    out.due_day = today_day + ct_fsrs_next_interval_days(f, out.stability);
    return out;
}
