#ifndef CT_GEOMETRY_CLOCK_H
#define CT_GEOMETRY_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

#include "config/config.h"
#include "engine/geometry.h"

enum {
    CT_CLOCK_FACE_CLOCKS = 9,
    CT_CLOCK_NAMES = 9,
    CT_CLOCK_FORMS = CT_CLOCK_HOURS,
    CT_CLOCK_AMOUNT_MAX = 6,
    CT_CLOCK_GRID = 3,
    CT_CLOCK_Y2 = CT_CLOCK_NAMES * CT_CLOCK_FORMS,
    CT_CLOCK_MOVES = CT_CLOCK_Y2 + 1
};

enum {
    CT_CLOCK_UR = 0,
    CT_CLOCK_DR,
    CT_CLOCK_DL,
    CT_CLOCK_UL,
    CT_CLOCK_U,
    CT_CLOCK_R,
    CT_CLOCK_D,
    CT_CLOCK_L,
    CT_CLOCK_ALL,
    CT_CLOCK_MOVE_Y2
};

typedef struct {
    uint8_t clocks[CT_CLOCK_CLOCKS];
    bool flipped;
} ct_clock_state;

const ct_geometry *ct_geometry_clock(void);
void ct_clock_state_solved(ct_clock_state *state);
int ct_clock_apply(ct_clock_state *state, int move_id);
int ct_clock_move_id(int name, int amount);

#endif
