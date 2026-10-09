#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "storage/db.h"
#include "storage/repo_cards.h"
#include "storage/repo_reviews.h"
#include "storage/repo_stats.h"
#include "config/config.h"

static int failures = 0;

static void check(int cond, const char *what) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    }
}

static void write_file(const char *path, const char *bytes, size_t len) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        printf("cannot write %s\n", path);
        exit(2);
    }
    fwrite(bytes, 1, len, f);
    fclose(f);
}

static int table_exists(sqlite3 *db, const char *name) {
    sqlite3_stmt *st;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", -1,
                           &st, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(st, 1, name, -1, SQLITE_STATIC);
    int rc = sqlite3_step(st) == SQLITE_ROW;
    sqlite3_finalize(st);
    return rc;
}

static void test_migration_runner(void) {
    const char *base = "playground/adv_b/mig";
    _mkdir("playground/adv_b/mig_v0");
    _mkdir("playground/adv_b/mig_crlf");
    _mkdir("playground/adv_b/mig_bom");
    _mkdir("playground/adv_b/mig_overflow");
    _mkdir("playground/adv_b/mig_dup");

    write_file("playground/adv_b/mig_v0/000_init.sql", "CREATE TABLE v0tab(a);", 23);
    write_file("playground/adv_b/mig_v0/001_real.sql", "CREATE TABLE real1(a);", 23);
    remove("playground/adv_b/v0.db");
    sqlite3 *db = NULL;
    check(ct_db_open(&db, "playground/adv_b/v0.db") == SQLITE_OK, "open v0 db");
    int rc = ct_db_migrate(db, "playground/adv_b/mig_v0");
    printf("v0dir migrate rc=%d user_version=%d v0tab=%d real1=%d\n", rc, ct_db_version(db),
           table_exists(db, "v0tab"), table_exists(db, "real1"));
    check(rc == 0, "v0 dir migrate ok");
    check(ct_db_version(db) == 1, "v0 dir user_version is 1");
    check(table_exists(db, "v0tab") == 0, "000_init never runs on fresh db");
    check(table_exists(db, "real1") == 1, "001 runs on fresh db");
    ct_db_close(db);

    const char *crlf = "CREATE TABLE crlf_t(a INTEGER);\r\nINSERT INTO crlf_t VALUES (7);\r\n";
    write_file("playground/adv_b/mig_crlf/001_crlf.sql", crlf, strlen(crlf));
    remove("playground/adv_b/crlf.db");
    check(ct_db_open(&db, "playground/adv_b/crlf.db") == SQLITE_OK, "open crlf db");
    rc = ct_db_migrate(db, "playground/adv_b/mig_crlf");
    printf("crlf migrate rc=%d user_version=%d table=%d\n", rc, ct_db_version(db),
           table_exists(db, "crlf_t"));
    check(rc == 0, "CRLF migration accepted");
    ct_db_close(db);

    const char *bom_sql = "\xEF\xBB\xBF" "CREATE TABLE bom_t(a);";
    write_file("playground/adv_b/mig_bom/001_bom.sql", bom_sql, 21);
    write_file("playground/adv_b/mig_bom/002_after.sql", "CREATE TABLE after_t(a);", 24);
    remove("playground/adv_b/bom.db");
    check(ct_db_open(&db, "playground/adv_b/bom.db") == SQLITE_OK, "open bom db");
    rc = ct_db_migrate(db, "playground/adv_b/mig_bom");
    printf("bom migrate rc=%d user_version=%d bom_t=%d after_t=%d\n", rc, ct_db_version(db),
           table_exists(db, "bom_t"), table_exists(db, "after_t"));
    ct_db_close(db);

    write_file("playground/adv_b/mig_overflow/001_ok.sql", "CREATE TABLE ok_t(a);", 22);
    write_file("playground/adv_b/mig_overflow/99999999999999999999_zz.sql",
               "THIS IS NOT EVEN SQL", 19);
    remove("playground/adv_b/ovf.db");
    check(ct_db_open(&db, "playground/adv_b/ovf.db") == SQLITE_OK, "open ovf db");
    rc = ct_db_migrate(db, "playground/adv_b/mig_overflow");
    printf("overflow-version migrate rc=%d user_version=%d ok_t=%d\n", rc, ct_db_version(db),
           table_exists(db, "ok_t"));
    check(rc == 0, "ERANGE version file silently skipped, no error");
    ct_db_close(db);

    write_file("playground/adv_b/mig_dup/005_a.sql", "CREATE TABLE dup_a(a);", 23);
    write_file("playground/adv_b/mig_dup/5_b.sql", "CREATE TABLE dup_b(a);", 23);
    remove("playground/adv_b/dup.db");
    check(ct_db_open(&db, "playground/adv_b/dup.db") == SQLITE_OK, "open dup db");
    rc = ct_db_migrate(db, "playground/adv_b/mig_dup");
    printf("dup migrate rc=%d user_version=%d dup_a=%d dup_b=%d\n", rc, ct_db_version(db),
           table_exists(db, "dup_a"), table_exists(db, "dup_b"));
    ct_db_close(db);

    write_file("playground/adv_b/mig_v0/002_more.sql", "CREATE TABLE more1(a);", 23);
    remove("playground/adv_b/v0.db.bak");
    check(ct_db_open(&db, "playground/adv_b/v0.db") == SQLITE_OK, "reopen v0 db");
    rc = ct_db_migrate(db, "playground/adv_b/mig_v0");
    FILE *bak = fopen("playground/adv_b/v0.db.bak", "rb");
    printf("pending migrate rc=%d bak-exists=%d more1=%d\n", rc, bak != NULL,
           table_exists(db, "more1"));
    if (bak != NULL) {
        fclose(bak);
    }
    ct_db_close(db);
}

static void test_like_glob_keys(void) {
    sqlite3 *db = NULL;
    check(ct_db_open(&db, ":memory:") == SQLITE_OK, "open memory db");
    ct_db_migrate(db, "db/migrations");
    static const char *keys[10] = {"100%",   "%wild", "_under", "[brack]", "star*",
                                   "q?mark", "ba\\ck", "]close", "a'b",    "q\"uote"};
    long ids[10];
    for (int i = 0; i < 10; i++) {
        ct_card c;
        memset(&c, 0, sizeof c);
        snprintf(c.deck, sizeof c.deck, "%s", "mdeck");
        snprintf(c.case_key, sizeof c.case_key, "%s", keys[i]);
        c.direction = i % 4;
        int rc = ct_repo_card_insert(db, &c);
        ids[i] = c.id;
        check(rc == CT_REPO_OK, "insert metachar card");
    }
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", "%deck");
    snprintf(c.case_key, sizeof c.case_key, "%s", "only");
    check(ct_repo_card_insert(db, &c) == CT_REPO_OK, "insert percent deck");
    long pct_deck_id = c.id;

    for (int i = 0; i < 10; i++) {
        ct_card out;
        int rc = ct_repo_card_get_by_key(db, "mdeck", keys[i], i % 4, &out);
        check(rc == CT_REPO_OK && out.id == ids[i], "get_by_key exact for metachar key");
        rc = ct_repo_card_get_by_key(db, "mdeck", "%", i % 4, &out);
        if (strcmp(keys[i], "%") == 0) {
            continue;
        }
        check(rc == CT_REPO_MISSING || strcmp(out.case_key, "%") == 0,
              "percent key lookup stays exact");
    }
    ct_card list[16];
    int n = ct_repo_card_list(db, "mdeck", list, 16);
    check(n == 10, "list mdeck returns exactly 10");
    n = ct_repo_card_list(db, "%deck", list, 16);
    check(n == 1 && list[0].id == pct_deck_id, "percent deck list stays exact");
    n = ct_repo_card_list(db, "mde%", list, 16);
    check(n == 0, "no LIKE expansion on deck list");

    long due[16];
    n = ct_repo_due_cards(db, "mdeck", "2026-10-10T00:00:00Z", due, 16);
    printf("due_cards on deck with no srs rows n=%d\n", n);
    check(n == 0, "no due cards without srs rows");
    ct_db_close(db);
}

static void test_100k_review_log(void) {
    sqlite3 *db = NULL;
    check(ct_db_open(&db, ":memory:") == SQLITE_OK, "open perf db");
    ct_db_migrate(db, "db/migrations");
    enum { cards = 200, per_card = 500 };
    ct_card c;
    for (int i = 0; i < cards; i++) {
        memset(&c, 0, sizeof c);
        snprintf(c.deck, sizeof c.deck, "%s", "perf");
        snprintf(c.case_key, sizeof c.case_key, "case_%03d", i);
        c.direction = 0;
        check(ct_repo_card_insert(db, &c) == CT_REPO_OK, "perf card insert");
    }
    clock_t t0 = clock();
    for (int i = 0; i < cards; i++) {
        for (int k = 0; k < per_card; k++) {
            int rating = 1 + (i + k) % 4;
            char at[32];
            snprintf(at, sizeof at, "2026-01-%02dT00:00:00Z", 1 + (i + k) % 28);
            check(ct_repo_review_log(db, i + 1, rating, at, 5.0, 5.0, (double)k, 3.0) ==
                      CT_REPO_OK,
                  "review log row");
        }
    }
    clock_t t1 = clock();
    printf("insert 100k review rows %.2f s\n", (double)(t1 - t0) / CLOCKS_PER_SEC);

    ct_srs_row row;
    memset(&row, 0, sizeof row);
    row.stability = 10;
    row.difficulty = 5;
    snprintf(row.due, sizeof row.due, "%s", "2026-01-01T00:00:00Z");
    row.reps = 1;
    row.state = 1;
    for (int i = 0; i < cards; i++) {
        if (i % 4 == 0) {
            snprintf(row.due, sizeof row.due, "%s", "2026-09-01T00:00:00Z");
        } else {
            snprintf(row.due, sizeof row.due, "%s", "2027-01-01T00:00:00Z");
        }
        check(ct_repo_srs_put(db, i + 1, &row) == CT_REPO_OK, "srs put perf");
    }
    long due[256];
    t0 = clock();
    int n = ct_repo_due_cards(db, "perf", "2026-10-10T00:00:00Z", due, 256);
    t1 = clock();
    printf("due_cards 200 cards / 100k log rows n=%d in %.4f s\n", n,
           (double)(t1 - t0) / CLOCKS_PER_SEC);
    int want_due = 0;
    for (int i = 0; i < cards; i++) {
        if (i % 4 == 0) {
            want_due++;
        }
    }
    check(n == want_due, "due count matches planted due set");
    for (int i = 0; i < n; i++) {
        check((due[i] - 1) % 4 == 0, "due id is from the due subset");
    }

    enum { acc_cap = 512 };
    static ct_case_accuracy_row acc[acc_cap];
    t0 = clock();
    int m = ct_repo_case_accuracy(db, "perf", acc, acc_cap);
    t1 = clock();
    printf("case_accuracy n=%d in %.4f s\n", m, (double)(t1 - t0) / CLOCKS_PER_SEC);
    check(m == cards, "case accuracy row count");
    long total_reviews = 0;
    long total_correct = 0;
    for (int i = 0; i < m; i++) {
        total_reviews += acc[i].reviews;
        total_correct += acc[i].correct;
    }
    check(total_reviews == (long)cards * per_card, "case accuracy aggregates all 100k rows");
    long want_correct = 0;
    for (int i = 0; i < cards; i++) {
        for (int k = 0; k < per_card; k++) {
            if (1 + (i + k) % 4 > CT_AGAIN) {
                want_correct++;
            }
        }
    }
    check(total_correct == want_correct, "case accuracy correct count matches oracle");
    for (int i = 0; i < m; i++) {
        if (strcmp(acc[i].case_key, "case_000") == 0) {
            check(acc[i].reviews == per_card, "per-case review count");
        }
    }
    ct_db_close(db);
}

int main(void) {
    test_migration_runner();
    test_like_glob_keys();
    test_100k_review_log();
    printf("%s failures=%d\n", failures == 0 ? "ALL-OK" : "HAD-FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
