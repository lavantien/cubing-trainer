#ifndef CT_STORAGE_DB_H
#define CT_STORAGE_DB_H

#include <sqlite3.h>

#include "config/config.h"

int ct_db_open(sqlite3 **db, const char *path);
void ct_db_close(sqlite3 *db);
int ct_db_migrate(sqlite3 *db, const char *migrations_dir);
int ct_db_version(sqlite3 *db);
int ct_db_schema_ok(sqlite3 *db);

typedef struct {
    char phoneme[CT_MEMO_TEXT_MAX];
    char image[CT_MEMO_TEXT_MAX];
} ct_memo_row;

typedef struct {
    char pair[CT_MEMO_PAIR_MAX];
    char phoneme[CT_MEMO_TEXT_MAX];
    char image[CT_MEMO_TEXT_MAX];
} ct_memo_list_row;

int ct_memo_get(sqlite3 *db, int orbit, const char *pair, ct_memo_row *out);
int ct_memo_set(sqlite3 *db, int orbit, const char *pair, const char *phoneme, const char *image);
int ct_memo_list(sqlite3 *db, int orbit, ct_memo_list_row *rows, int cap);

#endif
