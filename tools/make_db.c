#include <stdio.h>

#include "storage/db.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: make_db <out.db>\n");
        return 1;
    }
    sqlite3 *db = NULL;
    if (ct_db_open(&db, argv[1]) != SQLITE_OK) {
        fprintf(stderr, "make_db: ct_db_open(%s) failed\n", argv[1]);
        return 1;
    }
    if (ct_db_migrate(db, "db/migrations") != 0) {
        fprintf(stderr, "make_db: ct_db_migrate failed\n");
        ct_db_close(db);
        return 1;
    }
    int version = ct_db_version(db);
    ct_db_close(db);
    printf("%s user_version=%d\n", argv[1], version);
    return 0;
}
