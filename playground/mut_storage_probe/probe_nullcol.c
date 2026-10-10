#include <stdio.h>
#include <string.h>

#include "storage/db.h"
#include "storage/repo_algs.h"

int main(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK || ct_db_migrate(db, "db/migrations") != 0) {
        printf("open failed\n");
        return 1;
    }
    char *err = NULL;
    int rc = sqlite3_exec(db,
                          "INSERT INTO generated_alg(orbit, target_pair, buffer_mode, category,"
                          " sequence, classification, setup, interchange, insertion, qtm, score,"
                          " score_parts_json, params_hash, created_at)"
                          " VALUES(1, 'ZZ', 0, 0, 'x', 0, NULL, NULL, NULL, 1, 1.0, NULL, 'hx',"
                          " '2026-01-01T00:00:00Z')",
                          NULL, NULL, &err);
    printf("insert rc=%d err=%s\n", rc, err != NULL ? err : "(none)");
    sqlite3_free(err);
    sqlite3_stmt *st = NULL;
    sqlite3_prepare_v2(db, "SELECT setup, typeof(setup) FROM generated_alg", -1, &st, NULL);
    int step = sqlite3_step(st);
    const unsigned char *txt = sqlite3_column_text(st, 0);
    const unsigned char *ty = sqlite3_column_text(st, 1);
    printf("step=%d setup_ptr=%p typeof=%s\n", step, (const void *)txt,
           ty != NULL ? (const char *)ty : "(null)");
    sqlite3_finalize(st);

    ct_generated_alg got;
    memset(&got, 0xAA, sizeof got);
    printf("before get: setup[0]=%02x setup[1]=%02x setup[2]=%02x\n", (unsigned char)got.setup[0],
           (unsigned char)got.setup[1], (unsigned char)got.setup[2]);
    int grc = ct_repo_alg_get(db, CT_ORBIT_EDGE, "ZZ", CT_BUFFER_FIXED, 0, "hx", &got);
    printf("get rc=%d setup[0]=%02x setup[1]=%02x setup[2]=%02x setup[3]=%02x\n", grc,
           (unsigned char)got.setup[0], (unsigned char)got.setup[1], (unsigned char)got.setup[2],
           (unsigned char)got.setup[3]);
    ct_db_close(db);
    return 0;
}
