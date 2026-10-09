#include "db.h"

#include <string.h>

#include "config/config.h"

int ct_db_open(sqlite3 **db, const char *path) {
    if (db == NULL || path == NULL)
        return SQLITE_MISUSE;
    *db = NULL;
    int rc = sqlite3_open_v2(path, db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        if (*db != NULL) {
            sqlite3_close(*db);
            *db = NULL;
        }
        return rc;
    }
    sqlite3_busy_timeout(*db, CT_DB_BUSY_TIMEOUT_MS);
    char *err = NULL;
    rc = sqlite3_exec(*db, "PRAGMA foreign_keys=ON", NULL, NULL, &err);
    sqlite3_free(err);
    err = NULL;
    if (rc == SQLITE_OK && strcmp(path, ":memory:") != 0) {
        rc = sqlite3_exec(*db, "PRAGMA journal_mode=WAL", NULL, NULL, &err);
        sqlite3_free(err);
    }
    if (rc != SQLITE_OK) {
        sqlite3_close(*db);
        *db = NULL;
        return rc;
    }
    return SQLITE_OK;
}

void ct_db_close(sqlite3 *db) {
    if (db == NULL)
        return;
    sqlite3_close(db);
}

int ct_db_version(sqlite3 *db) {
    if (db == NULL)
        return -1;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return -1;
    }
    int v = -1;
    if (sqlite3_step(st) == SQLITE_ROW)
        v = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return v;
}
