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

#include "../src/config/config.h"
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

static long file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return size;
}

static int file_prefix_eq(const char *a, const char *b, size_t n) {
    FILE *fa = fopen(a, "rb");
    FILE *fb = fopen(b, "rb");
    if (fa == NULL || fb == NULL) {
        if (fa != NULL)
            fclose(fa);
        if (fb != NULL)
            fclose(fb);
        return 0;
    }
    char ba[64];
    char bb[64];
    size_t na = fread(ba, 1, n, fa);
    size_t nb = fread(bb, 1, n, fb);
    fclose(fa);
    fclose(fb);
    return na == n && nb == n && memcmp(ba, bb, n) == 0;
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

    db = (sqlite3 *)0x1;
    CT_ASSERT(ct_db_open(&db, CT_TEST_DIR "/no_such_dir/x.db") != SQLITE_OK);
    CT_ASSERT(db == NULL);
    CT_ASSERT_EQ(ct_db_open(NULL, ":memory:"), SQLITE_MISUSE);
    CT_ASSERT_EQ(ct_db_open(&db, NULL), SQLITE_MISUSE);
    CT_ASSERT_EQ(ct_db_version(NULL), -1);

    const char *near_path = "9z_close.db";
    cleanup_db_files(near_path);
    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, near_path), SQLITE_OK);
    query_text(db, "PRAGMA journal_mode", mode, sizeof mode);
    CT_ASSERT_STR_EQ(mode, "wal");
    ct_db_close(db);
    cleanup_db_files(near_path);
}

CT_SUITE(migrate_name_edges) {
    make_dir("build");
    make_dir(CT_TEST_DIR);
    make_dir(CT_TEST_DIR "/mig1x");
    make_dir(CT_TEST_DIR "/migtxt");
    make_dir(CT_TEST_DIR "/migcmp");
    make_dir(CT_TEST_DIR "/mig10");
    make_dir(CT_TEST_DIR "/miglen");
    remove_file(CT_TEST_DIR "/mig1x/1x.sql");
    remove_file(CT_TEST_DIR "/migtxt/1_x.txt");
    remove_file(CT_TEST_DIR "/migcmp/2_z.sql");
    remove_file(CT_TEST_DIR "/migcmp/03_a.sql");
    remove_file(CT_TEST_DIR "/mig10/10_x.sql");
    remove_file(CT_TEST_DIR "/miglen/2_.sql");

    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/mig1x/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/mig1x/1x.sql", "CREATE TABLE nope(id INTEGER);\n"));
    remove_file(CT_TEST_DIR "/mig1x/x1_y.sql");
    CT_ASSERT(write_file(CT_TEST_DIR "/mig1x/x1_y.sql", "CREATE TABLE nope2(id INTEGER);\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/mig1x"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migtxt/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/migtxt/1_x.txt", "CREATE TABLE nope(id INTEGER);\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migtxt"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migcmp/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/migcmp/2_z.sql", "CREATE TABLE cmp2(id INTEGER);\n"));
    CT_ASSERT(write_file(CT_TEST_DIR "/migcmp/03_a.sql", "CREATE TABLE cmp3(id INTEGER);\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migcmp"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 3);
    CT_ASSERT_EQ(query_int(db, "SELECT count(*) FROM cmp2"), 0);
    CT_ASSERT_EQ(query_int(db, "SELECT count(*) FROM cmp3"), 0);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/mig1x") != 0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/mig10/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/mig10/10_x.sql", "CREATE TABLE nope(id INTEGER);\n"));
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/mig10") != 0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/miglen/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/miglen/2_.sql", "CREATE TABLE len2(id INTEGER);\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/miglen"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(query_int(db, "SELECT count(*) FROM sqlite_master WHERE name = 'len2'"), 1);
    ct_db_close(db);

    make_dir(CT_TEST_DIR "/migbig");
    make_dir(CT_TEST_DIR "/migmany");
    make_dir(CT_TEST_DIR "/migempty");
    make_dir(CT_TEST_DIR "/migovf");
    remove_file(CT_TEST_DIR "/migbig/002_big.sql");
    remove_file(CT_TEST_DIR "/migovf/99999999999999999999_ovf.sql");

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migbig/001_init.sql"));
    char big[CT_MIG_COPY_BUF + 160];
    memset(big, '-', sizeof big);
    memcpy(big, "-- ", 3);
    big[CT_MIG_COPY_BUF + 100] = '\n';
    memcpy(big + CT_MIG_COPY_BUF + 101, "CREATE TABLE big_end(id INTEGER);\n", 34);
    big[CT_MIG_COPY_BUF + 135] = '\0';
    CT_ASSERT(write_file(CT_TEST_DIR "/migbig/002_big.sql", big));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migbig"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(query_int(db, "SELECT count(*) FROM sqlite_master WHERE name = 'big_end'"), 1);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migmany/001_init.sql"));
    char name[CT_DB_PATH_MAX];
    for (int v = 2; v <= 10; v++) {
        snprintf(name, sizeof name, "%s/migmany/%03d_step.sql", CT_TEST_DIR, v);
        remove_file(name);
        char body[64];
        snprintf(body, sizeof body, "-- step %d\n", v);
        CT_ASSERT(write_file(name, body));
    }
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migmany"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 10);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migovf/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/migovf/99999999999999999999_ovf.sql", "-- ovf\n"));
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migovf") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migempty"), 0);
    CT_ASSERT(ct_db_migrate(NULL, "db/migrations") != 0);
    CT_ASSERT(ct_db_migrate(db, NULL) != 0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/mig1x"), 0);
    ct_db_close(db);

    const char *noop_path = CT_TEST_DIR "/noop.db";
    cleanup_db_files(noop_path);
    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, noop_path), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    char noop_bak[512];
    snprintf(noop_bak, sizeof noop_bak, "%s.bak", noop_path);
    remove_file(noop_bak);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT(!file_exists(noop_bak));
    ct_db_close(db);
    cleanup_db_files(noop_path);

    cleanup_db_files("x");
    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, "x"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/mig1x"), 0);
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/miglen"), 0);
    CT_ASSERT(file_exists("x.bak"));
    CT_ASSERT(file_prefix_eq("x", "x.bak", 15));
    CT_ASSERT_EQ(file_size("x.bak"), file_size("x"));
    ct_db_close(db);
    cleanup_db_files("x");
}

CT_SUITE(migrate) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    int tables = query_int(db, "SELECT count(*) FROM sqlite_master"
                               " WHERE type='table' AND name NOT LIKE 'sqlite_%'");
    CT_ASSERT_EQ(tables, 10);
    int indexes = query_int(db, "SELECT count(*) FROM sqlite_master"
                                " WHERE type='index' AND name NOT LIKE 'sqlite_%'");
    CT_ASSERT_EQ(indexes, 5);
    CT_ASSERT_EQ(query_int(db, "SELECT count(*) FROM sqlite_master"
                               " WHERE type='table' AND name='memo_pair'"),
                 1);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/missing_migrations") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    ct_db_close(db);
}

CT_SUITE(migrate_backup) {
    make_dir("build");
    make_dir(CT_TEST_DIR);
    make_dir(CT_TEST_DIR "/mig");
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/mig/001_init.sql"));
    FILE *f = fopen(CT_TEST_DIR "/mig/003_dummy.sql", "wb");
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
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT(!file_exists(bak));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/mig"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 3);
    CT_ASSERT(file_exists(bak));
    int dummy =
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='dummy'");
    CT_ASSERT_EQ(dummy, 1);

    sqlite3 *backup = NULL;
    CT_ASSERT_EQ(sqlite3_open(bak, &backup), SQLITE_OK);
    CT_ASSERT_EQ(query_int(backup, "PRAGMA user_version"), 2);
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
    remove_file(CT_TEST_DIR "/migcrlf/002_crlf.sql");
    remove_file(CT_TEST_DIR "/migbom/001_bom.sql");
    remove_file(CT_TEST_DIR "/migbom/002_bom.sql");
    remove_file(CT_TEST_DIR "/migro/003_ro.sql");

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
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/miggap") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 0);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='flashcard'"),
        0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migcrlf/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/migcrlf/002_crlf.sql",
                         "CREATE TABLE crlf_t(id INTEGER);\r\nINSERT INTO crlf_t VALUES (7);\r\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migcrlf"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(query_int(db, "SELECT id FROM crlf_t"), 7);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migbom/001_init.sql"));
    CT_ASSERT(write_file(CT_TEST_DIR "/migbom/002_bom.sql", "\xEF\xBB\xBF"
                                                            "CREATE TABLE bom_t(id INTEGER);\r\n"
                                                            "INSERT INTO bom_t VALUES (1);\r\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migbom"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
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
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT(write_file(CT_TEST_DIR "/migro/003_ro.sql", "CREATE TABLE ro2(id INTEGER);\n"));
    make_dir(bak);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migro") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='ro2'"), 0);
    CT_ASSERT_EQ(query_int(db, "SELECT count(*) FROM sqlite_master"
                               " WHERE type='table' AND name NOT LIKE 'sqlite_%'"),
                 10);
    ct_db_close(db);
    remove_dir(bak);
    cleanup_db_files(path);
}

CT_SUITE(migrate_soundness) {
    make_dir("build");
    make_dir(CT_TEST_DIR);
    make_dir(CT_TEST_DIR "/migsnd");
    remove_file(CT_TEST_DIR "/migsnd/001_init.sql");
    remove_file(CT_TEST_DIR "/migsnd/002_next.sql");
    remove_file(CT_TEST_DIR "/migsnd/004_gap.sql");
    remove_file(CT_TEST_DIR "/migsnd/000_zero.sql");
    remove_file(CT_TEST_DIR "/migsnd/001_dupe.sql");
    remove_file(CT_TEST_DIR "/migsnd/2147483648_long.sql");
    remove_file(CT_TEST_DIR "/migsnd/00999999999999999999999_big.sql");
    remove_file(CT_TEST_DIR "/migsnd/002_nul.sql");
    CT_ASSERT(copy_file("db/migrations/001_init.sql", CT_TEST_DIR "/migsnd/001_init.sql"));

    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(write_file(CT_TEST_DIR "/migsnd/002_next.sql", "CREATE TABLE next2(id INTEGER);\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migsnd"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migsnd"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT(write_file(CT_TEST_DIR "/migsnd/004_gap.sql", "CREATE TABLE gap4(id INTEGER);\n"));
    remove_file(CT_TEST_DIR "/migsnd/002_next.sql");
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='gap4'"), 0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 0);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='flashcard'"),
        0);
    ct_db_close(db);
    remove_file(CT_TEST_DIR "/migsnd/004_gap.sql");
    CT_ASSERT(write_file(CT_TEST_DIR "/migsnd/002_next.sql", "CREATE TABLE next2(id INTEGER);\n"));

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_exec(db, "PRAGMA user_version = 5", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 5);
    CT_ASSERT_EQ(sqlite3_exec(db, "PRAGMA user_version = 2", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(write_file(CT_TEST_DIR "/migsnd/000_zero.sql", "CREATE TABLE zero0(id INTEGER);\n"));
    CT_ASSERT_EQ(ct_db_migrate(db, CT_TEST_DIR "/migsnd"), 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='zero0'"), 0);
    ct_db_close(db);
    remove_file(CT_TEST_DIR "/migsnd/000_zero.sql");

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(write_file(CT_TEST_DIR "/migsnd/001_dupe.sql", "CREATE TABLE dupe1(id INTEGER);\n"));
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 0);
    ct_db_close(db);
    remove_file(CT_TEST_DIR "/migsnd/001_dupe.sql");

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(
        write_file(CT_TEST_DIR "/migsnd/2147483648_long.sql", "CREATE TABLE bigv(id INTEGER);\n"));
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 0);
    ct_db_close(db);
    remove_file(CT_TEST_DIR "/migsnd/2147483648_long.sql");

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT(write_file(CT_TEST_DIR "/migsnd/00999999999999999999999_big.sql",
                         "CREATE TABLE bigv2(id INTEGER);\n"));
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 0);
    ct_db_close(db);
    remove_file(CT_TEST_DIR "/migsnd/00999999999999999999999_big.sql");

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    remove_file(CT_TEST_DIR "/migsnd/002_next.sql");
    FILE *nul = fopen(CT_TEST_DIR "/migsnd/002_nul.sql", "wb");
    CT_ASSERT(nul != NULL);
    CT_ASSERT(fputs("CREATE TABLE nul2(id INTEGER);\n", nul) >= 0);
    CT_ASSERT(fputc('\0', nul) >= 0);
    CT_ASSERT(fputs("CREATE TABLE nul3(id INTEGER);\n", nul) >= 0);
    fclose(nul);
    CT_ASSERT(ct_db_migrate(db, CT_TEST_DIR "/migsnd") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 1);
    CT_ASSERT_EQ(
        query_int(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='nul2'"), 0);
    ct_db_close(db);
    remove_file(CT_TEST_DIR "/migsnd/002_nul.sql");
}

CT_SUITE(db_schema_assert) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_schema_ok(NULL), 0);
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_schema_ok(db), 0);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_db_schema_ok(db), 1);

    CT_ASSERT_EQ(sqlite3_exec(db, "DROP TABLE flashcard", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_schema_ok(db), 0);
    CT_ASSERT(ct_db_migrate(db, "db/migrations") != 0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(sqlite3_exec(db, "DROP TABLE settings", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_schema_ok(db), 0);
    ct_db_close(db);

    db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(sqlite3_exec(db, "PRAGMA user_version = 1", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT(ct_db_migrate(db, "db/migrations") != 0);
    CT_ASSERT_EQ(ct_db_version(db), 2);
    CT_ASSERT_EQ(sqlite3_get_autocommit(db), 1);
    ct_db_close(db);
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
