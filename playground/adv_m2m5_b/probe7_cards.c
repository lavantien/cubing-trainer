#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "storage/db.h"
#include "storage/repo_cards.h"

static long insert(sqlite3 *db, const char *deck, const char *key, int dir) {
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    snprintf(c.case_key, sizeof c.case_key, "%s", key);
    c.direction = dir;
    int rc = ct_repo_card_insert(db, &c);
    return rc == CT_REPO_OK ? c.id : rc;
}

int main(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK || ct_db_migrate(db, "db/migrations") != 0) {
        printf("open/migrate failed\n");
        return 1;
    }
    long a = insert(db, "d", "pll.Aa", 0);
    long b = insert(db, "d", "pll.aa", 0);
    long c = insert(db, "d", "PLL.AA", 0);
    long dup = insert(db, "d", "pll.Aa", 0);
    long dir7 = insert(db, "d", "pll.Aa", 7);
    long dirneg = insert(db, "d", "pll.Aa", -3);
    printf("caseA=%ld case_a=%ld CASE_A=%ld dup=%ld dir7=%ld dirneg=%ld distinct_rows=%d\n", a, b, c,
           dup, dir7, dirneg, a > 0 && b > 0 && c > 0 && dup == CT_REPO_CONFLICT);

    sqlite3_exec(db,
                 "INSERT INTO flashcard(deck, case_key, direction, front_json, alg_3x3, notes,"
                 " created_at, updated_at) VALUES('big','k',0,'{}','"
                 "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"
                 "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"
                 "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"
                 "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR','n','2026-01-01T00:00:00Z',"
                 "'2026-01-01T00:00:00Z')",
                 NULL, NULL, NULL);
    ct_card got;
    long big_id = (long)sqlite3_last_insert_rowid(db);
    int rc = ct_repo_card_get(db, big_id, &got);
    int truncated = (int)strlen(got.alg_3x3);
    int update_rc = ct_repo_card_update(db, &got);
    sqlite3_stmt *st = NULL;
    sqlite3_prepare_v2(db, "SELECT length(alg_3x3) FROM flashcard WHERE id = ?", -1, &st, NULL);
    sqlite3_bind_int64(st, 1, big_id);
    int stored_len = sqlite3_step(st) == SQLITE_ROW ? sqlite3_column_int(st, 0) : -1;
    sqlite3_finalize(st);
    printf("oversized get=%d truncated_alg_len=%d update_rc=%d stored_len_after_update=%d "
           "silent_mutation=%d\n",
           rc, truncated, update_rc, stored_len, stored_len == truncated);

    ct_card nonterm;
    memset(&nonterm, 0, sizeof nonterm);
    memset(nonterm.alg_3x3, 'R', sizeof nonterm.alg_3x3);
    memset(nonterm.deck, 'd', sizeof nonterm.deck);
    snprintf(nonterm.case_key, sizeof nonterm.case_key, "%s", "nn");
    int nt = ct_repo_card_insert(db, &nonterm);
    printf("unterminated_insert_rc=%d\n", nt);
    ct_db_close(db);
    return 0;
}
