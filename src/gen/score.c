#include "gen/score.h"

#include <math.h>
#include <stdio.h>

static const unsigned score_face_bits[CT_CUBE_FACE_COUNT] = {
    [CT_FACE_U] = CT_MOVE_BIT_U, [CT_FACE_L] = CT_MOVE_BIT_L, [CT_FACE_F] = CT_MOVE_BIT_F,
    [CT_FACE_R] = CT_MOVE_BIT_R, [CT_FACE_B] = CT_MOVE_BIT_B, [CT_FACE_D] = CT_MOVE_BIT_D,
};

static const int score_hand_of_face[CT_CUBE_FACE_COUNT] = {
    [CT_FACE_U] = CT_SCORE_HAND_RIGHT,   [CT_FACE_L] = CT_SCORE_HAND_LEFT,
    [CT_FACE_F] = CT_SCORE_HAND_DYNAMIC, [CT_FACE_R] = CT_SCORE_HAND_RIGHT,
    [CT_FACE_B] = CT_SCORE_HAND_DYNAMIC, [CT_FACE_D] = CT_SCORE_HAND_LEFT,
};

static const int score_region_of_face[CT_CUBE_FACE_COUNT] = {
    [CT_FACE_U] = CT_SCORE_HAND_RIGHT, [CT_FACE_L] = CT_SCORE_HAND_LEFT,
    [CT_FACE_F] = CT_SCORE_HAND_NONE,  [CT_FACE_R] = CT_SCORE_HAND_RIGHT,
    [CT_FACE_B] = CT_SCORE_HAND_NONE,  [CT_FACE_D] = CT_SCORE_HAND_LEFT,
};

static int score_check_seq(const ct_move *seq, int count) {
    if (count < 0 || (seq == NULL && count > 0)) {
        return CT_SCORE_E_ARG;
    }
    for (int i = 0; i < count; i++) {
        if (seq[i].face < 0 || seq[i].face >= CT_CUBE_FACE_COUNT) {
            return CT_SCORE_E_MOVE;
        }
    }
    return CT_SCORE_OK;
}

static int score_face_bit_index(unsigned bit) {
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        if ((1u << i) == bit) {
            return i;
        }
    }
    return -1;
}

int ct_count_qtm(const ct_move *seq, int count) {
    int rc = score_check_seq(seq, count);
    if (rc != CT_SCORE_OK) {
        return rc;
    }
    int qtm = 0;
    for (int i = 0; i < count; i++) {
        if (seq[i].quarter == CT_SCORE_QUARTER_DOUBLE) {
            qtm += CT_SCORE_QUARTER_DOUBLE;
        } else if (seq[i].quarter == CT_SCORE_QUARTER_CW ||
                   seq[i].quarter == CT_SCORE_QUARTER_CCW) {
            qtm += 1;
        }
    }
    return qtm;
}

int ct_count_regrips(const ct_move *seq, int count) {
    int rc = score_check_seq(seq, count);
    if (rc != CT_SCORE_OK) {
        return rc;
    }
    int grip[CT_SCORE_HAND_SLOTS] = {CT_SCORE_GRIP_HOME, CT_SCORE_GRIP_HOME};
    int last = CT_SCORE_HAND_RIGHT;
    int regrips = 0;
    for (int i = 0; i < count; i++) {
        int hand = score_hand_of_face[seq[i].face];
        if (hand == CT_SCORE_HAND_DYNAMIC) {
            hand = last;
        } else {
            last = hand;
        }
        if (grip[hand] != CT_SCORE_GRIP_HOME) {
            regrips += 1;
        }
        grip[hand] = score_region_of_face[seq[i].face] == hand ? CT_SCORE_GRIP_HOME
                                                               : CT_SCORE_GRIP_DISPLACED;
    }
    return regrips;
}

int ct_score_alg(const ct_move *seq, int count, const ct_config *cfg, ct_score_parts *out) {
    if (cfg == NULL || out == NULL) {
        return CT_SCORE_E_ARG;
    }
    int rc = score_check_seq(seq, count);
    if (rc != CT_SCORE_OK) {
        return rc;
    }
    int qtm = 0;
    double subset = 0.0;
    double finger = 0.0;
    for (int i = 0; i < count; i++) {
        unsigned bit = score_face_bits[seq[i].face];
        int idx = score_face_bit_index(bit);
        int quarter = seq[i].quarter;
        if (quarter == CT_SCORE_QUARTER_DOUBLE) {
            qtm += CT_SCORE_QUARTER_DOUBLE;
        } else if (quarter == CT_SCORE_QUARTER_CW || quarter == CT_SCORE_QUARTER_CCW) {
            qtm += 1;
        }
        if ((cfg->gen.preferred_moves & bit) == 0) {
            subset += cfg->gen.penalty[idx];
        }
        int dir = quarter == CT_SCORE_QUARTER_CCW ? CT_FINGER_DIR_CCW : CT_FINGER_DIR_CW;
        finger += cfg->gen.finger_cost[idx][dir];
    }
    out->qtm = qtm;
    out->subset_penalty = subset;
    out->regrips = ct_count_regrips(seq, count);
    out->finger_penalty = finger;
    out->total = cfg->gen.weight_len * (double)qtm + cfg->gen.weight_subset * subset +
                 cfg->gen.weight_regrip * (double)out->regrips + cfg->gen.weight_finger * finger;
    return CT_SCORE_OK;
}

int ct_score_parts_json(const ct_score_parts *p, char *out, unsigned cap) {
    if (p == NULL || out == NULL || cap == 0) {
        return CT_SCORE_E_ARG;
    }
    if (!isfinite(p->subset_penalty) || !isfinite(p->finger_penalty) || !isfinite(p->total)) {
        return CT_SCORE_E_ARG;
    }
    static const char fmt[] =
        "{\"qtm\":%d,\"subset\":%.4f,\"regrip\":%d,\"finger\":%.4f,\"total\":%.4f}";
    int need =
        snprintf(NULL, 0, fmt, p->qtm, p->subset_penalty, p->regrips, p->finger_penalty, p->total);
    if (need < 0 || (unsigned)need >= cap) {
        return CT_SCORE_E_CAP;
    }
    snprintf(out, cap, fmt, p->qtm, p->subset_penalty, p->regrips, p->finger_penalty, p->total);
    return need;
}
