#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/storage/repo_settings.h"
#include "framework.h"

static sqlite3 *open_migrated(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    return db;
}

static int memo_count(sqlite3 *db) {
    sqlite3_stmt *st = NULL;
    int n = -1;
    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM memo_pair", -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW)
            n = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
    return n;
}

CT_SUITE(memo_repo) {
    sqlite3 *db = open_migrated();
    ct_memo_row corner;
    ct_memo_row edge;
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "AB", &corner), CT_REPO_OK);
    CT_ASSERT(corner.phoneme[0] != '\0');
    CT_ASSERT(corner.image[0] != '\0');
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_EDGE, "AB", &edge), CT_REPO_OK);
    CT_ASSERT_STR_EQ(corner.phoneme, edge.phoneme);
    CT_ASSERT_STR_EQ(corner.image, edge.image);

    CT_ASSERT_EQ(
        sqlite3_exec(db, "DELETE FROM memo_pair WHERE orbit = 0 AND pair = 'XX'", NULL, NULL, NULL),
        SQLITE_OK);
    memset(&corner, 0, sizeof corner);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "XX", &corner), CT_REPO_MISSING);
    CT_ASSERT(corner.phoneme[0] == '\0');
    CT_ASSERT(corner.image[0] == '\0');
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_EDGE, "XX", &edge), CT_REPO_OK);

    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "XX", "an1 aw5", "anchor apple"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "XX", &corner), CT_REPO_OK);
    CT_ASSERT_STR_EQ(corner.phoneme, "an1 aw5");
    CT_ASSERT_STR_EQ(corner.image, "anchor apple");

    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "XX", "zi2 ka5", "dragon castle"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "XX", &corner), CT_REPO_OK);
    CT_ASSERT_STR_EQ(corner.phoneme, "zi2 ka5");
    CT_ASSERT_STR_EQ(corner.image, "dragon castle");
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_EDGE, "XX", &edge), CT_REPO_OK);
    CT_ASSERT(strcmp(edge.phoneme, corner.phoneme) != 0);

    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_EDGE, "XX", "im1 ot5", "iron ocean"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_EDGE, "XX", &edge), CT_REPO_OK);
    CT_ASSERT_STR_EQ(edge.phoneme, "im1 ot5");
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "XX", &corner), CT_REPO_OK);
    CT_ASSERT_STR_EQ(corner.phoneme, "zi2 ka5");

    char tight[CT_MEMO_TEXT_MAX];
    memset(tight, 'p', sizeof tight);
    tight[CT_MEMO_TEXT_MAX - 1] = '\0';
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "QQ", tight, tight), CT_REPO_OK);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "QQ", &corner), CT_REPO_OK);
    CT_ASSERT_STR_EQ(corner.phoneme, tight);
    CT_ASSERT_STR_EQ(corner.image, tight);
    CT_ASSERT_EQ(memo_count(db), 1152);

    ct_memo_row row;
    char big[CT_MEMO_TEXT_MAX + 16];
    memset(big, 'x', sizeof big);
    big[sizeof big - 1] = '\0';
    char lost[CT_MEMO_TEXT_MAX];
    memset(lost, 'y', sizeof lost);

    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CENTER, "AB", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, -1, "AB", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, 7, "AB", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, NULL, &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "A", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "ab", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "aB", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "1B", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "AY", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "A1", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "ABC", &row), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "AB", NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_get(NULL, CT_ORBIT_CORNER, "AB", &row), CT_REPO_ERR_ARG);

    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CENTER, "AB", "p", "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "A", "p", "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "aB", "p", "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "ABC", "p", "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", NULL, "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", "p", NULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", "", "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", "p", ""), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", big, "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", "p", big), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", lost, "i"), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_set(NULL, CT_ORBIT_CORNER, "AB", "p", "i"), CT_REPO_ERR_ARG);

    char exact[CT_MEMO_TEXT_MAX];
    memset(exact, 'z', sizeof exact);
    exact[CT_MEMO_TEXT_MAX - 2] = '\0';
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", exact, exact), CT_REPO_OK);
    exact[CT_MEMO_TEXT_MAX - 2] = 'z';
    exact[CT_MEMO_TEXT_MAX - 1] = '\0';
    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", exact, exact), CT_REPO_OK);

    CT_ASSERT_EQ(memo_count(db), 1152);
    ct_memo_row seeded;
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_EDGE, "AB", &seeded), CT_REPO_OK);
    CT_ASSERT(seeded.phoneme[0] != '\0');
    ct_db_close(db);
}

enum { CT_MEMO_LIST_FULL = CT_LETTER_STICKERS * CT_LETTER_STICKERS };

CT_SUITE(memo_repo_list) {
    sqlite3 *db = open_migrated();
    ct_memo_list_row rows[CT_MEMO_LIST_FULL];
    int n = ct_memo_list(db, CT_ORBIT_CORNER, rows, CT_MEMO_LIST_FULL);
    CT_ASSERT_EQ(n, CT_MEMO_LIST_FULL);
    CT_ASSERT_STR_EQ(rows[0].pair, "AA");
    CT_ASSERT_STR_EQ(rows[0].phoneme, "a^w1 a^w1");
    CT_ASSERT_STR_EQ(rows[0].image, "atlas atlas");
    CT_ASSERT_STR_EQ(rows[CT_MEMO_LIST_FULL - 1].pair, "XX");
    int bx = ('B' - 'A') * CT_LETTER_STICKERS + ('X' - 'A');
    CT_ASSERT_STR_EQ(rows[bx].pair, "BX");
    ct_memo_row one;
    CT_ASSERT_EQ(ct_memo_get(db, CT_ORBIT_CORNER, "BX", &one), CT_REPO_OK);
    CT_ASSERT_STR_EQ(rows[bx].phoneme, one.phoneme);
    CT_ASSERT_STR_EQ(rows[bx].image, one.image);
    int e = ct_memo_list(db, CT_ORBIT_EDGE, rows, CT_MEMO_LIST_FULL);
    CT_ASSERT_EQ(e, CT_MEMO_LIST_FULL);
    CT_ASSERT_STR_EQ(rows[0].pair, "AA");
    CT_ASSERT_STR_EQ(rows[1].pair, "AB");

    CT_ASSERT_EQ(ct_memo_list(db, CT_ORBIT_EDGE, rows, 3), 3);
    CT_ASSERT_STR_EQ(rows[0].pair, "AA");
    CT_ASSERT_STR_EQ(rows[1].pair, "AB");
    CT_ASSERT_STR_EQ(rows[2].pair, "AC");
    snprintf(rows[0].pair, sizeof rows[0].pair, "zz");
    CT_ASSERT_EQ(ct_memo_list(db, CT_ORBIT_EDGE, rows, 0), 0);
    CT_ASSERT_STR_EQ(rows[0].pair, "zz");
    CT_ASSERT_EQ(ct_memo_list(db, CT_ORBIT_CENTER, rows, CT_MEMO_LIST_FULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_list(db, -1, rows, CT_MEMO_LIST_FULL), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_list(db, CT_ORBIT_CORNER, NULL, 4), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_memo_list(NULL, CT_ORBIT_CORNER, rows, CT_MEMO_LIST_FULL), CT_REPO_ERR_ARG);

    CT_ASSERT_EQ(ct_memo_set(db, CT_ORBIT_CORNER, "AB", "ah1 bo1", "amber boat"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_memo_list(db, CT_ORBIT_CORNER, rows, CT_MEMO_LIST_FULL), CT_MEMO_LIST_FULL);
    CT_ASSERT_STR_EQ(rows[1].image, "amber boat");
    CT_ASSERT_STR_EQ(rows[1].phoneme, "ah1 bo1");
    CT_ASSERT_EQ(ct_memo_list(db, CT_ORBIT_EDGE, rows, CT_MEMO_LIST_FULL), CT_MEMO_LIST_FULL);
    CT_ASSERT_STR_EQ(rows[1].image, "atlas bridge");

    CT_ASSERT_EQ(
        sqlite3_exec(db, "DELETE FROM memo_pair WHERE orbit = 0 AND pair = 'AB'", NULL, NULL, NULL),
        SQLITE_OK);
    CT_ASSERT_EQ(ct_memo_list(db, CT_ORBIT_CORNER, rows, CT_MEMO_LIST_FULL), CT_MEMO_LIST_FULL - 1);
    CT_ASSERT_STR_EQ(rows[0].pair, "AA");
    CT_ASSERT_STR_EQ(rows[1].pair, "AC");
    ct_db_close(db);
}
