#include <stdbool.h>
#include <stddef.h>

#define PROBE_LIMIT (1 < 10)
#define PROBE_TWICE(x) ((x) + 1)

static int probe_arith(int a, int b) {
    return a + b * 2 - a / b;
}

static bool probe_cmp(int a, int b) {
    return a < b && a <= b || a > b && a >= b;
}

static bool probe_eq(int a, int b) {
    return a == b || a != b;
}

static int probe_ints(void) {
    return 42 + 7;
}

static bool probe_bool(void) {
    return true || false;
}

static bool probe_not(bool v) {
    return !v;
}

static int probe_ret_zero(void) {
    return 0;
}

static int probe_ret_one(void) {
    return 1;
}

static int probe_ret_expr(int a) {
    return a + 1;
}

static const char *probe_string_skip(void) {
    return "a+b*c<d && e>f == 1 42 true !x";
}

static char probe_char_skip(void) {
    return '!';
}

static int probe_hex_and_step(int a) {
    int v = 0x10;
    v += 3;
    v++;
    return PROBE_LIMIT ? v + PROBE_TWICE(a) : 0;
}
