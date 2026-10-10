#include "framework.h"
#include "srs/fsrs.h"

#include <limits.h>
#include <math.h>

static ct_fsrs make_fsrs(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    return f;
}

static ct_fsrs_card fresh_card(void) {
    ct_fsrs_card c = {0};
    return c;
}

static ct_fsrs_card learning_card(void) {
    ct_fsrs_card c = {0};
    c.stability = 10.0;
    c.difficulty = 5.0;
    c.due_day = 104;
    c.last_review_day = 100;
    c.reps = 1;
    c.lapses = 0;
    c.state = 1;
    return c;
}

static ct_fsrs_card same_day_card(void) {
    ct_fsrs_card c = {0};
    c.stability = 10.0;
    c.difficulty = 5.0;
    c.due_day = 104;
    c.last_review_day = 104;
    c.reps = 2;
    c.lapses = 0;
    c.state = 1;
    return c;
}

static bool cards_equal(const ct_fsrs_card *a, const ct_fsrs_card *b) {
    return a->stability == b->stability && a->difficulty == b->difficulty &&
           a->due_day == b->due_day && a->last_review_day == b->last_review_day &&
           a->reps == b->reps && a->lapses == b->lapses && a->state == b->state;
}

CT_SUITE(fsrs_init) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    bool weights_ok = true;
    for (int i = 0; i < CT_FSRS_WEIGHT_COUNT; i++) {
        if (f.w[i] != cfg.fsrs.w[i]) {
            weights_ok = false;
        }
    }
    CT_ASSERT(weights_ok);
    CT_ASSERT_DBL_EQ(f.request_retention, CT_FSRS_REQUEST_RETENTION, 1e-9);
    CT_ASSERT_DBL_EQ(f.max_ivl_days, CT_FSRS_MAX_IVL_DAYS, 1e-9);

    cfg.fsrs.w[0] = 0.123456;
    cfg.fsrs.request_retention = 0.8;
    cfg.fsrs.max_ivl_days = 400.0;
    ct_fsrs g;
    ct_fsrs_init(&g, &cfg);
    CT_ASSERT_DBL_EQ(g.w[0], 0.123456, 1e-9);
    CT_ASSERT_DBL_EQ(g.request_retention, 0.8, 1e-9);
    CT_ASSERT_DBL_EQ(g.max_ivl_days, 400.0, 1e-9);
}

CT_SUITE(fsrs_golden_first_review) {
    static const double gold_s[4] = {0.40255, 1.18385, 3.173, 15.69105};
    static const double gold_d[4] = {7.1949, 6.488305268471453, 5.282434422319005,
                                     3.2245015893713678};
    ct_fsrs f = make_fsrs();
    for (int g = 1; g <= 4; g++) {
        ct_fsrs_card in = fresh_card();
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 0.0, 100);
        CT_ASSERT_DBL_EQ(out.stability, gold_s[g - 1], 1e-9);
        CT_ASSERT_DBL_EQ(out.difficulty, gold_d[g - 1], 1e-9);
        CT_ASSERT_EQ(out.state, 1);
        CT_ASSERT_EQ(out.reps, 1);
        CT_ASSERT_EQ(out.lapses, 0);
        CT_ASSERT_EQ(out.due_day, 100);
        CT_ASSERT_EQ(out.last_review_day, 100);
        CT_ASSERT(cards_equal(&in, &(ct_fsrs_card){0}));
    }
}

CT_SUITE(fsrs_golden_recall) {
    static const double gold_s[3] = {11.962060841460307, 19.791582142057077, 43.20991950624265};
    static const double gold_d[3] = {5.7994339073111085, 4.991832707311108, 4.184231507311108};
    static const long gold_i[3] = {12, 20, 43};
    ct_fsrs f = make_fsrs();
    ct_fsrs_card hard_in = learning_card();
    ct_fsrs_card hard_out = ct_fsrs_review(&f, &hard_in, CT_HARD, 4.0, 104);
    CT_ASSERT_DBL_EQ(hard_out.stability, gold_s[0], 1e-9);
    CT_ASSERT_DBL_EQ(hard_out.difficulty, gold_d[0], 1e-9);
    CT_ASSERT_EQ(hard_out.due_day, 104);
    CT_ASSERT_EQ(hard_out.state, 1);
    CT_ASSERT_EQ(hard_out.lapses, 0);
    for (int g = 3; g <= 4; g++) {
        ct_fsrs_card in = learning_card();
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 4.0, 104);
        CT_ASSERT_DBL_EQ(out.stability, gold_s[g - 2], 1e-9);
        CT_ASSERT_DBL_EQ(out.difficulty, gold_d[g - 2], 1e-9);
        CT_ASSERT_EQ(out.due_day, 104 + gold_i[g - 2]);
        CT_ASSERT_EQ(out.reps, 2);
        CT_ASSERT_EQ(out.lapses, 0);
        CT_ASSERT_EQ(out.state, 2);
        CT_ASSERT_EQ(out.last_review_day, 104);
        ct_fsrs_card expect_in = learning_card();
        CT_ASSERT(cards_equal(&in, &expect_in));
    }
}

CT_SUITE(fsrs_golden_forget) {
    ct_fsrs f = make_fsrs();
    ct_fsrs_card in = learning_card();
    ct_fsrs_card out = ct_fsrs_review(&f, &in, CT_AGAIN, 4.0, 104);
    CT_ASSERT_DBL_EQ(out.stability, 1.7994687509165919, 1e-9);
    CT_ASSERT_DBL_EQ(out.difficulty, 6.607035107311108, 1e-9);
    CT_ASSERT_EQ(out.due_day, 104);
    CT_ASSERT_EQ(out.reps, 2);
    CT_ASSERT_EQ(out.lapses, 0);
    CT_ASSERT_EQ(out.state, 1);
    CT_ASSERT_EQ(out.last_review_day, 104);
    ct_fsrs_card expect_in = learning_card();
    CT_ASSERT(cards_equal(&in, &expect_in));
}

CT_SUITE(fsrs_golden_same_day) {
    static const double gold_s[4] = {5.010285241935083, 8.398413743809318, 14.077712147375411,
                                     23.59754893600546};
    ct_fsrs f = make_fsrs();
    for (int g = 1; g <= 4; g++) {
        ct_fsrs_card in = same_day_card();
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 0.0, 104);
        CT_ASSERT_DBL_EQ(out.stability, gold_s[g - 1], 1e-9);
        CT_ASSERT_DBL_EQ(out.difficulty, 5.0, 1e-9);
        CT_ASSERT_EQ(out.due_day, 104);
        CT_ASSERT_EQ(out.last_review_day, 104);
        CT_ASSERT_EQ(out.reps, 3);
        CT_ASSERT_EQ(out.lapses, 0);
        CT_ASSERT_EQ(out.state, 1);
    }
    for (int g = 1; g < 4; g++) {
        ct_fsrs_card lo = same_day_card();
        ct_fsrs_card hi = same_day_card();
        ct_fsrs_card olo = ct_fsrs_review(&f, &lo, (ct_grade)g, 0.0, 104);
        ct_fsrs_card ohi = ct_fsrs_review(&f, &hi, (ct_grade)(g + 1), 0.0, 104);
        CT_ASSERT(olo.stability < ohi.stability);
    }
}

CT_SUITE(fsrs_golden_interval_table) {
    static const double stab[7] = {1.0, 5.0, 10.0, 50.0, 100.0, 365.0, 3650.0};
    static const long gold[7] = {1, 5, 10, 50, 100, 365, 3650};
    ct_fsrs f = make_fsrs();
    for (int i = 0; i < 7; i++) {
        CT_ASSERT_EQ(ct_fsrs_next_interval_days(&f, stab[i]), gold[i]);
    }
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(10.0, 4.0), 0.9561491614605041, 1e-9);
}

CT_SUITE(fsrs_retrievability) {
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(10.0, 0.0), 1.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(0.1, 0.0), 1.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(36500.0, 0.0), 1.0, 1e-12);
    bool in_range = true;
    for (double s = 0.1; s <= 1e5; s *= 3.7) {
        for (double t = 0.25; t <= 4096.0; t *= 2.0) {
            double r = ct_fsrs_retrievability(s, t);
            if (!(r > 0.0 && r < 1.0)) {
                in_range = false;
            }
        }
    }
    CT_ASSERT(in_range);
    bool decreasing = true;
    double prev = ct_fsrs_retrievability(7.3, 0.0);
    for (int step = 1; step <= 2000; step++) {
        double r = ct_fsrs_retrievability(7.3, (double)step * 0.25);
        if (!(r < prev)) {
            decreasing = false;
        }
        prev = r;
    }
    CT_ASSERT(decreasing);
}

CT_SUITE(fsrs_stability_properties) {
    ct_fsrs f = make_fsrs();
    bool grows = true;
    for (int g = 2; g <= 4; g++) {
        for (int di = 0; di < 10; di++) {
            ct_fsrs_card c = learning_card();
            c.difficulty = 1.0 + (double)di;
            ct_fsrs_card out = ct_fsrs_review(&f, &c, (ct_grade)g, 4.0, 104);
            if (!(out.stability > c.stability)) {
                grows = false;
            }
        }
    }
    CT_ASSERT(grows);
    bool forget_below = true;
    for (int di = 0; di < 10; di++) {
        for (int ei = 1; ei <= 6; ei++) {
            ct_fsrs_card good = learning_card();
            ct_fsrs_card again = learning_card();
            good.difficulty = 1.0 + (double)di;
            again.difficulty = 1.0 + (double)di;
            double elapsed = (double)ei * 7.0;
            ct_fsrs_card og = ct_fsrs_review(&f, &good, CT_GOOD, elapsed, 100 + ei * 7);
            ct_fsrs_card oa = ct_fsrs_review(&f, &again, CT_AGAIN, elapsed, 100 + ei * 7);
            if (!(oa.stability < og.stability)) {
                forget_below = false;
            }
        }
    }
    CT_ASSERT(forget_below);
}

CT_SUITE(fsrs_interval_properties) {
    ct_fsrs f = make_fsrs();
    bool nondecreasing = true;
    long prev = 0;
    for (double s = 0.1; s <= 1e10; s *= 1.3) {
        long ivl = ct_fsrs_next_interval_days(&f, s);
        if (ivl < prev || ivl < 1) {
            nondecreasing = false;
        }
        prev = ivl;
    }
    CT_ASSERT(nondecreasing);
    CT_ASSERT_EQ(ct_fsrs_next_interval_days(&f, 0.1), 1);
    CT_ASSERT_EQ(ct_fsrs_next_interval_days(&f, 0.4), 1);
    CT_ASSERT_EQ(ct_fsrs_next_interval_days(&f, 1e12), 36500);
    ct_fsrs capped = make_fsrs();
    capped.max_ivl_days = 100.0;
    CT_ASSERT_EQ(ct_fsrs_next_interval_days(&capped, 1000.0), 100);
    CT_ASSERT_EQ(ct_fsrs_next_interval_days(&capped, 1e9), 100);
}

CT_SUITE(fsrs_branch_selection) {
    ct_fsrs f = make_fsrs();
    ct_fsrs_card stale = learning_card();
    ct_fsrs_card out = ct_fsrs_review(&f, &stale, CT_GOOD, 0.0, 104);
    CT_ASSERT(out.due_day > 104);
    CT_ASSERT(out.difficulty < 5.0);
    CT_ASSERT_DBL_EQ(out.stability, 10.0, 1e-9);
    CT_ASSERT_EQ(out.lapses, 0);
    CT_ASSERT_EQ(out.state, 2);
}

static ct_fsrs_card state_card(int state) {
    ct_fsrs_card c = learning_card();
    c.state = state;
    return c;
}

CT_SUITE(fsrs_state_machine) {
    ct_fsrs f = make_fsrs();
    for (int g = 1; g <= 4; g++) {
        ct_fsrs_card in = fresh_card();
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 0.0, 100);
        CT_ASSERT_EQ(out.state, 1);
        CT_ASSERT_EQ(out.due_day, 100);
        CT_ASSERT_EQ(out.lapses, 0);
    }
    static const int study_states[] = {1, 3};
    for (size_t i = 0; i < sizeof study_states / sizeof study_states[0]; i++) {
        ct_fsrs_card again = state_card(study_states[i]);
        ct_fsrs_card out_again = ct_fsrs_review(&f, &again, CT_AGAIN, 4.0, 104);
        CT_ASSERT_EQ(out_again.state, study_states[i]);
        CT_ASSERT_EQ(out_again.due_day, 104);
        CT_ASSERT_EQ(out_again.lapses, 0);
        ct_fsrs_card hard = state_card(study_states[i]);
        ct_fsrs_card out_hard = ct_fsrs_review(&f, &hard, CT_HARD, 4.0, 104);
        CT_ASSERT_EQ(out_hard.state, study_states[i]);
        CT_ASSERT_EQ(out_hard.due_day, 104);
        CT_ASSERT_EQ(out_hard.lapses, 0);
        for (int g = CT_GOOD; g <= CT_EASY; g++) {
            ct_fsrs_card in = state_card(study_states[i]);
            ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 4.0, 104);
            CT_ASSERT_EQ(out.state, 2);
            CT_ASSERT_EQ(out.due_day, 104 + ct_fsrs_next_interval_days(&f, out.stability));
            CT_ASSERT_EQ(out.lapses, 0);
        }
    }
    ct_fsrs_card lapse = state_card(2);
    ct_fsrs_card out_lapse = ct_fsrs_review(&f, &lapse, CT_AGAIN, 4.0, 104);
    CT_ASSERT_EQ(out_lapse.state, 3);
    CT_ASSERT_EQ(out_lapse.due_day, 104);
    CT_ASSERT_EQ(out_lapse.lapses, 1);
    for (int g = CT_HARD; g <= CT_EASY; g++) {
        ct_fsrs_card in = state_card(2);
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 4.0, 104);
        CT_ASSERT_EQ(out.state, 2);
        CT_ASSERT_EQ(out.due_day, 104 + ct_fsrs_next_interval_days(&f, out.stability));
        CT_ASSERT_EQ(out.lapses, 0);
    }
    ct_fsrs_card same_learning = state_card(1);
    same_learning.last_review_day = 104;
    ct_fsrs_card out_sl = ct_fsrs_review(&f, &same_learning, CT_EASY, 0.0, 104);
    CT_ASSERT_EQ(out_sl.state, 1);
    CT_ASSERT_EQ(out_sl.due_day, 104);
    CT_ASSERT_EQ(out_sl.lapses, 0);
    ct_fsrs_card same_relearning = state_card(3);
    same_relearning.last_review_day = 104;
    ct_fsrs_card out_sr = ct_fsrs_review(&f, &same_relearning, CT_GOOD, 0.0, 104);
    CT_ASSERT_EQ(out_sr.state, 3);
    CT_ASSERT_EQ(out_sr.due_day, 104);
    CT_ASSERT_EQ(out_sr.lapses, 0);
    ct_fsrs_card same_review = state_card(2);
    same_review.last_review_day = 104;
    ct_fsrs_card out_sv = ct_fsrs_review(&f, &same_review, CT_GOOD, 0.0, 104);
    CT_ASSERT_EQ(out_sv.state, 2);
    CT_ASSERT_EQ(out_sv.due_day, 104 + ct_fsrs_next_interval_days(&f, out_sv.stability));
    CT_ASSERT_EQ(out_sv.lapses, 0);
    ct_fsrs_card same_lapse = state_card(2);
    same_lapse.last_review_day = 104;
    ct_fsrs_card out_sla = ct_fsrs_review(&f, &same_lapse, CT_AGAIN, 0.0, 104);
    CT_ASSERT_EQ(out_sla.state, 3);
    CT_ASSERT_EQ(out_sla.due_day, 104);
    CT_ASSERT_EQ(out_sla.lapses, 1);
}

CT_SUITE(fsrs_clamp_proof) {
    ct_fsrs f = make_fsrs();
    ct_fsrs_card hot = learning_card();
    hot.difficulty = 42.0;
    ct_fsrs_card hot_out = ct_fsrs_review(&f, &hot, CT_GOOD, 5.0, 110);
    CT_ASSERT(hot_out.difficulty >= CT_FSRS_MIN_DIFFICULTY);
    CT_ASSERT(hot_out.difficulty <= CT_FSRS_MAX_DIFFICULTY);
    ct_fsrs_card cold = learning_card();
    cold.difficulty = -9.5;
    ct_fsrs_card cold_out = ct_fsrs_review(&f, &cold, CT_HARD, 5.0, 110);
    CT_ASSERT(cold_out.difficulty >= CT_FSRS_MIN_DIFFICULTY);
    CT_ASSERT(cold_out.difficulty <= CT_FSRS_MAX_DIFFICULTY);
    ct_fsrs_card big = learning_card();
    big.stability = 1e12;
    ct_fsrs_card big_out = ct_fsrs_review(&f, &big, CT_GOOD, 4.0, 104);
    CT_ASSERT(big_out.stability <= CT_FSRS_MAX_IVL_DAYS);
    CT_ASSERT(big_out.stability >= CT_FSRS_MIN_STABILITY);
    ct_fsrs_card tiny = learning_card();
    tiny.stability = 0.001;
    ct_fsrs_card tiny_out = ct_fsrs_review(&f, &tiny, CT_AGAIN, 4.0, 104);
    CT_ASSERT(tiny_out.stability >= CT_FSRS_MIN_STABILITY);
    CT_ASSERT(tiny_out.stability <= CT_FSRS_MAX_IVL_DAYS);
}

CT_SUITE(fsrs_nonfinite_difficulty) {
    ct_fsrs f = make_fsrs();
    static const double bad[] = {INFINITY, -INFINITY, NAN};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        for (int g = 1; g <= 4; g++) {
            ct_fsrs_card in = learning_card();
            in.difficulty = bad[i];
            ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 4.0, 104);
            CT_ASSERT(isfinite(out.difficulty));
            CT_ASSERT(out.difficulty >= CT_FSRS_MIN_DIFFICULTY);
            CT_ASSERT(out.difficulty <= CT_FSRS_MAX_DIFFICULTY);
            CT_ASSERT(isfinite(out.stability));
            CT_ASSERT(out.stability >= CT_FSRS_MIN_STABILITY);
            CT_ASSERT(out.stability <= CT_FSRS_MAX_IVL_DAYS);
            CT_ASSERT(out.due_day >= 104);
        }
        ct_fsrs_card same = same_day_card();
        same.difficulty = bad[i];
        ct_fsrs_card out_same = ct_fsrs_review(&f, &same, CT_GOOD, 0.0, 104);
        CT_ASSERT(isfinite(out_same.difficulty));
        CT_ASSERT(out_same.difficulty >= CT_FSRS_MIN_DIFFICULTY);
        CT_ASSERT(out_same.difficulty <= CT_FSRS_MAX_DIFFICULTY);
    }
}

CT_SUITE(fsrs_negative_elapsed) {
    ct_fsrs f = make_fsrs();
    ct_fsrs_card in = learning_card();
    ct_fsrs_card out = ct_fsrs_review(&f, &in, CT_GOOD, -1.0, 104);
    CT_ASSERT(isfinite(out.stability));
    CT_ASSERT(isfinite(out.difficulty));
    CT_ASSERT_DBL_EQ(out.stability, 10.0, 1e-9);
    CT_ASSERT_EQ(out.due_day, 104 + ct_fsrs_next_interval_days(&f, out.stability));
    ct_fsrs_card deep = learning_card();
    ct_fsrs_card deep_out = ct_fsrs_review(&f, &deep, CT_GOOD, -1e9, 104);
    CT_ASSERT_DBL_EQ(deep_out.stability, out.stability, 0.0);
    CT_ASSERT_DBL_EQ(deep_out.difficulty, out.difficulty, 0.0);
    CT_ASSERT_EQ(deep_out.due_day, out.due_day);
    CT_ASSERT(isfinite(ct_fsrs_retrievability(0.1, -1.0)));
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(0.1, -1.0), 1.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(0.1, -1e9), 1.0, 1e-12);
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(0.1, -7.5), ct_fsrs_retrievability(0.1, 0.0), 0.0);
    ct_fsrs_card back = same_day_card();
    ct_fsrs_card back_out = ct_fsrs_review(&f, &back, CT_GOOD, -0.25, 104);
    CT_ASSERT(back_out.difficulty < back.difficulty);
    CT_ASSERT(back_out.due_day >= 105);
    CT_ASSERT_DBL_EQ(back_out.stability, 10.0, 1e-9);
}

CT_SUITE(fsrs_today_overflow) {
    ct_fsrs f = make_fsrs();
    ct_fsrs_card in = learning_card();
    in.stability = 3.173;
    in.last_review_day = LONG_MAX - 40000;
    in.due_day = LONG_MAX - 40000;
    ct_fsrs_card out = ct_fsrs_review(&f, &in, CT_GOOD, 4.0, LONG_MAX);
    CT_ASSERT(out.due_day > 0);
    CT_ASSERT_EQ(out.due_day, LONG_MAX);
    ct_fsrs_card near = learning_card();
    ct_fsrs_card near_out = ct_fsrs_review(&f, &near, CT_GOOD, 4.0, LONG_MAX - 40000);
    CT_ASSERT_EQ(near_out.due_day,
                 LONG_MAX - 40000 + ct_fsrs_next_interval_days(&f, near_out.stability));
    ct_fsrs_card same_edge = learning_card();
    same_edge.last_review_day = LONG_MAX;
    ct_fsrs_card same_out = ct_fsrs_review(&f, &same_edge, CT_GOOD, 0.0, LONG_MAX);
    CT_ASSERT_EQ(same_out.due_day, LONG_MAX);
}

CT_SUITE(fsrs_invalid_grade) {
    ct_fsrs f = make_fsrs();
    static const int bad[] = {0, -1, -99, 5, 6, 9, 99};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        ct_fsrs_card in = learning_card();
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)bad[i], 4.0, 104);
        CT_ASSERT(cards_equal(&out, &in));
        ct_fsrs_card fresh = fresh_card();
        ct_fsrs_card born = ct_fsrs_review(&f, &fresh, (ct_grade)bad[i], 0.0, 100);
        CT_ASSERT(cards_equal(&born, &fresh));
    }
    ct_fsrs_card in = learning_card();
    ct_fsrs_card again = ct_fsrs_review(&f, &in, CT_AGAIN, 4.0, 104);
    CT_ASSERT(again.reps == in.reps + 1);
    ct_fsrs_card easy = ct_fsrs_review(&f, &in, CT_EASY, 4.0, 104);
    CT_ASSERT(easy.reps == in.reps + 1);
}

CT_SUITE(fsrs_degenerate_stability) {
    ct_fsrs f = make_fsrs();
    static const double degenerate[] = {0.0, -3.0, 0.5 * CT_FSRS_MIN_STABILITY};
    for (size_t i = 0; i < sizeof degenerate / sizeof degenerate[0]; i++) {
        for (int g = 1; g <= 4; g++) {
            ct_fsrs_card zero = learning_card();
            zero.stability = degenerate[i];
            ct_fsrs_card out = ct_fsrs_review(&f, &zero, (ct_grade)g, 4.0, 104);
            CT_ASSERT(isfinite(out.stability));
            CT_ASSERT(out.stability >= CT_FSRS_MIN_STABILITY);
            CT_ASSERT(out.stability <= CT_FSRS_MAX_IVL_DAYS);
            CT_ASSERT(isfinite(out.difficulty));
            CT_ASSERT(out.due_day >= 104);
        }
        ct_fsrs_card same = same_day_card();
        same.stability = degenerate[i];
        ct_fsrs_card out = ct_fsrs_review(&f, &same, CT_GOOD, 0.0, 104);
        CT_ASSERT(isfinite(out.stability));
        CT_ASSERT(out.stability >= CT_FSRS_MIN_STABILITY);
    }
    CT_ASSERT(isfinite(ct_fsrs_retrievability(0.0, 4.0)));
    CT_ASSERT(isfinite(ct_fsrs_retrievability(-7.0, 4.0)));
    CT_ASSERT_DBL_EQ(ct_fsrs_retrievability(0.0, 4.0),
                     ct_fsrs_retrievability(CT_FSRS_MIN_STABILITY, 4.0), 0.0);
}

CT_SUITE(fsrs_interval_boundaries) {
    ct_fsrs cap = make_fsrs();
    cap.request_retention = 0.6;
    cap.max_ivl_days = pow(0.6, 1.0 / CT_FSRS_DECAY) - 1.0;
    CT_ASSERT_EQ(ct_fsrs_next_interval_days(&cap, CT_FSRS_FACTOR), 2);

    ct_fsrs floor_cap = make_fsrs();
    floor_cap.request_retention = 0.5;
    floor_cap.max_ivl_days = 0.5;
    double edge = nextafter(1.0 / 3.0, 2.0) * CT_FSRS_FACTOR;
    CT_ASSERT_EQ(ct_fsrs_next_interval_days(&floor_cap, edge), 0);
}

CT_SUITE(fsrs_random_walk) {
    ct_fsrs f = make_fsrs();
    ct_sfc64 rng;
    ct_sfc64_init(&rng, 20261007);
    ct_fsrs_card card = fresh_card();
    int expected_lapses = 0;
    long today = 500;
    bool d_ok = true;
    bool s_ok = true;
    bool reps_ok = true;
    bool bookkeeping_ok = true;
    bool immutability_ok = true;
    bool lapses_ok = true;
    bool state_ok = true;
    for (int i = 0; i < 200; i++) {
        bool same_day =
            card.state != 0 && card.last_review_day == today && ct_sfc64_range(&rng, 4) == 0;
        ct_grade g = (ct_grade)(1 + (int)ct_sfc64_range(&rng, 4));
        double elapsed = 0.0;
        if (!same_day) {
            elapsed = 1.0 + (double)ct_sfc64_range(&rng, 30);
            today += (long)elapsed;
        }
        ct_fsrs_card before = card;
        ct_fsrs_card out = ct_fsrs_review(&f, &card, g, elapsed, today);
        if (!cards_equal(&card, &before)) {
            immutability_ok = false;
        }
        card = out;
        if (card.difficulty < CT_FSRS_MIN_DIFFICULTY || card.difficulty > CT_FSRS_MAX_DIFFICULTY) {
            d_ok = false;
        }
        if (card.stability < CT_FSRS_MIN_STABILITY || card.stability > CT_FSRS_MAX_IVL_DAYS) {
            s_ok = false;
        }
        if (card.reps != i + 1) {
            reps_ok = false;
        }
        if (card.last_review_day != today) {
            bookkeeping_ok = false;
        }
        int expect_state = before.state;
        long expect_due = today;
        if (before.state == 0) {
            expect_state = 1;
        } else if (same_day) {
            if (before.state == 2 && g == CT_AGAIN) {
                expect_state = 3;
                expected_lapses++;
            } else if (before.state == 2) {
                expect_due = today + ct_fsrs_next_interval_days(&f, card.stability);
            }
        } else if (g == CT_AGAIN) {
            if (before.state == 2) {
                expect_state = 3;
                expected_lapses++;
            }
        } else if (g >= CT_GOOD || before.state == 2) {
            expect_state = 2;
            expect_due = today + ct_fsrs_next_interval_days(&f, card.stability);
        }
        if (card.state != expect_state) {
            state_ok = false;
        }
        if (card.due_day != expect_due) {
            bookkeeping_ok = false;
        }
        if (card.lapses != expected_lapses) {
            lapses_ok = false;
        }
    }
    CT_ASSERT(d_ok);
    CT_ASSERT(s_ok);
    CT_ASSERT(reps_ok);
    CT_ASSERT(bookkeeping_ok);
    CT_ASSERT(immutability_ok);
    CT_ASSERT(lapses_ok);
    CT_ASSERT(state_ok);
    CT_ASSERT(card.reps == 200);
    CT_ASSERT(card.state >= 1);
    CT_ASSERT(card.state <= 3);
}
