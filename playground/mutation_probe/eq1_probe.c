#include <math.h>
#include <stdio.h>

int main(void) {
    double f = 19.0 / 81.0;
    double fac = pow(0.5, -2.0) - 1.0;
    printf("fac=%.17g\n", fac);
    for (int k = -4; k <= 4; k++) {
        double s = f * (double)(1 << (k < 0 ? -k : k));
        if (k < 0) {
            s = f / (double)(1 << -k);
        }
        double raw = (s / f) * fac;
        printf("k=%d raw=%.17g eq1=%d\n", k, raw, raw == 1.0);
    }
    double t = nextafter(1.0 / 3.0, 2.0);
    printf("3t=%.17g eq=%d\n", 3.0 * t, 3.0 * t == 1.0);
    double base = t * f;
    for (int u = -3; u <= 3; u++) {
        double s = base;
        for (int j = 0; j < (u < 0 ? -u : u); j++) {
            s = nextafter(s, u < 0 ? -1.0 : 2.0);
        }
        double raw = (s / f) * fac;
        printf("u=%d raw=%.17g eq1=%d\n", u, raw, raw == 1.0);
    }
    for (int u = -3; u <= 3; u++) {
        double r = 0.6;
        for (int j = 0; j < (u < 0 ? -u : u); j++) {
            r = nextafter(r, u < 0 ? -1.0 : 2.0);
        }
        double raw = (f / f) * (pow(r, -2.0) - 1.0);
        printf("r=%d raw=%.17g eq1=%d\n", u, raw, raw == 1.0);
    }
    return 0;
}
