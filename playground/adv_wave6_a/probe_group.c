#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "config/config.h"
#include "storage/db.h"
#include "trainer/selector.h"

static int die(sqlite3 *db, const char *what) {
    fprintf(stderr, "FAIL %s: %s\n", what, db ? sqlite3_errmsg(db) : "no db");
    sqlite3_close(db);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: probe_group <db>\n");
        return 2;
    }
    sqlite3 *db = NULL;
    remove(argv[1]);
    if (sqlite3_open(argv[1], &db) != SQLITE_OK) {
        return die(db, "open");
    }
    if (ct_db_migrate(db, "../../db/migrations") != 0) {
        return die(db, "migrate");
    }
    const char *sql =
        "INSERT INTO event_pack(event, method, stage, case_key, alg, front_json) VALUES"
        " (%d, %d, %d, ?, 'A', '{}');";
    const char *keys[] = {"flip.01", "twist.02", "parity.03", "ltct.04",
                          "clock.01", "", "flip", "flip.x", "flip.", "parity.99", "zzz"};
    for (unsigned long i = 0; i < sizeof keys / sizeof keys[0]; i++) {
        char stmt[512];
        snprintf(stmt, sizeof stmt, sql, CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT);
        sqlite3_stmt *st = NULL;
        if (sqlite3_prepare_v2(db, stmt, -1, &st, NULL) != SQLITE_OK) {
            return die(db, "prepare");
        }
        sqlite3_bind_text(st, 1, keys[i], -1, SQLITE_STATIC);
        if (sqlite3_step(st) != SQLITE_DONE) {
            return die(db, "insert");
        }
        sqlite3_finalize(st);
    }
    for (int i = 0; i < 11; i++) {
        ct_pack_entry e;
        int prc = ct_selector_pack_case(db, CT_EVENT_CLOCK, CT_METHOD_LBL, 0, i, &e);
        int g = ct_selector_case_group(db, CT_EVENT_CLOCK, CT_METHOD_LBL, 0, i);
        printf("index %d pack_case %d actual_key '%s' -> group %d label %s\n", i, prc,
               prc == 0 ? e.case_key : "?", g, g >= 0 ? ct_selector_group_label(g) : "(null)");
    }
    printf("pack count %d\n", ct_pack_count(db, CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT));
    sqlite3_close(db);
    return 0;
}
