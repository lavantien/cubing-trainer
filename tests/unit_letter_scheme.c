#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_settings.h"
#include "framework.h"

static sqlite3 *open_migrated(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    return db;
}

static char *read_seed_sql(void) {
    FILE *f = fopen("db/seeds/speffz.sql", "rb");
    CT_ASSERT(f != NULL);
    CT_ASSERT(fseek(f, 0, SEEK_END) == 0);
    long size = ftell(f);
    CT_ASSERT(size > 0);
    rewind(f);
    char *buf = malloc((size_t)size + 1);
    CT_ASSERT(buf != NULL);
    CT_ASSERT(fread(buf, 1, (size_t)size, f) == (size_t)size);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static void assert_speffz_map(const ct_letter_map *m) {
    for (int i = 0; i < 24; i++) {
        CT_ASSERT_EQ(m->corner[i], 'A' + i);
        CT_ASSERT_EQ(m->edge[i], 'A' + i);
    }
}

CT_SUITE(settings_repo)
{
    sqlite3 *db = open_migrated();
    char buf[64];
    CT_ASSERT_EQ(ct_repo_settings_get(db, "missing", buf, sizeof buf), 1);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "theme", "dark"), 0);
    CT_ASSERT_EQ(ct_repo_settings_get(db, "theme", buf, sizeof buf), 0);
    CT_ASSERT_STR_EQ(buf, "dark");
    CT_ASSERT_EQ(ct_repo_settings_set(db, "theme", "light"), 0);
    CT_ASSERT_EQ(ct_repo_settings_get(db, "theme", buf, sizeof buf), 0);
    CT_ASSERT_STR_EQ(buf, "light");
    CT_ASSERT_EQ(ct_repo_settings_set(db, "alpha", "abcdefgh"), 0);
    char small[4];
    CT_ASSERT_EQ(ct_repo_settings_get(db, "alpha", small, sizeof small), 0);
    CT_ASSERT_STR_EQ(small, "abc");
    char one[1];
    CT_ASSERT_EQ(ct_repo_settings_get(db, "alpha", one, sizeof one), 0);
    CT_ASSERT_STR_EQ(one, "");
    CT_ASSERT_EQ(ct_repo_settings_set(db, "empty", ""), 0);
    CT_ASSERT_EQ(ct_repo_settings_get(db, "empty", buf, sizeof buf), 0);
    CT_ASSERT_STR_EQ(buf, "");
    CT_ASSERT(ct_repo_settings_get(NULL, "theme", buf, sizeof buf) < 0);
    CT_ASSERT(ct_repo_settings_get(db, NULL, buf, sizeof buf) < 0);
    CT_ASSERT(ct_repo_settings_get(db, "theme", NULL, sizeof buf) < 0);
    CT_ASSERT(ct_repo_settings_get(db, "theme", buf, 0) < 0);
    CT_ASSERT(ct_repo_settings_set(NULL, "theme", "dark") < 0);
    CT_ASSERT(ct_repo_settings_set(db, NULL, "dark") < 0);
    CT_ASSERT(ct_repo_settings_set(db, "theme", NULL) < 0);
    ct_db_close(db);
}

CT_SUITE(letter_scheme_lifecycle)
{
    sqlite3 *db = open_migrated();
    ct_letter_map m;
    CT_ASSERT_EQ(ct_letter_map_load(db, 1, &m), 0);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db), 0);
    int id = 0;
    CT_ASSERT_EQ(ct_letter_scheme_create(db, "mine", &id), 0);
    CT_ASSERT(id >= 1);
    int dup = 0;
    CT_ASSERT(ct_letter_scheme_create(db, "mine", &dup) < 0);
    int other = 0;
    CT_ASSERT_EQ(ct_letter_scheme_create(db, "other", &other), 0);
    CT_ASSERT(other != id);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db), 0);
    CT_ASSERT_EQ(ct_letter_scheme_set_default(db, id), 0);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db), id);
    CT_ASSERT_EQ(ct_letter_scheme_set_default(db, other), 0);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db), other);
    CT_ASSERT(ct_letter_scheme_set_default(db, 4242) < 0);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db), other);
    CT_ASSERT(ct_letter_scheme_set_default(db, 0) < 0);
    CT_ASSERT(ct_letter_scheme_create(db, NULL, &dup) < 0);
    CT_ASSERT(ct_letter_scheme_create(db, "solo", NULL) < 0);
    CT_ASSERT(ct_letter_scheme_default_id(NULL) < 0);
    CT_ASSERT(ct_letter_map_load(db, other, &m) >= 0);
    ct_db_close(db);
}

CT_SUITE(letter_map_validation)
{
    sqlite3 *db = open_migrated();
    int id = 0;
    CT_ASSERT_EQ(ct_letter_scheme_create(db, "s", &id), 0);
    ct_letter_map m;
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 0, 'A'), 0);
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 1);
    CT_ASSERT_EQ(m.corner[0], 'A');
    CT_ASSERT_EQ(m.edge[0], '\0');
    CT_ASSERT_EQ(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 0, 'B'), 0);
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 1);
    CT_ASSERT_EQ(m.corner[0], 'B');
    CT_ASSERT_EQ(ct_letter_map_set(db, id, CT_ORBIT_EDGE, 23, 'X'), 0);
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 2);
    CT_ASSERT_EQ(m.edge[23], 'X');
    CT_ASSERT(ct_letter_map_set(db, id, CT_ORBIT_CENTER, 0, 'A') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, -1, 0, 'A') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, 3, 0, 'A') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, CT_ORBIT_CORNER, -1, 'A') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 24, 'A') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 0, 'a') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 0, 'Y') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 0, '7') < 0);
    CT_ASSERT(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 0, '\0') < 0);
    CT_ASSERT(ct_letter_map_set(db, 987, CT_ORBIT_CORNER, 0, 'A') < 0);
    CT_ASSERT(ct_letter_map_set(NULL, id, CT_ORBIT_CORNER, 0, 'A') < 0);
    CT_ASSERT(ct_letter_map_load(db, id, NULL) < 0);
    CT_ASSERT(ct_letter_map_load(NULL, id, &m) < 0);
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 2);
    ct_db_close(db);
}

CT_SUITE(speffz_seed)
{
    sqlite3 *db = open_migrated();
    ct_letter_map m;
    CT_ASSERT_EQ(ct_letter_seed_speffz(db), 0);
    int id = ct_letter_scheme_default_id(db);
    CT_ASSERT(id >= 1);
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 48);
    assert_speffz_map(&m);
    CT_ASSERT_EQ(m.corner[0], 'A');
    CT_ASSERT_EQ(m.edge[2], 'C');
    CT_ASSERT_EQ(m.corner[8], 'I');
    CT_ASSERT_EQ(m.corner[20], 'U');
    CT_ASSERT_EQ(m.corner[23], 'X');
    CT_ASSERT_EQ(m.edge[11], 'L');
    CT_ASSERT_EQ(m.edge[23], 'X');
    CT_ASSERT_EQ(ct_letter_seed_speffz(db), 0);
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 48);
    assert_speffz_map(&m);
    CT_ASSERT_EQ(ct_letter_map_set(db, id, CT_ORBIT_CORNER, 5, 'Q'), 0);
    CT_ASSERT_EQ(ct_letter_seed_speffz(db), 0);
    CT_ASSERT_EQ(ct_letter_map_load(db, id, &m), 48);
    CT_ASSERT_EQ(m.corner[5], 'Q');
    for (int i = 0; i < 24; i++) {
        if (i == 5) continue;
        CT_ASSERT_EQ(m.corner[i], 'A' + i);
        CT_ASSERT_EQ(m.edge[i], 'A' + i);
    }
    CT_ASSERT(ct_letter_seed_speffz(NULL) < 0);
    ct_db_close(db);
}

CT_SUITE(speffz_seed_default)
{
    sqlite3 *db = open_migrated();
    int user = 0;
    CT_ASSERT_EQ(ct_letter_scheme_create(db, "user", &user), 0);
    CT_ASSERT_EQ(ct_letter_scheme_set_default(db, user), 0);
    CT_ASSERT_EQ(ct_letter_seed_speffz(db), 0);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db), user);
    ct_letter_map m;
    CT_ASSERT_EQ(ct_letter_map_load(db, user, &m), 0);
    ct_db_close(db);

    sqlite3 *db2 = open_migrated();
    int pre = 0;
    CT_ASSERT_EQ(ct_letter_scheme_create(db2, "speffz", &pre), 0);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db2), 0);
    CT_ASSERT_EQ(ct_letter_seed_speffz(db2), 0);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(db2), pre);
    CT_ASSERT_EQ(ct_letter_map_load(db2, pre, &m), 48);
    assert_speffz_map(&m);
    ct_db_close(db2);
}

CT_SUITE(speffz_sql_agreement)
{
    sqlite3 *c = open_migrated();
    CT_ASSERT_EQ(ct_letter_seed_speffz(c), 0);
    int cid = ct_letter_scheme_default_id(c);
    CT_ASSERT_EQ(cid, 1);
    ct_letter_map cm;
    CT_ASSERT_EQ(ct_letter_map_load(c, cid, &cm), 48);

    char *sql = read_seed_sql();
    sqlite3 *s = open_migrated();
    char *err = NULL;
    int rc = sqlite3_exec(s, sql, NULL, NULL, &err);
    if (err != NULL) {
        fprintf(stderr, "speffz.sql exec error: %s\n", err);
    }
    sqlite3_free(err);
    CT_ASSERT_EQ(rc, SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_exec(s, sql, NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT_EQ(ct_letter_scheme_default_id(s), 1);
    ct_letter_map sm;
    CT_ASSERT_EQ(ct_letter_map_load(s, 1, &sm), 48);
    CT_ASSERT(memcmp(cm.corner, sm.corner, sizeof cm.corner) == 0);
    CT_ASSERT(memcmp(cm.edge, sm.edge, sizeof cm.edge) == 0);
    free(sql);
    ct_db_close(c);
    ct_db_close(s);
}
