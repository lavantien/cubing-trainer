#ifndef CT_GEOMETRY_SQ1_H
#define CT_GEOMETRY_SQ1_H

#include <stdbool.h>
#include <stdint.h>

#include "config/config.h"
#include "engine/geometry.h"

enum {
    CT_SQ1_LAYERS = 2,
    CT_SQ1_BLOCKS = 2,
    CT_SQ1_TURN_MIN = -5,
    CT_SQ1_TURN_MAX = 6,
    CT_SQ1_TWISTS = CT_SQ1_UNITS * CT_SQ1_UNITS - 1,
    CT_SQ1_SLASH = CT_SQ1_TWISTS,
    CT_SQ1_MOVES = CT_SQ1_TWISTS + 1,
    CT_SQ1_STICKERS = CT_SQ1_BLOCKS * CT_SQ1_LAYERS * CT_SQ1_UNITS,
    CT_SQ1_CUT_HALF = CT_SQ1_UNITS / CT_SQ1_LAYERS,
    CT_SQ1_ZERO_POS = -CT_SQ1_TURN_MIN * CT_SQ1_UNITS - CT_SQ1_TURN_MIN,
    CT_SQ1_SIDE_Y = 1,
    CT_SQ1_FACE_Y = 2
};

enum { CT_SQ1_MOVE_TWIST = 0, CT_SQ1_MOVE_SLASH = 1 };

typedef struct {
    uint8_t units[CT_SQ1_LAYERS * CT_SQ1_UNITS];
    bool slice_flipped;
} ct_sq1_state;

const ct_geometry *ct_geometry_sq1(void);
void ct_sq1_state_solved(ct_sq1_state *state);
int ct_sq1_can_slash(const ct_sq1_state *state);
int ct_sq1_apply(ct_sq1_state *state, int move_id);
int ct_sq1_move_id(int x, int y);
int ct_sq1_move_pair(int move_id, int *x, int *y);

#endif
