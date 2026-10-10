#include "config/config.h"
#include "framework.h"
#include "storage/db.h"
#include "storage/repo_algs.h"
#include "storage/repo_cards.h"
#include "storage/repo_reviews.h"
#include "storage/repo_settings.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

int ct_fuzz_iterations(void);

enum {
    CT_DBS_ITERS = 10000,
    CT_DBS_TEXT_MAX = 640,
    CT_DBS_DUE_CAP = 24,
    CT_DBS_ROWS_CAP = 32,
    CT_DBS_IDS_CAP = 40
};

#define CT_FZ(cond, seed)                                                                          \
    do {                                                                                           \
        if (cond) {                                                                                \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_eq(__FILE__, __LINE__, #cond, "seed", (unsigned long long)(seed), 0);          \
        }                                                                                          \
    } while (0)

static char ct_dbs_byte(ct_sfc64 *s) {
    static const char nasties[] = {'"', '\'', ';', '%', '_', '-', '\\', '\n', '\r', '\t'};
    switch ((int)ct_sfc64_range(s, 4)) {
    case 0:
        return nasties[ct_sfc64_range(s, sizeof nasties)];
    case 1:
        return (char)('a' + ct_sfc64_range(s, 26));
    case 2:
        return (char)(0x80 + ct_sfc64_range(s, 0x80));
    default:
        return (char)(1 + ct_sfc64_range(s, 255));
    }
}

static unsigned ct_dbs_fill(ct_sfc64 *s, char *dst, unsigned cap, unsigned want) {
    unsigned n = want < cap - 1 ? want : cap - 1;
    for (unsigned k = 0; k < n; k++)
        dst[k] = ct_dbs_byte(s);
    dst[n] = '\0';
    return n;
}

static int ct_dbs_open(sqlite3 **db) {
    *db = NULL;
    if (ct_db_open(db, ":memory:") != SQLITE_OK) {
        return -1;
    }
    if (ct_db_migrate(*db, "db/migrations") != 0) {
        ct_db_close(*db);
        *db = NULL;
        return -1;
    }
    return 0;
}

static bool ct_dbs_case_key_canonical(const char *key, unsigned len) {
    if (len == 0 || len >= CT_CARD_CASE_KEY_MAX)
        return false;
    for (unsigned k = 0; k < len; k++) {
        unsigned char ch = (unsigned char)key[k];
        if (ch <= CT_CASE_KEY_FORBIDDEN || ch == CT_CASE_KEY_DELETE)
            return false;
    }
    return true;
}

static void ct_dbs_field(ct_sfc64 *s, char *dst, unsigned cap, bool terminate) {
    unsigned want = (unsigned)ct_sfc64_range(s, cap + 4);
    unsigned n = terminate ? (want < cap - 1 ? want : cap - 1) : cap;
    for (unsigned k = 0; k < n; k++)
        dst[k] = ct_dbs_byte(s);
    if (terminate)
        dst[n] = '\0';
}

static bool ct_dbs_dir_ok(int direction) {
    return direction >= CT_DIR_FR && direction <= CT_DIR_BL;
}

CT_SUITE(fuzz_db_card_strings) {
    int iters = ct_fuzz_iterations();
    int count = iters < CT_DBS_ITERS ? iters : CT_DBS_ITERS;
    CT_ASSERT(count >= 1);
    for (int i = 0; i < count; i++) {
        uint64_t seed = 0xD3C00000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        sqlite3 *db = NULL;
        CT_FZ(ct_dbs_open(&db) == 0, seed);
        char deck_raw[CT_DBS_TEXT_MAX];
        char case_raw[CT_DBS_TEXT_MAX];
        unsigned deck_want = (unsigned)ct_sfc64_range(&s, CT_CARD_DECK_MAX + 8);
        unsigned case_want = (unsigned)ct_sfc64_range(&s, CT_CARD_CASE_KEY_MAX + 8);
        unsigned deck_len = ct_dbs_fill(&s, deck_raw, sizeof deck_raw, deck_want);
        unsigned case_len = ct_dbs_fill(&s, case_raw, sizeof case_raw, case_want);
        bool deck_fits = deck_len < CT_CARD_DECK_MAX && deck_len > 0;
        bool case_fits = ct_dbs_case_key_canonical(case_raw, case_len);
        ct_card probe;
        if (deck_len == 0 || deck_len >= CT_CARD_DECK_MAX) {
            CT_FZ(ct_repo_card_get_by_key(db, deck_raw, "k", 0, &probe) == CT_REPO_ERR_ARG, seed);
            CT_FZ(ct_repo_card_list(db, deck_raw, &probe, 1) == CT_REPO_ERR_ARG, seed);
        }
        if (!case_fits) {
            CT_FZ(ct_repo_card_get_by_key(db, "d", case_raw, 0, &probe) == CT_REPO_ERR_ARG, seed);
        }
        if (!deck_fits || !case_fits) {
            ct_db_close(db);
            continue;
        }
        ct_card c;
        memset(&c, 0, sizeof c);
        memcpy(c.deck, deck_raw, deck_len + 1);
        memcpy(c.case_key, case_raw, case_len + 1);
        c.direction = (int)ct_sfc64_range(&s, 8) - 2;
        ct_dbs_fill(&s, c.front_json, sizeof c.front_json,
                    (unsigned)ct_sfc64_range(&s, CT_CARD_FRONT_JSON_MAX + 16));
        ct_dbs_fill(&s, c.notes, sizeof c.notes,
                    (unsigned)ct_sfc64_range(&s, CT_CARD_NOTES_MAX + 16));
        ct_dbs_fill(&s, c.alg_3x3, sizeof c.alg_3x3,
                    (unsigned)ct_sfc64_range(&s, CT_CARD_ALG_MAX + 16));
        ct_dbs_fill(&s, c.alg_big, sizeof c.alg_big,
                    (unsigned)ct_sfc64_range(&s, CT_CARD_ALG_MAX + 16));
        ct_dbs_fill(&s, c.alg_oh, sizeof c.alg_oh,
                    (unsigned)ct_sfc64_range(&s, CT_CARD_ALG_MAX + 16));
        int rc;
        bool text_unterminated = ct_sfc64_range(&s, 4) == 0;
        if (text_unterminated)
            memset(c.notes, 'z', sizeof c.notes);
        rc = ct_repo_card_insert(db, &c);
        if (text_unterminated) {
            CT_FZ(rc == CT_REPO_ERR_ARG, seed);
            ct_db_close(db);
            continue;
        }
        if (!ct_dbs_dir_ok(c.direction)) {
            CT_FZ(rc == CT_REPO_ERR_ARG, seed);
            CT_FZ(ct_repo_card_get_by_key(db, c.deck, c.case_key, c.direction, &probe) ==
                      CT_REPO_ERR_ARG,
                  seed);
            ct_db_close(db);
            continue;
        }
        CT_FZ(rc == CT_REPO_OK, seed);
        CT_FZ(c.id > 0, seed);
        ct_card back;
        CT_FZ(ct_repo_card_get(db, c.id, &back) == CT_REPO_OK, seed);
        CT_FZ(back.id == c.id, seed);
        CT_FZ(back.direction == c.direction, seed);
        CT_FZ(strcmp(back.deck, c.deck) == 0, seed);
        CT_FZ(strcmp(back.case_key, c.case_key) == 0, seed);
        CT_FZ(strcmp(back.front_json, c.front_json) == 0, seed);
        CT_FZ(strcmp(back.notes, c.notes) == 0, seed);
        CT_FZ(strcmp(back.alg_3x3, c.alg_3x3) == 0, seed);
        CT_FZ(strcmp(back.alg_big, c.alg_big) == 0, seed);
        CT_FZ(strcmp(back.alg_oh, c.alg_oh) == 0, seed);
        CT_FZ(back.created_at[0] != '\0', seed);
        CT_FZ(back.updated_at[0] != '\0', seed);
        ct_dbs_fill(&s, c.front_json, sizeof c.front_json,
                    (unsigned)ct_sfc64_range(&s, CT_CARD_FRONT_JSON_MAX + 16));
        ct_dbs_fill(&s, c.notes, sizeof c.notes,
                    (unsigned)ct_sfc64_range(&s, CT_CARD_NOTES_MAX + 16));
        CT_FZ(ct_repo_card_update(db, &c) == CT_REPO_OK, seed);
        CT_FZ(ct_repo_card_get(db, c.id, &back) == CT_REPO_OK, seed);
        CT_FZ(strcmp(back.deck, c.deck) == 0, seed);
        CT_FZ(strcmp(back.case_key, c.case_key) == 0, seed);
        CT_FZ(strcmp(back.front_json, c.front_json) == 0, seed);
        CT_FZ(strcmp(back.notes, c.notes) == 0, seed);
        ct_card dup;
        memset(&dup, 0, sizeof dup);
        memcpy(dup.deck, c.deck, sizeof c.deck);
        memcpy(dup.case_key, c.case_key, sizeof c.case_key);
        dup.direction = c.direction;
        CT_FZ(ct_repo_card_insert(db, &dup) == CT_REPO_CONFLICT, seed);
        CT_FZ(ct_repo_card_get_by_key(db, c.deck, c.case_key, c.direction, &back) == CT_REPO_OK,
              seed);
        CT_FZ(back.id == c.id, seed);

        ct_generated_alg a;
        memset(&a, 0, sizeof a);
        a.orbit = CT_ORBIT_CORNER;
        a.buffer_mode = CT_BUFFER_FIXED;
        snprintf(a.target_pair, sizeof a.target_pair, "%s", "FU");
        bool seq_terminated = ct_sfc64_range(&s, 4) != 0;
        ct_dbs_field(&s, a.sequence, sizeof a.sequence, seq_terminated);
        unsigned seq_len =
            seq_terminated ? (unsigned)strlen(a.sequence) : (unsigned)sizeof a.sequence;
        ct_dbs_field(&s, a.setup, sizeof a.setup, true);
        ct_dbs_field(&s, a.interchange, sizeof a.interchange, true);
        ct_dbs_field(&s, a.insertion, sizeof a.insertion, true);
        ct_dbs_field(&s, a.score_parts_json, sizeof a.score_parts_json, true);
        int arc = ct_repo_alg_put(db, &a, "fz");
        if (!seq_terminated || seq_len == 0) {
            CT_FZ(arc == CT_REPO_ERR_ARG, seed);
        } else {
            CT_FZ(arc == CT_REPO_OK, seed);
            ct_generated_alg aback;
            memset(&aback, 0, sizeof aback);
            CT_FZ(ct_repo_alg_get(db, CT_ORBIT_CORNER, "FU", CT_BUFFER_FIXED, 0, "fz", &aback) ==
                      CT_REPO_OK,
                  seed);
            CT_FZ(strcmp(aback.sequence, a.sequence) == 0, seed);
            CT_FZ(strcmp(aback.setup, a.setup) == 0, seed);
            CT_FZ(strcmp(aback.interchange, a.interchange) == 0, seed);
            CT_FZ(strcmp(aback.insertion, a.insertion) == 0, seed);
            CT_FZ(strcmp(aback.score_parts_json, a.score_parts_json) == 0, seed);
        }
        ct_db_close(db);
    }
}

static void ct_dbs_stamp(ct_sfc64 *s, char *dst, unsigned cap, unsigned *len, bool *ok) {
    unsigned mode = (unsigned)ct_sfc64_range(s, 4);
    if (mode == 0) {
        snprintf(dst, cap, "20%02d-%02d-%02dT%02d:04:05Z", (unsigned)ct_sfc64_range(s, 100),
                 1 + (unsigned)ct_sfc64_range(s, 12), 1 + (unsigned)ct_sfc64_range(s, 28),
                 (unsigned)ct_sfc64_range(s, 24));
        *len = (unsigned)strlen(dst);
        *ok = true;
        return;
    }
    unsigned want = mode == 1
                        ? 1 + (unsigned)ct_sfc64_range(s, CT_CARD_STAMP_MAX)
                        : (mode == 2 ? 0 : CT_CARD_STAMP_MAX + (unsigned)ct_sfc64_range(s, 8));
    *len = ct_dbs_fill(s, dst, cap, want);
    *ok = *len >= 1 && *len < CT_CARD_STAMP_MAX;
}

CT_SUITE(fuzz_db_review_rows) {
    static const int ratings[] = {0, 1, 2, 3, 4, 5, -1, 7, INT_MAX, INT_MIN};
    int iters = ct_fuzz_iterations();
    int count = iters < CT_DBS_ITERS ? iters : CT_DBS_ITERS;
    CT_ASSERT(count >= 1);
    for (int i = 0; i < count; i++) {
        uint64_t seed = 0x5EED0000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        sqlite3 *db = NULL;
        CT_FZ(ct_dbs_open(&db) == 0, seed);
        ct_card anchor;
        memset(&anchor, 0, sizeof anchor);
        snprintf(anchor.deck, sizeof anchor.deck, "deck%d", i % 7);
        snprintf(anchor.case_key, sizeof anchor.case_key, "case%d", i % 5);
        anchor.direction = (int)ct_sfc64_range(&s, 4);
        CT_FZ(ct_repo_card_insert(db, &anchor) == CT_REPO_OK, seed);
        char stamp[CT_DBS_TEXT_MAX];
        unsigned stamp_len;
        bool stamp_ok;
        ct_dbs_stamp(&s, stamp, sizeof stamp, &stamp_len, &stamp_ok);
        int rating;
        if (ct_sfc64_range(&s, 2) == 0) {
            rating = ratings[ct_sfc64_range(&s, sizeof ratings / sizeof ratings[0])];
        } else {
            rating = (int)ct_sfc64_range(&s, 9) - 2;
        }
        bool rating_ok = rating >= CT_AGAIN && rating <= CT_EASY;
        double stab = ct_sfc64_uniform(&s) * 2000.0 - 1000.0;
        double diff = ct_sfc64_uniform(&s) * 20.0 - 5.0;
        double elapsed = ct_sfc64_uniform(&s) * 1000000.0;
        double sched = ct_sfc64_uniform(&s) * 1e18 - 5e17;
        int rc = ct_repo_review_log(db, anchor.id, rating, stamp, stab, diff, elapsed, sched);
        if (rating_ok && stamp_ok) {
            CT_FZ(rc == CT_REPO_OK, seed);
            ct_review_row rows[CT_DBS_ROWS_CAP];
            int n = ct_repo_reviews_for_card(db, anchor.id, rows, CT_DBS_ROWS_CAP);
            CT_FZ(n == 1, seed);
            CT_FZ(rows[0].card_id == anchor.id, seed);
            CT_FZ(rows[0].rating == rating, seed);
            CT_FZ(strcmp(rows[0].reviewed_at, stamp) == 0, seed);
            CT_FZ(rows[0].stability == stab, seed);
            CT_FZ(rows[0].difficulty == diff, seed);
            CT_FZ(rows[0].elapsed_days == elapsed, seed);
            CT_FZ(rows[0].scheduled_days == sched, seed);
        } else {
            CT_FZ(rc == CT_REPO_ERR_ARG, seed);
        }
        CT_FZ(ct_repo_review_log(db, 0, CT_GOOD, stamp, 1.0, 5.0, 1.0, 1.0) == CT_REPO_ERR_ARG,
              seed);
        long ghost = 1000000 + (long)ct_sfc64_range(&s, 100);
        CT_FZ(ct_repo_review_log(db, ghost, CT_GOOD, "2026-01-01T00:00:00Z", 1.0, 5.0, 1.0, 1.0) ==
                  CT_REPO_MISSING,
              seed);
        char deck[CT_CARD_DECK_MAX];
        ct_dbs_fill(&s, deck, sizeof deck, 1 + (unsigned)ct_sfc64_range(&s, 40));
        int due_count = 1 + (int)ct_sfc64_range(&s, CT_DBS_DUE_CAP);
        long ids[CT_DBS_IDS_CAP];
        char dues[CT_DBS_IDS_CAP][CT_CARD_STAMP_MAX];
        for (int k = 0; k < due_count; k++) {
            ct_card c;
            memset(&c, 0, sizeof c);
            memcpy(c.deck, deck, strlen(deck) + 1);
            snprintf(c.case_key, sizeof c.case_key, "k%d", k);
            CT_FZ(ct_repo_card_insert(db, &c) == CT_REPO_OK, seed);
            ids[k] = c.id;
            ct_srs_row row;
            memset(&row, 0, sizeof row);
            row.stability = ct_sfc64_uniform(&s) * 40000.0;
            row.difficulty = ct_sfc64_uniform(&s) * 10.0;
            ct_dbs_fill(&s, dues[k], CT_CARD_STAMP_MAX,
                        1 + (unsigned)ct_sfc64_range(&s, CT_CARD_STAMP_MAX - 1));
            memcpy(row.due, dues[k], strlen(dues[k]) + 1);
            row.reps = (int)ct_sfc64_range(&s, 100);
            row.lapses = (int)ct_sfc64_range(&s, 50);
            row.state = (int)ct_sfc64_range(&s, 2);
            CT_FZ(ct_repo_srs_put(db, ids[k], &row) == CT_REPO_OK, seed);
        }
        char now_raw[CT_DBS_TEXT_MAX];
        unsigned now_mode = (unsigned)ct_sfc64_range(&s, 4);
        const char *now_arg = now_raw;
        if (now_mode == 0) {
            snprintf(now_raw, sizeof now_raw, "2026-%02d-%02d",
                     1 + (unsigned)ct_sfc64_range(&s, 12), 1 + (unsigned)ct_sfc64_range(&s, 28));
        } else if (now_mode == 1) {
            ct_dbs_fill(&s, now_raw, sizeof now_raw,
                        1 + (unsigned)ct_sfc64_range(&s, CT_CARD_STAMP_MAX - 1));
        } else {
            ct_dbs_fill(&s, now_raw, sizeof now_raw,
                        now_mode == 2 ? 0 : CT_CARD_STAMP_MAX + (unsigned)ct_sfc64_range(&s, 8));
        }
        bool now_ok = strlen(now_arg) >= 1 && strlen(now_arg) < CT_CARD_STAMP_MAX;
        long got[CT_DBS_IDS_CAP];
        int cap = (int)ct_sfc64_range(&s, CT_DBS_IDS_CAP);
        int n_due = ct_repo_due_cards(db, deck, now_arg, got, cap);
        if (!now_ok) {
            CT_FZ(n_due == CT_REPO_ERR_ARG, seed);
        } else {
            int matches = 0;
            for (int k = 0; k < due_count; k++) {
                if (strcmp(dues[k], now_arg) <= 0) {
                    matches++;
                }
            }
            int expected = matches < cap ? matches : cap;
            CT_FZ(n_due == expected, seed);
            for (int k = 0; k < n_due; k++) {
                int found = -1;
                for (int j = 0; j < due_count; j++) {
                    if (ids[j] == got[k]) {
                        found = j;
                        break;
                    }
                }
                CT_FZ(found >= 0, seed);
                CT_FZ(strcmp(dues[found], now_arg) <= 0, seed);
                if (k > 0) {
                    int prev = -1;
                    for (int j = 0; j < due_count; j++) {
                        if (ids[j] == got[k - 1]) {
                            prev = j;
                            break;
                        }
                    }
                    int by_due = strcmp(dues[prev], dues[found]);
                    CT_FZ(by_due < 0 || (by_due == 0 && got[k - 1] < got[k]), seed);
                }
            }
        }
        ct_db_close(db);
    }
}

CT_SUITE(fuzz_db_settings_keys) {
    int iters = ct_fuzz_iterations();
    int count = iters < CT_DBS_ITERS ? iters : CT_DBS_ITERS;
    CT_ASSERT(count >= 1);
    for (int i = 0; i < count; i++) {
        uint64_t seed = 0x5E771000u + (uint64_t)i;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        sqlite3 *db = NULL;
        CT_FZ(ct_dbs_open(&db) == 0, seed);
        char key[128];
        ct_dbs_fill(&s, key, sizeof key, 1 + (unsigned)ct_sfc64_range(&s, 80));
        char value[CT_DBS_TEXT_MAX];
        unsigned value_len =
            ct_dbs_fill(&s, value, sizeof value, (unsigned)ct_sfc64_range(&s, 600));
        CT_FZ(ct_repo_settings_set(db, key, value) == 0, seed);
        char got[CT_DBS_TEXT_MAX];
        CT_FZ(ct_repo_settings_get(db, key, got, sizeof got) == 0, seed);
        CT_FZ(strcmp(got, value) == 0, seed);
        char small[64];
        unsigned cap = 1 + (unsigned)ct_sfc64_range(&s, 48);
        CT_FZ(ct_repo_settings_get(db, key, small, cap) == 0, seed);
        unsigned prefix = value_len < cap ? value_len : cap - 1;
        CT_FZ(memcmp(small, value, prefix) == 0, seed);
        CT_FZ(small[prefix] == '\0', seed);
        char second[CT_DBS_TEXT_MAX];
        ct_dbs_fill(&s, second, sizeof second, (unsigned)ct_sfc64_range(&s, 200));
        CT_FZ(ct_repo_settings_set(db, key, second) == 0, seed);
        CT_FZ(ct_repo_settings_get(db, key, got, sizeof got) == 0, seed);
        CT_FZ(strcmp(got, second) == 0, seed);
        char absent[130];
        snprintf(absent, sizeof absent, "%s%c", key, '\x01');
        CT_FZ(ct_repo_settings_get(db, absent, got, sizeof got) == 1, seed);
        CT_FZ(got[0] == '\0', seed);
        ct_db_close(db);
    }
}
