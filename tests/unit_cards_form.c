#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../src/ui/ui.h"
#include "framework.h"

static sqlite3 *form_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static long seed(sqlite3 *db, const char *deck, const char *key, const char *alg) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", key);
    c.direction = CT_DIR_FR;
    snprintf(c.alg_3x3, sizeof c.alg_3x3, "%s", alg);
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    CT_ASSERT(c.id > 0);
    return c.id;
}

static long seed_full(sqlite3 *db, const char *deck, const char *key) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", key);
    c.direction = CT_DIR_BL;
    snprintf(c.front_json, sizeof c.front_json, "%s", "{\"scramble\":\"F R U\"}");
    snprintf(c.alg_3x3, sizeof c.alg_3x3, "%s", "R U R'");
    snprintf(c.alg_big, sizeof c.alg_big, "%s", "Rw U Rw'");
    snprintf(c.alg_oh, sizeof c.alg_oh, "%s", "R U2 R'");
    snprintf(c.notes, sizeof c.notes, "%s", "keep me");
    CT_ASSERT_EQ(ct_repo_card_insert(db, &c), CT_REPO_OK);
    return c.id;
}

CT_SUITE(cards_form_open) {
    sqlite3 *db = form_db();
    for (int i = 0; i < 5; i++) {
        char key[8];
        snprintf(key, sizeof key, "k%d", i + 1);
        seed(db, "alpha", key, "R U");
    }
    seed(db, "beta", "b1", "R");
    seed(db, "beta", "b2", "R'");

    ct_cards_form f;
    ct_cards_form_open(&f, db, "alpha");
    CT_ASSERT_EQ(f.row_count, 5);
    CT_ASSERT_EQ(f.selected, 0);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT(!f.editing);
    CT_ASSERT(!f.dirty);
    CT_ASSERT_EQ(f.card.id, f.ids[0]);
    CT_ASSERT_STR_EQ(f.card.case_key, "k1");
    CT_ASSERT_STR_EQ(f.deck_filter, "alpha");

    ct_cards_form_open(&f, db, "");
    CT_ASSERT_EQ(f.row_count, 7);

    seed(db, "alpha", "k6", "U");
    ct_cards_form_reload(&f, db);
    CT_ASSERT_EQ(f.row_count, 8);
    CT_ASSERT_EQ(f.selected, 0);
    CT_ASSERT(!f.editing);
    CT_ASSERT(!f.dirty);
    CT_ASSERT_STR_EQ(f.card.case_key, "k1");

    ct_cards_form_open(&f, db, "ghost");
    CT_ASSERT_EQ(f.row_count, 0);
    CT_ASSERT_EQ(f.selected, -1);
    CT_ASSERT_EQ(f.card.id, 0);
    CT_ASSERT_EQ(f.card.deck[0], '\0');

    ct_cards_form_open(&f, NULL, "alpha");
    CT_ASSERT_EQ(f.row_count, 0);
    CT_ASSERT(f.status < 0);
    ct_cards_form_reload(&f, NULL);
    CT_ASSERT_EQ(f.row_count, 0);
    ct_db_close(db);
}

CT_SUITE(cards_form_select) {
    sqlite3 *db = form_db();
    seed(db, "d", "a", "R");
    seed(db, "d", "b", "R U");
    seed(db, "d", "c", "R U'");

    ct_cards_form f;
    ct_cards_form_open(&f, db, "d");
    CT_ASSERT_EQ(ct_cards_form_select(&f, 2), CT_REPO_OK);
    CT_ASSERT_EQ(f.selected, 2);
    CT_ASSERT_STR_EQ(f.card.case_key, "c");

    CT_ASSERT_EQ(ct_cards_form_select(&f, 99), CT_REPO_OK);
    CT_ASSERT_EQ(f.selected, 2);
    CT_ASSERT_STR_EQ(f.card.case_key, "c");

    CT_ASSERT_EQ(ct_cards_form_select(&f, -7), CT_REPO_OK);
    CT_ASSERT_EQ(f.selected, 0);
    CT_ASSERT_STR_EQ(f.card.case_key, "a");

    CT_ASSERT_EQ(ct_cards_form_begin_edit(&f), CT_REPO_OK);
    CT_ASSERT(f.editing);

    ct_cards_form empty;
    ct_cards_form_open(&empty, db, "none");
    CT_ASSERT_EQ(ct_cards_form_select(&empty, 0), CT_REPO_MISSING);
    CT_ASSERT_EQ(empty.selected, -1);
    CT_ASSERT_EQ(ct_cards_form_begin_edit(&empty), CT_REPO_MISSING);
    CT_ASSERT(!empty.editing);
    ct_db_close(db);
}

CT_SUITE(cards_form_edit_save) {
    sqlite3 *db = form_db();
    long id = seed_full(db, "ed", "one");

    ct_cards_form f;
    ct_cards_form_open(&f, db, "ed");
    CT_ASSERT_EQ(ct_cards_form_begin_edit(&f), CT_REPO_OK);
    snprintf(f.card.alg_3x3, sizeof f.card.alg_3x3, "%s", "F R U R' U'");
    snprintf(f.card.alg_big, sizeof f.card.alg_big, "%s", "Fw Rw U");
    f.dirty = true;
    CT_ASSERT_EQ(ct_cards_form_save(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT(!f.dirty);
    CT_ASSERT(!f.editing);

    ct_card back;
    CT_ASSERT_EQ(ct_repo_card_get(db, id, &back), CT_REPO_OK);
    CT_ASSERT_STR_EQ(back.alg_3x3, "F R U R' U'");
    CT_ASSERT_STR_EQ(back.alg_big, "Fw Rw U");
    CT_ASSERT_STR_EQ(back.alg_oh, "R U2 R'");
    CT_ASSERT_STR_EQ(back.notes, "keep me");
    CT_ASSERT_STR_EQ(back.front_json, "{\"scramble\":\"F R U\"}");
    CT_ASSERT_STR_EQ(back.case_key, "one");
    CT_ASSERT_STR_EQ(back.deck, "ed");
    CT_ASSERT_EQ(back.direction, CT_DIR_BL);

    CT_ASSERT_EQ(ct_cards_form_begin_edit(&f), CT_REPO_OK);
    f.dirty = true;
    CT_ASSERT_EQ(ct_cards_form_save(&f, db), CT_REPO_OK);
    CT_ASSERT(!f.dirty);

    CT_ASSERT_EQ(ct_cards_form_begin_edit(&f), CT_REPO_OK);
    f.card.case_key[0] = '\0';
    CT_ASSERT_EQ(ct_cards_form_save(&f, db), CT_REPO_ERR_ARG);
    ct_cards_form_cancel(&f);
    CT_ASSERT_STR_EQ(f.card.case_key, "one");
    ct_db_close(db);
}

CT_SUITE(cards_form_create) {
    sqlite3 *db = form_db();
    seed(db, "mk", "new-2", "R U R'");
    seed(db, "mk", "new-3", "R U2 R'");
    ct_cards_form f;
    ct_cards_form_open(&f, db, "mk");
    CT_ASSERT_EQ(f.row_count, 2);

    char seen[12][CT_CARD_CASE_KEY_MAX];
    int created = 0;
    for (int i = 0; i < 10; i++) {
        CT_ASSERT_EQ(ct_cards_form_create(&f, db, "mk"), CT_REPO_OK);
        CT_ASSERT_EQ(f.status, CT_REPO_OK);
        CT_ASSERT_EQ(f.row_count, 3 + i);
        CT_ASSERT(f.selected >= 0 && f.selected < f.row_count);
        CT_ASSERT_EQ(f.card.id, f.ids[f.selected]);
        CT_ASSERT_STR_EQ(f.card.deck, "mk");
        CT_ASSERT(f.editing);
        CT_ASSERT(!f.dirty);
        snprintf(seen[created], sizeof seen[created], "%s", f.card.case_key);
        ct_card back;
        CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "mk", seen[created], CT_DIR_FR, &back),
                     CT_REPO_OK);
        CT_ASSERT_EQ(back.id, f.card.id);
        CT_ASSERT(strcmp(seen[created], "new-2") != 0);
        CT_ASSERT(strcmp(seen[created], "new-3") != 0);
        for (int j = 0; j < created; j++) {
            CT_ASSERT(strcmp(seen[j], seen[created]) != 0);
        }
        created++;
    }
    CT_ASSERT_EQ(f.row_count, 12);

    CT_ASSERT_EQ(ct_cards_form_create(&f, db, "side"), CT_REPO_OK);
    CT_ASSERT_EQ(f.row_count, 12);
    CT_ASSERT(f.selected >= 0 && f.selected < f.row_count);

    CT_ASSERT_EQ(ct_cards_form_create(&f, db, ""), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_create(&f, db, NULL), CT_REPO_ERR_ARG);
    ct_db_close(db);
}

CT_SUITE(cards_form_delete) {
    sqlite3 *db = form_db();
    seed(db, "d", "a", "R");
    seed(db, "d", "b", "R U");
    long victim = seed(db, "d", "c", "R U'");

    ct_cards_form f;
    ct_cards_form_open(&f, db, "d");
    CT_ASSERT_EQ(ct_cards_form_select(&f, 2), CT_REPO_OK);
    CT_ASSERT_EQ(ct_cards_form_delete(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT_EQ(f.row_count, 2);
    CT_ASSERT_EQ(f.selected, 1);
    CT_ASSERT_STR_EQ(f.card.case_key, "b");
    CT_ASSERT(!f.editing);
    CT_ASSERT(!f.dirty);
    ct_card back;
    CT_ASSERT_EQ(ct_repo_card_get(db, victim, &back), CT_REPO_MISSING);

    ct_cards_form empty;
    ct_cards_form_open(&empty, db, "none");
    CT_ASSERT_EQ(ct_cards_form_delete(&empty, db), CT_REPO_MISSING);
    ct_db_close(db);
}

CT_SUITE(cards_form_cancel) {
    sqlite3 *db = form_db();
    long id = seed_full(db, "cx", "k");

    ct_cards_form f;
    ct_cards_form_open(&f, db, "cx");
    CT_ASSERT_EQ(ct_cards_form_begin_edit(&f), CT_REPO_OK);
    snprintf(f.card.alg_3x3, sizeof f.card.alg_3x3, "%s", "Z Z Z");
    snprintf(f.card.notes, sizeof f.card.notes, "%s", "thrown away");
    f.card.direction = CT_DIR_FR;
    f.dirty = true;
    ct_cards_form_cancel(&f);
    CT_ASSERT_STR_EQ(f.card.alg_3x3, "R U R'");
    CT_ASSERT_STR_EQ(f.card.notes, "keep me");
    CT_ASSERT_EQ(f.card.direction, CT_DIR_BL);
    CT_ASSERT(!f.editing);
    CT_ASSERT(!f.dirty);

    ct_card back;
    CT_ASSERT_EQ(ct_repo_card_get(db, id, &back), CT_REPO_OK);
    CT_ASSERT_STR_EQ(back.alg_3x3, "R U R'");
    CT_ASSERT_STR_EQ(back.notes, "keep me");

    ct_cards_form empty;
    ct_cards_form_open(&empty, db, "none");
    ct_cards_form_cancel(&empty);
    CT_ASSERT_EQ(empty.card.id, 0);
    ct_db_close(db);
}

CT_SUITE(cards_form_status) {
    sqlite3 *db = form_db();
    long a = seed(db, "st", "ka", "R");
    seed(db, "st", "kb", "U");

    ct_cards_form f;
    ct_cards_form_open(&f, db, "st");
    CT_ASSERT_EQ(ct_cards_form_select(&f, 1), CT_REPO_OK);
    CT_ASSERT_EQ(ct_cards_form_begin_edit(&f), CT_REPO_OK);
    snprintf(f.card.case_key, sizeof f.card.case_key, "%s", "ka");
    CT_ASSERT_EQ(ct_cards_form_save(&f, db), CT_REPO_CONFLICT);
    CT_ASSERT_EQ(f.status, CT_REPO_CONFLICT);
    CT_ASSERT(f.editing);

    ct_cards_form_cancel(&f);
    CT_ASSERT_STR_EQ(f.card.case_key, "kb");

    CT_ASSERT_EQ(ct_cards_form_select(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(ct_repo_card_delete(db, a), CT_REPO_OK);
    CT_ASSERT_EQ(ct_cards_form_begin_edit(&f), CT_REPO_OK);
    CT_ASSERT_EQ(ct_cards_form_save(&f, db), CT_REPO_MISSING);
    CT_ASSERT_EQ(f.status, CT_REPO_MISSING);
    ct_db_close(db);
}

CT_SUITE(cards_form_guards) {
    sqlite3 *db = form_db();
    seed(db, "g", "k", "R");
    ct_cards_form f;
    ct_cards_form_open(&f, db, "g");

    ct_cards_form_open(NULL, db, "g");
    ct_cards_form_reload(NULL, db);
    ct_cards_form_cancel(NULL);
    CT_ASSERT_EQ(ct_cards_form_select(NULL, 0), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_begin_edit(NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_save(NULL, db), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_save(&f, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_delete(NULL, db), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_delete(&f, NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_create(NULL, db, "g"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_create(&f, NULL, "g"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_cards_form_create(&f, db, ""), CT_REPO_ERR_ARG);

    f.selected = -1;
    CT_ASSERT_EQ(ct_cards_form_save(&f, db), CT_REPO_MISSING);
    CT_ASSERT_EQ(ct_cards_form_delete(&f, db), CT_REPO_MISSING);
    ct_db_close(db);
}
