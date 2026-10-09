#ifndef CT_NOTATION_H
#define CT_NOTATION_H

#include "engine/cube.h"

enum { CT_NOTATION_DIGIT_MIN = 2, CT_NOTATION_DIGIT_MAX = 7, CT_NOTATION_TOKEN_MAX = 8 };

enum {
    CT_NOTATION_E_INPUT = -1,
    CT_NOTATION_E_TOKEN = -2,
    CT_NOTATION_E_SUFFIX = -3,
    CT_NOTATION_E_DIGIT = -4,
    CT_NOTATION_E_DEPTH = -5,
    CT_NOTATION_E_CAP = -6,
    CT_NOTATION_E_MOVE = -7
};

enum { CT_NOTATION_KIND_FACE, CT_NOTATION_KIND_ROTATION, CT_NOTATION_KIND_SLICE };

typedef struct {
    char letter;
    int face;
    int kind;
} ct_notation_entry;

int ct_notation_parse(const char *alg, int n, ct_move *out, int cap);
int ct_notation_format(const ct_move *moves, int count, int n, char *out, unsigned cap);
int ct_notation_parse_one(const char *token, int n, ct_move *out);
int ct_notation_format_one(const ct_move *m, int n, char *out, unsigned cap);

#endif
