#ifndef CT_SCORE_H
#define CT_SCORE_H

#include "config/config.h"
#include "engine/cube.h"

enum {
    CT_SCORE_OK = 0,
    CT_SCORE_E_ARG = -1,
    CT_SCORE_E_MOVE = -2,
    CT_SCORE_E_CAP = CT_SCORE_E_MOVE - 1
};

enum {
    CT_SCORE_QUARTER_CW = 1,
    CT_SCORE_QUARTER_DOUBLE = 2,
    CT_SCORE_QUARTER_CCW = CT_SCORE_QUARTER_DOUBLE + 1
};

enum {
    CT_SCORE_HAND_RIGHT = 0,
    CT_SCORE_HAND_LEFT = 1,
    CT_SCORE_HAND_DYNAMIC = 2,
    CT_SCORE_HAND_NONE = CT_SCORE_HAND_DYNAMIC + 1,
    CT_SCORE_HAND_SLOTS = CT_SCORE_HAND_LEFT + 1
};

enum { CT_SCORE_GRIP_HOME = 0, CT_SCORE_GRIP_DISPLACED = 1 };

typedef struct {
    int qtm;
    double subset_penalty;
    int regrips;
    double finger_penalty;
    double total;
} ct_score_parts;

int ct_score_alg(const ct_move *seq, int count, const ct_config *cfg, ct_orbit orbit,
                 ct_score_parts *out);
int ct_score_parts_json(const ct_score_parts *p, char *out, unsigned cap);
int ct_count_qtm(const ct_move *seq, int count);
int ct_count_regrips(const ct_move *seq, int count);

#endif
