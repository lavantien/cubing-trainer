#include "framework.h"
#include "srs/fsrs.h"

#include <limits.h>
#include <math.h>

int ct_fuzz_iterations(void);

enum {
    CT_FSR_WALK_MAX = 500,
    CT_FSR_TODAY_SPAN = 100000,
    CT_FSR_ELAPSED_SPAN = 200,
    CT_FSR_PROBE_SPAN = 5000,
    CT_FSR_GRADES = 4,
    CT_FSR_STABILITY_KINDS = 6,
    CT_FSR_DIFFICULTY_KINDS = 5,
    CT_FSR_TODAY_POOL = 10
};

static double ct_fsr_today_max_add(void) {
    ct_config cfg = ct_config_defaults();
    return cfg.fsrs.max_ivl_days;
}

#define CT_FZ(cond, seed)                                                                          \
    do {                                                                                           \
        if (cond) {                                                                                \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_eq(__FILE__, __LINE__, #cond, "seed", (unsigned long long)(seed), 0);          \
        }                                                                                          \
    } while (0)

static ct_fsrs ct_fsr_engine(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    return f;
}

static double ct_fsr_stability(ct_sfc64 *s) {
    switch ((int)ct_sfc64_range(s, CT_FSR_STABILITY_KINDS)) {
    case 0:
        return CT_FSRS_MIN_STABILITY;
    case 1:
        return CT_FSRS_MAX_IVL_DAYS;
    case 2:
        return 1.0;
    case 3:
        return 0.1 * exp(ct_sfc64_uniform(s) * log(CT_FSRS_MAX_DIFFICULTY * CT_FSRS_MAX_IVL_DAYS));
    case 4:
        return CT_FSRS_MIN_STABILITY + ct_sfc64_uniform(s) * 0.5;
    default:
        return exp(ct_sfc64_uniform(s) * log(CT_FSRS_MAX_IVL_DAYS));
    }
}

static double ct_fsr_difficulty(ct_sfc64 *s) {
    switch ((int)ct_sfc64_range(s, CT_FSR_DIFFICULTY_KINDS)) {
    case 0:
        return CT_FSRS_MIN_DIFFICULTY;
    case 1:
        return CT_FSRS_MAX_DIFFICULTY;
    case 2:
        return 5.0;
    case 3:
        return CT_FSRS_MAX_DIFFICULTY - ct_sfc64_uniform(s) * 0.5;
    default:
        return CT_FSRS_MIN_DIFFICULTY +
               ct_sfc64_uniform(s) * (CT_FSRS_MAX_DIFFICULTY - CT_FSRS_MIN_DIFFICULTY);
    }
}

static ct_fsrs_card ct_fsr_card(ct_sfc64 *s, long today) {
    ct_fsrs_card c = {0};
    c.reps = (int)ct_sfc64_range(s, 60);
    c.lapses = (int)ct_sfc64_range(s, 30);
    if (ct_sfc64_range(s, 4) == 0) {
        c.state = 0;
        c.last_review_day = today;
        return c;
    }
    c.state = 1 + (int)ct_sfc64_range(s, 3);
    c.stability = ct_fsr_stability(s);
    c.difficulty = ct_fsr_difficulty(s);
    if (ct_sfc64_range(s, 3) == 0) {
        c.last_review_day = today < LONG_MIN + 64 ? today : today - 1 - (long)ct_sfc64_range(s, 30);
    } else {
        c.last_review_day = today;
    }
    c.due_day = today > LONG_MAX - 64 ? today : c.last_review_day + (long)ct_sfc64_range(s, 50);
    return c;
}

static bool ct_fsr_finite(double x) {
    return x == x && x > -INFINITY && x < INFINITY;
}

static bool ct_fsr_card_sane(const ct_fsrs_card *c, long today) {
    bool due_ok = today == LONG_MIN || c->due_day >= today - 1;
    return ct_fsr_finite(c->stability) && c->stability >= CT_FSRS_MIN_STABILITY &&
           c->stability <= CT_FSRS_MAX_IVL_DAYS && ct_fsr_finite(c->difficulty) &&
           c->difficulty >= CT_FSRS_MIN_DIFFICULTY && c->difficulty <= CT_FSRS_MAX_DIFFICULTY &&
           due_ok && c->state >= 1 && c->state <= 3;
}

CT_SUITE(fuzz_srs_walk) {
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    ct_fsrs f = ct_fsr_engine();
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0x5F5B0000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        long today = (long)ct_sfc64_range(&s, CT_FSR_TODAY_SPAN);
        ct_fsrs_card card = ct_fsr_card(&s, today);
        int steps = 1 + (int)ct_sfc64_range(&s, CT_FSR_WALK_MAX);
        for (int k = 0; k < steps; k++) {
            bool fresh = card.state == 0;
            bool same_day = !fresh && card.last_review_day == today && ct_sfc64_range(&s, 4) == 0;
            ct_grade g = (ct_grade)(1 + (int)ct_sfc64_range(&s, CT_FSR_GRADES));
            double elapsed = 0.0;
            if (!same_day) {
                elapsed = 1.0 + (double)ct_sfc64_range(&s, CT_FSR_ELAPSED_SPAN) +
                          0.5 * ct_sfc64_uniform(&s);
                today += (long)elapsed;
            }
            int prev_lapses = card.lapses;
            double prev_difficulty = card.difficulty;
            ct_fsrs_card out = ct_fsrs_review(&f, &card, g, elapsed, today);
            CT_FZ(ct_fsr_card_sane(&out, today), seed);
            CT_FZ(out.reps == card.reps + 1, seed);
            CT_FZ(out.last_review_day == today, seed);
            long ivl = ct_fsrs_next_interval_days(&f, out.stability);
            CT_FZ(ivl >= 1, seed);
            CT_FZ((double)ivl <= f.max_ivl_days, seed);
            if (fresh) {
                CT_FZ(out.due_day == today, seed);
                CT_FZ(out.state == 1, seed);
                CT_FZ(out.lapses == prev_lapses, seed);
            } else if (same_day) {
                if (card.state == 2 && g == CT_AGAIN) {
                    CT_FZ(out.due_day == today, seed);
                    CT_FZ(out.state == 3, seed);
                    CT_FZ(out.difficulty == prev_difficulty, seed);
                    CT_FZ(out.lapses == prev_lapses + 1, seed);
                } else if (card.state == 2) {
                    CT_FZ(out.due_day == today + ivl, seed);
                    CT_FZ(out.state == 2, seed);
                    CT_FZ(out.difficulty == prev_difficulty, seed);
                    CT_FZ(out.lapses == prev_lapses, seed);
                } else {
                    CT_FZ(out.due_day == today, seed);
                    CT_FZ(out.state == card.state, seed);
                    CT_FZ(out.difficulty == prev_difficulty, seed);
                    CT_FZ(out.lapses == prev_lapses, seed);
                }
            } else if (g == CT_AGAIN) {
                CT_FZ(out.due_day == today, seed);
                CT_FZ(out.state == (card.state == 2 ? 3 : card.state), seed);
                CT_FZ(out.lapses == prev_lapses + (card.state == 2 ? 1 : 0), seed);
            } else if (g == CT_HARD && card.state != 2) {
                CT_FZ(out.due_day == today, seed);
                CT_FZ(out.state == card.state, seed);
                CT_FZ(out.lapses == prev_lapses, seed);
            } else {
                CT_FZ(out.due_day == today + ivl, seed);
                CT_FZ(out.state == 2, seed);
                CT_FZ(out.lapses == prev_lapses, seed);
            }
            double r_short = ct_fsrs_retrievability(out.stability, 1.0);
            double r_long = ct_fsrs_retrievability(out.stability, 100.0);
            CT_FZ(ct_fsr_finite(r_short) && r_short > 0.0 && r_short < 1.0, seed);
            CT_FZ(ct_fsr_finite(r_long) && r_long > 0.0 && r_long < 1.0, seed);
            card = out;
        }
    }
}

static double ct_fsr_elapsed_draw(ct_sfc64 *s, double stability) {
    static const double pool[] = {0.0, 1.0, 0.5, 1e9, 1e12, 100000.0, 12345.75, -0.25};
    switch ((int)ct_sfc64_range(s, 6)) {
    case 0:
        return pool[ct_sfc64_range(s, sizeof pool / sizeof pool[0])];
    case 1:
        return 0.0;
    case 2:
        return ct_sfc64_uniform(s) * CT_FSR_PROBE_SPAN;
    case 3:
        return 1.0 + (double)ct_sfc64_range(s, 1000000);
    case 4:
        return -ct_sfc64_uniform(s) * 0.9 * stability / CT_FSRS_FACTOR;
    default:
        return -0.25 - ct_sfc64_uniform(s) * 0.1;
    }
}

static long ct_fsr_today_draw(ct_sfc64 *s) {
    static const long pool[CT_FSR_TODAY_POOL] = {
        0,        1,          42,        -1000000, LONG_MAX, LONG_MAX - 1, LONG_MAX - 36500,
        LONG_MIN, 2147447146, 2147483000};
    if (ct_sfc64_range(s, 2) == 0) {
        return pool[ct_sfc64_range(s, CT_FSR_TODAY_POOL)];
    }
    return (long)ct_sfc64_range(s, 1u << 30);
}

CT_SUITE(fuzz_srs_elapsed) {
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    ct_fsrs f = ct_fsr_engine();
    double max_add = ct_fsr_today_max_add();
    long add_guard = LONG_MAX - (long)max_add - 1;
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0xE1A50000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        long today = ct_fsr_today_draw(&s);
        ct_fsrs_card card = ct_fsr_card(&s, today);
        double elapsed = ct_fsr_elapsed_draw(&s, card.stability);
        bool adds_interval = card.state != 0 && !(elapsed == 0.0 && card.last_review_day == today);
        if (adds_interval && today > add_guard) {
            card.last_review_day = today;
            elapsed = 0.0;
            adds_interval = false;
        }
        ct_grade g = (ct_grade)(1 + (int)ct_sfc64_range(&s, CT_FSR_GRADES));
        ct_fsrs_card out = ct_fsrs_review(&f, &card, g, elapsed, today);
        CT_FZ(ct_fsr_card_sane(&out, today), seed);
        CT_FZ(out.reps == card.reps + 1, seed);
        CT_FZ(out.last_review_day == today, seed);
        long ivl = ct_fsrs_next_interval_days(&f, out.stability);
        long interval_due = today > LONG_MAX - ivl ? LONG_MAX : today + ivl;
        if (card.state == 0) {
            CT_FZ(out.due_day == today, seed);
            CT_FZ(out.state == 1, seed);
            CT_FZ(out.lapses == card.lapses, seed);
        } else if (elapsed == 0.0 && card.last_review_day == today) {
            CT_FZ(out.difficulty == card.difficulty, seed);
            if (card.state == 2 && g == CT_AGAIN) {
                CT_FZ(out.due_day == today, seed);
                CT_FZ(out.state == 3, seed);
                CT_FZ(out.lapses == card.lapses + 1, seed);
            } else if (card.state == 2) {
                CT_FZ(out.due_day == interval_due, seed);
                CT_FZ(out.state == 2, seed);
                CT_FZ(out.lapses == card.lapses, seed);
            } else {
                CT_FZ(out.due_day == today, seed);
                CT_FZ(out.state == card.state, seed);
                CT_FZ(out.lapses == card.lapses, seed);
            }
        } else if (g == CT_AGAIN) {
            CT_FZ(out.due_day == today, seed);
            CT_FZ(out.state == (card.state == 2 ? 3 : card.state), seed);
            CT_FZ(out.lapses == card.lapses + (card.state == 2 ? 1 : 0), seed);
        } else if (g == CT_HARD && card.state != 2) {
            CT_FZ(out.due_day == today, seed);
            CT_FZ(out.state == card.state, seed);
            CT_FZ(out.lapses == card.lapses, seed);
        } else {
            CT_FZ(out.due_day == interval_due, seed);
            CT_FZ(out.state == 2, seed);
            CT_FZ(out.lapses == card.lapses, seed);
        }
        ct_fsrs_card edge = card;
        if (edge.state == 0) {
            edge.stability = 10.0;
            edge.difficulty = 5.0;
        }
        edge.state = 1;
        edge.last_review_day = today;
        for (int gidx = 0; gidx < CT_FSR_GRADES; gidx++) {
            ct_fsrs_card same = ct_fsrs_review(&f, &edge, (ct_grade)(gidx + 1), 0.0, today);
            CT_FZ(ct_fsr_card_sane(&same, today), seed);
            CT_FZ(same.due_day == today, seed);
            CT_FZ(same.difficulty == edge.difficulty, seed);
        }
        ct_fsrs_card stale = card;
        if (stale.state == 0) {
            stale.stability = 10.0;
            stale.difficulty = 5.0;
        }
        stale.state = 1;
        if (today <= add_guard && today > LONG_MIN) {
            stale.last_review_day = today - 1;
            for (int gidx = 0; gidx < CT_FSR_GRADES; gidx++) {
                ct_fsrs_card moved = ct_fsrs_review(&f, &stale, (ct_grade)(gidx + 1), 0.0, today);
                CT_FZ(ct_fsr_card_sane(&moved, today), seed);
                if (gidx + 1 <= CT_HARD) {
                    CT_FZ(moved.due_day == today, seed);
                    CT_FZ(moved.state == 1, seed);
                } else {
                    CT_FZ(moved.due_day >= today + 1, seed);
                    CT_FZ(moved.due_day <= today + (long)max_add, seed);
                    CT_FZ(moved.state == 2, seed);
                }
            }
        }
        ct_fsrs_card fresh = {0};
        fresh.last_review_day = today;
        for (int gidx = 0; gidx < CT_FSR_GRADES; gidx++) {
            ct_fsrs_card born = ct_fsrs_review(&f, &fresh, (ct_grade)(gidx + 1), elapsed, today);
            CT_FZ(ct_fsr_card_sane(&born, today), seed);
            CT_FZ(born.due_day == today, seed);
            CT_FZ(born.state == 1, seed);
        }
    }
}

CT_SUITE(fuzz_srs_grade_space) {
    int iters = ct_fuzz_iterations();
    CT_ASSERT(iters >= 1);
    ct_fsrs f = ct_fsr_engine();
    for (int i = 0; i < iters; i++) {
        uint64_t seed = 0x6ADE0000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        ct_fsrs_card card = ct_fsr_card(&s, 1000);
        if (card.state == 0) {
            card.stability = 10.0;
            card.difficulty = 5.0;
        }
        card.state = 1;
        card.last_review_day = 1000;
        double elapsed = 1.0 + ct_sfc64_uniform(&s) * CT_FSR_PROBE_SPAN;
        long today = 1000 + (long)elapsed;
        ct_fsrs_card graded[CT_FSR_GRADES];
        for (int g = 0; g < CT_FSR_GRADES; g++) {
            graded[g] = ct_fsrs_review(&f, &card, (ct_grade)(g + 1), elapsed, today);
            CT_FZ(ct_fsr_card_sane(&graded[g], today), seed);
        }
        for (int g = 1; g + 1 < CT_FSR_GRADES; g++) {
            CT_FZ(graded[g].stability <= graded[g + 1].stability, seed);
        }
        if (graded[2].stability < CT_FSRS_MAX_IVL_DAYS) {
            CT_FZ(graded[0].stability < graded[2].stability, seed);
            CT_FZ(graded[1].stability < graded[2].stability, seed);
            CT_FZ(graded[2].stability < graded[3].stability, seed);
        }
        for (int g = 0; g + 1 < CT_FSR_GRADES; g++) {
            CT_FZ(graded[g].difficulty >= graded[g + 1].difficulty, seed);
        }
        CT_FZ(graded[0].state == 1, seed);
        CT_FZ(graded[1].state == 1, seed);
        CT_FZ(graded[2].state == 2, seed);
        CT_FZ(graded[3].state == 2, seed);
        CT_FZ(graded[0].due_day == today, seed);
        CT_FZ(graded[1].due_day == today, seed);
        for (int g = 2; g < CT_FSR_GRADES; g++) {
            CT_FZ(graded[g].due_day == today + ct_fsrs_next_interval_days(&f, graded[g].stability),
                  seed);
        }
        CT_FZ(graded[0].lapses == card.lapses, seed);
        CT_FZ(graded[1].lapses == card.lapses, seed);
        CT_FZ(graded[2].lapses == card.lapses, seed);
        CT_FZ(graded[3].lapses == card.lapses, seed);
        ct_fsrs_card review_base = card;
        review_base.state = 2;
        ct_fsrs_card rv_graded[CT_FSR_GRADES];
        for (int g = 0; g < CT_FSR_GRADES; g++) {
            rv_graded[g] = ct_fsrs_review(&f, &review_base, (ct_grade)(g + 1), elapsed, today);
            CT_FZ(ct_fsr_card_sane(&rv_graded[g], today), seed);
        }
        CT_FZ(rv_graded[0].state == 3, seed);
        CT_FZ(rv_graded[0].due_day == today, seed);
        CT_FZ(rv_graded[0].lapses == review_base.lapses + 1, seed);
        for (int g = 1; g < CT_FSR_GRADES; g++) {
            CT_FZ(rv_graded[g].state == 2, seed);
            CT_FZ(rv_graded[g].due_day ==
                      today + ct_fsrs_next_interval_days(&f, rv_graded[g].stability),
                  seed);
            CT_FZ(rv_graded[g].lapses == review_base.lapses, seed);
        }
        ct_fsrs_card fresh = {0};
        fresh.last_review_day = today;
        ct_fsrs_card born[CT_FSR_GRADES];
        for (int g = 0; g < CT_FSR_GRADES; g++) {
            born[g] = ct_fsrs_review(&f, &fresh, (ct_grade)(g + 1), 0.0, today);
            CT_FZ(ct_fsr_card_sane(&born[g], today), seed);
        }
        CT_FZ(born[0].stability < born[1].stability, seed);
        CT_FZ(born[1].stability < born[2].stability, seed);
        CT_FZ(born[2].stability < born[3].stability, seed);
        for (int g = 0; g + 1 < CT_FSR_GRADES; g++) {
            CT_FZ(born[g].difficulty >= born[g + 1].difficulty, seed);
        }
        ct_fsrs_card same_base = card;
        same_base.last_review_day = today;
        ct_fsrs_card same[CT_FSR_GRADES];
        for (int g = 0; g < CT_FSR_GRADES; g++) {
            same[g] = ct_fsrs_review(&f, &same_base, (ct_grade)(g + 1), 0.0, today);
            CT_FZ(ct_fsr_card_sane(&same[g], today), seed);
            CT_FZ(same[g].due_day == today, seed);
        }
        for (int g = 0; g + 1 < CT_FSR_GRADES; g++) {
            CT_FZ(same[g].stability <= same[g + 1].stability, seed);
        }
    }
}
