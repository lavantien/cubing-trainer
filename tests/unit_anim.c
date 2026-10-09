#include "framework.h"

#include "../src/render/anim.h"

#include <stdbool.h>

static const double MS = 0.25;

static bool move_eq(ct_move a, ct_move b) {
    return a.face == b.face && a.depth_from == b.depth_from && a.depth_to == b.depth_to &&
           a.quarter == b.quarter;
}

static ct_move move_make(int face, int depth_from, int depth_to, int quarter) {
    ct_move m = {face, depth_from, depth_to, quarter};
    return m;
}

static bool move_is_zero(ct_move m) {
    return m.face == 0 && m.depth_from == 0 && m.depth_to == 0 && m.quarter == 0;
}

CT_SUITE(unit_anim_ease) {
    CT_ASSERT(ct_ease_cubic_in_out(0.0) == 0.0);
    CT_ASSERT(ct_ease_cubic_in_out(1.0) == 1.0);
    CT_ASSERT(ct_ease_cubic_in_out(-0.25) == 0.0);
    CT_ASSERT(ct_ease_cubic_in_out(1.75) == 1.0);
    CT_ASSERT_DBL_EQ(ct_ease_cubic_in_out(0.5), 0.5, 1e-15);

    double prev = ct_ease_cubic_in_out(0.0);
    for (int i = 1; i <= 1024; i++) {
        double t = (double)i / 1024.0;
        double cur = ct_ease_cubic_in_out(t);
        CT_ASSERT(cur >= prev);
        CT_ASSERT(cur >= 0.0);
        CT_ASSERT(cur <= 1.0);
        prev = cur;
    }
    CT_ASSERT_DBL_EQ(prev, 1.0, 0.0);

    for (int i = 0; i <= 1000; i++) {
        double t = (double)i / 1000.0;
        CT_ASSERT_DBL_EQ(ct_ease_cubic_in_out(t) + ct_ease_cubic_in_out(1.0 - t), 1.0, 1e-12);
    }

    ct_sfc64 rng;
    ct_sfc64_init(&rng, 20261009);
    for (int i = 0; i < 10000; i++) {
        double t = ct_sfc64_uniform(&rng);
        double v = ct_ease_cubic_in_out(t);
        CT_ASSERT(v >= 0.0);
        CT_ASSERT(v <= 1.0);
        CT_ASSERT(ct_ease_cubic_in_out(t + 1.0) == 1.0);
        CT_ASSERT(ct_ease_cubic_in_out(t - 1.0) == 0.0);
    }
}

CT_SUITE(unit_anim_queue) {
    ct_anim a;
    ct_anim_init(&a);
    CT_ASSERT(!ct_anim_active(&a));
    CT_ASSERT_EQ(a.count, 0);
    CT_ASSERT_EQ(a.head, 0);
    CT_ASSERT_DBL_EQ(a.speed, 1.0, 0.0);
    CT_ASSERT(a.playing);

    ct_move ru = move_make(CT_FACE_R, 0, 0, 1);
    ct_move uwide = move_make(CT_FACE_U, 0, 1, 2);
    CT_ASSERT_EQ(ct_anim_push(&a, ru), 0);
    CT_ASSERT(ct_anim_active(&a));
    CT_ASSERT_EQ(a.count, 1);

    ct_anim_step_result r = ct_anim_update(&a, 0.0, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.0, 0.0);
    CT_ASSERT(move_eq(r.move, ru));

    r = ct_anim_update(&a, 0.0625, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT(r.progress > 0.0);
    CT_ASSERT(r.progress < 1.0);
    CT_ASSERT_DBL_EQ(r.progress, 0.0625, 1e-15);
    CT_ASSERT(move_eq(r.move, ru));

    CT_ASSERT_EQ(ct_anim_push(&a, uwide), 0);

    r = ct_anim_update(&a, 0.0625, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.5, 1e-15);
    CT_ASSERT(move_eq(r.move, ru));
    r = ct_anim_update(&a, 0.0625, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.9375, 1e-15);

    r = ct_anim_update(&a, 0.0625, MS);
    CT_ASSERT(r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 1.0, 0.0);
    CT_ASSERT(move_eq(r.move, ru));
    CT_ASSERT_EQ(a.count, 1);
    CT_ASSERT(ct_anim_active(&a));

    r = ct_anim_update(&a, 0.0, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.0, 0.0);
    CT_ASSERT(move_eq(r.move, uwide));

    r = ct_anim_update(&a, 10.0, MS);
    CT_ASSERT(r.just_completed);
    CT_ASSERT(move_eq(r.move, uwide));
    CT_ASSERT_DBL_EQ(r.progress, 1.0, 0.0);
    CT_ASSERT(!ct_anim_active(&a));

    r = ct_anim_update(&a, 0.0625, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.0, 0.0);
    CT_ASSERT(move_is_zero(r.move));
}

CT_SUITE(unit_anim_speed) {
    ct_anim a;
    ct_anim_init(&a);
    ct_move m = move_make(CT_FACE_F, 0, 0, 3);

    ct_anim_push(&a, m);
    ct_anim_set_speed(&a, 2.0);
    CT_ASSERT_DBL_EQ(a.speed, 2.0, 0.0);
    ct_anim_step_result r = ct_anim_update(&a, MS / 2.0, MS);
    CT_ASSERT(r.just_completed);
    CT_ASSERT(move_eq(r.move, m));

    ct_anim_push(&a, m);
    ct_anim_set_speed(&a, 4.0);
    r = ct_anim_update(&a, MS / 4.0, MS);
    CT_ASSERT(r.just_completed);

    ct_anim b;
    ct_anim_init(&b);
    ct_anim_push(&b, m);
    r = ct_anim_update(&b, 0.5, 1.0);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.5, 1e-15);
    ct_anim_set_speed(&b, 2.0);
    r = ct_anim_update(&b, 0.1875, 1.0);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.9921875, 1e-15);
    r = ct_anim_update(&b, 0.0625, 1.0);
    CT_ASSERT(r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 1.0, 0.0);
    CT_ASSERT(!ct_anim_active(&b));
}

CT_SUITE(unit_anim_pause) {
    ct_anim a;
    ct_anim_init(&a);
    ct_move m = move_make(CT_FACE_D, 0, 0, 2);
    ct_anim_push(&a, m);

    ct_anim_step_result r = ct_anim_update(&a, 0.0625, MS);
    CT_ASSERT(!r.just_completed);
    double before = r.progress;

    ct_anim_pause(&a);
    CT_ASSERT(!a.playing);
    CT_ASSERT(ct_anim_active(&a));
    r = ct_anim_update(&a, 0.1875, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, before, 0.0);
    CT_ASSERT(move_eq(r.move, m));
    r = ct_anim_update(&a, 0.0, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, before, 0.0);
    CT_ASSERT_EQ(a.count, 1);

    ct_anim_resume(&a);
    CT_ASSERT(a.playing);
    r = ct_anim_update(&a, 0.0625, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.5, 1e-15);
    r = ct_anim_update(&a, 0.125, MS);
    CT_ASSERT(r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 1.0, 0.0);
    CT_ASSERT(!ct_anim_active(&a));

    ct_anim b;
    ct_anim_init(&b);
    ct_move n = move_make(CT_FACE_L, 0, 0, 1);
    ct_anim_push(&b, n);
    ct_anim_update(&b, 0.0625, MS);
    ct_anim_pause(&b);
    ct_anim_step_result s = ct_anim_step(&b);
    CT_ASSERT(s.just_completed);
    CT_ASSERT_DBL_EQ(s.progress, 1.0, 0.0);
    CT_ASSERT(move_eq(s.move, n));
    CT_ASSERT(!ct_anim_active(&b));
    s = ct_anim_step(&b);
    CT_ASSERT(!s.just_completed);
    CT_ASSERT_DBL_EQ(s.progress, 0.0, 0.0);
    CT_ASSERT(move_is_zero(s.move));
}

CT_SUITE(unit_anim_prefix) {
    ct_anim a;
    ct_anim_init(&a);
    ct_move m0 = move_make(CT_FACE_U, 0, 0, 1);
    ct_move m1 = move_make(CT_FACE_R, 0, 0, 1);
    ct_move m2 = move_make(CT_FACE_F, 0, 0, 2);
    ct_anim_push(&a, m0);
    ct_anim_push(&a, m1);
    ct_anim_push(&a, m2);

    ct_move out[CT_ANIM_QUEUE_CAP];
    CT_ASSERT_EQ(ct_anim_prefix(&a, 0, out, CT_ANIM_QUEUE_CAP), 0);
    CT_ASSERT_EQ(ct_anim_prefix(&a, 1, out, CT_ANIM_QUEUE_CAP), 1);
    CT_ASSERT(move_eq(out[0], m0));
    CT_ASSERT_EQ(ct_anim_prefix(&a, 3, out, CT_ANIM_QUEUE_CAP), 3);
    CT_ASSERT(move_eq(out[0], m0));
    CT_ASSERT(move_eq(out[1], m1));
    CT_ASSERT(move_eq(out[2], m2));
    CT_ASSERT_EQ(ct_anim_prefix(&a, 5, out, CT_ANIM_QUEUE_CAP), 3);
    CT_ASSERT_EQ(ct_anim_prefix(&a, 3, out, 2), 2);
    CT_ASSERT(move_eq(out[0], m0));
    CT_ASSERT(move_eq(out[1], m1));
    CT_ASSERT_EQ(ct_anim_prefix(&a, -1, out, CT_ANIM_QUEUE_CAP), 0);

    ct_anim_update(&a, MS, MS);
    CT_ASSERT_EQ(ct_anim_prefix(&a, 10, out, CT_ANIM_QUEUE_CAP), 2);
    CT_ASSERT(move_eq(out[0], m1));
    CT_ASSERT(move_eq(out[1], m2));
}

CT_SUITE(unit_anim_edges) {
    ct_anim a;
    ct_anim_init(&a);
    ct_anim_step_result r = ct_anim_update(&a, 1.0, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 0.0, 0.0);
    CT_ASSERT(move_is_zero(r.move));

    for (int i = 0; i < CT_ANIM_QUEUE_CAP; i++) {
        ct_move m = move_make(i % 6, 0, i % 2, i % 3 + 1);
        CT_ASSERT_EQ(ct_anim_push(&a, m), 0);
    }
    CT_ASSERT_EQ(a.count, CT_ANIM_QUEUE_CAP);
    ct_move overflow = move_make(CT_FACE_B, 0, 0, 3);
    CT_ASSERT_EQ(ct_anim_push(&a, overflow), -1);
    CT_ASSERT_EQ(a.count, CT_ANIM_QUEUE_CAP);

    ct_move out[CT_ANIM_QUEUE_CAP];
    CT_ASSERT_EQ(ct_anim_prefix(&a, CT_ANIM_QUEUE_CAP, out, CT_ANIM_QUEUE_CAP), CT_ANIM_QUEUE_CAP);
    CT_ASSERT(move_eq(out[CT_ANIM_QUEUE_CAP - 1], move_make(3, 0, 1, 1)));

    r = ct_anim_update(&a, MS, MS);
    CT_ASSERT(r.just_completed);
    CT_ASSERT_EQ(a.count, CT_ANIM_QUEUE_CAP - 1);
    CT_ASSERT_EQ(ct_anim_push(&a, overflow), 0);
    CT_ASSERT_EQ(a.count, CT_ANIM_QUEUE_CAP);

    CT_ASSERT_EQ(ct_anim_prefix(&a, CT_ANIM_QUEUE_CAP, out, CT_ANIM_QUEUE_CAP), CT_ANIM_QUEUE_CAP);
    for (int i = 0; i < CT_ANIM_QUEUE_CAP - 1; i++) {
        ct_move want = move_make((i + 1) % 6, 0, (i + 1) % 2, (i + 1) % 3 + 1);
        CT_ASSERT(move_eq(out[i], want));
    }
    CT_ASSERT(move_eq(out[CT_ANIM_QUEUE_CAP - 1], overflow));

    r = ct_anim_step(&a);
    CT_ASSERT(r.just_completed);
    r = ct_anim_step(&a);
    CT_ASSERT(r.just_completed);
    CT_ASSERT_EQ(ct_anim_prefix(&a, CT_ANIM_QUEUE_CAP, out, CT_ANIM_QUEUE_CAP), CT_ANIM_QUEUE_CAP - 2);
    CT_ASSERT(move_eq(out[0], move_make(3, 0, 1, 1)));

    ct_anim b;
    ct_anim_init(&b);
    ct_anim_set_speed(&b, 0.0);
    CT_ASSERT(b.speed > 0.0);
    CT_ASSERT_DBL_EQ(b.speed, 1.0 / 1000000.0, 0.0);
    ct_anim_set_speed(&b, -1.5);
    CT_ASSERT(b.speed > 0.0);
    CT_ASSERT_DBL_EQ(b.speed, 1.0 / 1000000.0, 0.0);
    ct_move m = move_make(CT_FACE_B, 0, 0, 1);
    ct_anim_push(&b, m);
    r = ct_anim_update(&b, MS, MS);
    CT_ASSERT(!r.just_completed);
    CT_ASSERT_EQ(b.count, 1);
    ct_anim_set_speed(&b, 1.0);
    r = ct_anim_update(&b, MS, MS);
    CT_ASSERT(r.just_completed);
    CT_ASSERT_DBL_EQ(r.progress, 1.0, 0.0);
}

CT_SUITE(unit_anim_determinism) {
    enum { N_DTS = 64, N_MOVES = 3, N_RUNS = 2 };
    double dts[N_DTS];
    ct_sfc64 rng;
    ct_sfc64_init(&rng, 20261009);
    for (int i = 0; i < N_DTS; i++) {
        dts[i] = ct_sfc64_uniform(&rng) * 0.0625;
    }

    ct_move script[N_MOVES] = {
        move_make(CT_FACE_R, 0, 0, 1),
        move_make(CT_FACE_U, 0, 0, 2),
        move_make(CT_FACE_F, 0, 1, 3),
    };

    ct_anim runs[N_RUNS];
    for (int run = 0; run < N_RUNS; run++) {
        ct_anim_init(&runs[run]);
        for (int i = 0; i < N_MOVES; i++) {
            ct_anim_push(&runs[run], script[i]);
        }
    }

    bool done_log[N_RUNS][N_DTS];
    double progress_log[N_RUNS][N_DTS];
    int completed_quarter[N_RUNS][N_MOVES];
    int completions[N_RUNS] = {0};
    for (int i = 0; i < N_DTS; i++) {
        for (int run = 0; run < N_RUNS; run++) {
            ct_anim_step_result r = ct_anim_update(&runs[run], dts[i], MS);
            done_log[run][i] = r.just_completed;
            progress_log[run][i] = r.progress;
            if (r.just_completed) {
                completed_quarter[run][completions[run]] = r.move.quarter;
                completions[run]++;
            }
        }
    }

    for (int i = 0; i < N_DTS; i++) {
        CT_ASSERT_EQ(done_log[0][i], done_log[1][i]);
        CT_ASSERT_DBL_EQ(progress_log[0][i], progress_log[1][i], 0.0);
    }
    CT_ASSERT_EQ(completions[0], N_MOVES);
    CT_ASSERT_EQ(completions[1], N_MOVES);
    for (int i = 0; i < N_MOVES; i++) {
        CT_ASSERT_EQ(completed_quarter[0][i], script[i].quarter);
        CT_ASSERT_EQ(completed_quarter[1][i], script[i].quarter);
    }
    for (int i = 0; i < N_DTS; i++) {
        if (done_log[0][i]) {
            CT_ASSERT_DBL_EQ(progress_log[0][i], 1.0, 0.0);
        } else {
            CT_ASSERT(progress_log[0][i] >= 0.0);
            CT_ASSERT(progress_log[0][i] < 1.0);
        }
    }
}
