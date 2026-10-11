#ifndef CT_FRAMEWORK_H
#define CT_FRAMEWORK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CT_MAX_SUITES 512
#define CT_TEST_GUARD_PNG "build/test_shots/guard.png"

void ct_suite_register(const char *name, void (*fn)(void));
int ct_run_all(const char *filter);

void ct_count_pass(void);
[[noreturn]] void ct_fail_plain(const char *file, int line, const char *expr);
[[noreturn]] void ct_fail_eq(const char *file, int line, const char *ea, const char *eb,
                             unsigned long long a, unsigned long long b);
[[noreturn]] void ct_fail_dbl(const char *file, int line, const char *ea, const char *eb, double a,
                              double b, double eps);
[[noreturn]] void ct_fail_str(const char *file, int line, const char *ea, const char *eb,
                              const char *a, const char *b);
bool ct_dbl_within(double a, double b, double eps);
bool ct_str_eq(const char *a, const char *b);

typedef struct {
    uint64_t a, b, c, d;
} ct_sfc64;
void ct_sfc64_init(ct_sfc64 *s, uint64_t seed);
uint64_t ct_sfc64_next(ct_sfc64 *s);
double ct_sfc64_uniform(ct_sfc64 *s);
uint64_t ct_sfc64_range(ct_sfc64 *s, uint64_t n);

#define CT_CAT2(x, y) x##y
#define CT_CAT(x, y) CT_CAT2(x, y)

#define CT_SUITE(name)                                                                             \
    static void CT_CAT(ct_body_, name)(void);                                                      \
    __attribute__((constructor)) static void CT_CAT(ct_reg_, name)(void) {                         \
        ct_suite_register(#name, CT_CAT(ct_body_, name));                                          \
    }                                                                                              \
    static void CT_CAT(ct_body_, name)(void)

#define CT_ASSERT(cond)                                                                            \
    do {                                                                                           \
        if (cond) {                                                                                \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_plain(__FILE__, __LINE__, #cond);                                              \
        }                                                                                          \
    } while (0)

#define CT_ASSERT_EQ(a, b)                                                                         \
    do {                                                                                           \
        if ((a) == (b)) {                                                                          \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_eq(__FILE__, __LINE__, #a, #b, (unsigned long long)(a),                        \
                       (unsigned long long)(b));                                                   \
        }                                                                                          \
    } while (0)

#define CT_ASSERT_DBL_EQ(a, b, eps)                                                                \
    do {                                                                                           \
        double ct_va_ = (double)(a);                                                               \
        double ct_vb_ = (double)(b);                                                               \
        double ct_ve_ = (double)(eps);                                                             \
        if (ct_dbl_within(ct_va_, ct_vb_, ct_ve_)) {                                               \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_dbl(__FILE__, __LINE__, #a, #b, ct_va_, ct_vb_, ct_ve_);                       \
        }                                                                                          \
    } while (0)

#define CT_ASSERT_STR_EQ(a, b)                                                                     \
    do {                                                                                           \
        const char *ct_sa_ = (a);                                                                  \
        const char *ct_sb_ = (b);                                                                  \
        if (ct_str_eq(ct_sa_, ct_sb_)) {                                                           \
            ct_count_pass();                                                                       \
        } else {                                                                                   \
            ct_fail_str(__FILE__, __LINE__, #a, #b, ct_sa_, ct_sb_);                               \
        }                                                                                          \
    } while (0)

#endif
