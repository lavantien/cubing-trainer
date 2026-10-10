#include <math.h>
#include <stdio.h>

#include "config/config.h"
#include "srs/fsrs.h"

static int failures = 0;

static void check(int cond, const char *what) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    }
}

static ct_fsrs_card base(int state) {
    ct_fsrs_card c = {0};
    c.stability = 10.0;
    c.difficulty = 5.0;
    c.due_day = 104;
    c.last_review_day = 100;
    c.reps = 1;
    c.state = state;
    return c;
}

int main(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);

    for (int g = CT_AGAIN; g <= CT_EASY; g++) {
        ct_fsrs_card in = base(0);
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 0.0, 100);
        check(out.state == 1, "new enters learning");
        check(out.due_day == 100, "new due today");
    }

    static const int study[] = {1, 3};
    for (int i = 0; i < 2; i++) {
        ct_fsrs_card again = base(study[i]);
        ct_fsrs_card oagain = ct_fsrs_review(&f, &again, CT_AGAIN, 4.0, 104);
        check(oagain.state == study[i], "again stays in study state");
        check(oagain.due_day == 104, "again due today");
        check(oagain.lapses == 0, "again outside review is no lapse");

        ct_fsrs_card hard = base(study[i]);
        ct_fsrs_card ohard = ct_fsrs_review(&f, &hard, CT_HARD, 4.0, 104);
        check(ohard.state == study[i], "hard stays in study state");
        check(ohard.due_day == 104, "hard keeps a same-day step");
        check(ohard.lapses == 0, "hard is no lapse");

        for (int g = CT_GOOD; g <= CT_EASY; g++) {
            ct_fsrs_card in = base(study[i]);
            ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 4.0, 104);
            long ivl = ct_fsrs_next_interval_days(&f, out.stability);
            check(out.state == 2, "success graduates to review");
            check(out.due_day == 104 + ivl, "graduated due is interval based");
        }
    }

    ct_fsrs_card lapse = base(2);
    ct_fsrs_card olapse = ct_fsrs_review(&f, &lapse, CT_AGAIN, 4.0, 104);
    check(olapse.state == 3, "review again demotes to relearning");
    check(olapse.due_day == 104, "relearning step is same day");
    check(olapse.lapses == 1, "review again counts the lapse");

    for (int g = CT_HARD; g <= CT_EASY; g++) {
        ct_fsrs_card in = base(2);
        ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 4.0, 104);
        long ivl = ct_fsrs_next_interval_days(&f, out.stability);
        check(out.state == 2, "review success stays review");
        check(out.due_day == 104 + ivl, "review due is interval based");
        check(out.lapses == 0, "review success is no lapse");
    }

    ct_fsrs_card same_review = base(2);
    same_review.last_review_day = 104;
    ct_fsrs_card osame = ct_fsrs_review(&f, &same_review, CT_GOOD, 0.0, 104);
    check(osame.state == 2, "same-day review stays review");
    check(osame.due_day == 104 + ct_fsrs_next_interval_days(&f, osame.stability),
          "same-day review reschedules by interval");

    ct_fsrs_card same_lapse = base(2);
    same_lapse.last_review_day = 104;
    ct_fsrs_card osame_lapse = ct_fsrs_review(&f, &same_lapse, CT_AGAIN, 0.0, 104);
    check(osame_lapse.state == 3, "same-day review again demotes");
    check(osame_lapse.lapses == 1, "same-day review again lapses");

    static const double bad[] = {INFINITY, -INFINITY, NAN};
    for (int i = 0; i < 3; i++) {
        for (int g = CT_AGAIN; g <= CT_EASY; g++) {
            ct_fsrs_card in = base(1);
            in.difficulty = bad[i];
            ct_fsrs_card out = ct_fsrs_review(&f, &in, (ct_grade)g, 4.0, 104);
            check(isfinite(out.difficulty) && out.difficulty >= CT_FSRS_MIN_DIFFICULTY &&
                      out.difficulty <= CT_FSRS_MAX_DIFFICULTY,
                  "non-finite difficulty clamps");
            check(isfinite(out.stability) && out.stability >= CT_FSRS_MIN_STABILITY &&
                      out.stability <= CT_FSRS_MAX_IVL_DAYS,
                  "non-finite difficulty keeps stability finite");
        }
        ct_fsrs_card same = base(1);
        same.difficulty = bad[i];
        same.last_review_day = 104;
        ct_fsrs_card out = ct_fsrs_review(&f, &same, CT_GOOD, 0.0, 104);
        check(isfinite(out.difficulty) && out.difficulty >= CT_FSRS_MIN_DIFFICULTY &&
                  out.difficulty <= CT_FSRS_MAX_DIFFICULTY,
              "same-day non-finite difficulty clamps");
    }

    if (failures == 0) {
        printf("all checks passed\n");
        return 0;
    }
    printf("%d failures\n", failures);
    return 1;
}
