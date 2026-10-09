#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "storage/db.h"

static int failures = 0;

static void check(const char *label, int cond) {
    printf("%s: %s\n", cond ? "OK  " : "FAIL", label);
    if (!cond)
        failures++;
}

static void write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        printf("cannot write %s\n", path);
        failures++;
        return;
    }
    fwrite(text, 1, strlen(text), f);
    fclose(f);
}

static void write_file_n(const char *path, const char *text, size_t len) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        printf("cannot write %s\n", path);
        failures++;
        return;
    }
    fwrite(text, 1, len, f);
    fclose(f);
}

static int table_exists(sqlite3 *db, const char *name) {
    sqlite3_stmt *st = NULL;
    int found = 0;
    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?", -1,
                           &st, NULL) == SQLITE_OK) {
        sqlite3_bind_text(st, 1, name, -1, SQLITE_STATIC);
        if (sqlite3_step(st) == SQLITE_ROW)
            found = sqlite3_column_int(st, 0) > 0;
    }
    sqlite3_finalize(st);
    return found;
}

static void rm(const char *path) {
    remove(path);
}

int main(void) {
    _mkdir("playground/adv_a/store");
    _mkdir("playground/adv_a/store/mig_gap");
    _mkdir("playground/adv_a/store/mig_dup");
    _mkdir("playground/adv_a/store/mig_nul");
    _mkdir("playground/adv_a/store/mig_junk");
    _mkdir("playground/adv_a/store/mig_sort");

    printf("== gap: 001 then 003, no 002 ==\n");
    {
        write_file("playground/adv_a/store/mig_gap/001_a.sql", "CREATE TABLE ga(a);");
        write_file("playground/adv_a/store/mig_gap/003_c.sql", "CREATE TABLE gc(c);");
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/gap.db");
        ct_db_open(&db, "playground/adv_a/store/gap.db");
        int rc = ct_db_migrate(db, "playground/adv_a/store/mig_gap");
        check("gap migrate rc 0", rc == 0);
        int v = ct_db_version(db);
        printf("version after gap migrations: %d\n", v);
        check("gap version jumps to 3", v == 3);
        check("table ga exists", table_exists(db, "ga"));
        check("table gc exists", table_exists(db, "gc"));
        ct_db_close(db);
    }

    printf("== hand edited user_version above max ==\n");
    {
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/high.db");
        ct_db_open(&db, "playground/adv_a/store/high.db");
        sqlite3_exec(db, "CREATE TABLE t(a)", NULL, NULL, NULL);
        sqlite3_exec(db, "PRAGMA user_version = 5", NULL, NULL, NULL);
        int rc = ct_db_migrate(db, "db/migrations");
        check("migrate with future version rc 0", rc == 0);
        check("version stays 5", ct_db_version(db) == 5);
        check("real schema not applied", !table_exists(db, "flashcard"));
        ct_db_close(db);
    }

    printf("== hand edited user_version 0 with schema present ==\n");
    {
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/reset.db");
        ct_db_open(&db, "playground/adv_a/store/reset.db");
        int rc1 = ct_db_migrate(db, "db/migrations");
        sqlite3_exec(db, "PRAGMA user_version = 0", NULL, NULL, NULL);
        int rc2 = ct_db_migrate(db, "db/migrations");
        printf("first migrate rc=%d, after version reset rc=%d\n", rc1, rc2);
        check("version reset re-run fails on existing tables", rc2 != 0);
        check("version stays 0 after failed re-run", ct_db_version(db) == 0);
        ct_db_close(db);
    }

    printf("== embedded NUL truncates migration SQL ==\n");
    {
        const char sql[] = {'C', 'R', 'E', 'A', 'T', 'E', ' ', 'T', 'A', 'B', 'L', 'E',
                            ' ', 'n', 'u', 'l', '1', '(', 'a', ')', ';', '\0', 'C', 'R',
                            'E', 'A', 'T', 'E', ' ', 'T', 'A', 'B', 'L', 'E', ' ', 'n',
                            'u', 'l', '2', '(', 'a', ')', ';', '\0'};
        write_file_n("playground/adv_a/store/mig_nul/002_nul.sql", sql, sizeof sql);
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/nul.db");
        ct_db_open(&db, "playground/adv_a/store/nul.db");
        sqlite3_exec(db, "CREATE TABLE base(a)", NULL, NULL, NULL);
        sqlite3_exec(db, "PRAGMA user_version = 1", NULL, NULL, NULL);
        int rc = ct_db_migrate(db, "playground/adv_a/store/mig_nul");
        check("nul migration rc 0", rc == 0);
        check("version bumped to 2", ct_db_version(db) == 2);
        check("statement before NUL ran", table_exists(db, "nul1"));
        check("statement after NUL silently dropped", !table_exists(db, "nul2"));
        ct_db_close(db);
    }

    printf("== non-UTF8 and bad SQL roll back ==\n");
    {
        const char junk[] = {'C', 'R', 'E', 'A', 'T', 'E', ' ', 'T', 'A', 'B', 'L', 'E', ' ',
                             'x', '(', 'a', ')', ';', 0xFF, 0xFE, ' ', 'G', 'A', 'R', 'B', 'A',
                             'G', 'E', '(', 0x80, ')', ';', '\0'};
        write_file_n("playground/adv_a/store/mig_junk/002_junk.sql", junk, sizeof junk);
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/junk.db");
        ct_db_open(&db, "playground/adv_a/store/junk.db");
        sqlite3_exec(db, "CREATE TABLE base(a)", NULL, NULL, NULL);
        sqlite3_exec(db, "PRAGMA user_version = 1", NULL, NULL, NULL);
        int rc = ct_db_migrate(db, "playground/adv_a/store/mig_junk");
        printf("junk migration rc=%d\n", rc);
        check("bad sql fails migrate", rc != 0);
        check("version not bumped", ct_db_version(db) == 1);
        check("partial table rolled back", !table_exists(db, "x"));
        ct_db_close(db);
    }

    printf("== duplicate versions apply in name order ==\n");
    {
        write_file("playground/adv_a/store/mig_dup/002_a.sql", "CREATE TABLE dupa(a);");
        write_file("playground/adv_a/store/mig_dup/002_b.sql", "CREATE TABLE dupb(b);");
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/dup.db");
        ct_db_open(&db, "playground/adv_a/store/dup.db");
        int rc = ct_db_migrate(db, "playground/adv_a/store/mig_dup");
        check("dup versions rc 0", rc == 0);
        check("both dup tables exist", table_exists(db, "dupa") && table_exists(db, "dupb"));
        printf("version after dupes: %d\n", ct_db_version(db));
        ct_db_close(db);
    }

    printf("== numeric sort: 9 before 10 ==\n");
    {
        write_file("playground/adv_a/store/mig_sort/0009_a.sql", "CREATE TABLE s9(a);");
        write_file("playground/adv_a/store/mig_sort/0010_b.sql", "CREATE TABLE s10(a);");
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/sort.db");
        ct_db_open(&db, "playground/adv_a/store/sort.db");
        int rc = ct_db_migrate(db, "playground/adv_a/store/mig_sort");
        check("numeric sort migrate rc 0", rc == 0);
        check("both sorted tables exist", table_exists(db, "s9") && table_exists(db, "s10"));
        check("version is 10", ct_db_version(db) == 10);
        ct_db_close(db);
    }

    printf("== overflow version silently skipped ==\n");
    {
        _mkdir("playground/adv_a/store/mig_ovf");
        write_file("playground/adv_a/store/mig_ovf/99999999999999999999_z.sql", "CREATE TABLE ovf(a);");
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/ovf.db");
        ct_db_open(&db, "playground/adv_a/store/ovf.db");
        int rc = ct_db_migrate(db, "playground/adv_a/store/mig_ovf");
        check("overflow migrate rc 0", rc == 0);
        printf("version after overflow file: %d, ovf table exists: %d\n", ct_db_version(db),
               table_exists(db, "ovf"));
        ct_db_close(db);
    }

    printf("== backup blocked: .bak path is a directory ==\n");
    {
        write_file("playground/adv_a/store/mig_gap/004_d.sql", "CREATE TABLE gd(d);");
        sqlite3 *db = NULL;
        rm("playground/adv_a/store/bak.db");
        ct_db_open(&db, "playground/adv_a/store/bak.db");
        int rc0 = ct_db_migrate(db, "playground/adv_a/store/mig_gap");
        rm("playground/adv_a/store/bak2.db");
        ct_db_close(db);
        ct_db_open(&db, "playground/adv_a/store/bak2.db");
        sqlite3_exec(db, "CREATE TABLE base(a)", NULL, NULL, NULL);
        sqlite3_exec(db, "PRAGMA user_version = 2", NULL, NULL, NULL);
        _mkdir("playground/adv_a/store/bak2.db.bak");
        int rc = ct_db_migrate(db, "playground/adv_a/store/mig_gap");
        printf("blocked backup migrate rc=%d (rc0=%d)\n", rc, rc0);
        check("blocked backup fails migrate", rc != 0);
        check("version not bumped while blocked", ct_db_version(db) == 2);
        check("migration not applied while blocked", !table_exists(db, "ga"));
        ct_db_close(db);
    }

    printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
