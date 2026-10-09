#include <math.h>
#include <stdio.h>
#include <string.h>

#include "storage/db.h"
#include "storage/repo_cards.h"
#include "storage/repo_reviews.h"

int main(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK) {
        return 2;
    }
    ct_db_migrate(db, "db/migrations");
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", "d");
    snprintf(c.case_key, sizeof c.case_key, "%s", "k");
    if (ct_repo_card_insert(db, &c) != CT_REPO_OK) {
        return 2;
    }
    ct_srs_row row;
    memset(&row, 0, sizeof row);
    row.stability = NAN;
    row.difficulty = 5;
    snprintf(row.due, sizeof row.due, "%s", "2026-10-10T00:00:00Z");
    row.reps = 1;
    row.state = 1;
    int rc = ct_repo_srs_put(db, c.id, &row);
    printf("srs_put with NaN stability rc=%d\n", rc);
    ct_srs_row back;
    int grc = ct_repo_srs_get(db, c.id, &back);
    printf("srs_get rc=%d stability=%f\n", grc, back.stability);
    int rr = ct_repo_review_log(db, c.id, 3, "2026-10-10T00:00:00Z", NAN, 5.0, 1.0, 2.0);
    printf("review_log with NaN stability rc=%d\n", rr);
    return 0;
}
