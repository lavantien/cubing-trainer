#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oracle_dates.h"
#include "trainer/session.h"

static int failures = 0;

static void check(int cond, const char *what) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    }
}

static unsigned long long adv_sfc_state = 0xADDB202610100001ULL;

static unsigned long long adv_sfc_next(void) {
    unsigned long long s = adv_sfc_state;
    s ^= s << 12;
    s ^= s >> 25;
    s ^= s << 27;
    adv_sfc_state = s + 0x9E3779B97F4A7C15ULL;
    return s;
}

int main(void) {
    check(ct_session_date_to_day(1970, 1, 1) == 0, "epoch anchor 1970-01-01 = 0");
    check(ct_session_date_to_day(1969, 12, 31) == -1, "anchor 1969-12-31 = -1");
    check(ct_session_date_to_day(1, 1, 1) == -719162, "anchor proleptic 0001-01-01 = -719162");

    long mismatches = 0;
    for (long y = 1; y <= 9999; y++) {
        for (int m = 1; m <= 12; m++) {
            int last = adv_last_day(y, m);
            for (int d = 1; d <= last; d++) {
                long got = ct_session_date_to_day(y, m, d);
                long want = adv_epoch_day(y, m, d);
                if (got != want) {
                    mismatches++;
                    if (mismatches <= 10) {
                        printf("FAIL oracle mismatch %04ld-%02d-%02d got %ld want %ld\n", y, m, d,
                               got, want);
                    }
                }
                long ry;
                int rm, rd;
                ct_session_day_to_date(got, &ry, &rm, &rd);
                if (ry != y || rm != m || rd != d) {
                    mismatches++;
                    if (mismatches <= 10) {
                        printf("FAIL roundtrip %04ld-%02d-%02d -> %04ld-%02d-%02d\n", y, m, d, ry,
                               rm, rd);
                    }
                }
            }
        }
    }
    printf("full civil sweep 0001-01-01..9999-12-31 mismatches=%ld\n", mismatches);
    check(mismatches == 0, "full civil sweep matches independent oracle");

    struct {
        const char *iso;
        long add;
    } big[10] = {
        {"2026-10-10", 36500}, {"2000-02-29", 36500}, {"1900-03-01", 36500},
        {"2100-02-28", 36500}, {"2024-02-29", 1461},  {"1900-02-28", 1},
        {"2100-02-28", 1},     {"2000-02-28", 1},     {"9999-12-31", 1},
        {"0001-01-01", -1},
    };
    for (size_t i = 0; i < sizeof big / sizeof big[0]; i++) {
        char out[32];
        int rc = ct_session_iso_add_days(big[i].iso, big[i].add, out, (unsigned)sizeof out);
        printf("add_days %s %+ld rc=%d out=%s\n", big[i].iso, big[i].add, rc, out);
    }

    long arith_bad = 0;
    long out_of_range = 0;
    for (int i = 0; i < 200000; i++) {
        long y = 1 + (long)(adv_sfc_next() % 9999ULL);
        int m = 1 + (int)(adv_sfc_next() % 12ULL);
        int d = 1 + (int)(adv_sfc_next() % (unsigned long long)adv_last_day(y, m));
        long days = (long)(adv_sfc_next() % 200001ULL) - 100000;
        long base = ct_session_date_to_day(y, m, d);
        long want = adv_epoch_day(y, m, d) + days;
        long ry;
        int rm, rd;
        ct_session_day_to_date(base + days, &ry, &rm, &rd);
        long back = ct_session_date_to_day(ry, rm, rd);
        if (back != want) {
            arith_bad++;
            if (arith_bad < 10) {
                printf("FAIL arith random add %04ld-%02d-%02d %+ld -> %04ld-%02d-%02d back=%ld "
                       "want=%ld\n",
                       y, m, d, days, ry, rm, rd, back, want);
            }
        }
        if (ry < 1 || ry > 9999) {
            out_of_range++;
        }
    }
    printf("random add-day sweep: arithmetic mismatches=%ld out-of-range-results=%ld\n", arith_bad,
           out_of_range);
    check(arith_bad == 0, "random add-day arithmetic matches oracle everywhere");

    char out[32];
    int rc = ct_session_iso_add_days("9999-12-31T23:59:59Z", 1, out, (unsigned)sizeof out);
    printf("9999 overflow with time suffix rc=%d out=[%s] len=%zu\n", rc, out, strlen(out));
    long vy;
    int vm, vd;
    ct_session s;
    memset(&s, 0, sizeof s);
    printf("overflow due stamp rejected by ct_session_begin=%d\n",
           ct_session_begin(&s, (sqlite3 *)1, (const ct_fsrs *)1, "d", out, 0, 0) !=
               CT_SESSION_OK);
    printf("note: iso_add_days returns OK for [%s], a 5-digit year that later scans reject\n",
           out);

    rc = ct_session_iso_add_days("0001-01-01", -1, out, (unsigned)sizeof out);
    printf("year 0 stamp rc=%d out=[%s]\n", rc, out);
    rc = ct_session_iso_add_days("0001-01-01", -366, out, (unsigned)sizeof out);
    printf("year -1 stamp rc=%d out=[%s]\n", rc, out);

    rc = ct_session_iso_add_days("2026-01-01XYZ9876", 5, out, (unsigned)sizeof out);
    printf("arbitrary suffix rc=%d out=[%s]\n", rc, out);
    rc = ct_session_iso_add_days("2026-01-01T00:00:00Z", 5, out, 21);
    printf("exact cap 21 rc=%d out=[%s]\n", rc, out);
    rc = ct_session_iso_add_days("2026-01-01T00:00:00Z", 5, out, 20);
    printf("cap 20 rc=%d\n", rc);

    long y0;
    int m0, d0;
    ct_session_day_to_date(2932896, &y0, &m0, &d0);
    printf("day 2932896 -> %04ld-%02d-%02d\n", y0, m0, d0);
    ct_session_day_to_date(-719162, &y0, &m0, &d0);
    printf("day -719162 -> %04ld-%02d-%02d\n", y0, m0, d0);

    printf("%s failures=%d\n", failures == 0 ? "ALL-OK" : "HAD-FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
