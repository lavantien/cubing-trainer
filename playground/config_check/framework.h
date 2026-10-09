#ifndef CT_PLAYGROUND_FRAMEWORK_H
#define CT_PLAYGROUND_FRAMEWORK_H

#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

typedef void (*ct_suite_fn)(void);

typedef struct {
    const char *name;
    ct_suite_fn fn;
} ct_suite_rec;

#define CT_SUITE_MAX 256

extern ct_suite_rec ct_suite_table[CT_SUITE_MAX];
extern int ct_suite_count;
extern jmp_buf ct_fail_jmp;
extern int ct_active_failures;

void ct_register_suite(const char *name, ct_suite_fn fn);
void ct_report_failure(const char *file, int line, const char *expr);

#define CT_CAT_(a, b) a##b
#define CT_CAT(a, b) CT_CAT_(a, b)

#define CT_SUITE(name)                                                           \
    static void CT_CAT(ct_suite_body_, name)(void);                              \
    __attribute__((constructor)) static void CT_CAT(ct_suite_reg_, name)(void) { \
        ct_register_suite(#name, CT_CAT(ct_suite_body_, name));                  \
    }                                                                            \
    static void CT_CAT(ct_suite_body_, name)(void)

#define CT_CHECK(cond, text)                          \
    do {                                              \
        if (!(cond)) {                                \
            ct_report_failure(__FILE__, __LINE__, text); \
        }                                             \
    } while (0)

#define CT_ASSERT(cond) CT_CHECK(cond, #cond)
#define CT_ASSERT_EQ(a, b) CT_CHECK((a) == (b), #a " == " #b)
#define CT_ASSERT_DBL_EQ(a, b, eps) CT_CHECK(fabs((a) - (b)) <= (eps), #a " ~= " #b)
#define CT_ASSERT_STR_EQ(a, b) CT_CHECK(strcmp((a), (b)) == 0, #a " == " #b)

#endif
