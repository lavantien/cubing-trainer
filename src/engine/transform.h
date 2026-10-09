#ifndef CT_TRANSFORM_H
#define CT_TRANSFORM_H

#include "engine/cube.h"

int ct_transform_inverse(const ct_move *in, int count, ct_move *out);
int ct_transform_mirror(const ct_move *in, int count, ct_move *out);
int ct_transform_rotate_y(const ct_move *in, int count, int quarter_turns, ct_move *out);
int ct_transform_conjugate(const ct_move *setup, int setup_count, const ct_move *inner,
                           int inner_count, ct_move *out, int cap);
int ct_transform_simplify(const ct_move *in, int count, ct_move *out, int cap);

#endif
