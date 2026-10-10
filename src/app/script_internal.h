#ifndef CT_APP_SCRIPT_INTERNAL_H
#define CT_APP_SCRIPT_INTERNAL_H

#include "app/script.h"

enum {
    CT_SCRIPT_ARGC_0 = 0,
    CT_SCRIPT_ARGC_1 = 1,
    CT_SCRIPT_ARGC_2 = 2,
    CT_SCRIPT_ARGC_3 = 3,
    CT_SCRIPT_ARGC_4 = 4,
    CT_SCRIPT_ARGC_5 = 5,
    CT_SCRIPT_NE = 3
};

int ct_script_fail(char *err, unsigned cap, const ct_script_row *c, const char *what, int code);
int ct_script_long(const char *s, long *out);
int ct_script_double(const char *s, double *out);
int ct_script_op(const char *s);
int ct_script_long_ok(int op, long a, long b);
int ct_script_double_ok(int op, double a, double b);
int ct_script_iso_day(const char *iso, long *day);
int ct_script_now(char *now, unsigned cap, long *day);
int ct_script_expect(ct_app *app, ct_script_rt *rt, const ct_script *s, const ct_script_row *c,
                     char *err, unsigned cap);

#endif
