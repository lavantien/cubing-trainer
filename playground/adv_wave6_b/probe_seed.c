#include "config/config.h"
#include "packs/packs.h"
#include "storage/db.h"
#include "storage/repo_cards.h"

#include <stdio.h>
#include <string.h>

static sqlite3 *open_db(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK)
        return NULL;
    if (ct_db_migrate(db, "db/migrations") != CT_REPO_OK) {
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

static long file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fclose(f);
    return n;
}

static long last_newline_at_or_before(const char *path, long from) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    long at = -1;
    long pos = 0;
    int c;
    while ((c = fgetc(f)) != EOF && pos < from) {
        if (c == '\n')
            at = pos;
        pos++;
    }
    fclose(f);
    return at;
}

static int copy_range(const char *src, const char *dst, long bytes) {
    FILE *in = fopen(src, "rb");
    if (in == NULL)
        return -1;
    FILE *out = fopen(dst, "wb");
    if (out == NULL) {
        fclose(in);
        return -1;
    }
    char buf[4096];
    long left = bytes;
    while (left > 0) {
        long want = left < (long)sizeof buf ? left : (long)sizeof buf;
        size_t got = fread(buf, 1, (size_t)want, in);
        if (got == 0)
            break;
        fwrite(buf, 1, got, out);
        left -= (long)got;
    }
    fclose(in);
    fclose(out);
    return 0;
}

static int append_text(const char *dst, const char *text) {
    FILE *out = fopen(dst, "ab");
    if (out == NULL)
        return -1;
    fwrite(text, 1, strlen(text), out);
    fclose(out);
    return 0;
}

static int copy_crlf(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    FILE *out = fopen(dst, "wb");
    if (in == NULL || out == NULL) {
        if (in)
            fclose(in);
        if (out)
            fclose(out);
        return -1;
    }
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c == '\n')
            fputc('\r', out);
        fputc(c, out);
    }
    fclose(in);
    fclose(out);
    return 0;
}

static int write_text(const char *dst, const char *text) {
    FILE *out = fopen(dst, "wb");
    if (out == NULL)
        return -1;
    fwrite(text, 1, strlen(text), out);
    fclose(out);
    return 0;
}

static void run_case(sqlite3 *db, const char *path, const char *label) {
    int rc = ct_pack_seed_file(db, path, CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY);
    int count = ct_pack_count(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY);
    printf("%s rc %d count %d\n", label, rc, count);
}

int main(void) {
    const char *src = "db/seeds/packs/bld_3style_specials.tsv";
    long size = file_size(src);
    long l1 = last_newline_at_or_before(src, size);
    long l2 = last_newline_at_or_before(src, l1);
    printf("source_bytes %ld last_lf %ld prev_lf %ld\n", size, l1, l2);

    sqlite3 *db = open_db();
    if (db == NULL) {
        printf("db open failed\n");
        return 1;
    }
    run_case(db, src, "golden");

    copy_range(src, "pgwb/seed_nolf.tsv", l1);
    run_case(db, "pgwb/seed_nolf.tsv", "all_rows_no_trailing_lf");

    copy_range(src, "pgwb/seed_drop_last.tsv", l2 + 1);
    run_case(db, "pgwb/seed_drop_last.tsv", "silent_tail_row_loss");

    copy_range(src, "pgwb/seed_midrow.tsv", l1 - 30);
    run_case(db, "pgwb/seed_midrow.tsv", "truncated_midrow");

    copy_range(src, "pgwb/seed_dup.tsv", size);
    append_text("pgwb/seed_dup.tsv",
                "flip.01\tR U R' U'\t{\"dup\":true}\thttps://example.test/dup\t2026-10-11\n");
    run_case(db, "pgwb/seed_dup.tsv", "duplicate_key_last_wins");

    ct_pack_entry got;
    if (ct_pack_get(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY, "flip.01", &got) ==
        CT_PACK_OK) {
        printf("flip01_alg_after_dup %s\n", got.alg);
    }

    write_text("pgwb/seed_reorder.tsv",
               "source_at\talg\tcase_key\tfront_json\tsource_url\n"
               "2026-10-11\tR U R'\tzz.01\t{\"reordered\":true}\thttps://e.test/1\n");
    run_case(db, "pgwb/seed_reorder.tsv", "reordered_columns");
    if (ct_pack_get(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY, "zz.01", &got) ==
        CT_PACK_OK) {
        printf("reordered_alg %s reordered_at %s\n", got.alg, got.source_at);
    }

    write_text("pgwb/seed_badcol.tsv",
               "case_key\twrong\tfront_json\tsource_url\tsource_at\n"
               "zz.02\tR U R'\t{\"x\":1}\thttps://e.test/2\t2026-10-11\n");
    run_case(db, "pgwb/seed_badcol.tsv", "unknown_column");

    copy_range(src, "pgwb/seed_six.tsv", size);
    append_text("pgwb/seed_six.tsv",
                "flip.99\tR U R'\t{\"x\":1}\thttps://e.test/1\t2026-10-11\textra\n");
    run_case(db, "pgwb/seed_six.tsv", "six_fields");

    copy_range(src, "pgwb/seed_four.tsv", size);
    append_text("pgwb/seed_four.tsv", "flip.99\tR U R'\t{\"x\":1}\thttps://e.test/1\n");
    run_case(db, "pgwb/seed_four.tsv", "four_fields");

    copy_range(src, "pgwb/seed_emptyalg.tsv", size);
    append_text("pgwb/seed_emptyalg.tsv", "flip.99\t\t{\"x\":1}\thttps://e.test/1\t2026-10-11\n");
    run_case(db, "pgwb/seed_emptyalg.tsv", "empty_alg_field");

    copy_crlf(src, "pgwb/seed_crlf.tsv");
    run_case(db, "pgwb/seed_crlf.tsv", "crlf");

    copy_range(src, "pgwb/seed_repeat_header.tsv", size);
    append_text("pgwb/seed_repeat_header.tsv",
                "case_key\talg\tfront_json\tsource_url\tsource_at\n");
    run_case(db, "pgwb/seed_repeat_header.tsv", "repeated_header_tail");

    char longline[900];
    memset(longline, 'R', sizeof longline);
    longline[sizeof longline - 1] = '\0';
    char joined[1024];
    snprintf(joined, sizeof joined, "flip.98\t%s\t{\"x\":1}\thttps://e.test/1\t2026-10-11\n",
             longline);
    write_text("pgwb/seed_long.tsv",
               "case_key\talg\tfront_json\tsource_url\tsource_at\n");
    append_text("pgwb/seed_long.tsv", joined);
    run_case(db, "pgwb/seed_long.tsv", "oversized_line");

    sqlite3_close(db);
    return 0;
}
