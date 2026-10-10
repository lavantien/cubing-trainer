#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "packs/packs.h"
#include "storage/db.h"

static const char *tmpdir;

static void wfile(const char *name, const char *body) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", tmpdir, name);
    FILE *f = fopen(path, "wb");
    fwrite(body, 1, strlen(body), f);
    fclose(f);
}

static int count_case(sqlite3 *db, const char *key) {
    sqlite3_stmt *st = NULL;
    sqlite3_prepare_v2(db, "SELECT count(*) FROM event_pack WHERE case_key = ?", -1, &st, NULL);
    sqlite3_bind_text(st, 1, key, -1, SQLITE_STATIC);
    int n = sqlite3_step(st) == SQLITE_ROW ? sqlite3_column_int(st, 0) : -1;
    sqlite3_finalize(st);
    return n;
}

int main(int argc, char **argv) {
    tmpdir = argc > 1 ? argv[1] : ".";
    wfile("good.tsv",
          "case_key\talg\tfront_json\tsource_url\tsource_at\n"
          "a.one\tR U R'\t{}\thttps://x\t2026-10-10\n"
          "a.two\tR U'\t{}\thttps://x\t2026-10-10\n");
    wfile("midheader.tsv",
          "case_key\talg\tfront_json\tsource_url\tsource_at\n"
          "a.one\tR U R'\t{}\thttps://x\t2026-10-10\n"
          "case_key\talg\tfront_json\tsource_url\tsource_at\n"
          "a.two\tR U'\t{}\thttps://x\t2026-10-10\n");
    wfile("emptyfield.tsv",
          "case_key\talg\tfront_json\tsource_url\tsource_at\n"
          "a.one\tR U R'\t{}\thttps://x\t2026-10-10\n"
          "a.two\t\t{}\thttps://x\t2026-10-10\n");
    wfile("shortrow.tsv",
          "case_key\talg\tfront_json\tsource_url\tsource_at\n"
          "a.one\tR U R'\t{}\thttps://x\t2026-10-10\n"
          "a.two\tR U'\t{}\n");
    wfile("dupkey.tsv",
          "case_key\talg\tfront_json\tsource_url\tsource_at\n"
          "a.one\tR U R'\t{}\thttps://x\t2026-10-10\n"
          "a.one\tR U'\t{}\thttps://x\t2026-10-10\n");
    wfile("noheader.tsv", "a.one\tR U R'\t{}\thttps://x\t2026-10-10\n");

    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK || ct_db_migrate(db, "db/migrations") != 0) {
        printf("open/migrate failed\n");
        return 1;
    }
    char path[512];
    snprintf(path, sizeof path, "%s/good.tsv", tmpdir);
    int r1 = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL);
    int c1 = ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL);
    int r2 = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL);
    int c2 = ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL);
    printf("good seed1=%d count1=%d seed2=%d count2=%d idempotent=%d\n", r1, c1, r2, c2,
           c1 == c2 && r1 == r2);

    sqlite3_exec(db, "DELETE FROM event_pack WHERE case_key = 'a.two'", NULL, NULL, NULL);
    int cd = ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL);
    int r3 = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL);
    int c3 = ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL);
    printf("after delete=%d reseed=%d count=%d restored=%d\n", cd, r3, c3, c3 == c1);

    snprintf(path, sizeof path, "%s/midheader.tsv", tmpdir);
    int rm = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_OLL);
    int garbage = count_case(db, "case_key");
    printf("midheader seed=%d garbage_rows=%d\n", rm, garbage);

    snprintf(path, sizeof path, "%s/emptyfield.tsv", tmpdir);
    int re = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L);
    int ce = ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L);
    printf("emptyfield seed=%d count=%d rolled_back=%d\n", re, ce, ce == 0);

    snprintf(path, sizeof path, "%s/shortrow.tsv", tmpdir);
    int rs = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_CROSS);
    int cs = ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_CROSS);
    printf("shortrow seed=%d count=%d rolled_back=%d\n", rs, cs, cs == 0);

    snprintf(path, sizeof path, "%s/dupkey.tsv", tmpdir);
    int rd = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_CLL);
    ct_pack_entry got;
    ct_pack_get(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_CLL, "a.one", &got);
    printf("dupkey seed=%d winner_alg=%s\n", rd, got.alg);

    snprintf(path, sizeof path, "%s/noheader.tsv", tmpdir);
    int rn = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_CO);
    int cn = ct_pack_count(db, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_CO);
    printf("noheader seed=%d count=%d accepted_without_header=%d\n", rn, cn, cn == 1);

    snprintf(path, sizeof path, "%s/missing.tsv", tmpdir);
    int rmiss = ct_pack_seed_file(db, path, CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_EO);
    printf("missing seed=%d\n", rmiss);

    ct_db_close(db);
    return 0;
}
