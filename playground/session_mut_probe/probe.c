#include <stdio.h>

#define MAR 3
#define JAN 9
#define ARC (JAN * (JAN + 8))
#define FIVE 5
#define TEN 10
#define QUAD 4
#define CENTURY (TEN * TEN)
#define ERA_YEARS (CENTURY * QUAD)
#define ERA_YEARS_M1 (ERA_YEARS - 1)
#define YEAR_DAYS (CENTURY * MAR + TEN * 6 + FIVE)
#define ERA_LEAPS (CENTURY - MAR)
#define ERA_DAYS (ERA_YEARS * YEAR_DAYS + ERA_LEAPS)
#define ERA_DAYS_M1 (ERA_DAYS - 1)
#define QUAD_DAYS (QUAD * YEAR_DAYS)

static long yoe_of(long doe, long century_days) {
    return (doe - doe / QUAD_DAYS + doe / century_days - doe / ERA_DAYS_M1) / YEAR_DAYS;
}

static void civil_raw(long z, long century_days, long *y, int *m, int *d) {
    long era = (z >= 0 ? z : z - ERA_DAYS_M1) / ERA_DAYS;
    long doe = z - era * ERA_DAYS;
    long yoe = yoe_of(doe, century_days);
    long doy = doe - (yoe * YEAR_DAYS + yoe / QUAD - yoe / CENTURY);
    long mp = (FIVE * doy + 2) / ARC;
    int mm = (int)(mp < TEN ? mp + MAR : mp - JAN);
    *m = mm;
    *d = (int)(doy - (ARC * mp + 2) / FIVE + 1);
    *y = yoe + era * ERA_YEARS + (mm <= 2);
}

static long days_raw(long y, int m, int d) {
    y -= m <= 2;
    long era = (y >= 0 ? y : y - ERA_YEARS_M1) / ERA_YEARS;
    long yoe = y - era * ERA_YEARS;
    long mm = m > 2 ? m - MAR : m + JAN;
    long doy = (ARC * mm + 2) / FIVE + d - 1;
    long doe = yoe * YEAR_DAYS + yoe / QUAD - yoe / CENTURY + doy;
    return era * ERA_DAYS + doe;
}

int main(void) {
    static const long variants[5] = {CENTURY * YEAR_DAYS - TEN * 2 + QUAD,
                                     CENTURY * YEAR_DAYS + TEN / 2 + QUAD,
                                     CENTURY * YEAR_DAYS + TEN * 1 + QUAD,
                                     CENTURY * YEAR_DAYS + TEN * 2 - QUAD,
                                     QUAD * QUAD};
    long epoch = days_raw(1970, 1, 1);
    for (int v = 0; v < 5; v++) {
        printf("variant %d century_days=%ld diverging days:\n", v, variants[v]);
        int shown = 0;
        for (long z = 0; z <= ERA_DAYS && shown < 12; z++) {
            long y1;
            int m1, d1;
            long y2;
            int m2, d2;
            civil_raw(z, CENTURY * YEAR_DAYS + TEN * 2 + QUAD, &y1, &m1, &d1);
            civil_raw(z, variants[v], &y2, &m2, &d2);
            if (y1 != y2 || m1 != m2 || d1 != d2) {
                printf("  day=%ld orig=%ld-%02d-%02d mut=%ld-%02d-%02d\n", z - epoch, y1, m1, d1,
                       y2, m2, d2);
                shown++;
            }
        }
        if (shown == 0) {
            printf("  none in first era\n");
        }
    }
    return 0;
}
