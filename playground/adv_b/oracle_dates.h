#ifndef ADV_B_ORACLE_DATES_H
#define ADV_B_ORACLE_DATES_H

static long adv_leap(long y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static long adv_days_before_year(long y) {
    long t = y - 1;
    return 365 * t + t / 4 - t / 100 + t / 400;
}

static long adv_day_of_year(long y, int m, int d) {
    static const long cum[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    long doy = cum[m - 1] + d;
    if (m > 2 && adv_leap(y)) {
        doy += 1;
    }
    return doy;
}

static long adv_epoch_day(long y, int m, int d) {
    return adv_days_before_year(y) + adv_day_of_year(y, m, d) - adv_days_before_year(1970) - 1;
}

static int adv_last_day(long y, int m) {
    static const int md[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && adv_leap(y)) {
        return 29;
    }
    return md[m - 1];
}

#endif
