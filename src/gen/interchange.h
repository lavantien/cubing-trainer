#ifndef CT_INTERCHANGE_H
#define CT_INTERCHANGE_H

#include "engine/cube.h"

enum {
    CT_INTERCHANGE_MAX_CANDIDATES = 180,
};

typedef struct {
    ct_move move;
} ct_interchange_move;

int ct_sticker_perm(ct_move m, int n,
                    uint8_t perm[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE]);
int ct_interchanges_for(int orbit, int x, int y, int n, ct_move *out, int cap);
int ct_candidate_moves(int n, ct_move *out, int cap);

#endif
