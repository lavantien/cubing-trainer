#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "srs/fsrs.h"
#include "trainer/session.h"

static void roundtrip(long y, int m, int d) {
    long day = ct_session_date_to_day(y, m, d);
    long ry = 0;
    int rm = 0;
    int rd = 0;
    ct_session_day_to_date(day, &ry, &rm, &rd);
    printf("date %04ld-%02d-%02d day=%ld back=%04ld-%02d-%02d %s\n", y, m, d, day, ry, rm, rd,
           (ry == y && rm == m && rd == d) ? "ok" : "MISMATCH");
}

static void add_days_case(const char *iso, long days) {
    char out[40];
    int rc = ct_session_iso_add_days(iso, days, out, sizeof out);
    printf("add %s %+ld rc=%d out=[%s]\n", iso, days, rc, rc == 0 ? out : "");
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    roundtrip(1970, 1, 1);
    roundtrip(1900, 2, 28);
    roundtrip(1900, 3, 1);
    roundtrip(2000, 2, 29);
    roundtrip(2100, 2, 28);
    roundtrip(2100, 3, 1);
    roundtrip(2026, 10, 10);
    roundtrip(1, 1, 1);
    roundtrip(9999, 12, 31);
    roundtrip(4, 2, 29);
    add_days_case("2026-10-10", 0);
    add_days_case("2026-10-10", 1);
    add_days_case("2026-02-28", 1);
    add_days_case("2028-02-28", 1);
    add_days_case("9999-12-31", 1);
    if (argc > 1 && strcmp(argv[1], "overflow") == 0) {
        add_days_case("9999-12-31", LONG_MAX);
        add_days_case("0001-01-01", LONG_MIN);
    }
    add_days_case("2026-10-10", LONG_MIN / 2);
    add_days_case("2026-10-10T12:34:56Z", 3);
    add_days_case("0000-01-01", 5);
    add_days_case("2026-13-01", 5);
    add_days_case("2026-02-30", 5);
    add_days_case("2026-10-10", 5);

    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    double s = 10.0;
    for (int g = CT_AGAIN; g <= CT_EASY; g++) {
        ct_fsrs_card c = {0};
        c.stability = s;
        c.difficulty = 5.0;
        c.state = 1;
        c.last_review_day = 20000;
        ct_fsrs_card out = ct_fsrs_review(&f, &c, (ct_grade)g, 100.0, 20636);
        printf("grade=%d stab=%.10f diff=%.10f due_off=%ld\n", g, out.stability, out.difficulty,
               out.due_day - 20636);
    }
    ct_fsrs_card first = {0};
    ct_fsrs_card outf = ct_fsrs_review(&f, &first, CT_GOOD, 0.0, 20636);
    printf("first good stab=%.10f diff=%.10f due_off=%ld\n", outf.stability, outf.difficulty,
           outf.due_day - 20636);
    printf("retr(10, 0)=%.10f retr(10, 100)=%.10f ivl(10)=%ld ivl(0.1)=%ld ivl(36500)=%ld\n",
           ct_fsrs_retrievability(10.0, 0.0), ct_fsrs_retrievability(10.0, 100.0),
           ct_fsrs_next_interval_days(&f, 10.0), ct_fsrs_next_interval_days(&f, 0.1),
           ct_fsrs_next_interval_days(&f, 36500.0));
    return 0;
}
