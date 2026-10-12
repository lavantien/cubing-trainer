#include <math.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/srs/fsrs.h"
#include "../src/stats/stats.h"
#include "../src/ui/ui.h"
#include "../src/ui/widgets.h"
#include "framework.h"

static const char NOW[] = "2026-06-01T00:00:00Z";
static const char STAMP_A[] = "2026-05-01T00:00:00Z";
static const char STAMP_B[] = "2026-05-20T00:00:00Z";

enum {
    SEED_CARDS = 8,
    SEED_MAIN_ROWS = 16,
    SEED_WALK_DAY2 = 10,
    SEED_KEYS_MAX = 8,
    SEED_LABEL_MAX = 8
};

static const int SEED_G1[SEED_CARDS] = {CT_EASY, CT_GOOD,  CT_GOOD,  CT_HARD,
                                        CT_HARD, CT_AGAIN, CT_AGAIN, CT_AGAIN};
static const double SEED_T2[SEED_CARDS] = {16.0, 10.0, 21.0, 10.0, 20.0, 12.0, 20.0, 60.0};
static const int SEED_G2[SEED_CARDS] = {CT_GOOD,  CT_HARD, CT_EASY, CT_GOOD,
                                        CT_AGAIN, CT_GOOD, CT_EASY, CT_HARD};
static const char *const SEED_DUE[SEED_CARDS] = {"2026-06-01T00:00:00Z",
                                                 "2026-06-02T00:00:00Z",
                                                 "2026-06-05T00:00:00Z",
                                                 "2026-06-30T00:00:00Z",
                                                 "2026-07-15T00:00:00Z",
                                                 "2026-06-01T00:00:00Z",
                                                 NULL,
                                                 NULL};
static const int SEED_DIR[SEED_CARDS] = {CT_DIR_FR, CT_DIR_FR, CT_DIR_FR, CT_DIR_FR,
                                         CT_DIR_FR, CT_DIR_FR, CT_DIR_FR, CT_DIR_FL};

typedef struct {
    sqlite3 *db;
    long ids[SEED_CARDS];
    ct_fsrs_card final[SEED_CARDS];
} seed_state;

static sqlite3 *seed_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static void seed_insert_card(sqlite3 *db, const char *deck, const char *key, int dir, long *id) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", key);
    c.direction = dir;
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    CT_ASSERT(c.id > 0);
    *id = c.id;
}

static void seed_build(seed_state *s) {
    s->db = seed_db();
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    for (int i = 0; i < SEED_CARDS; i++) {
        char key[SEED_KEYS_MAX];
        snprintf(key, sizeof key, "c%d", i + 1);
        seed_insert_card(s->db, "main", key, SEED_DIR[i], &s->ids[i]);
        ct_fsrs_card fresh;
        memset(&fresh, 0, sizeof fresh);
        ct_fsrs_card after1 = ct_fsrs_review(&f, &fresh, (ct_grade)SEED_G1[i], 0.0, 0);
        CT_ASSERT_EQ(ct_repo_review_log(s->db, s->ids[i], SEED_G1[i], STAMP_A, after1.stability,
                                        after1.difficulty, 0.0, 1.0),
                     CT_REPO_OK);
        s->final[i] = ct_fsrs_review(&f, &after1, (ct_grade)SEED_G2[i], SEED_T2[i], SEED_WALK_DAY2);
        CT_ASSERT_EQ(ct_repo_review_log(
                         s->db, s->ids[i], SEED_G2[i], STAMP_B, after1.stability, after1.difficulty,
                         SEED_T2[i], (double)ct_fsrs_next_interval_days(&f, s->final[i].stability)),
                     CT_REPO_OK);
        if (SEED_DUE[i] == NULL)
            continue;
        ct_srs_row row;
        memset(&row, 0, sizeof row);
        row.stability = s->final[i].stability;
        row.difficulty = s->final[i].difficulty;
        snprintf(row.due, sizeof row.due, "%s", SEED_DUE[i]);
        snprintf(row.last_review, sizeof row.last_review, "%s", STAMP_B);
        row.reps = 2;
        row.lapses = s->final[i].lapses;
        row.state = 1;
        CT_ASSERT_EQ(ct_repo_srs_put(s->db, s->ids[i], &row), CT_REPO_OK);
    }
    long quiet;
    seed_insert_card(s->db, "quiet", "q1", CT_DIR_FR, &quiet);
    CT_ASSERT(quiet > 0);
}

static int seed_correct(int i, int review) {
    int rating = review == 0 ? SEED_G1[i] : SEED_G2[i];
    return rating > CT_AGAIN ? 1 : 0;
}

static void seed_expected_buckets(ct_retention_bucket *out) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    for (int i = 0; i < CT_STATS_RETENTION_BUCKETS; i++) {
        out[i].bucket_mid = (2.0 * i + 1.0) / (2.0 * CT_STATS_RETENTION_BUCKETS);
        out[i].reviews = 0;
        out[i].actual_recall = 0;
        out[i].predicted_recall = 0.0;
    }
    for (int i = 0; i < SEED_CARDS; i++) {
        ct_fsrs_card fresh;
        memset(&fresh, 0, sizeof fresh);
        ct_fsrs_card after1 = ct_fsrs_review(&f, &fresh, (ct_grade)SEED_G1[i], 0.0, 0);
        double stability = after1.stability;
        double elapsed[2] = {0.0, SEED_T2[i]};
        for (int rev = 0; rev < 2; rev++) {
            double r = ct_fsrs_retrievability(stability, elapsed[rev]);
            if (!(r > 0.0))
                r = 0.0;
            if (r > 1.0)
                r = 1.0;
            int idx = (int)(r * CT_STATS_RETENTION_BUCKETS);
            if (idx >= CT_STATS_RETENTION_BUCKETS)
                idx = CT_STATS_RETENTION_BUCKETS - 1;
            out[idx].reviews++;
            out[idx].actual_recall += seed_correct(i, rev);
            out[idx].predicted_recall += r;
        }
    }
    for (int i = 0; i < CT_STATS_RETENTION_BUCKETS; i++) {
        if (out[i].reviews > 0)
            out[i].predicted_recall /= (double)out[i].reviews;
    }
}

CT_SUITE(stats_screen_view_load) {
    seed_state s;
    seed_build(&s);

    ct_stats_view v;
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NOW, NULL), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_count, 2);
    CT_ASSERT_STR_EQ(v.decks[0].deck, "main");
    CT_ASSERT_STR_EQ(v.decks[1].deck, "quiet");
    CT_ASSERT_EQ(v.decks[0].total_cards, 8);
    CT_ASSERT_EQ(v.decks[0].due_now, 2);
    CT_ASSERT_EQ(v.decks[0].learning, 6);
    CT_ASSERT_EQ(v.decks[0].review_state, 0);
    double sum_s = 0.0;
    double sum_d = 0.0;
    for (int i = 0; i < 6; i++) {
        sum_s += s.final[i].stability;
        sum_d += s.final[i].difficulty;
    }
    CT_ASSERT_DBL_EQ(v.decks[0].avg_stability, sum_s / 6.0, 1e-9);
    CT_ASSERT_DBL_EQ(v.decks[0].avg_difficulty, sum_d / 6.0, 1e-9);
    CT_ASSERT_EQ(v.decks[1].total_cards, 1);
    CT_ASSERT_EQ(v.decks[1].due_now, 0);
    CT_ASSERT_EQ(v.deck_selected, 0);

    CT_ASSERT_EQ(v.forecast_count, 30);
    CT_ASSERT_STR_EQ(v.forecast[0].day, "2026-06-01");
    CT_ASSERT_STR_EQ(v.forecast[29].day, "2026-06-30");
    CT_ASSERT_EQ(v.forecast[0].due_count, 2);
    CT_ASSERT_EQ(v.forecast[1].due_count, 1);
    CT_ASSERT_EQ(v.forecast[2].due_count, 0);
    CT_ASSERT_EQ(v.forecast[4].due_count, 1);
    CT_ASSERT_EQ(v.forecast[29].due_count, 1);
    long total = 0;
    for (int i = 0; i < v.forecast_count; i++)
        total += v.forecast[i].due_count;
    CT_ASSERT_EQ(total, 5);

    CT_ASSERT_EQ(v.case_count, 8);
    static const char *const WANT_KEYS[8] = {"c5", "c6", "c7", "c8", "c1", "c2", "c3", "c4"};
    static const int WANT_DIRS[8] = {CT_DIR_FR, CT_DIR_FR, CT_DIR_FR, CT_DIR_FL,
                                     CT_DIR_FR, CT_DIR_FR, CT_DIR_FR, CT_DIR_FR};
    static const int WANT_CORRECT[8] = {1, 1, 1, 1, 2, 2, 2, 2};
    for (int i = 0; i < 8; i++) {
        CT_ASSERT_STR_EQ(v.cases[i].case_key, WANT_KEYS[i]);
        CT_ASSERT_EQ(v.cases[i].direction, WANT_DIRS[i]);
        CT_ASSERT_EQ(v.cases[i].reviews, 2);
        CT_ASSERT_EQ(v.cases[i].correct, WANT_CORRECT[i]);
        CT_ASSERT_DBL_EQ(v.cases[i].accuracy, WANT_CORRECT[i] / 2.0, 1e-9);
    }

    ct_retention_bucket want[CT_STATS_RETENTION_BUCKETS];
    seed_expected_buckets(want);
    CT_ASSERT_EQ(v.retention_count, CT_STATS_RETENTION_BUCKETS);
    long want_reviews = 0;
    for (int i = 0; i < CT_STATS_RETENTION_BUCKETS; i++) {
        CT_ASSERT_DBL_EQ(v.retention[i].bucket_mid, want[i].bucket_mid, 1e-12);
        CT_ASSERT_EQ(v.retention[i].reviews, want[i].reviews);
        CT_ASSERT_EQ(v.retention[i].actual_recall, want[i].actual_recall);
        CT_ASSERT_DBL_EQ(v.retention[i].predicted_recall, want[i].predicted_recall, 1e-9);
        want_reviews += want[i].reviews;
    }
    CT_ASSERT_EQ(want_reviews, 16);
    CT_ASSERT_EQ(want[0].reviews, 0);
    CT_ASSERT_EQ(want[9].reviews, 8);
    CT_ASSERT_EQ(want[9].actual_recall, 5);
    for (int i = 1; i < 9; i++)
        CT_ASSERT_EQ(want[i].reviews, 1);
    ct_retention_summary want_summary =
        ct_stats_retention_summary(want, CT_STATS_RETENTION_BUCKETS);
    CT_ASSERT_DBL_EQ(v.retention_summary.total_reviews, want_summary.total_reviews, 1e-12);
    CT_ASSERT_DBL_EQ(v.retention_summary.total_correct, want_summary.total_correct, 1e-12);
    CT_ASSERT_DBL_EQ(v.retention_summary.weighted_error, want_summary.weighted_error, 1e-9);
    CT_ASSERT_DBL_EQ(v.retention_summary.max_abs_error, want_summary.max_abs_error, 1e-9);
    CT_ASSERT_EQ(v.retention_summary.worst_bucket, want_summary.worst_bucket);

    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NOW, "quiet"), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, 1);
    CT_ASSERT_EQ(v.case_count, 0);

    ct_db_close(s.db);
}

CT_SUITE(stats_screen_deck_switch) {
    seed_state s;
    seed_build(&s);

    ct_stats_view v;
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NOW, NULL), CT_REPO_OK);

    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "quiet"), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, 1);
    CT_ASSERT_EQ(v.case_count, 0);
    CT_ASSERT_EQ(v.retention_count, CT_STATS_RETENTION_BUCKETS);
    for (int i = 0; i < CT_STATS_RETENTION_BUCKETS; i++) {
        CT_ASSERT_EQ(v.retention[i].reviews, 0);
        CT_ASSERT_EQ(v.retention[i].actual_recall, 0);
        CT_ASSERT_DBL_EQ(v.retention[i].predicted_recall, 0.0, 1e-15);
    }
    CT_ASSERT_DBL_EQ(v.retention_summary.total_reviews, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(v.retention_summary.total_correct, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(v.retention_summary.weighted_error, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(v.retention_summary.max_abs_error, 0.0, 1e-15);
    CT_ASSERT_EQ(v.retention_summary.worst_bucket, -1);
    CT_ASSERT_EQ(v.forecast_count, 30);
    CT_ASSERT_EQ(v.forecast[0].due_count, 2);

    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "ghost"), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, -1);
    CT_ASSERT_EQ(v.case_count, 0);
    for (int i = 0; i < CT_STATS_RETENTION_BUCKETS; i++)
        CT_ASSERT_EQ(v.retention[i].reviews, 0);

    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "main"), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, 0);
    CT_ASSERT_EQ(v.case_count, 8);
    CT_ASSERT_EQ(v.retention[9].reviews, 8);

    ct_db_close(s.db);
}

CT_SUITE(stats_screen_worst_cases) {
    seed_state s;
    seed_build(&s);

    ct_stats_view v;
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NOW, NULL), CT_REPO_OK);

    CT_ASSERT_EQ(ct_stats_view_worst_cases(&v, 3), 3);
    static const char *const WANT_ORDER[8] = {"c5", "c6", "c7", "c8", "c1", "c2", "c3", "c4"};
    double prev = 0.0;
    for (int i = 0; i < 3; i++) {
        int idx = v.case_order[i];
        CT_ASSERT(idx >= 0 && idx < v.case_count);
        CT_ASSERT_STR_EQ(v.cases[idx].case_key, WANT_ORDER[i]);
        CT_ASSERT(v.cases[idx].accuracy >= prev);
        prev = v.cases[idx].accuracy;
    }
    CT_ASSERT_DBL_EQ(prev, 0.5, 1e-9);

    CT_ASSERT_EQ(ct_stats_view_worst_cases(&v, 100), 8);
    prev = 0.0;
    for (int i = 0; i < 8; i++) {
        int idx = v.case_order[i];
        CT_ASSERT_STR_EQ(v.cases[idx].case_key, WANT_ORDER[i]);
        CT_ASSERT(v.cases[idx].accuracy >= prev);
        prev = v.cases[idx].accuracy;
    }
    CT_ASSERT_DBL_EQ(prev, 1.0, 1e-9);

    CT_ASSERT_EQ(ct_stats_view_worst_cases(&v, 0), 0);
    CT_ASSERT_EQ(ct_stats_view_worst_cases(&v, -1), 0);
    CT_ASSERT_EQ(ct_stats_view_worst_cases(NULL, 4), 0);

    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "quiet"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_stats_view_worst_cases(&v, 5), 0);

    ct_db_close(s.db);
}

CT_SUITE(stats_screen_malformed) {
    seed_state s;
    seed_build(&s);

    ct_stats_view v;
    memset(&v, 0, sizeof v);
    CT_ASSERT_EQ(ct_stats_view_load(NULL, s.db, NOW, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_stats_view_load(&v, NULL, NOW, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NULL, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, "", NULL), CT_REPO_ERR_ARG);
    char long_now[CT_STATS_STAMP_MAX + 1];
    memset(long_now, 'n', sizeof long_now - 1);
    long_now[sizeof long_now - 1] = '\0';
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, long_now, NULL), CT_REPO_ERR_ARG);

    CT_ASSERT_EQ(ct_stats_view_select_deck(NULL, "main"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "main"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NOW, NULL), CT_REPO_OK);
    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, ""), CT_REPO_ERR_ARG);
    char big_deck[CT_STATS_DECK_MAX + 1];
    memset(big_deck, 'd', sizeof big_deck - 1);
    big_deck[sizeof big_deck - 1] = '\0';
    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, big_deck), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(v.case_count, 8);

    ct_db_close(s.db);
}

CT_SUITE(stats_screen_chart_layout) {
    Rectangle b = {10.0f, 20.0f, 400.0f, 100.0f};
    double vals[4] = {1.0, 2.0, 4.0, 8.0};
    ct_chart_bar bars[4];
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 4, bars, 4), 4);
    double want_x[4] = {22.5, 122.5, 222.5, 322.5};
    double want_h[4] = {12.5, 25.0, 50.0, 100.0};
    for (int i = 0; i < 4; i++) {
        CT_ASSERT_DBL_EQ(bars[i].bar.x, want_x[i], 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.y, 20.0 + 100.0 - want_h[i], 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.width, 75.0, 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.height, want_h[i], 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].value, vals[i], 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].label_day, (double)i, 1e-12);
    }

    double zeros[2] = {0.0, 0.0};
    CT_ASSERT_EQ(ct_chart_layout(b, zeros, 2, bars, 2), 2);
    for (int i = 0; i < 2; i++) {
        CT_ASSERT_DBL_EQ(bars[i].bar.height, 0.0, 1e-12);
        CT_ASSERT_DBL_EQ(bars[i].bar.y, 120.0, 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.x, 10.0 + (double)i * 200.0 + 25.0, 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.width, 150.0, 1e-9);
    }

    double negs[3] = {2.0, -5.0, 8.0};
    CT_ASSERT_EQ(ct_chart_layout(b, negs, 3, bars, 3), 3);
    CT_ASSERT_DBL_EQ(bars[1].bar.height, 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(bars[1].bar.y, 120.0, 1e-9);
    CT_ASSERT_DBL_EQ(bars[0].bar.height, 25.0, 1e-9);
    CT_ASSERT_DBL_EQ(bars[2].bar.height, 100.0, 1e-9);

    ct_chart_bar full[4];
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 4, full, 4), 4);
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 4, bars, 2), 2);
    for (int i = 0; i < 2; i++) {
        CT_ASSERT_DBL_EQ(bars[i].bar.x, full[i].bar.x, 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.y, full[i].bar.y, 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.width, full[i].bar.width, 1e-9);
        CT_ASSERT_DBL_EQ(bars[i].bar.height, full[i].bar.height, 1e-9);
    }

    double one[1] = {5.0};
    CT_ASSERT_EQ(ct_chart_layout(b, one, 1, bars, 1), 1);
    CT_ASSERT_DBL_EQ(bars[0].bar.x, 60.0, 1e-9);
    CT_ASSERT_DBL_EQ(bars[0].bar.width, 300.0, 1e-9);
    CT_ASSERT_DBL_EQ(bars[0].bar.height, 100.0, 1e-9);
    CT_ASSERT_DBL_EQ(bars[0].bar.y, 20.0, 1e-9);

    CT_ASSERT_EQ(ct_chart_layout(b, vals, 0, bars, 4), 0);
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 4, bars, 0), 0);
    CT_ASSERT_EQ(ct_chart_layout(b, NULL, 2, bars, 4), -1);
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 2, NULL, 4), -1);
    CT_ASSERT_EQ(ct_chart_layout(b, vals, -1, bars, 4), -1);
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 2, bars, -1), -1);
    CT_ASSERT_EQ(ct_chart_layout(b, NULL, 0, bars, 4), 0);
    bars[0].bar.x = 777.0;
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 4, NULL, 0), 0);
    CT_ASSERT_DBL_EQ(bars[0].bar.x, 777.0, 1e-12);

    CT_ASSERT_EQ(ct_chart_layout(b, NULL, 1, bars, 4), -1);
    CT_ASSERT_EQ(ct_chart_layout(b, vals, 2, NULL, 1), -1);

    struct {
        double guard_lo;
        double boxed[3];
        double guard_hi;
    } box = {1.0e9, {1.0, INFINITY, 3.0}, 2.0e9};
    struct {
        ct_chart_bar guard;
        ct_chart_bar bars[3];
    } outbox;
    outbox.guard.bar.x = 777.0;
    outbox.bars[2].bar.x = 778.0;
    CT_ASSERT_EQ(ct_chart_layout(b, box.boxed, 3, outbox.bars, 2), 2);
    CT_ASSERT_DBL_EQ(outbox.bars[0].bar.height, 100.0 / 3.0, 1e-4);
    CT_ASSERT_DBL_EQ(outbox.bars[1].bar.height, 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(outbox.bars[2].bar.x, 778.0, 1e-12);
    CT_ASSERT_DBL_EQ(outbox.guard.bar.x, 777.0, 1e-12);
}

CT_SUITE(stats_screen_heatmap_grid) {
    Rectangle b = {0.0f, 0.0f, 240.0f, 300.0f};
    Rectangle r;
    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, 0, &r), 0);
    CT_ASSERT_DBL_EQ(r.x, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(r.y, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(r.width, 40.0, 1e-9);
    CT_ASSERT_DBL_EQ(r.height, 75.0, 1e-9);
    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, 7, &r), 0);
    CT_ASSERT_DBL_EQ(r.x, 40.0, 1e-9);
    CT_ASSERT_DBL_EQ(r.y, 75.0, 1e-9);
    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, 23, &r), 0);
    CT_ASSERT_DBL_EQ(r.x, 200.0, 1e-9);
    CT_ASSERT_DBL_EQ(r.y, 225.0, 1e-9);

    for (int i = 0; i < 24; i++) {
        CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, i, &r), 0);
        Vector2 center = {r.x + r.width / 2.0f, r.y + r.height / 2.0f};
        CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, center), i);
        Vector2 corner = {r.x + 0.5f, r.y + 0.5f};
        CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, corner), i);
    }

    Vector2 left_out = {-1.0f, 10.0f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, left_out), -1);
    Vector2 below = {10.0f, 500.0f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, below), -1);
    Vector2 right_edge = {240.0f, 150.0f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, right_edge), -1);
    Vector2 top_edge = {100.0f, -0.5f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, top_edge), -1);
    Vector2 last_inside = {239.5f, 299.0f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, last_inside), 23);

    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, -1, &r), -1);
    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, 24, &r), -1);
    CT_ASSERT_EQ(ct_heatmap_cell(b, 0, 4, 0, &r), -1);
    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, -1, 0, &r), -1);
    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, 0, NULL), -1);
    Vector2 origin = {0.0f, 0.0f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 0, 4, origin), -1);
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 0, origin), -1);
    r.x = 777.0;
    CT_ASSERT_EQ(ct_heatmap_cell(b, 6, 4, 0, &r), 0);
    CT_ASSERT_DBL_EQ(r.x, 0.0, 1e-9);

    CT_ASSERT_EQ(ct_heatmap_cell(b, 1, 1, 0, &r), 0);
    CT_ASSERT_EQ(ct_heatmap_pick(b, 1, 4, origin), 0);
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 1, origin), 0);
    Vector2 exact_origin = {0.0f, 0.0f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, exact_origin), 0);
    Vector2 bottom_edge = {100.0f, 300.0f};
    CT_ASSERT_EQ(ct_heatmap_pick(b, 6, 4, bottom_edge), -1);

    Rectangle shifted = {10.0f, 20.0f, 240.0f, 300.0f};
    Vector2 shifted_center = {10.0f + 100.0f, 20.0f + 112.5f};
    CT_ASSERT_EQ(ct_heatmap_pick(shifted, 6, 4, shifted_center), 8);
    CT_ASSERT_EQ(ct_heatmap_pick(shifted, 6, 4, exact_origin), -1);
}

CT_SUITE(stats_screen_boundaries) {
    sqlite3 *db = seed_db();
    ct_stats_view v;
    CT_ASSERT_EQ(ct_stats_view_load(&v, db, NOW, NULL), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_count, 0);
    CT_ASSERT_EQ(v.deck_selected, -1);
    CT_ASSERT_EQ(v.forecast_count, CT_STATS_FORECAST_DAYS);
    CT_ASSERT_EQ(v.session_count, 0);
    CT_ASSERT_EQ(v.case_count, 0);
    ct_db_close(db);

    db = seed_db();
    long only;
    seed_insert_card(db, "only", "o1", CT_DIR_FR, &only);
    CT_ASSERT(only > 0);
    CT_ASSERT_EQ(ct_stats_view_load(&v, db, NOW, NULL), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_count, 1);
    CT_ASSERT_EQ(v.deck_selected, 0);
    CT_ASSERT_EQ(v.case_count, 0);
    ct_db_close(db);

    seed_state s;
    seed_build(&s);
    long zid;
    seed_insert_card(s.db, "z", "z1", CT_DIR_FR, &zid);
    CT_ASSERT(zid > 0);
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NOW, "z"), CT_REPO_OK);
    CT_ASSERT_EQ(v.case_count, 0);
    CT_ASSERT_STR_EQ(v.decks[v.deck_selected].deck, "z");

    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "ghost"), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, -1);
    CT_ASSERT_EQ(v.case_count, 0);
    CT_ASSERT_EQ(v.retention_count, CT_STATS_RETENTION_BUCKETS);
    for (int i = 0; i < CT_STATS_RETENTION_BUCKETS; i++) {
        CT_ASSERT_EQ(v.retention[i].reviews, 0);
    }
    ct_db_close(s.db);
}

CT_SUITE(stats_screen_heatmap_color) {
    Color r0 = ct_heatmap_color(0.0);
    CT_ASSERT_EQ(r0.r, 255);
    CT_ASSERT_EQ(r0.g, 0);
    CT_ASSERT_EQ(r0.b, 0);
    CT_ASSERT_EQ(r0.a, 255);
    Color g1 = ct_heatmap_color(1.0);
    CT_ASSERT_EQ(g1.r, 0);
    CT_ASSERT_EQ(g1.g, 255);
    CT_ASSERT_EQ(g1.b, 0);
    CT_ASSERT_EQ(g1.a, 255);
    Color mid = ct_heatmap_color(0.5);
    CT_ASSERT_EQ(mid.r, 255);
    CT_ASSERT_EQ(mid.g, 255);
    CT_ASSERT_EQ(mid.b, 0);
    CT_ASSERT_EQ(mid.a, 255);
    Color mid2 = ct_heatmap_color(0.5);
    CT_ASSERT_EQ(mid2.r, mid.r);
    CT_ASSERT_EQ(mid2.g, mid.g);
    CT_ASSERT_EQ(mid2.b, mid.b);
    CT_ASSERT_EQ(mid2.a, mid.a);

    Color clamped_lo = ct_heatmap_color(-0.5);
    CT_ASSERT_EQ(clamped_lo.r, r0.r);
    CT_ASSERT_EQ(clamped_lo.g, r0.g);
    Color clamped_hi = ct_heatmap_color(2.5);
    CT_ASSERT_EQ(clamped_hi.r, g1.r);
    CT_ASSERT_EQ(clamped_hi.g, g1.g);
    double nan_v = 0.0 / 0.0;
    CT_ASSERT(!isfinite(nan_v));
    Color clamped_nan = ct_heatmap_color(nan_v);
    CT_ASSERT_EQ(clamped_nan.r, r0.r);
    CT_ASSERT_EQ(clamped_nan.g, r0.g);

    Color prev = ct_heatmap_color(0.0);
    for (int k = 0; k <= 20; k++) {
        double t = (double)k / 20.0;
        Color c = ct_heatmap_color(t);
        CT_ASSERT(c.r <= prev.r);
        CT_ASSERT(c.g >= prev.g);
        CT_ASSERT_EQ(c.b, 0);
        CT_ASSERT_EQ(c.a, 255);
        prev = c;
    }
    Color quarter = ct_heatmap_color(0.25);
    CT_ASSERT_EQ(quarter.r, 255);
    CT_ASSERT_EQ(quarter.g, 127);
    Color three_quarter = ct_heatmap_color(0.75);
    CT_ASSERT_EQ(three_quarter.r, 127);
    CT_ASSERT_EQ(three_quarter.g, 255);
}

CT_SUITE(stats_screen_deck_reset) {
    seed_state s;
    seed_build(&s);

    ct_stats_view v;
    CT_ASSERT_EQ(ct_stats_view_load(&v, s.db, NOW, NULL), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, 0);
    CT_ASSERT_EQ(v.case_count, 8);
    CT_ASSERT_EQ(v.retention[9].reviews, 8);
    CT_ASSERT_DBL_EQ(v.retention_summary.total_reviews, 16.0, 1e-12);

    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "ghost"), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, -1);
    CT_ASSERT_EQ(v.case_count, 0);
    CT_ASSERT_EQ(v.retention_count, CT_STATS_RETENTION_BUCKETS);
    CT_ASSERT_EQ(v.retention[9].reviews, 0);
    CT_ASSERT_DBL_EQ(v.retention_summary.total_reviews, 0.0, 1e-15);
    CT_ASSERT_DBL_EQ(v.retention_summary.total_correct, 0.0, 1e-15);
    CT_ASSERT_EQ(v.retention_summary.worst_bucket, -1);

    ct_db_close(s.db);
}

CT_SUITE(stats_screen_deck_lookup_full) {
    sqlite3 *db = seed_db();
    for (int i = 0; i < CT_STATS_MAX_DECKS; i++) {
        char deck[SEED_LABEL_MAX];
        char key[SEED_KEYS_MAX];
        snprintf(deck, sizeof deck, "d%02d", i);
        snprintf(key, sizeof key, "k%d", i);
        long id;
        seed_insert_card(db, deck, key, CT_DIR_FR, &id);
        CT_ASSERT(id > 0);
    }

    ct_stats_view v;
    CT_ASSERT_EQ(ct_stats_view_load(&v, db, NOW, NULL), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_count, CT_STATS_MAX_DECKS);
    CT_ASSERT_EQ(ct_stats_view_select_deck(&v, "@"), CT_REPO_OK);
    CT_ASSERT_EQ(v.deck_selected, -1);
    CT_ASSERT_EQ(v.case_count, 0);
    CT_ASSERT_EQ(v.retention_count, CT_STATS_RETENTION_BUCKETS);

    ct_db_close(db);
}

CT_SUITE(stats_screen_update_guard) {
    ct_screen_stats_update(NULL);
    ct_screen_stats_update(NULL);
    CT_ASSERT(1);
}
