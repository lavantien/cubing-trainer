#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "storage/db.h"
#include "storage/repo_settings.h"

static const char *tmpdir;

static void wfile(const char *name, const char *body) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", tmpdir, name);
    FILE *f = fopen(path, "wb");
    fwrite(body, 1, strlen(body), f);
    fclose(f);
}

static int set_version(sqlite3 *db, int v) {
    char sql[64];
    snprintf(sql, sizeof sql, "PRAGMA user_version = %d", v);
    return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

int main(int argc, char **argv) {
    tmpdir = argc > 1 ? argv[1] : ".";
    int with_dup = argc > 2 && strcmp(argv[2], "dup") == 0;
    if (with_dup) {
        wfile("002_a.sql", "CREATE TABLE probe_b(x);\n");
        wfile("002_b.sql", "CREATE TABLE probe_c(x);\n");
        wfile("003_skip.sql", "CREATE TABLE probe_d(x);\n");
        wfile("bad_005.sql", "CREATE TABLE probe_e(x;\n");
    }

    sqlite3 *db = NULL;
    ct_db_open(&db, ":memory:");
    int r_empty = ct_db_migrate(db, tmpdir);
    printf("fresh migrate=%d version=%d\n", r_empty, ct_db_version(db));
    ct_db_close(db);

    ct_db_open(&db, ":memory:");
    set_version(db, 9);
    int r_ahead = ct_db_migrate(db, tmpdir);
    printf("version_ahead migrate=%d\n", r_ahead);
    ct_db_close(db);

    ct_db_open(&db, ":memory:");
    set_version(db, 1);
    int r_v1 = ct_db_migrate(db, tmpdir);
    int merge_rc = ct_repo_settings_merge(db, NULL);
    sqlite3_int64 dummy = 0;
    (void)dummy;
    printf("version1_no_tables migrate=%d merge=%d\n", r_v1, merge_rc);
    ct_db_close(db);

    ct_db_open(&db, ":memory:");
    sqlite3_exec(db, "CREATE TABLE settings(key TEXT PRIMARY KEY, value TEXT)", NULL, NULL, NULL);
    sqlite3_exec(db, "INSERT INTO settings VALUES('srs.request_retention','0.95')", NULL, NULL,
                 NULL);
    sqlite3_exec(db, "INSERT INTO settings VALUES('gen.preferred_moves','xyz')", NULL, NULL, NULL);
    sqlite3_exec(db, "INSERT INTO settings VALUES('bld.buffer_corner','7')", NULL, NULL, NULL);
    set_version(db, 1);
    int r_partial = ct_db_migrate(db, tmpdir);
    ct_config cfg = ct_config_defaults();
    int merge2 = ct_repo_settings_merge(db, &cfg);
    printf("partial_schema migrate=%d merge=%d retention=%.2f mask=%x\n", r_partial, merge2,
           cfg.fsrs.request_retention, cfg.gen.preferred_moves);
    ct_db_close(db);

    ct_db_open(&db, ":memory:");
    set_version(db, 0);
    sqlite3_exec(db, "CREATE TABLE flashcard(id INTEGER PRIMARY KEY)", NULL, NULL, NULL);
    int r_clash = ct_db_migrate(db, tmpdir);
    printf("clash_schema migrate=%d\n", r_clash);
    ct_db_close(db);

    ct_db_open(&db, ":memory:");
    int r_bad = ct_db_migrate(db, "no/such/dir");
    printf("missing_dir migrate=%d\n", r_bad);
    ct_db_close(db);
    return 0;
}
