#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_settings.h"
#include "framework.h"

#define CT_MEMO_ATOM_MAX 16

static sqlite3 *open_migrated(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    return db;
}

static int atom_far(const char *a, const char *b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    unsigned prev[CT_MEMO_ATOM_MAX + 1];
    unsigned cur[CT_MEMO_ATOM_MAX + 1];
    for (size_t j = 0; j <= lb; j++)
        prev[j] = (unsigned)j;
    for (size_t i = 1; i <= la; i++) {
        cur[0] = (unsigned)i;
        for (size_t j = 1; j <= lb; j++) {
            unsigned sub = prev[j - 1] + (a[i - 1] == b[j - 1] ? 0u : 1u);
            unsigned del = prev[j] + 1;
            unsigned ins = cur[j - 1] + 1;
            unsigned best = sub < del ? sub : del;
            cur[j] = best < ins ? best : ins;
        }
        memcpy(prev, cur, (lb + 1) * sizeof prev[0]);
    }
    return prev[lb] >= 2;
}

CT_SUITE(memo_seed) {
    sqlite3 *db = open_migrated();
    sqlite3_stmt *st = NULL;
    CT_ASSERT_EQ(sqlite3_prepare_v2(db, "SELECT orbit, count(*) FROM memo_pair GROUP BY orbit", -1,
                                    &st, NULL),
                 SQLITE_OK);
    int orbits = 0;
    int rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        int orbit = sqlite3_column_int(st, 0);
        int n = sqlite3_column_int(st, 1);
        CT_ASSERT(orbit == CT_ORBIT_CORNER || orbit == CT_ORBIT_EDGE);
        CT_ASSERT_EQ(n, 576);
        orbits++;
    }
    sqlite3_finalize(st);
    CT_ASSERT_EQ(rc, SQLITE_DONE);
    CT_ASSERT_EQ(orbits, 2);
    CT_ASSERT_EQ(ct_db_version(db), 2);

    char atoms[CT_LETTER_STICKERS][CT_MEMO_ATOM_MAX];
    memset(atoms, 0, sizeof atoms);
    int seen[CT_LETTER_STICKERS][2];
    memset(seen, 0, sizeof seen);
    sqlite3_stmt *scan = NULL;
    CT_ASSERT_EQ(sqlite3_prepare_v2(db, "SELECT orbit, pair, phoneme, image FROM memo_pair", -1,
                                    &scan, NULL),
                 SQLITE_OK);
    int rows = 0;
    while ((rc = sqlite3_step(scan)) == SQLITE_ROW) {
        int orbit = sqlite3_column_int(scan, 0);
        const unsigned char *pair = sqlite3_column_text(scan, 1);
        const unsigned char *phoneme = sqlite3_column_text(scan, 2);
        const unsigned char *image = sqlite3_column_text(scan, 3);
        CT_ASSERT(pair != NULL && phoneme != NULL && image != NULL);
        CT_ASSERT(orbit == CT_ORBIT_CORNER || orbit == CT_ORBIT_EDGE);
        CT_ASSERT_EQ(strlen((const char *)pair), 2);
        int first = pair[0] - CT_LETTER_FIRST;
        int second = pair[1] - CT_LETTER_FIRST;
        CT_ASSERT(first >= 0 && first < CT_LETTER_STICKERS);
        CT_ASSERT(second >= 0 && second < CT_LETTER_STICKERS);

        const char *split = strchr((const char *)phoneme, ' ');
        CT_ASSERT(split != NULL);
        CT_ASSERT(strchr(split + 1, ' ') == NULL);
        size_t head = (size_t)(split - (const char *)phoneme);
        CT_ASSERT(head > 0 && head < CT_MEMO_ATOM_MAX);
        CT_ASSERT(strlen(split + 1) > 0 && strlen(split + 1) < CT_MEMO_ATOM_MAX);
        char atom0[CT_MEMO_ATOM_MAX];
        char atom1[CT_MEMO_ATOM_MAX];
        memcpy(atom0, phoneme, head);
        atom0[head] = '\0';
        snprintf(atom1, sizeof atom1, "%s", split + 1);
        seen[first][0]++;
        seen[second][1]++;
        if (atoms[first][0] == '\0')
            memcpy(atoms[first], atom0, head + 1);
        else
            CT_ASSERT_STR_EQ(atoms[first], atom0);
        if (atoms[second][0] == '\0')
            snprintf(atoms[second], sizeof atoms[second], "%s", atom1);
        else
            CT_ASSERT_STR_EQ(atoms[second], atom1);

        const char *isplit = strchr((const char *)image, ' ');
        CT_ASSERT(isplit != NULL);
        CT_ASSERT(strchr(isplit + 1, ' ') == NULL);
        CT_ASSERT(isplit != (const char *)image);
        CT_ASSERT(strlen(isplit + 1) > 0);
        CT_ASSERT(image[0] == CT_LETTER_FIRST + first + 'a' - 'A');
        CT_ASSERT(isplit[1] == CT_LETTER_FIRST + second + 'a' - 'A');
        rows++;
    }
    sqlite3_finalize(scan);
    CT_ASSERT_EQ(rc, SQLITE_DONE);
    CT_ASSERT_EQ(rows, 1152);
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        CT_ASSERT(atoms[i][0] != '\0');
        CT_ASSERT(strchr(atoms[i], ' ') == NULL);
        CT_ASSERT_EQ(seen[i][0], 48);
        CT_ASSERT_EQ(seen[i][1], 48);
    }
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        for (int j = i + 1; j < CT_LETTER_STICKERS; j++) {
            CT_ASSERT(strcmp(atoms[i], atoms[j]) != 0);
            CT_ASSERT(atom_far(atoms[i], atoms[j]));
        }
    }
    ct_db_close(db);
}
