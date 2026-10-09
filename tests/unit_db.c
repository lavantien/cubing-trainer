#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <sqlite3.h>

#include "../src/storage/db.h"
#include "framework.h"

#define CT_TEST_DIR "build/test_db"

static void make_dir(const char *path) {
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void remove_file(const char *path) {
    remove(path);
}

static void remove_dir(const char *path) {
#if defined(_WIN32)
    _rmdir(path);
#else
    rmdir(path);
#endif
}

static int write_file(const char *path, const char *data) {
    FILE *f = fopen(path, "wb");
    if (f == NULL)
        return 0;
    size_t len = strlen(data);
    int ok = fwrite(data, 1, len, f) == len;
    if (fclose(f) != 0)
        ok = 0;
    return ok;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return 0;
    fclose(f);
    return 1;
}

static int copy_file(const char *from, const char *to) {
    FILE *src = fopen(from, "rb");
    if (src == NULL)
        return 0;
    FILE *dst = fopen(to, "wb");
    if (dst == NULL) {
        fclose(src);
        return 0;
    }
    char buf[4096];
    size_t n;
    int ok = 1;
    while ((n = fread(buf, 1, sizeof buf, src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            ok = 0;
            break;
        }
    }
    if (ferror(src))
        ok = 0;
    fclose(src);
    fclose(dst);
    return ok;
}

static int query_int(sqlite3 *db, const char *sql) {
    sqlite3_stmt *st = NULL;
    int v = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW)
            v = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
    return v;
}

static void query_text(sqlite3 *db, const char *sql, char *out, size_t cap) {
    sqlite3_stmt *st = NULL;
    out[0] = '\0';
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) {
            const unsigned char *s = sqlite3_column_text(st, 0);
            if (s != NULL)
                snprintf(out, cap, "%s", (const char *)s);
        }
    }
    sqlite3_finalize(st);
}

static void cleanup_db_files(const char *path) {
    char wal[512];
    char shm[512];
    char bak[512];
    snprintf(wal, sizeof wal, "%s-wal", path);
    snprintf(shm, sizeof shm, "%s-shm", path);
    snprintf(bak, sizeof bak, "%s.bak", path);
    remove_file(path);
    remove_file(wal);
    remove_file(shm);
    remove_file(bak);
}

CT_SUITE(db_open) {
    sqlite3 *db = NULL;
    ct_db_close(NULL);
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(db != NULL);
    ct_db_close(db);
    db = NULL;
    ct_db_close(db);

    make_dir("build");
    make_dir(CT_TEST_DIR);
    const char *path = CT_TEST_DIR "/open_test.db";
    cleanup_db_files(path);
    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, path), SQLITE_OK);
    CT_ASSERT(file_exists(path));
    char mode[32];
    query_text(db, "PRAGMA journal_mode", mode, sizeof mode);
    CT_ASSERT_STR_EQ(mode, "wal");
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    query_text(db, "PRAGMA journal_mode", mode, sizeof mode);
    CT_ASSERT_STR_EQ(mode, "memory");
    ct_db_close(db);
}

CT_SUITE(migrate) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    int tables = query_int(db, "SELECT count(*) FROM sqlite_master"
                               " WHERE type='table' AND name NOT LIKE 'sqlite_%'");
    CT_ASSERT_EQ(tables, 9);
    int indexes = query_int(db, "SELECT count(*) FROM sqlite_master"
                                " WHERE type='index' AND name NOT LIKE 'sqlite_%'");
    CT_ASSERT_EQ(indexes, 5);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/missing_migrations") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    ct_db_close(db);
}

CT_SUITE(migrate_backup) {
    make_dir("build");
    make_dir(CT_TEST_DIR);
    make_dir(CT_TEST_DIR "/mig");
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/mig/001_init.sql"));
    FILE *f = fopen(CT_TEST_DIR "/mig/002_dummy.sql", "wb");
    CT_ASSERT(f != NULL);
    CT_ASSERT(fputs("CREATE TABLE dummy(id INTEGER)", f) >= 0);
    fclose(f);

    const char *path = CT_TEST_DIR "/backup_test.db";
    char bak[512];
    snprintf(bak, sizeof bak, "%s.bak", path);
    cleanup_db_files(path);

    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, path), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT(!file_exists(bak));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/mig"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT(file_exists(bak));
    int dummy =
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='dummy'");
    CT_ASSERT_EQ(dummy, 1);

    sqlite3 *backup = NULL;
    CT_ASSERT_EQ(sqlite3_open(bak, &backup), SQLITE_OK);
    CT_ASSERT_EQ(query_int(backup, "PRAGMA user_version"), 1);
    int backup_dummy =
        query_int(backup, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='dummy'");
    CT_ASSERT_EQ(backup_dummy, 0);
    sqlite3_close(backup);
    ct_db_close(db);
}

CT_SUITE(migrate_error_paths) {
    make_dir("build");
    make_dir(CT_TEST_DIR);
    make_dir(CT_TEST_DIR "/migerr");
    make_dir(CT_TEST_DIR "/miggap");
    make_dir(CT_TEST_DIR "/migcrlf");
    make_dir(CT_TEST_DIR "/migbom");
    make_dir(CT_TEST_DIR "/migro");
    remove_file(CT_TEST_DIR "/migerr/002_bad.sql");
    remove_file(CT_TEST_DIR "/miggap/003_gap.sql");
    remove_file(CT_TEST_DIR "/migcrlf/001_crlf.sql");
    remove_file(CT_TEST_DIR "/migbom/001_bom.sql");
    remove_file(CT_TEST_DIR "/migro/002_ro.sql");

    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migerr/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/migerr/002_bad.sql",
                         "CREATE TABLE half(id INTEGER);\nCREATE TABLE 2bad(x INTEGER);\n"));
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migerr") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='half'"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/miggap/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/miggap/003_gap.sql", "CREATE TABLE gap3(id INTEGER);\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/miggap"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 3);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='gap3'"), 1);
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/miggap"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 3);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(write_file(CT_TEST_DIR "/migcrlf/001_crlf.sql",
                         "CREATE TABLE crlf_t(id INTEGER);\r\nINSERT INTO crlf_t VALUES (7);\r\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migcrlf"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT_EQ(query_int(db, "SELECT id FROM crlf_t"), 7);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(write_file(CT_TEST_DIR "/migbom/001_bom.sql", "\xEF\xBB\xBF"
                                                            "CREATE TABLE bom_t(id INTEGER);\r\n"
                                                            "INSERT INTO bom_t VALUES (1);\r\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migbom"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT_EQ(query_int(db, "SELECT id FROM bom_t"), 1);
    ct_db_close(db);

    const char *path = CT_TEST_DIR "/ro_test.db";
    char bak[512];
    snprintf(bak, sizeof bak, "%s.bak", path);
    cleanup_db_files(path);
    remove_dir(bak);
    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, path), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT(write_file(CT_TEST_DIR "/migro/002_ro.sql", "CREATE TABLE ro2(id INTEGER);\n"));
    make_dir(bak);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migro") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='ro2'"), 0);
    CT_ASSERT_EQ(query_int(db, "SELECT count(*) FROM sqlite_master"
                               " WHERE type='table' AND name NOT LIKE 'sqlite_%'"),
                 9);
    ct_db_close(db);
    remove_dir(bak);
    cleanup_db_files(path);
}

CT_SUITE(pragmas) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(query_int(db, "PRAGMA foreign_keys"), 1);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    int rc = sqlite3_exec(db,
                          "INSERT INTO srs_state(card_id, stability, difficulty, due)"
                          " VALUES (999, 1.0, 5.0, '2026-01-01')",
                          NULL, NULL, NULL);
    CT_ASSERT_EQ(rc, SQLITE_CONSTRAINT);
    ct_db_close(db);
}
