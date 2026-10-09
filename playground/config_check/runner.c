#include "framework.h"

ct_suite_rec ct_suite_table[CT_SUITE_MAX];
int ct_suite_count;
jmp_buf ct_fail_jmp;
int ct_active_failures;

void ct_register_suite(const char *name, ct_suite_fn fn) {
    if (ct_suite_count < CT_SUITE_MAX) {
        ct_suite_table[ct_suite_count].name = name;
        ct_suite_table[ct_suite_count].fn = fn;
        ct_suite_count++;
    }
}

void ct_report_failure(const char *file, int line, const char *expr) {
    fprintf(stderr, "fail %s:%d %s\n", file, line, expr);
    ct_active_failures++;
    longjmp(ct_fail_jmp, 1);
}

int main(void) {
    int failed_suites = 0;
    for (int i = 0; i < ct_suite_count; i++) {
        ct_active_failures = 0;
        if (setjmp(ct_fail_jmp) == 0) {
            ct_suite_table[i].fn();
        }
        printf("suite %-28s %s\n", ct_suite_table[i].name, ct_active_failures > 0 ? "FAIL" : "pass");
        if (ct_active_failures > 0) {
            failed_suites++;
        }
    }
    printf("%d suites, %d failed\n", ct_suite_count, failed_suites);
    return failed_suites == 0 ? 0 : 1;
}
