#include "engine/geometry.h"
#include "engine/notation.h"

#include <string.h>

static bool geo_valid_n(int n) {
    return n >= CT_GEOMETRY_MIN_N && n <= CT_GEOMETRY_MAX_N;
}

static int geo_half(int n) {
    return (n + 1) / 2;
}

static int geo_pair_count(int n) {
    int half = geo_half(n);
    return half * (half + 1) / 2;
}

static int geo_block(int n) {
    return geo_pair_count(n) * (CT_MOVE_QUARTERS - 1);
}

static int geo_decode(int move_id, int n, ct_move *out) {
    if (!geo_valid_n(n) || out == NULL || move_id < 0 ||
        move_id >= geo_block(n) * CT_CUBE_FACE_COUNT) {
        return CT_GEOMETRY_ERR;
    }
    int half = geo_half(n);
    int block = geo_block(n);
    int face = move_id / block;
    int rest = move_id % block;
    int pair = rest / (CT_MOVE_QUARTERS - 1);
    int quarter = rest % (CT_MOVE_QUARTERS - 1) + 1;
    int width = half;
    int from = 1;
    while (pair >= width) {
        pair -= width;
        from++;
        width = half - from + 1;
    }
    *out = (ct_move){face, from, from + pair, quarter};
    return 0;
}

static int geo_encode(const ct_move *m, int n) {
    if (!geo_valid_n(n) || m == NULL || m->depth_to > geo_half(n)) {
        return CT_GEOMETRY_ERR;
    }
    int half = geo_half(n);
    int before = (m->depth_from - 1) * half - (m->depth_from - 1) * (m->depth_from - 2) / 2;
    int pair = before + (m->depth_to - m->depth_from);
    return m->face * geo_block(n) + pair * (CT_MOVE_QUARTERS - 1) + (m->quarter - 1);
}

static int nxn_sticker_count(int n) {
    if (!geo_valid_n(n)) {
        return CT_GEOMETRY_ERR;
    }
    return CT_CUBE_FACE_COUNT * n * n;
}

static int nxn_move_count(int n) {
    if (!geo_valid_n(n)) {
        return CT_GEOMETRY_ERR;
    }
    return geo_block(n) * CT_CUBE_FACE_COUNT;
}

static int nxn_move_facelets(const ct_geometry *g, int move_id, int n, uint8_t *perm) {
    if (g == NULL || perm == NULL || n < CT_GEOMETRY_MIN_N || n > CT_GEOMETRY_PERM_MAX_N) {
        return CT_GEOMETRY_ERR;
    }
    ct_move m;
    if (geo_decode(move_id, n, &m) != 0) {
        return CT_GEOMETRY_ERR;
    }
    ct_cube c;
    memset(perm, 0, sizeof c.facelet);
    c.n = n;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                c.facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col] =
                    (uint8_t)(face * n * n + row * n + col);
            }
        }
    }
    ct_apply(&c, m);
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                perm[c.facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE +
                               col]] = (uint8_t)(face * n * n + row * n + col);
            }
        }
    }
    return 0;
}

static int nxn_sticker_geometry(const ct_geometry *g, int sticker, int n, ct_v3 *pos,
                                ct_v3 *normal) {
    if (g == NULL || pos == NULL || normal == NULL || !geo_valid_n(n) || sticker < 0 ||
        sticker >= CT_CUBE_FACE_COUNT * n * n) {
        return CT_GEOMETRY_ERR;
    }
    int face = sticker / (n * n);
    int cell = sticker % (n * n);
    ct_sticker_world(n, face, cell / n, cell % n, pos, normal);
    return 0;
}

static int nxn_turn_axis(const ct_geometry *g, int move_id, int n, ct_v3 *axis) {
    if (g == NULL || axis == NULL) {
        return CT_GEOMETRY_ERR;
    }
    ct_move m;
    if (geo_decode(move_id, n, &m) != 0) {
        return CT_GEOMETRY_ERR;
    }
    ct_v3 center;
    ct_sticker_world(n, m.face, 0, 0, &center, axis);
    return 0;
}

static int nxn_move_from_token(const ct_geometry *g, const char *token, int n, ct_move *out) {
    if (g == NULL || token == NULL || out == NULL || !geo_valid_n(n)) {
        return CT_GEOMETRY_ERR;
    }
    ct_move m;
    if (ct_notation_parse_one(token, n, &m) != 0) {
        return CT_GEOMETRY_ERR;
    }
    int id = geo_encode(&m, n);
    if (id < 0) {
        return CT_GEOMETRY_ERR;
    }
    *out = m;
    return id;
}

static const ct_geometry ct_nxn_driver = {
    .name = "nxn",
    .sticker_count = nxn_sticker_count,
    .move_count = nxn_move_count,
    .move_facelets = nxn_move_facelets,
    .move_facelets_state = NULL,
    .sticker_geometry = nxn_sticker_geometry,
    .turn_axis = nxn_turn_axis,
    .move_from_token = nxn_move_from_token,
};

const ct_geometry *ct_geometry_nxn(void) {
    return &ct_nxn_driver;
}

int ct_geometry_apply(const ct_geometry *g, int move_id, int n, uint8_t *facelet) {
    if (g == NULL || facelet == NULL || g != ct_geometry_nxn() || !geo_valid_n(n)) {
        return CT_GEOMETRY_ERR;
    }
    ct_move m;
    if (geo_decode(move_id, n, &m) != 0) {
        return CT_GEOMETRY_ERR;
    }
    ct_cube c;
    c.n = n;
    memcpy(c.facelet, facelet, sizeof c.facelet);
    ct_apply(&c, m);
    memcpy(facelet, c.facelet, sizeof c.facelet);
    return 0;
}
