#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "srs/fsrs.h"
#include "storage/db.h"
#include "storage/repo_cards.h"
#include "storage/repo_settings.h"
#include "ui/ui.h"

static const char day1[] = "2026-10-09T00:00:00Z";
static const char day2[] = "2026-10-10T00:00:00Z";

static long run(const char *retention, const char *label) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != 0 || ct_db_migrate(db, "db/migrations") != 0) {
        fprintf(stderr, "db setup failed\n");
        return -1000;
    }
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "rev");
    snprintf(c.case_key, sizeof c.case_key, "a");
    snprintf(c.alg_3x3, sizeof c.alg_3x3, "R U R'");
    if (ct_repo_card_insert(db, &c) != 0)
        return -1001;
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    snprintf(row.due, sizeof row.due, "2026-10-01T00:00:00Z");
    ct_repo_srs_put(db, c.id, &row);
    if (retention != NULL)
        ct_repo_settings_set(db, "srs.request_retention", retention);

    ct_review_form f;
    int b1 = ct_review_form_begin(&f, db, "rev", day1, 20735, 0);
    int g1 = ct_review_form_grade(&f, CT_GOOD, day1);
    int b2 = ct_review_form_begin(&f, db, "rev", day2, 20736, 0);
    int g2 = ct_review_form_grade(&f, CT_GOOD, day2);
    printf("  b1=%d g1=%d b2=%d g2=%d\n", b1, g1, b2, g2);

    sqlite3_stmt *st = NULL;
    long scheduled = -1;
    if (sqlite3_prepare_v2(db, "SELECT scheduled_days FROM review_log ORDER BY id DESC LIMIT 1",
                           -1, &st, NULL) == SQLITE_OK &&
        sqlite3_step(st) == SQLITE_ROW)
        scheduled = (long)sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    printf("%s: retention=%s scheduled=%ld\n", label, retention ? retention : "default", scheduled);
    ct_db_close(db);
    return scheduled;
}

int main(void) {
    long a = run("0.7", "lo");
    long b = run("0.99", "hi");
    long c = run(NULL, "def");
    printf("summary lo=%ld hi=%ld def=%ld\n", a, b, c);
    return 0;
}
