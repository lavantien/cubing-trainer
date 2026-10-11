#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/storage/repo_reviews.h"
#include "framework.h"

static sqlite3 *cards_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static ct_card base_card(const char *deck, const char *case_key, int direction) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", case_key);
    c.direction = direction;
    snprintf(c.front_json, sizeof c.front_json, "%s", "{\"scramble\":\"R U R' F2\"}");
    snprintf(c.alg_3x3, sizeof c.alg_3x3, "%s", "R U R' U'");
    snprintf(c.alg_big, sizeof c.alg_big, "%s", "Rw U Rw' U'");
    snprintf(c.alg_oh, sizeof c.alg_oh, "%s", "R U2 R'");
    snprintf(c.notes, sizeof c.notes, "%s", "train daily");
    return c;
}

static long add_card(sqlite3 *db, const char *deck, const char *case_key, int direction) {
    ct_card c = base_card(deck, case_key, direction);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    CT_ASSERT(c.id > 0);
    return c.id;
}

static ct_srs_row srs_at(const char *due) {
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    row.stability = 3.5;
    row.difficulty = 5.5;
    snprintf(row.due, sizeof row.due, "%s", due);
    snprintf(row.last_review, sizeof row.last_review, "%s", "2026-05-01T00:00:00Z");
    row.reps = 2;
    row.lapses = 1;
    row.state = 1;
    return row;
}

CT_SUITE(card_insert_get) {
    sqlite3 *db = cards_db();
    ct_card c = base_card("cfop-oll", "oll-27", CT_DIR_FL);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    long id = c.id;
    CT_ASSERT(id > 0);
    CT_ASSERT_EQ(strlen(c.created_at), 20u);
    CT_ASSERT_EQ(strlen(c.updated_at), 20u);
    CT_ASSERT_STR_EQ(c.created_at, c.updated_at);

    ct_card back;
    memset(&back, 0xAA, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, id, &back), CT_REPO_OK);
    CT_ASSERT_EQ(back.id, id);
    CT_ASSERT_STR_EQ(back.deck, "cfop-oll");
    CT_ASSERT_STR_EQ(back.case_key, "oll-27");
    CT_ASSERT_EQ(back.direction, CT_DIR_FL);
    CT_ASSERT_STR_EQ(back.front_json, "{\"scramble\":\"R U R' F2\"}");
    CT_ASSERT_STR_EQ(back.alg_3x3, "R U R' U'");
    CT_ASSERT_STR_EQ(back.alg_big, "Rw U Rw' U'");
    CT_ASSERT_STR_EQ(back.alg_oh, "R U2 R'");
    CT_ASSERT_STR_EQ(back.notes, "train daily");
    CT_ASSERT_STR_EQ(back.created_at, c.created_at);
    CT_ASSERT_STR_EQ(back.updated_at, c.updated_at);

    CT_ASSERT_EQ(ct_repo_card_delete(db, id), CT_REPO_OK);
    memset(&back, 0xAA, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, id, &back), CT_REPO_MISSING);
    CT_ASSERT_EQ(back.id, 0);
    CT_ASSERT_EQ(back.deck[0], '\0');

    char long_deck[CT_CARD_DECK_MAX + 1];
    memset(long_deck, 'd', CT_CARD_DECK_MAX);
    long_deck[CT_CARD_DECK_MAX] = '\0';
    char sql[320];
    snprintf(sql, sizeof sql,
             "INSERT INTO flashcard(deck, case_key, direction, created_at, updated_at)"
             " VALUES ('%s', 'long', 0, '2026-06-01T00:00:00Z', '2026-06-01T00:00:00Z')",
             long_deck);
    CT_ASSERT_EQ(sqlite3_exec(db, sql, NULL, NULL, NULL), SQLITE_OK);
    long over_id = (long)sqlite3_last_insert_rowid(db);
    memset(&back, 0xAA, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, over_id, &back), CT_REPO_OK);
    CT_ASSERT_EQ(strlen(back.deck), CT_CARD_DECK_MAX - 1);
    CT_ASSERT_EQ(back.deck[CT_CARD_DECK_MAX - 1], '\0');
    CT_ASSERT_EQ(sqlite3_exec(db, "DELETE FROM flashcard WHERE id < 0", NULL, NULL, NULL),
                 SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_exec(db, "DELETE FROM flashcard", NULL, NULL, NULL), SQLITE_OK);
    ct_db_close(db);
}

CT_SUITE(card_unique_conflict) {
    sqlite3 *db = cards_db();
    ct_card a = base_card("deck", "key", CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &a), CT_REPO_OK);

    ct_card dup = base_card("deck", "key", CT_DIR_FR);
    snprintf(dup.notes, sizeof dup.notes, "%s", "other");
    CT_ASSERT_EQ(ct_repo_card_insert(db, &dup), CT_REPO_CONFLICT);
    CT_ASSERT_EQ(dup.id, 0);

    ct_card back;
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, a.id, &back), CT_REPO_OK);
    CT_ASSERT_STR_EQ(back.notes, "train daily");

    ct_card other_dir = base_card("deck", "key", CT_DIR_BR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &other_dir), CT_REPO_OK);
    ct_card other_deck = base_card("deck2", "key", CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &other_deck), CT_REPO_OK);
    ct_db_close(db);
}

CT_SUITE(card_update) {
    sqlite3 *db = cards_db();
    ct_card c = base_card("up", "one", CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    char created[CT_CARD_STAMP_MAX];
    snprintf(created, sizeof created, "%s", c.created_at);

    time_t t0 = time(NULL);
    while (time(NULL) == t0) {
    }

    snprintf(c.alg_3x3, sizeof c.alg_3x3, "%s", "F R U R'");
    snprintf(c.notes, sizeof c.notes, "%s", "renoted");
    c.direction = CT_DIR_BL;
    CT_ASSERT_EQ(ct_repo_card_update(db, &c), CT_REPO_OK);

    ct_card back;
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, c.id, &back), CT_REPO_OK);
    CT_ASSERT_STR_EQ(back.alg_3x3, "F R U R'");
    CT_ASSERT_STR_EQ(back.notes, "renoted");
    CT_ASSERT_EQ(back.direction, CT_DIR_BL);
    CT_ASSERT_STR_EQ(back.created_at, created);
    CT_ASSERT(strcmp(back.updated_at, created) > 0);
    CT_ASSERT(strcmp(back.updated_at, back.created_at) > 0);

    ct_card ghost = base_card("up", "ghost", CT_DIR_FR);
    ghost.id = c.id + 1000;
    CT_ASSERT_EQ(ct_repo_card_update(db, &ghost), CT_REPO_MISSING);

    ct_card b2 = base_card("up", "two", CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &b2), CT_REPO_OK);
    ct_card clash = base_card("up", "one", CT_DIR_BL);
    clash.id = b2.id;
    CT_ASSERT_EQ(ct_repo_card_update(db, &clash), CT_REPO_CONFLICT);
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, b2.id, &back), CT_REPO_OK);
    CT_ASSERT_STR_EQ(back.case_key, "two");
    ct_db_close(db);
}

CT_SUITE(card_delete_cascade) {
    sqlite3 *db = cards_db();
    long a = add_card(db, "d", "a", CT_DIR_FR);
    long b = add_card(db, "d", "b", CT_DIR_FR);

    ct_srs_row row = srs_at("2026-06-01T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, a, &row), CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_review_log(db, a, CT_GOOD, "2026-05-01T00:00:00Z", 3.25, 5.5, 12.0, 21.0),
                 CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_review_log(db, a, CT_EASY, "2026-05-02T00:00:00Z", 4.0, 5.0, 1.0, 30.0),
                 CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_review_log(db, b, CT_HARD, "2026-05-03T00:00:00Z", 2.0, 6.0, 2.0, 8.0),
                 CT_REPO_OK);

    ct_review_row revs[4];
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, a, revs, 4), 2);

    CT_ASSERT_EQ(ct_repo_card_delete(db, a), CT_REPO_OK);

    ct_srs_row srs_back;
    memset(&srs_back, 0xAA, sizeof srs_back);
    CT_ASSERT_EQ(ct_repo_srs_get(db, a, &srs_back), CT_REPO_MISSING);
    CT_ASSERT_EQ(srs_back.reps, 0);
    CT_ASSERT_EQ(srs_back.due[0], '\0');
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, a, revs, 4), 0);
    CT_ASSERT_EQ(ct_repo_review_log(db, a, CT_GOOD, "2026-05-04T00:00:00Z", 1.0, 5.0, 0.0, 1.0),
                 CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, b, revs, 4), 1);
    CT_ASSERT_EQ(revs[0].card_id, b);
    CT_ASSERT_EQ(ct_repo_card_delete(db, a), CT_REPO_MISSING);
    ct_db_close(db);
}

CT_SUITE(card_list) {
    sqlite3 *db = cards_db();
    long c_id = add_card(db, "alpha", "c", CT_DIR_FR);
    long a_id = add_card(db, "alpha", "a", CT_DIR_FR);
    long b_id = add_card(db, "alpha", "b", CT_DIR_FR);
    add_card(db, "beta", "z", CT_DIR_FR);

    ct_card cards[8];
    memset(cards, 0xAA, sizeof cards);
    CT_ASSERT_EQ(ct_repo_card_list(db, "alpha", cards, 8), 3);
    CT_ASSERT_EQ(cards[0].id, c_id);
    CT_ASSERT_EQ(cards[1].id, a_id);
    CT_ASSERT_EQ(cards[2].id, b_id);
    CT_ASSERT_STR_EQ(cards[0].case_key, "c");
    CT_ASSERT_EQ(cards[3].id, 0);

    ct_card two[2];
    memset(two, 0, sizeof two);
    CT_ASSERT_EQ(ct_repo_card_list(db, "alpha", two, 2), 2);
    CT_ASSERT_EQ(two[0].id, c_id);
    CT_ASSERT_EQ(two[1].id, a_id);

    CT_ASSERT_EQ(ct_repo_card_list(db, "beta", two, 2), 1);
    CT_ASSERT_EQ(ct_repo_card_list(db, "gamma", two, 2), 0);
    CT_ASSERT_EQ(ct_repo_card_list(db, "alpha", two, 0), 0);
    CT_ASSERT(ct_repo_card_list(db, NULL, two, 2) < 0);
    CT_ASSERT_EQ(ct_repo_card_list(db, "alpha", NULL, 0), 0);
    CT_ASSERT(ct_repo_card_list(db, "alpha", NULL, 1) < 0);
    ct_db_close(db);
}

CT_SUITE(card_get_by_key) {
    sqlite3 *db = cards_db();
    ct_card fr = base_card("deck", "key", CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &fr), CT_REPO_OK);
    ct_card bl = base_card("deck", "key", CT_DIR_BL);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &bl), CT_REPO_OK);

    ct_card out;
    memset(&out, 0, sizeof out);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "deck", "key", CT_DIR_FR, &out), CT_REPO_OK);
    CT_ASSERT_EQ(out.id, fr.id);
    CT_ASSERT_EQ(out.direction, CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "deck", "key", CT_DIR_BL, &out), CT_REPO_OK);
    CT_ASSERT_EQ(out.id, bl.id);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "deck", "key", CT_DIR_FL, &out), CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "other", "key", CT_DIR_FR, &out), CT_REPO_MISSING);

    char key63[CT_CARD_CASE_KEY_MAX];
    memset(key63, 'k', sizeof key63 - 1);
    key63[sizeof key63 - 1] = '\0';
    ct_card k63 = base_card("deck", key63, CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &k63), CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "deck", key63, CT_DIR_FR, &out), CT_REPO_OK);
    CT_ASSERT_EQ(out.id, k63.id);

    char big[CT_CARD_DECK_MAX + 1];
    memset(big, 'd', sizeof big - 1);
    big[sizeof big - 1] = '\0';
    CT_ASSERT(ct_repo_card_get_by_key(db, big, "key", CT_DIR_FR, &out) < 0);
    CT_ASSERT(ct_repo_card_get_by_key(db, "deck", "", CT_DIR_FR, &out) < 0);

    memset(&out, 0xAA, sizeof out);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "deck", "key", CT_DIR_FL, &out), CT_REPO_MISSING);
    CT_ASSERT_EQ(out.id, 0);
    CT_ASSERT_EQ(out.deck[0], '\0');
    ct_db_close(db);
}

CT_SUITE(srs_roundtrip) {
    sqlite3 *db = cards_db();
    long id = add_card(db, "d", "k", CT_DIR_FR);

    ct_srs_row back;
    memset(&back, 0xAA, sizeof back);
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &back), CT_REPO_MISSING);
    CT_ASSERT_EQ(back.reps, 0);
    CT_ASSERT_EQ(back.due[0], '\0');
    CT_ASSERT(ct_repo_srs_get(db, 0, &back) < 0);

    ct_srs_row row = srs_at("2026-06-01T00:00:00Z");
    row.stability = 12.5;
    row.difficulty = 6.25;
    row.last_review[0] = '\0';
    snprintf(row.last_review, sizeof row.last_review, "%s", "2026-05-01T12:00:00Z");
    row.reps = 9;
    row.lapses = 3;
    row.state = 2;
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);

    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &back), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(back.stability, 12.5, 1e-9);
    CT_ASSERT_DBL_EQ(back.difficulty, 6.25, 1e-9);
    CT_ASSERT_STR_EQ(back.due, "2026-06-01T00:00:00Z");
    CT_ASSERT_STR_EQ(back.last_review, "2026-05-01T12:00:00Z");
    CT_ASSERT_EQ(back.reps, 9);
    CT_ASSERT_EQ(back.lapses, 3);
    CT_ASSERT_EQ(back.state, 2);

    row.stability = 40.0;
    row.reps = 10;
    row.lapses = 4;
    snprintf(row.due, sizeof row.due, "%s", "2026-08-01T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, id, &row), CT_REPO_OK);
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_srs_get(db, id, &back), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(back.stability, 40.0, 1e-9);
    CT_ASSERT_EQ(back.reps, 10);
    CT_ASSERT_EQ(back.lapses, 4);
    CT_ASSERT_STR_EQ(back.due, "2026-08-01T00:00:00Z");

    CT_ASSERT_EQ(ct_repo_srs_put(db, id + 500, &row), CT_REPO_MISSING);
    ct_srs_row bad;
    memset(&bad, 0, sizeof bad);
    CT_ASSERT(ct_repo_srs_put(db, id, &bad) < 0);
    CT_ASSERT(ct_repo_srs_put(db, id, NULL) < 0);
    ct_db_close(db);
}

CT_SUITE(review_log_rows) {
    sqlite3 *db = cards_db();
    long a = add_card(db, "d", "a", CT_DIR_FR);
    long b = add_card(db, "d", "b", CT_DIR_FR);

    CT_ASSERT_EQ(ct_repo_review_log(db, a, CT_AGAIN, "2026-05-01T00:00:00Z", 1.5, 7.0, 0.0, 0.5),
                 CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_review_log(db, a, CT_GOOD, "2026-05-02T00:00:00Z", 4.5, 6.0, 1.0, 6.0),
                 CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_review_log(db, a, CT_EASY, "2026-05-03T00:00:00Z", 9.0, 5.0, 1.0, 12.0),
                 CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_review_log(db, b, CT_HARD, "2026-05-01T06:00:00Z", 2.5, 6.5, 3.0, 3.0),
                 CT_REPO_OK);

    ct_review_row rows[6];
    memset(rows, 0, sizeof rows);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, a, rows, 6), 3);
    CT_ASSERT(rows[0].id < rows[1].id);
    CT_ASSERT(rows[1].id < rows[2].id);
    CT_ASSERT_EQ(rows[0].card_id, a);
    CT_ASSERT_EQ(rows[1].card_id, a);
    CT_ASSERT_EQ(rows[2].card_id, a);
    CT_ASSERT_EQ(rows[0].rating, CT_AGAIN);
    CT_ASSERT_STR_EQ(rows[0].reviewed_at, "2026-05-01T00:00:00Z");
    CT_ASSERT_DBL_EQ(rows[0].stability, 1.5, 1e-9);
    CT_ASSERT_DBL_EQ(rows[0].difficulty, 7.0, 1e-9);
    CT_ASSERT_DBL_EQ(rows[0].elapsed_days, 0.0, 1e-9);
    CT_ASSERT_DBL_EQ(rows[0].scheduled_days, 0.5, 1e-9);
    CT_ASSERT_EQ(rows[2].rating, CT_EASY);
    CT_ASSERT_DBL_EQ(rows[2].stability, 9.0, 1e-9);
    CT_ASSERT_DBL_EQ(rows[2].scheduled_days, 12.0, 1e-9);

    memset(rows, 0, sizeof rows);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, b, rows, 6), 1);
    CT_ASSERT_EQ(rows[0].card_id, b);
    CT_ASSERT_EQ(rows[0].rating, CT_HARD);
    CT_ASSERT_DBL_EQ(rows[0].elapsed_days, 3.0, 1e-9);

    memset(rows, 0, sizeof rows);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, a, rows, 2), 2);
    CT_ASSERT_EQ(rows[0].rating, CT_AGAIN);
    CT_ASSERT_EQ(rows[1].rating, CT_GOOD);
    CT_ASSERT_EQ(ct_repo_reviews_for_card(db, 4242, rows, 6), 0);

    CT_ASSERT(ct_repo_review_log(db, a, 0, "2026-05-04T00:00:00Z", 1.0, 5.0, 0.0, 1.0) < 0);
    CT_ASSERT(ct_repo_review_log(db, a, 5, "2026-05-04T00:00:00Z", 1.0, 5.0, 0.0, 1.0) < 0);
    CT_ASSERT(ct_repo_review_log(db, a, CT_GOOD, "", 1.0, 5.0, 0.0, 1.0) < 0);
    CT_ASSERT(ct_repo_review_log(db, a, CT_GOOD, NULL, 1.0, 5.0, 0.0, 1.0) < 0);
    char long_at[CT_CARD_STAMP_MAX + 1];
    memset(long_at, 'x', sizeof long_at - 1);
    long_at[sizeof long_at - 1] = '\0';
    CT_ASSERT(ct_repo_review_log(db, a, CT_GOOD, long_at, 1.0, 5.0, 0.0, 1.0) < 0);
    CT_ASSERT(ct_repo_review_log(NULL, a, CT_GOOD, "2026-05-04T00:00:00Z", 1.0, 5.0, 0.0, 1.0) < 0);
    CT_ASSERT_EQ(
        ct_repo_review_log(db, a + 900, CT_GOOD, "2026-05-04T00:00:00Z", 1.0, 5.0, 0.0, 1.0),
        CT_REPO_MISSING);
    ct_db_close(db);
}

CT_SUITE(due_cards) {
    sqlite3 *db = cards_db();
    const char *now = "2026-06-01T00:00:00Z";
    long before = add_card(db, "due", "before", CT_DIR_FR);
    long equal = add_card(db, "due", "equal", CT_DIR_FR);
    long after = add_card(db, "due", "after", CT_DIR_FR);
    long other = add_card(db, "other", "x", CT_DIR_FR);
    add_card(db, "due", "nosrs", CT_DIR_FR);

    ct_srs_row row = srs_at("2026-05-31T23:59:59Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, before, &row), CT_REPO_OK);
    snprintf(row.due, sizeof row.due, "%s", now);
    CT_ASSERT_EQ(ct_repo_srs_put(db, equal, &row), CT_REPO_OK);
    snprintf(row.due, sizeof row.due, "%s", "2026-06-02T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, after, &row), CT_REPO_OK);
    snprintf(row.due, sizeof row.due, "%s", "2026-05-01T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, other, &row), CT_REPO_OK);

    long ids[8];
    memset(ids, 0, sizeof ids);
    CT_ASSERT_EQ(ct_repo_due_cards(db, "due", now, ids, 8), 2);
    CT_ASSERT_EQ(ids[0], before);
    CT_ASSERT_EQ(ids[1], equal);

    memset(ids, 0, sizeof ids);
    CT_ASSERT_EQ(ct_repo_due_cards(db, "due", now, ids, 1), 1);
    CT_ASSERT_EQ(ids[0], before);

    CT_ASSERT_EQ(ct_repo_due_cards(db, "other", now, ids, 8), 1);
    CT_ASSERT_EQ(ids[0], other);
    CT_ASSERT_EQ(ct_repo_due_cards(db, "empty", now, ids, 8), 0);

    long tie_a = add_card(db, "due2", "tie-a", CT_DIR_FR);
    long tie_b = add_card(db, "due2", "tie-b", CT_DIR_FR);
    ct_srs_row tie = srs_at("2026-05-15T00:00:00Z");
    CT_ASSERT_EQ(ct_repo_srs_put(db, tie_a, &tie), CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_srs_put(db, tie_b, &tie), CT_REPO_OK);
    memset(ids, 0, sizeof ids);
    CT_ASSERT_EQ(ct_repo_due_cards(db, "due2", now, ids, 8), 2);
    CT_ASSERT_EQ(ids[0], tie_a);
    CT_ASSERT_EQ(ids[1], tie_b);

    CT_ASSERT(ct_repo_due_cards(db, NULL, now, ids, 8) < 0);
    CT_ASSERT(ct_repo_due_cards(db, "due", NULL, ids, 8) < 0);
    CT_ASSERT(ct_repo_due_cards(db, "due", "", ids, 8) < 0);
    char big[CT_CARD_DECK_MAX + 1];
    memset(big, 'd', sizeof big - 1);
    big[sizeof big - 1] = '\0';
    CT_ASSERT(ct_repo_due_cards(db, big, now, ids, 8) < 0);
    char long_now[CT_CARD_STAMP_MAX + 1];
    memset(long_now, 'n', sizeof long_now - 1);
    long_now[sizeof long_now - 1] = '\0';
    CT_ASSERT(ct_repo_due_cards(db, "due", long_now, ids, 8) < 0);
    ct_db_close(db);
}

CT_SUITE(card_write_boundaries) {
    sqlite3 *db = cards_db();
    ct_card dir_bad = base_card("d", "k7", CT_DIR_FR);
    dir_bad.direction = CT_DIR_BL + 1;
    CT_ASSERT_EQ(ct_repo_card_insert(db, &dir_bad), CT_REPO_ERR_ARG);
    dir_bad.direction = CT_DIR_FR - 1;
    CT_ASSERT_EQ(ct_repo_card_insert(db, &dir_bad), CT_REPO_ERR_ARG);
    dir_bad.direction = 7;
    CT_ASSERT_EQ(ct_repo_card_insert(db, &dir_bad), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_card_update(db, &dir_bad), CT_REPO_ERR_ARG);

    const char *bad_keys[] = {" lead",       "trail ",     "embed\ttab",
                              "two  spaces", "nl\ninside", "cr\rinside"};
    for (unsigned i = 0; i < sizeof bad_keys / sizeof bad_keys[0]; i++) {
        ct_card ck = base_card("d", bad_keys[i], CT_DIR_FR);
        CT_ASSERT_EQ(ct_repo_card_insert(db, &ck), CT_REPO_ERR_ARG);
    }

    char key63[CT_CARD_CASE_KEY_MAX];
    memset(key63, 'k', sizeof key63 - 1);
    key63[sizeof key63 - 1] = '\0';
    ct_card full = base_card("full", key63, CT_DIR_BL);
    memset(full.front_json, 'j', CT_CARD_FRONT_JSON_MAX - 1);
    full.front_json[CT_CARD_FRONT_JSON_MAX - 1] = '\0';
    memset(full.alg_3x3, 'R', CT_CARD_ALG_MAX - 1);
    full.alg_3x3[CT_CARD_ALG_MAX - 1] = '\0';
    memset(full.alg_big, 'L', CT_CARD_ALG_MAX - 1);
    full.alg_big[CT_CARD_ALG_MAX - 1] = '\0';
    memset(full.alg_oh, 'U', CT_CARD_ALG_MAX - 1);
    full.alg_oh[CT_CARD_ALG_MAX - 1] = '\0';
    memset(full.notes, 'n', CT_CARD_NOTES_MAX - 1);
    full.notes[CT_CARD_NOTES_MAX - 1] = '\0';
    CT_ASSERT_EQ(ct_repo_card_insert(db, &full), CT_REPO_OK);
    ct_card back;
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, full.id, &back), CT_REPO_OK);
    CT_ASSERT_EQ(strlen(back.case_key), (size_t)(CT_CARD_CASE_KEY_MAX - 1));
    CT_ASSERT_EQ(strlen(back.front_json), (size_t)(CT_CARD_FRONT_JSON_MAX - 1));
    CT_ASSERT_EQ(strlen(back.alg_3x3), (size_t)(CT_CARD_ALG_MAX - 1));
    CT_ASSERT_EQ(strlen(back.alg_big), (size_t)(CT_CARD_ALG_MAX - 1));
    CT_ASSERT_EQ(strlen(back.alg_oh), (size_t)(CT_CARD_ALG_MAX - 1));
    CT_ASSERT_EQ(strlen(back.notes), (size_t)(CT_CARD_NOTES_MAX - 1));
    CT_ASSERT_STR_EQ(back.case_key, full.case_key);
    CT_ASSERT_STR_EQ(back.front_json, full.front_json);
    CT_ASSERT_STR_EQ(back.notes, full.notes);

    ct_card unterm = base_card("d", "unterm", CT_DIR_FR);
    memset(unterm.alg_3x3, 'R', sizeof unterm.alg_3x3);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &unterm), CT_REPO_ERR_ARG);
    unterm = base_card("d", "unterm2", CT_DIR_FR);
    memset(unterm.front_json, 'j', sizeof unterm.front_json);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &unterm), CT_REPO_ERR_ARG);
    unterm = base_card("d", "unterm3", CT_DIR_FR);
    memset(unterm.notes, 'n', sizeof unterm.notes);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &unterm), CT_REPO_ERR_ARG);
    unterm = base_card("d", "unterm4", CT_DIR_FR);
    memset(unterm.deck, 'd', sizeof unterm.deck);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &unterm), CT_REPO_ERR_ARG);

    ct_card empty_optional = base_card("d", "empty-opt", CT_DIR_FR);
    empty_optional.front_json[0] = '\0';
    empty_optional.alg_3x3[0] = '\0';
    empty_optional.alg_big[0] = '\0';
    empty_optional.alg_oh[0] = '\0';
    empty_optional.notes[0] = '\0';
    CT_ASSERT_EQ(ct_repo_card_insert(db, &empty_optional), CT_REPO_OK);
    memset(&back, 0, sizeof back);
    CT_ASSERT_EQ(ct_repo_card_get(db, empty_optional.id, &back), CT_REPO_OK);
    CT_ASSERT_EQ(back.front_json[0], '\0');
    CT_ASSERT_EQ(back.notes[0], '\0');

    ct_card out;
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "d", "k7", CT_DIR_BL + 1, &out), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "d", " lead", CT_DIR_FR, &out), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "d", "trail ", CT_DIR_FR, &out), CT_REPO_ERR_ARG);
    ct_db_close(db);
}

CT_SUITE(card_bad_args) {
    sqlite3 *db = cards_db();
    ct_card c = base_card("d", "k", CT_DIR_FR);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    ct_card out;
    ct_srs_row row = srs_at("2026-06-01T00:00:00Z");
    ct_review_row rev;

    CT_ASSERT(ct_repo_card_insert(NULL, &c) < 0);
    CT_ASSERT(ct_repo_card_insert(db, NULL) < 0);
    ct_card empty_deck = base_card("", "k", CT_DIR_FR);
    CT_ASSERT(ct_repo_card_insert(db, &empty_deck) < 0);
    ct_card empty_key = base_card("d", "", CT_DIR_FR);
    CT_ASSERT(ct_repo_card_insert(db, &empty_key) < 0);

    CT_ASSERT(ct_repo_card_update(NULL, &c) < 0);
    CT_ASSERT(ct_repo_card_update(db, NULL) < 0);
    ct_card zero_id = base_card("d", "k2", CT_DIR_FR);
    zero_id.id = 0;
    CT_ASSERT(ct_repo_card_update(db, &zero_id) < 0);
    ct_card up_empty = base_card("", "k3", CT_DIR_FR);
    up_empty.id = c.id;
    CT_ASSERT(ct_repo_card_update(db, &up_empty) < 0);

    CT_ASSERT(ct_repo_card_get(NULL, 1, &out) < 0);
    CT_ASSERT(ct_repo_card_get(db, 1, NULL) < 0);
    CT_ASSERT(ct_repo_card_get(db, 0, &out) < 0);

    CT_ASSERT(ct_repo_card_delete(NULL, 1) < 0);
    CT_ASSERT(ct_repo_card_delete(db, 0) < 0);

    CT_ASSERT(ct_repo_card_get_by_key(NULL, "d", "k", CT_DIR_FR, &out) < 0);
    CT_ASSERT(ct_repo_card_get_by_key(db, "d", "k", CT_DIR_FR, NULL) < 0);
    CT_ASSERT(ct_repo_card_get_by_key(db, NULL, "k", CT_DIR_FR, &out) < 0);

    CT_ASSERT(ct_repo_card_list(NULL, "d", &out, 1) < 0);
    CT_ASSERT(ct_repo_card_list(db, "", &out, 1) < 0);
    char big[CT_CARD_DECK_MAX + 1];
    memset(big, 'd', sizeof big - 1);
    big[sizeof big - 1] = '\0';
    CT_ASSERT(ct_repo_card_list(db, big, &out, 1) < 0);
    CT_ASSERT(ct_repo_card_list(db, "d", &out, -1) < 0);

    CT_ASSERT(ct_repo_srs_get(NULL, 1, &row) < 0);
    CT_ASSERT(ct_repo_srs_get(db, 1, NULL) < 0);
    CT_ASSERT(ct_repo_srs_put(NULL, 1, &row) < 0);
    CT_ASSERT(ct_repo_srs_put(db, 0, &row) < 0);

    CT_ASSERT(ct_repo_reviews_for_card(NULL, 1, &rev, 1) < 0);
    CT_ASSERT(ct_repo_reviews_for_card(db, 1, NULL, 1) < 0);
    CT_ASSERT(ct_repo_reviews_for_card(db, 0, &rev, 1) < 0);
    CT_ASSERT(ct_repo_reviews_for_card(db, 1, &rev, -1) < 0);
    CT_ASSERT(ct_repo_due_cards(NULL, "d", "2026-06-01T00:00:00Z", NULL, 0) < 0);
    ct_db_close(db);
}

CT_SUITE(now_iso) {
    char stamp[CT_CARD_STAMP_MAX];
    memset(stamp, 'x', sizeof stamp);
    ct_now_utc_iso(stamp, sizeof stamp);
    CT_ASSERT_EQ(strlen(stamp), 20u);
    CT_ASSERT_EQ(stamp[19], 'Z');
    char exact[21];
    memset(exact, 'x', sizeof exact);
    ct_now_utc_iso(exact, 21);
    CT_ASSERT_EQ(strlen(exact), 20u);
    static const char pattern[] = "0000-00-00T00:00:00Z";
    for (int i = 0; i < 20; i++) {
        if (pattern[i] == '0') {
            CT_ASSERT(stamp[i] >= '0' && stamp[i] <= '9');
        } else {
            CT_ASSERT_EQ(stamp[i], pattern[i]);
        }
    }
    CT_ASSERT(strcmp(stamp, "2026-01-01T00:00:00Z") > 0);

    char small[CT_CARD_STAMP_MAX];
    memset(small, 'y', sizeof small);
    ct_now_utc_iso(small, 20);
    CT_ASSERT_EQ(small[0], '\0');

    ct_now_utc_iso(NULL, sizeof stamp);
    CT_ASSERT(strlen(stamp) == 20);
}
