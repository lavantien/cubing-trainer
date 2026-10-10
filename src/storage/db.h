#ifndef CT_STORAGE_DB_H
#define CT_STORAGE_DB_H

#include <sqlite3.h>

int ct_db_open(sqlite3 **db, const char *path);
void ct_db_close(sqlite3 *db);
int ct_db_migrate(sqlite3 *db, const char *migrations_dir);
int ct_db_version(sqlite3 *db);
int ct_db_schema_ok(sqlite3 *db);

#endif
