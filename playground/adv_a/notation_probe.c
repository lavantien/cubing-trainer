#include <stdio.h>
#include <string.h>

#include "engine/notation.h"

static int failures = 0;
static int checks = 0;

struct token_case {
    const char *token;
    int n;
    int expect_rc;
    int face;
    int from;
    int to;
    int quarter;
    const char *canonical;
};

static const struct token_case cases[] = {
    {"R", 3, 0, CT_FACE_R, 1, 1, 1, "R"},
    {"R2", 3, 0, CT_FACE_R, 1, 1, 2, "R2"},
    {"R'", 3, 0, CT_FACE_R, 1, 1, 3, "R'"},
    {"R'2", 3, 0, CT_FACE_R, 1, 1, 2, "R2"},
    {"R2'", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"R''", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"R'2'", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"R'3", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"0R", 3, CT_NOTATION_E_DIGIT, 0, 0, 0, 0, NULL},
    {"1R", 3, CT_NOTATION_E_DIGIT, 0, 0, 0, 0, NULL},
    {"8R", 3, CT_NOTATION_E_DIGIT, 0, 0, 0, 0, NULL},
    {"9Rw", 3, CT_NOTATION_E_DIGIT, 0, 0, 0, 0, NULL},
    {"12R", 3, CT_NOTATION_E_DIGIT, 0, 0, 0, 0, NULL},
    {"12R", 7, CT_NOTATION_E_DIGIT, 0, 0, 0, 0, NULL},
    {"77R", 7, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"4R", 3, CT_NOTATION_E_DEPTH, 0, 0, 0, 0, NULL},
    {"7R", 7, 0, CT_FACE_R, 7, 7, 1, "7R"},
    {"2R", 2, 0, CT_FACE_R, 2, 2, 1, "2R"},
    {"2R'", 2, 0, CT_FACE_R, 2, 2, 3, "2R'"},
    {"3R", 2, CT_NOTATION_E_DEPTH, 0, 0, 0, 0, NULL},
    {"3Rw", 2, CT_NOTATION_E_DEPTH, 0, 0, 0, 0, NULL},
    {"3Rw'", 7, 0, CT_FACE_R, 1, 3, 3, "3Rw'"},
    {"Rw", 2, 0, CT_FACE_R, 1, 2, 1, "x"},
    {"Rw", 3, 0, CT_FACE_R, 1, 2, 1, "Rw"},
    {"3Rw", 3, 0, CT_FACE_R, 1, 3, 1, "x"},
    {"4Rw", 7, 0, CT_FACE_R, 1, 4, 1, "4Rw"},
    {"7Rw", 7, 0, CT_FACE_R, 1, 7, 1, "x"},
    {"8Rw", 7, CT_NOTATION_E_DIGIT, 0, 0, 0, 0, NULL},
    {"2R2", 7, 0, CT_FACE_R, 2, 2, 2, "2R2"},
    {"2Rw2", 7, 0, CT_FACE_R, 1, 2, 2, "Rw2"},
    {"r", 5, 0, CT_FACE_R, 1, 2, 1, "Rw"},
    {"3r", 5, 0, CT_FACE_R, 1, 3, 1, "3Rw"},
    {"rw", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"Mw", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"xw", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"2x", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"2M", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"m", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"e", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"s", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"X", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"Y", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"Z", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"2", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"w", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"'", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"''", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"\xC3\xA9", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"\xD0\xB0R", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"\xEF\xBC\xB2", 3, CT_NOTATION_E_TOKEN, 0, 0, 0, 0, NULL},
    {"R\xC2\xA0", 3, CT_NOTATION_E_SUFFIX, 0, 0, 0, 0, NULL},
    {"Rw'", 7, 0, CT_FACE_R, 1, 2, 3, "Rw'"},
    {"Rw2", 7, 0, CT_FACE_R, 1, 2, 2, "Rw2"},
    {"M", 3, 0, CT_FACE_L, 2, 2, 1, "M"},
    {"M'", 3, 0, CT_FACE_L, 2, 2, 3, "M'"},
    {"E2", 3, 0, CT_FACE_D, 2, 2, 2, "E2"},
    {"S", 3, 0, CT_FACE_F, 2, 2, 1, "S"},
    {"2L", 3, 0, CT_FACE_L, 2, 2, 1, "M"},
    {"x", 3, 0, CT_FACE_R, 1, 3, 1, "x"},
    {"x'", 3, 0, CT_FACE_R, 1, 3, 3, "x'"},
    {"y2", 3, 0, CT_FACE_U, 1, 3, 2, "y2"},
    {"z", 3, 0, CT_FACE_F, 1, 3, 1, "z"},
    {"R", 1, 0, CT_FACE_R, 1, 1, 1, "x"},
    {"R", 0, CT_NOTATION_E_INPUT, 0, 0, 0, 0, NULL},
    {"R", 8, CT_NOTATION_E_INPUT, 0, 0, 0, 0, NULL},
    {"R", -3, CT_NOTATION_E_INPUT, 0, 0, 0, 0, NULL},
};

static void run_cases(void) {
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        const struct token_case *c = &cases[k];
        ct_move m = {-99, -99, -99, -99};
        int rc = ct_notation_parse_one(c->token, c->n, &m);
        checks++;
        if (rc != c->expect_rc) {
            printf("FAIL token '%s' n=%d rc=%d want %d\n", c->token, c->n, rc, c->expect_rc);
            failures++;
            continue;
        }
        if (rc != 0)
            continue;
        if (m.face != c->face || m.depth_from != c->from || m.depth_to != c->to ||
            m.quarter != c->quarter) {
            printf("FAIL token '%s' n=%d move %d,%d,%d,%d want %d,%d,%d,%d\n", c->token, c->n, m.face,
                   m.depth_from, m.depth_to, m.quarter, c->face, c->from, c->to, c->quarter);
            failures++;
            continue;
        }
        if (c->canonical != NULL) {
            char buf[CT_NOTATION_TOKEN_MAX];
            int len = ct_notation_format_one(&m, c->n, buf, sizeof buf);
            checks++;
            if (len < 0 || strcmp(buf, c->canonical) != 0) {
                printf("FAIL canonical '%s' n=%d got '%s' (len %d) want '%s'\n", c->token, c->n,
                       len >= 0 ? buf : "<err>", len, c->canonical);
                failures++;
            }
        }
    }
}

static void check_rc(const char *label, int got, int want) {
    checks++;
    if (got != want) {
        printf("FAIL %s rc=%d want %d\n", label, got, want);
        failures++;
    }
}

int main(void) {
    run_cases();

    printf("== alg level ==\n");
    ct_move moves[8];
    check_rc("tab separator", ct_notation_parse("R\tU", 3, moves, 8), CT_NOTATION_E_SUFFIX);
    check_rc("newline separator", ct_notation_parse("R\nU", 3, moves, 8), CT_NOTATION_E_SUFFIX);
    check_rc("cr separator", ct_notation_parse("R\rU", 3, moves, 8), CT_NOTATION_E_SUFFIX);
    check_rc("double space", ct_notation_parse("R  U", 3, moves, 8), 2);
    check_rc("leading spaces", ct_notation_parse("   R", 3, moves, 8), 1);
    check_rc("trailing spaces", ct_notation_parse("R   ", 3, moves, 8), 1);
    check_rc("empty", ct_notation_parse("", 3, moves, 8), 0);
    check_rc("spaces only", ct_notation_parse("     ", 3, moves, 8), 0);
    check_rc("cap 0", ct_notation_parse("R", 3, moves, 0), CT_NOTATION_E_CAP);
    check_rc("cap exact", ct_notation_parse("R U", 3, moves, 2), 2);
    check_rc("cap over by one", ct_notation_parse("R U", 3, moves, 1), CT_NOTATION_E_CAP);
    check_rc("null alg", ct_notation_parse(NULL, 3, moves, 8), CT_NOTATION_E_INPUT);
    check_rc("null out cap>0", ct_notation_parse("R", 3, NULL, 8), CT_NOTATION_E_INPUT);
    check_rc("null out cap 0", ct_notation_parse("R", 3, NULL, 0), CT_NOTATION_E_CAP);
    check_rc("neg cap", ct_notation_parse("R", 3, moves, -1), CT_NOTATION_E_INPUT);

    printf("== long alg over cap ==\n");
    char big[4096];
    size_t pos = 0;
    const char *unit = "R U R' U' ";
    while (pos + strlen(unit) < sizeof big) {
        memcpy(big + pos, unit, strlen(unit));
        pos += strlen(unit);
    }
    big[pos] = '\0';
    int tokens = (int)(pos / strlen(unit)) * 4;
    ct_move out[2000];
    int rc = ct_notation_parse(big, 3, out, 2000);
    checks++;
    if (rc != tokens) {
        printf("FAIL long alg rc=%d want %d\n", rc, tokens);
        failures++;
    }
    check_rc("long alg cap 1635", ct_notation_parse(big, 3, out, tokens - 1), CT_NOTATION_E_CAP);

    char fbuf[8192];
    int flen = ct_notation_format(out, rc, 3, fbuf, sizeof fbuf);
    checks++;
    if (flen != (int)strlen(fbuf) || flen <= 0) {
        printf("FAIL format long alg flen=%d strlen=%d\n", flen, (int)strlen(fbuf));
        failures++;
    }
    ct_move back[2000];
    int r2 = ct_notation_parse(fbuf, 3, back, 2000);
    checks++;
    if (r2 != rc || memcmp(out, back, (size_t)rc * sizeof(ct_move)) != 0) {
        printf("FAIL long alg round trip r2=%d rc=%d\n", r2, rc);
        failures++;
    }

    printf("== format cap boundaries ==\n");
    ct_move pair[2] = {{CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 1, 3}};
    check_rc("format cap 4 for 'R U''", ct_notation_format(pair, 2, 3, fbuf, 4), CT_NOTATION_E_CAP);
    check_rc("format cap 5 exact", ct_notation_format(pair, 2, 3, fbuf, 5), 4);
    char one[2] = {0};
    check_rc("format_one cap 1 for R", ct_notation_format_one(&pair[0], 3, one, 1), CT_NOTATION_E_CAP);
    check_rc("format_one cap 2 for R", ct_notation_format_one(&pair[0], 3, one, 2), 1);
    char oneb[8] = {0};
    check_rc("format_one null move", ct_notation_format_one(NULL, 3, oneb, 8), CT_NOTATION_E_INPUT);
    check_rc("format_one null out", ct_notation_format_one(&pair[0], 3, NULL, 8), CT_NOTATION_E_INPUT);
    check_rc("format_one bad n", ct_notation_format_one(&pair[0], 9, oneb, 8), CT_NOTATION_E_INPUT);
    ct_move deep = {CT_FACE_R, 1, 8, 1};
    check_rc("format depth 8 on n=7", ct_notation_format_one(&deep, 7, oneb, 8), CT_NOTATION_E_MOVE);
    ct_move span = {CT_FACE_R, 2, 4, 1};
    check_rc("format inner span", ct_notation_format_one(&span, 7, oneb, 8), CT_NOTATION_E_MOVE);

    printf("== embedded NUL and high bytes via alg ==\n");
    char weird[16];
    memcpy(weird, "R \x01 U", 8);
    check_rc("control byte token", ct_notation_parse(weird, 3, moves, 8), CT_NOTATION_E_TOKEN);
    char nul_mid[16] = "R U";
    nul_mid[1] = '\0';
    check_rc("embedded NUL truncates", ct_notation_parse(nul_mid, 3, moves, 8), 1);
    char high[8];
    high[0] = (char)0xFF;
    high[1] = 'R';
    high[2] = '\0';
    check_rc("0xFF prefix byte", ct_notation_parse(high, 3, moves, 8), CT_NOTATION_E_TOKEN);

    printf("checks=%d failures=%d\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
