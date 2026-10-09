#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/trainer/cfop.h"
#include "framework.h"

enum { SPEC_PLL_CASES = 21, SPEC_OLL_CASES = 57, SPEC_F2L_PER_SLOT = 41, SPEC_F2L_TOTAL = 164 };

static const char *const spec_pll_names[] = {"Aa", "Ab", "E",  "F",  "Ga", "Gb", "Gc",
                                             "Gd", "H",  "Ja", "Jb", "Na", "Nb", "Ra",
                                             "Rb", "T",  "Ua", "Ub", "V",  "Y",  "Z"};

static const char *const spec_f2l_slots[] = {"fr", "fl", "br", "bl"};
static const int spec_f2l_dirs[] = {CT_DIR_FR, CT_DIR_FL, CT_DIR_BR, CT_DIR_BL};

static sqlite3 *cfop_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static int cmp_str(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int all_unique(const char (*keys)[CT_CARD_CASE_KEY_MAX], int n) {
    const char *ptrs[SPEC_F2L_TOTAL];
    for (int i = 0; i < n; i++) {
        ptrs[i] = keys[i];
    }
    qsort(ptrs, (size_t)n, sizeof ptrs[0], cmp_str);
    for (int i = 1; i < n; i++) {
        if (strcmp(ptrs[i - 1], ptrs[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

CT_SUITE(cfop_case_counts) {
    CT_ASSERT_EQ(ct_cfop_case_count(CT_STAGE_PLL), SPEC_PLL_CASES);
    CT_ASSERT_EQ(ct_cfop_case_count(CT_STAGE_OLL), SPEC_OLL_CASES);
    CT_ASSERT_EQ(ct_cfop_case_count(CT_STAGE_F2L), SPEC_F2L_TOTAL);
    CT_ASSERT(ct_cfop_case_count(CT_STAGE_CROSS) < 0);
    CT_ASSERT(ct_cfop_case_count(CT_STAGE_CLL) < 0);
    CT_ASSERT(ct_cfop_case_count(CT_STAGE_CONCEPT) < 0);
}

CT_SUITE(cfop_pll_keys) {
    int n = ct_cfop_case_count(CT_STAGE_PLL);
    CT_ASSERT_EQ(n, SPEC_PLL_CASES);
    static char keys[SPEC_PLL_CASES][CT_CARD_CASE_KEY_MAX];
    char expect[CT_CARD_CASE_KEY_MAX];
    for (int i = 0; i < n; i++) {
        CT_ASSERT_EQ(ct_cfop_case_key(CT_STAGE_PLL, i, keys[i], sizeof keys[i]), 0);
        snprintf(expect, sizeof expect, "pll.%s", spec_pll_names[i]);
        CT_ASSERT_STR_EQ(keys[i], expect);
    }
    CT_ASSERT(all_unique(keys, n));

    char buf[CT_CARD_CASE_KEY_MAX];
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_PLL, -1, buf, sizeof buf) < 0);
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_PLL, n, buf, sizeof buf) < 0);
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_CROSS, 0, buf, sizeof buf) < 0);
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_PLL, 0, NULL, sizeof buf) < 0);
}

CT_SUITE(cfop_oll_keys) {
    int n = ct_cfop_case_count(CT_STAGE_OLL);
    CT_ASSERT_EQ(n, SPEC_OLL_CASES);
    static char keys[SPEC_OLL_CASES][CT_CARD_CASE_KEY_MAX];
    for (int i = 0; i < n; i++) {
        CT_ASSERT_EQ(ct_cfop_case_key(CT_STAGE_OLL, i, keys[i], sizeof keys[i]), 0);
        CT_ASSERT_EQ(strlen(keys[i]), 6u);
        CT_ASSERT(strncmp(keys[i], "oll.", 4) == 0);
        CT_ASSERT(keys[i][4] >= '0' && keys[i][4] <= '5');
        CT_ASSERT(keys[i][5] >= '0' && keys[i][5] <= '9');
        CT_ASSERT_EQ(atoi(keys[i] + 4), i + 1);
    }
    CT_ASSERT_STR_EQ(keys[0], "oll.01");
    CT_ASSERT_STR_EQ(keys[SPEC_OLL_CASES - 1], "oll.57");
    CT_ASSERT(all_unique(keys, n));

    char buf[CT_CARD_CASE_KEY_MAX];
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_OLL, n, buf, sizeof buf) < 0);
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_OLL, -1, buf, sizeof buf) < 0);
}

CT_SUITE(cfop_f2l_keys) {
    int n = ct_cfop_case_count(CT_STAGE_F2L);
    CT_ASSERT_EQ(n, SPEC_F2L_TOTAL);
    static char keys[SPEC_F2L_TOTAL][CT_CARD_CASE_KEY_MAX];
    char expect[CT_CARD_CASE_KEY_MAX];
    int per_dir[4] = {0, 0, 0, 0};
    for (int i = 0; i < n; i++) {
        CT_ASSERT_EQ(ct_cfop_case_key(CT_STAGE_F2L, i, keys[i], sizeof keys[i]), 0);
        int slot = i / SPEC_F2L_PER_SLOT;
        int num = i % SPEC_F2L_PER_SLOT + 1;
        snprintf(expect, sizeof expect, "f2l.%s.%02d", spec_f2l_slots[slot], num);
        CT_ASSERT_STR_EQ(keys[i], expect);
        int dir = ct_cfop_case_direction(CT_STAGE_F2L, i);
        CT_ASSERT_EQ(dir, spec_f2l_dirs[slot]);
        CT_ASSERT(dir >= CT_DIR_FR && dir <= CT_DIR_BL);
        per_dir[dir]++;
    }
    CT_ASSERT_EQ(per_dir[CT_DIR_FR], SPEC_F2L_PER_SLOT);
    CT_ASSERT_EQ(per_dir[CT_DIR_FL], SPEC_F2L_PER_SLOT);
    CT_ASSERT_EQ(per_dir[CT_DIR_BR], SPEC_F2L_PER_SLOT);
    CT_ASSERT_EQ(per_dir[CT_DIR_BL], SPEC_F2L_PER_SLOT);
    CT_ASSERT_STR_EQ(keys[0], "f2l.fr.01");
    CT_ASSERT_STR_EQ(keys[SPEC_F2L_PER_SLOT - 1], "f2l.fr.41");
    CT_ASSERT_STR_EQ(keys[SPEC_F2L_PER_SLOT], "f2l.fl.01");
    CT_ASSERT_STR_EQ(keys[n - 1], "f2l.bl.41");
    CT_ASSERT(all_unique(keys, n));

    char buf[CT_CARD_CASE_KEY_MAX];
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_F2L, n, buf, sizeof buf) < 0);
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_F2L, -1, buf, sizeof buf) < 0);
}

CT_SUITE(cfop_directions) {
    CT_ASSERT_EQ(ct_cfop_case_direction(CT_STAGE_PLL, 0), CT_DIR_FR);
    CT_ASSERT_EQ(ct_cfop_case_direction(CT_STAGE_PLL, SPEC_PLL_CASES - 1), CT_DIR_FR);
    CT_ASSERT_EQ(ct_cfop_case_direction(CT_STAGE_OLL, 0), CT_DIR_FR);
    CT_ASSERT_EQ(ct_cfop_case_direction(CT_STAGE_OLL, SPEC_OLL_CASES - 1), CT_DIR_FR);
    for (int i = 0; i < SPEC_OLL_CASES; i++) {
        CT_ASSERT_EQ(ct_cfop_case_direction(CT_STAGE_OLL, i), CT_DIR_FR);
    }
    for (int i = 0; i < SPEC_PLL_CASES; i++) {
        CT_ASSERT_EQ(ct_cfop_case_direction(CT_STAGE_PLL, i), CT_DIR_FR);
    }
    CT_ASSERT(ct_cfop_case_direction(CT_STAGE_PLL, -1) < 0);
    CT_ASSERT(ct_cfop_case_direction(CT_STAGE_OLL, SPEC_OLL_CASES) < 0);
    CT_ASSERT(ct_cfop_case_direction(CT_STAGE_F2L, SPEC_F2L_TOTAL) < 0);
    CT_ASSERT(ct_cfop_case_direction(CT_STAGE_CROSS, 0) < 0);
}

CT_SUITE(cfop_deck_names) {
    char deck[CT_CARD_DECK_MAX];
    CT_ASSERT_EQ(ct_cfop_deck_name(CT_STAGE_PLL, deck, sizeof deck), 0);
    CT_ASSERT_STR_EQ(deck, "cfop_pll");
    CT_ASSERT_EQ(ct_cfop_deck_name(CT_STAGE_OLL, deck, sizeof deck), 0);
    CT_ASSERT_STR_EQ(deck, "cfop_oll");
    CT_ASSERT_EQ(ct_cfop_deck_name(CT_STAGE_F2L, deck, sizeof deck), 0);
    CT_ASSERT_STR_EQ(deck, "cfop_f2l");
    CT_ASSERT(ct_cfop_deck_name(CT_STAGE_CROSS, deck, sizeof deck) < 0);
    CT_ASSERT(ct_cfop_deck_name(CT_STAGE_PLL, NULL, sizeof deck) < 0);
    CT_ASSERT(ct_cfop_deck_name(CT_STAGE_PLL, deck, strlen("cfop_pll")) < 0);
    CT_ASSERT_EQ(ct_cfop_deck_name(CT_STAGE_PLL, deck, strlen("cfop_pll") + 1), 0);
    CT_ASSERT_STR_EQ(deck, "cfop_pll");
}

CT_SUITE(cfop_key_cap_safety) {
    char buf[8];
    memset(buf, 'q', sizeof buf);
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_OLL, 0, buf, 6) < 0);
    CT_ASSERT_EQ(buf[0], 'q');
    char big[CT_CARD_CASE_KEY_MAX];
    memset(big, 'q', sizeof big - 1);
    big[sizeof big - 1] = '\0';
    CT_ASSERT(ct_cfop_case_key(CT_STAGE_F2L, SPEC_F2L_TOTAL - 1, big, 9) < 0);
    CT_ASSERT_EQ(big[0], 'q');
    char fits[CT_CARD_CASE_KEY_MAX];
    CT_ASSERT_EQ(ct_cfop_case_key(CT_STAGE_F2L, SPEC_F2L_TOTAL - 1, fits, 10), 0);
    CT_ASSERT_STR_EQ(fits, "f2l.bl.41");
    CT_ASSERT_EQ(ct_cfop_case_key(CT_STAGE_OLL, 0, fits, 7), 0);
    CT_ASSERT_STR_EQ(fits, "oll.01");
}

CT_SUITE(cfop_populate_pll) {
    sqlite3 *db = cfop_db();
    CT_ASSERT_EQ(ct_cfop_populate(db, CT_STAGE_PLL), SPEC_PLL_CASES);

    static ct_card cards[SPEC_PLL_CASES + 4];
    memset(cards, 0, sizeof cards);
    CT_ASSERT_EQ(ct_repo_card_list(db, "cfop_pll", cards, SPEC_PLL_CASES + 4), SPEC_PLL_CASES);

    ct_card c;
    memset(&c, 0, sizeof c);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_pll", "pll.T", CT_DIR_FR, &c), CT_REPO_OK);
    CT_ASSERT(c.id > 0);
    CT_ASSERT_STR_EQ(c.deck, "cfop_pll");
    CT_ASSERT_EQ(c.direction, CT_DIR_FR);
    CT_ASSERT_STR_EQ(c.front_json, "{\"stage\":\"pll\",\"case\":\"T\"}");
    CT_ASSERT_STR_EQ(c.alg_3x3, "");
    CT_ASSERT_STR_EQ(c.alg_big, "");
    CT_ASSERT_STR_EQ(c.alg_oh, "");
    CT_ASSERT_STR_EQ(c.notes, "");

    CT_ASSERT_EQ(ct_cfop_populate(db, CT_STAGE_PLL), 0);
    CT_ASSERT_EQ(ct_repo_card_list(db, "cfop_pll", cards, SPEC_PLL_CASES + 4), SPEC_PLL_CASES);

    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_pll", "pll.T", CT_DIR_FR, &c), CT_REPO_OK);
    snprintf(c.alg_3x3, sizeof c.alg_3x3, "%s", "r U R' U' R' F R2 U' R' U' R U R' F'");
    snprintf(c.notes, sizeof c.notes, "%s", "yaw before T");
    CT_ASSERT_EQ(ct_repo_card_update(db, &c), CT_REPO_OK);

    CT_ASSERT_EQ(ct_cfop_populate(db, CT_STAGE_PLL), 0);
    CT_ASSERT_EQ(ct_repo_card_list(db, "cfop_pll", cards, SPEC_PLL_CASES + 4), SPEC_PLL_CASES);
    memset(&c, 0, sizeof c);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_pll", "pll.T", CT_DIR_FR, &c), CT_REPO_OK);
    CT_ASSERT_STR_EQ(c.alg_3x3, "r U R' U' R' F R2 U' R' U' R U R' F'");
    CT_ASSERT_STR_EQ(c.notes, "yaw before T");
    ct_db_close(db);
}

CT_SUITE(cfop_populate_oll_f2l) {
    sqlite3 *db = cfop_db();
    CT_ASSERT_EQ(ct_cfop_populate(db, CT_STAGE_OLL), SPEC_OLL_CASES);
    CT_ASSERT_EQ(ct_cfop_populate(db, CT_STAGE_OLL), 0);
    CT_ASSERT_EQ(ct_cfop_populate(db, CT_STAGE_F2L), SPEC_F2L_TOTAL);
    CT_ASSERT_EQ(ct_cfop_populate(db, CT_STAGE_F2L), 0);

    static ct_card cards[SPEC_F2L_TOTAL + 4];
    CT_ASSERT_EQ(ct_repo_card_list(db, "cfop_oll", cards, SPEC_OLL_CASES), SPEC_OLL_CASES);
    CT_ASSERT_EQ(ct_repo_card_list(db, "cfop_f2l", cards, SPEC_F2L_TOTAL), SPEC_F2L_TOTAL);

    ct_card c;
    memset(&c, 0, sizeof c);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_oll", "oll.01", CT_DIR_FR, &c), CT_REPO_OK);
    CT_ASSERT_STR_EQ(c.front_json, "{\"stage\":\"oll\",\"case\":\"01\"}");
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_oll", "oll.57", CT_DIR_FR, &c), CT_REPO_OK);
    CT_ASSERT_STR_EQ(c.front_json, "{\"stage\":\"oll\",\"case\":\"57\"}");
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_f2l", "f2l.fr.01", CT_DIR_FR, &c), CT_REPO_OK);
    CT_ASSERT_STR_EQ(c.front_json, "{\"stage\":\"f2l\",\"case\":\"fr.01\"}");
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_f2l", "f2l.bl.41", CT_DIR_BL, &c), CT_REPO_OK);
    CT_ASSERT_EQ(c.direction, CT_DIR_BL);
    CT_ASSERT_STR_EQ(c.front_json, "{\"stage\":\"f2l\",\"case\":\"bl.41\"}");
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_f2l", "f2l.fl.20", CT_DIR_FL, &c), CT_REPO_OK);
    CT_ASSERT_EQ(c.direction, CT_DIR_FL);
    CT_ASSERT_EQ(ct_repo_card_get_by_key(db, "cfop_f2l", "f2l.br.07", CT_DIR_BR, &c), CT_REPO_OK);
    CT_ASSERT_EQ(c.direction, CT_DIR_BR);

    CT_ASSERT(ct_cfop_populate(NULL, CT_STAGE_PLL) < 0);
    CT_ASSERT(ct_cfop_populate(db, CT_STAGE_CROSS) < 0);
    ct_db_close(db);
}
