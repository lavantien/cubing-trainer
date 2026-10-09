#include "engine/geometry_sq1.h"

#include <string.h>

static const int8_t ct_sq1_ring_x[CT_SQ1_UNITS] = {1, 1, 2, 2, 1, 1, -1, -1, -2, -2, -1, -1};
static const int8_t ct_sq1_ring_z[CT_SQ1_UNITS] = {2, 1, 1, -1, -1, -2, -2, -1, -1, 1, 1, 2};

static bool sq1_valid_n(int n) {
    return n == CT_GEOMETRY_SINGLE_N;
}

static int sq1_wrap(int value) {
    return ((value % CT_SQ1_UNITS) + CT_SQ1_UNITS) % CT_SQ1_UNITS;
}

static int sq1_sticker_count(int n) {
    return sq1_valid_n(n) ? CT_SQ1_STICKERS : CT_GEOMETRY_ERR;
}

static int sq1_move_count(int n) {
    return sq1_valid_n(n) ? CT_SQ1_MOVES : CT_GEOMETRY_ERR;
}

int ct_sq1_move_id(int x, int y) {
    if (x < CT_SQ1_TURN_MIN || x > CT_SQ1_TURN_MAX || y < CT_SQ1_TURN_MIN || y > CT_SQ1_TURN_MAX ||
        (x == 0 && y == 0)) {
        return CT_GEOMETRY_ERR;
    }
    int pos = (x - CT_SQ1_TURN_MIN) * CT_SQ1_UNITS + (y - CT_SQ1_TURN_MIN);
    return pos < CT_SQ1_ZERO_POS ? pos : pos - 1;
}

int ct_sq1_move_pair(int move_id, int *x, int *y) {
    if (x == NULL || y == NULL || move_id < 0 || move_id >= CT_SQ1_TWISTS) {
        return CT_GEOMETRY_ERR;
    }
    int pos = move_id < CT_SQ1_ZERO_POS ? move_id : move_id + 1;
    *x = pos / CT_SQ1_UNITS + CT_SQ1_TURN_MIN;
    *y = pos % CT_SQ1_UNITS + CT_SQ1_TURN_MIN;
    return 0;
}

void ct_sq1_state_solved(ct_sq1_state *state) {
    if (state == NULL) {
        return;
    }
    int pieces_per_layer = CT_SQ1_PIECES / CT_SQ1_LAYERS;
    for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
        int unit = 0;
        for (int piece = 0; piece < pieces_per_layer; piece++) {
            int width = piece % 2 == layer ? 2 : 1;
            uint8_t id = (uint8_t)(layer * pieces_per_layer + piece);
            for (int w = 0; w < width; w++) {
                state->units[layer * CT_SQ1_UNITS + unit] = id;
                unit++;
            }
        }
    }
    state->slice_flipped = false;
}

int ct_sq1_can_slash(const ct_sq1_state *state) {
    if (state == NULL) {
        return CT_GEOMETRY_ERR;
    }
    for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
        const uint8_t *units = state->units + (ptrdiff_t)(layer * CT_SQ1_UNITS);
        if (units[0] == units[CT_SQ1_UNITS - 1] ||
            units[CT_SQ1_CUT_HALF - 1] == units[CT_SQ1_CUT_HALF]) {
            return 0;
        }
    }
    return 1;
}

static void sq1_rotate(uint8_t *units, int turn) {
    uint8_t rotated[CT_SQ1_UNITS];
    int shift = sq1_wrap(turn);
    for (int u = 0; u < CT_SQ1_UNITS; u++) {
        rotated[u] = units[sq1_wrap(u - shift)];
    }
    memcpy(units, rotated, sizeof rotated);
}

int ct_sq1_apply(ct_sq1_state *state, int move_id) {
    if (state == NULL || move_id < 0 || move_id >= CT_SQ1_MOVES) {
        return CT_GEOMETRY_ERR;
    }
    if (move_id == CT_SQ1_SLASH) {
        if (!ct_sq1_can_slash(state)) {
            return CT_GEOMETRY_ERR;
        }
        for (int u = CT_SQ1_CUT_HALF; u < CT_SQ1_UNITS; u++) {
            uint8_t top = state->units[u];
            state->units[u] = state->units[CT_SQ1_UNITS + u];
            state->units[CT_SQ1_UNITS + u] = top;
        }
        state->slice_flipped = !state->slice_flipped;
        return 0;
    }
    int x;
    int y;
    if (ct_sq1_move_pair(move_id, &x, &y) != 0) {
        return CT_GEOMETRY_ERR;
    }
    sq1_rotate(state->units, x);
    sq1_rotate(state->units + CT_SQ1_UNITS, y);
    return 0;
}

static void sq1_perm_slash(uint8_t *perm) {
    for (int block = 0; block < CT_SQ1_BLOCKS; block++) {
        int base = block * CT_SQ1_LAYERS * CT_SQ1_UNITS;
        for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
            for (int u = 0; u < CT_SQ1_UNITS; u++) {
                int dst_layer = u >= CT_SQ1_CUT_HALF ? CT_SQ1_LAYERS - 1 - layer : layer;
                perm[base + layer * CT_SQ1_UNITS + u] =
                    (uint8_t)(base + dst_layer * CT_SQ1_UNITS + u);
            }
        }
    }
}

static int sq1_perm_twist(int move_id, uint8_t *perm) {
    int x;
    int y;
    if (ct_sq1_move_pair(move_id, &x, &y) != 0) {
        return CT_GEOMETRY_ERR;
    }
    const int turns[CT_SQ1_LAYERS] = {x, y};
    for (int block = 0; block < CT_SQ1_BLOCKS; block++) {
        int base = block * CT_SQ1_LAYERS * CT_SQ1_UNITS;
        for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
            for (int u = 0; u < CT_SQ1_UNITS; u++) {
                perm[base + layer * CT_SQ1_UNITS + u] =
                    (uint8_t)(base + layer * CT_SQ1_UNITS + sq1_wrap(u + turns[layer]));
            }
        }
    }
    return 0;
}

static int sq1_move_facelets(const ct_geometry *g, int move_id, int n, uint8_t *perm) {
    if (g == NULL || !sq1_valid_n(n) || perm == NULL) {
        return CT_GEOMETRY_ERR;
    }
    if (move_id == CT_SQ1_SLASH) {
        return CT_GEOMETRY_E_STATE;
    }
    return sq1_perm_twist(move_id, perm);
}

static int sq1_move_facelets_state(const ct_geometry *g, int move_id, int n, const void *state,
                                   uint8_t *perm) {
    if (g == NULL || !sq1_valid_n(n) || perm == NULL) {
        return CT_GEOMETRY_ERR;
    }
    if (move_id == CT_SQ1_SLASH) {
        if (state == NULL) {
            return CT_GEOMETRY_ERR;
        }
        if (!ct_sq1_can_slash(state)) {
            return CT_GEOMETRY_ERR;
        }
        sq1_perm_slash(perm);
        return 0;
    }
    return sq1_perm_twist(move_id, perm);
}

static int sq1_sticker_geometry(const ct_geometry *g, int sticker, int n, ct_v3 *pos,
                                ct_v3 *normal) {
    if (g == NULL || !sq1_valid_n(n) || pos == NULL || normal == NULL || sticker < 0 ||
        sticker >= CT_SQ1_STICKERS) {
        return CT_GEOMETRY_ERR;
    }
    int ring = sticker % (CT_SQ1_LAYERS * CT_SQ1_UNITS);
    int block = sticker / (CT_SQ1_LAYERS * CT_SQ1_UNITS);
    int layer = ring / CT_SQ1_UNITS;
    int unit = ring % CT_SQ1_UNITS;
    int r = layer == 0 ? unit : sq1_wrap(CT_SQ1_CUT_HALF - 1 - unit);
    if (block == 0) {
        *pos = (ct_v3){ct_sq1_ring_x[r], layer == 0 ? CT_SQ1_SIDE_Y : -CT_SQ1_SIDE_Y,
                       ct_sq1_ring_z[r]};
        *normal = (ct_v3){ct_sq1_ring_x[r], 0, ct_sq1_ring_z[r]};
    } else {
        *pos = (ct_v3){ct_sq1_ring_x[r], layer == 0 ? CT_SQ1_FACE_Y : -CT_SQ1_FACE_Y,
                       ct_sq1_ring_z[r]};
        *normal = (ct_v3){0, layer == 0 ? 1 : -1, 0};
    }
    return 0;
}

static int sq1_turn_axis(const ct_geometry *g, int move_id, int n, ct_v3 *axis) {
    if (g == NULL || !sq1_valid_n(n) || axis == NULL || move_id < 0 || move_id >= CT_SQ1_MOVES) {
        return CT_GEOMETRY_ERR;
    }
    if (move_id == CT_SQ1_SLASH) {
        *axis = (ct_v3){0, 0, 1};
    } else {
        *axis = (ct_v3){0, 1, 0};
    }
    return 0;
}

static int sq1_parse_int(const char *text, int *value) {
    int index = 0;
    int sign = 1;
    if (text[0] == '-') {
        sign = -1;
        index = 1;
    }
    if (text[index] < '0' || text[index] > '9') {
        return CT_GEOMETRY_ERR;
    }
    int digits = 0;
    int parsed = 0;
    while (text[index] >= '0' && text[index] <= '9') {
        parsed = parsed * CT_DECIMAL_BASE + (text[index] - '0');
        index++;
        digits++;
    }
    if (digits > 1 && parsed < CT_DECIMAL_BASE) {
        return CT_GEOMETRY_ERR;
    }
    *value = sign * parsed;
    return index;
}

static int sq1_move_from_token(const ct_geometry *g, const char *token, int n, ct_move *out) {
    if (g == NULL || token == NULL || out == NULL || !sq1_valid_n(n)) {
        return CT_GEOMETRY_ERR;
    }
    if (token[0] == '\0') {
        return CT_GEOMETRY_ERR;
    }
    if (strcmp(token, "/") == 0) {
        *out = (ct_move){CT_SQ1_MOVE_SLASH, 0, 0, 0};
        return CT_SQ1_SLASH;
    }
    if (token[0] != '(') {
        return CT_GEOMETRY_ERR;
    }
    int x;
    int used_x = sq1_parse_int(token + 1, &x);
    if (used_x < 0 || token[1 + used_x] != ',') {
        return CT_GEOMETRY_ERR;
    }
    int y;
    int used_y = sq1_parse_int(token + 2 + used_x, &y);
    if (used_y < 0 || token[2 + used_x + used_y] != ')' || token[3 + used_x + used_y] != '\0') {
        return CT_GEOMETRY_ERR;
    }
    int id = ct_sq1_move_id(x, y);
    if (id < 0) {
        return CT_GEOMETRY_ERR;
    }
    *out = (ct_move){CT_SQ1_MOVE_TWIST, x, y, 0};
    return id;
}

static const ct_geometry ct_sq1_driver = {
    .name = "sq1",
    .sticker_count = sq1_sticker_count,
    .move_count = sq1_move_count,
    .move_facelets = sq1_move_facelets,
    .move_facelets_state = sq1_move_facelets_state,
    .sticker_geometry = sq1_sticker_geometry,
    .turn_axis = sq1_turn_axis,
    .move_from_token = sq1_move_from_token,
};

const ct_geometry *ct_geometry_sq1(void) {
    return &ct_sq1_driver;
}
