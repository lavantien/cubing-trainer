#include "framework.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CT_SFC64_WARMUP 12
#define CT_SPLITMUX_GAMMA 0x9E3779B97F4A7C15ULL
#define CT_SPLITMUL1 0xBF58476D1CE4E5B9ULL
#define CT_SPLITMUL2 0x94D049BB133111EBULL

typedef struct {
    const char *name;
    void (*fn)(void);
} ct_suite;

static ct_suite ct_suites[CT_MAX_SUITES];
static size_t ct_suite_count;
static jmp_buf ct_jmp;
static bool ct_in_suite;
static const char *ct_active;
static uint64_t ct_pass;
static uint64_t ct_fail;

void ct_suite_register(const char *name, void (*fn)(void)) {
    if (ct_suite_count == CT_MAX_SUITES) {
        fprintf(stderr, "ct_suite_register: capacity %d exceeded, dropping %s\n", CT_MAX_SUITES,
                name);
        return;
    }
    ct_suites[ct_suite_count].name = name;
    ct_suites[ct_suite_count].fn = fn;
    ct_suite_count++;
}

void ct_count_pass(void) {
    ct_pass++;
}

[[noreturn]] static void ct_abort(void) {
    if (ct_in_suite) {
        longjmp(ct_jmp, 1);
    }
    exit(1);
}

void ct_fail_plain(const char *file, int line, const char *expr) {
    ct_fail++;
    fprintf(stderr, "%s: %s:%d: assert failed: %s\n", ct_active, file, line, expr);
    ct_abort();
}

void ct_fail_eq(const char *file, int line, const char *ea, const char *eb, unsigned long long a,
                unsigned long long b) {
    ct_fail++;
    fprintf(stderr, "%s: %s:%d: assert failed: %s == %s: %llu vs %llu\n", ct_active, file, line, ea,
            eb, a, b);
    ct_abort();
}

void ct_fail_dbl(const char *file, int line, const char *ea, const char *eb, double a, double b,
                 double eps) {
    ct_fail++;
    fprintf(stderr, "%s: %s:%d: assert failed: %s ~= %s (eps %.17g): %.17g vs %.17g\n", ct_active,
            file, line, ea, eb, eps, a, b);
    ct_abort();
}

static const char *ct_str_or_null(const char *s) {
    return s ? s : "(null)";
}

void ct_fail_str(const char *file, int line, const char *ea, const char *eb, const char *a,
                 const char *b) {
    ct_fail++;
    fprintf(stderr, "%s: %s:%d: assert failed: %s == %s: \"%s\" vs \"%s\"\n", ct_active, file, line,
            ea, eb, ct_str_or_null(a), ct_str_or_null(b));
    ct_abort();
}

bool ct_dbl_within(double a, double b, double eps) {
    double d = a - b;
    if (d < 0) {
        d = -d;
    }
    return d <= eps;
}

bool ct_str_eq(const char *a, const char *b) {
    return a != NULL && b != NULL && strcmp(a, b) == 0;
}

static uint64_t ct_clock_ms(clock_t t0, clock_t t1) {
    return (uint64_t)((double)(t1 - t0) * 1000.0 / (double)CLOCKS_PER_SEC);
}

size_t hd_suite_count(void) {
    return ct_suite_count;
}

const char *hd_suite_name(size_t i) {
    return i < ct_suite_count ? ct_suites[i].name : NULL;
}

void hd_heap_audit(void);
extern const char *hd_cur_suite;

uint64_t hd_run_one(size_t i) {
    clock_t t0 = clock();
    ct_pass = 0;
    ct_fail = 0;
    ct_active = ct_suites[i].name;
    hd_cur_suite = ct_suites[i].name;
    ct_in_suite = true;
    if (setjmp(ct_jmp) == 0) {
        ct_suites[i].fn();
    }
    ct_in_suite = false;
    hd_cur_suite = "(between-suites)";
    hd_heap_audit();
    uint64_t ms = ct_clock_ms(t0, clock());
    printf("%s %s: pass=%llu fail=%llu %llums\n", ct_fail ? "FAIL" : "PASS", ct_suites[i].name,
           (unsigned long long)ct_pass, (unsigned long long)ct_fail, (unsigned long long)ms);
    fflush(stdout);
    fflush(stderr);
    return ct_fail;
}

static long hd_find(const char *name) {
    for (size_t i = 0; i < ct_suite_count; i++) {
        if (strcmp(name, ct_suites[i].name) == 0) {
            return (long)i;
        }
    }
    return -1;
}

int hd_run_named(const char *name) {
    long i = hd_find(name);
    if (i < 0) {
        fprintf(stderr, "hd: no suite named %s\n", name);
        return 1;
    }
    hd_run_one((size_t)i);
    return 0;
}

int ct_run_all(const char *filter) {
    uint64_t total_pass = 0;
    uint64_t total_fail = 0;
    uint64_t nrun = 0;
    for (size_t i = 0; i < ct_suite_count; i++) {
        if (filter != NULL && filter[0] != '\0' && strcmp(filter, ct_suites[i].name) != 0) {
            continue;
        }
        hd_run_one(i);
        total_pass += ct_pass;
        total_fail += ct_fail;
        nrun++;
    }
    printf("total: pass=%llu fail=%llu\n", (unsigned long long)total_pass,
           (unsigned long long)total_fail);
    if (filter != NULL && filter[0] != '\0' && nrun == 0) {
        fprintf(stderr, "ct: no suite matches filter %s\n", filter);
        return 1;
    }
    return total_fail == 0 ? 0 : 1;
}

static uint64_t ct_splitmix64(uint64_t *x) {
    uint64_t z = (*x += CT_SPLITMUX_GAMMA);
    z = (z ^ (z >> 30)) * CT_SPLITMUL1;
    z = (z ^ (z >> 27)) * CT_SPLITMUL2;
    return z ^ (z >> 31);
}

void ct_sfc64_init(ct_sfc64 *s, uint64_t seed) {
    uint64_t sm = seed;
    s->a = ct_splitmix64(&sm);
    s->b = ct_splitmix64(&sm);
    s->c = ct_splitmix64(&sm);
    s->d = ct_splitmix64(&sm);
    for (int i = 0; i < CT_SFC64_WARMUP; i++) {
        (void)ct_sfc64_next(s);
    }
}

uint64_t ct_sfc64_next(ct_sfc64 *s) {
    uint64_t tmp = s->a + s->b + s->d;
    s->d++;
    s->a = s->b ^ (s->b >> 11);
    s->b = s->c + (s->c << 3);
    s->c = ((s->c << 24) | (s->c >> 40)) + tmp;
    return tmp;
}

double ct_sfc64_uniform(ct_sfc64 *s) {
    return (double)(ct_sfc64_next(s) >> 11) * 0x1.0p-53;
}

uint64_t ct_sfc64_range(ct_sfc64 *s, uint64_t n) {
    if (n < 2) {
        return 0;
    }
    uint64_t rem = (uint64_t)(0 - n) % n;
    uint64_t r;
    do {
        r = ct_sfc64_next(s);
    } while (r > UINT64_MAX - rem);
    return r % n;
}
