#ifndef CT_CLASSIFY_H
#define CT_CLASSIFY_H

#include "config/config.h"
#include "engine/cube.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CT_CLASSIFY_N = CT_MOVE_QUARTERS - 1,
    CT_CLASSIFY_CELLS = CT_CUBE_FACE_COUNT * CT_CLASSIFY_N * CT_CLASSIFY_N,
    CT_CLASSIFY_OK = 0,
    CT_CLASSIFY_E_ARG = -1
};

typedef struct {
    ct_classification cls;
    ct_move setup[CT_CLASSIFY_SETUP_MAX];
    int setup_len;
    ct_move interchange[CT_CLASSIFY_PART_MAX];
    int interchange_len;
    ct_move insertion[CT_CLASSIFY_PART_MAX];
    int insertion_len;
    bool insert_first;
    bool cyclic;
    int moves;
    int qtm;
    char label[CT_CLASSIFY_LABEL_MAX];
    char rationale[CT_CLASSIFY_REASON_MAX];
} ct_classify_result;

int ct_classify(const ct_move *seq, int count, ct_classify_result *out);
int ct_classify_alg(const char *alg, ct_classify_result *out);
const char *ct_classification_label(ct_classification cls);
const char *ct_classification_alias(ct_classification cls);
ct_classification ct_classification_from_label(const char *label);

#ifdef __cplusplus
}
#endif

#endif
