#ifndef CT_GEOMETRY_H
#define CT_GEOMETRY_H

#include <stdbool.h>
#include <stdint.h>

#include "engine/cube.h"

enum {
    CT_GEOMETRY_MIN_N = 2,
    CT_GEOMETRY_MAX_N = CT_CUBE_MAX_N,
    CT_GEOMETRY_PERM_MAX_N = 6,
    CT_GEOMETRY_ERR = -1
};

typedef struct ct_geometry ct_geometry;

struct ct_geometry {
    const char *name;
    int (*sticker_count)(int n);
    int (*move_count)(int n);
    int (*move_facelets)(const ct_geometry *g, int move_id, int n, uint8_t *perm);
    int (*sticker_geometry)(const ct_geometry *g, int sticker, int n, ct_v3 *pos, ct_v3 *normal);
    int (*turn_axis)(const ct_geometry *g, int move_id, int n, ct_v3 *axis);
    int (*move_from_token)(const ct_geometry *g, const char *token, int n, ct_move *out);
};

const ct_geometry *ct_geometry_nxn(void);
int ct_geometry_apply(const ct_geometry *g, int move_id, int n, uint8_t *facelet);

#endif
