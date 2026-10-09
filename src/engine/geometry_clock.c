#include "engine/geometry_clock.h"

#include <string.h>

static const char *const ct_clock_names[CT_CLOCK_NAMES] = {"UR", "DR", "DL", "UL", "U",
                                                           "R",  "D",  "L",  "ALL"};

static const int8_t ct_clock_vec[CT_CLOCK_NAMES][CT_CLOCK_CLOCKS] = {
    [CT_CLOCK_UR] = {0, 1, 1, 0, 1, 1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0},
    [CT_CLOCK_DR] = {0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, -1, 0, 0},
    [CT_CLOCK_DL] = {0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1},
    [CT_CLOCK_UL] = {1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0},
    [CT_CLOCK_U] = {1, 1, 1, 1, 1, 1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0},
    [CT_CLOCK_R] = {0, 1, 1, 0, 1, 1, 0, 1, 1, -1, 0, 0, 0, 0, 0, -1, 0, 0},
    [CT_CLOCK_D] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, -1, 0, -1},
    [CT_CLOCK_L] = {1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1},
    [CT_CLOCK_ALL] = {1, 1, 1, 1, 1, 1, 1, 1, 1, -1, 0, -1, 0, 0, 0, -1, 0, -1},
};

static bool clock_valid_n(int n) {
    return n == CT_GEOMETRY_SINGLE_N;
}

static int clock_sticker_count(int n) {
    return clock_valid_n(n) ? CT_CLOCK_CLOCKS : CT_GEOMETRY_ERR;
}

static int clock_move_count(int n) {
    return clock_valid_n(n) ? CT_CLOCK_MOVES : CT_GEOMETRY_ERR;
}

int ct_clock_move_id(int name, int amount) {
    if (name < 0 || name >= CT_CLOCK_NAMES) {
        return CT_GEOMETRY_ERR;
    }
    if (amount >= 0 && amount <= CT_CLOCK_AMOUNT_MAX) {
        return name * CT_CLOCK_FORMS + amount;
    }
    if (amount < 0 && amount > -CT_CLOCK_AMOUNT_MAX) {
        return name * CT_CLOCK_FORMS + CT_CLOCK_FORMS + amount;
    }
    return CT_GEOMETRY_ERR;
}

static int clock_form_amount(int form) {
    return form < 7 ? form : -CT_CLOCK_FORMS + form;
}

void ct_clock_state_solved(ct_clock_state *state) {
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof *state);
}

int ct_clock_apply(ct_clock_state *state, int move_id) {
    if (state == NULL || move_id < 0 || move_id >= CT_CLOCK_MOVES) {
        return CT_GEOMETRY_ERR;
    }
    if (move_id == CT_CLOCK_Y2) {
        for (int i = 0; i < CT_CLOCK_FACE_CLOCKS; i++) {
            uint8_t front = state->clocks[i];
            state->clocks[i] = state->clocks[CT_CLOCK_FACE_CLOCKS + i];
            state->clocks[CT_CLOCK_FACE_CLOCKS + i] = front;
        }
        state->flipped = !state->flipped;
        return 0;
    }
    int name = move_id / CT_CLOCK_FORMS;
    int amount = clock_form_amount(move_id % CT_CLOCK_FORMS);
    for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
        int8_t factor = ct_clock_vec[name][i];
        if (factor == 0) {
            continue;
        }
        int value = ((state->clocks[i] + factor * amount) % CT_CLOCK_HOURS + CT_CLOCK_HOURS) %
                    CT_CLOCK_HOURS;
        state->clocks[i] = (uint8_t)value;
    }
    return 0;
}

static int clock_move_facelets(const ct_geometry *g, int move_id, int n, uint8_t *perm) {
    if (g == NULL || !clock_valid_n(n) || perm == NULL || move_id < 0 ||
        move_id >= CT_CLOCK_MOVES) {
        return CT_GEOMETRY_ERR;
    }
    if (move_id != CT_CLOCK_Y2) {
        return CT_GEOMETRY_E_PERM;
    }
    for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
        perm[i] = (uint8_t)((i + CT_CLOCK_FACE_CLOCKS) % CT_CLOCK_CLOCKS);
    }
    return 0;
}

static int clock_sticker_geometry(const ct_geometry *g, int sticker, int n, ct_v3 *pos,
                                  ct_v3 *normal) {
    if (g == NULL || !clock_valid_n(n) || pos == NULL || normal == NULL || sticker < 0 ||
        sticker >= CT_CLOCK_CLOCKS) {
        return CT_GEOMETRY_ERR;
    }
    int face = sticker < CT_CLOCK_FACE_CLOCKS ? CT_FACE_F : CT_FACE_B;
    int rest = sticker % CT_CLOCK_FACE_CLOCKS;
    ct_sticker_world(CT_CLOCK_GRID, face, rest / CT_CLOCK_GRID, rest % CT_CLOCK_GRID, pos, normal);
    return 0;
}

static int clock_turn_axis(const ct_geometry *g, int move_id, int n, ct_v3 *axis) {
    if (g == NULL || !clock_valid_n(n) || axis == NULL || move_id < 0 ||
        move_id >= CT_CLOCK_MOVES) {
        return CT_GEOMETRY_ERR;
    }
    if (move_id == CT_CLOCK_Y2) {
        *axis = (ct_v3){0, 1, 0};
    } else {
        *axis = (ct_v3){0, 0, 1};
    }
    return 0;
}

static int clock_move_from_token(const ct_geometry *g, const char *token, int n, ct_move *out) {
    if (g == NULL || token == NULL || out == NULL || !clock_valid_n(n)) {
        return CT_GEOMETRY_ERR;
    }
    if (strcmp(token, "y2") == 0) {
        *out = (ct_move){CT_CLOCK_MOVE_Y2, 0, 0, 0};
        return CT_CLOCK_Y2;
    }
    int letters = 0;
    while (token[letters] >= 'A' && token[letters] <= 'Z') {
        letters++;
    }
    int name = -1;
    for (int k = 0; k < CT_CLOCK_NAMES; k++) {
        if ((int)strlen(ct_clock_names[k]) == letters &&
            strncmp(token, ct_clock_names[k], (size_t)letters) == 0) {
            name = k;
            break;
        }
    }
    if (name < 0 || token[letters] < '0' || token[letters] > '6') {
        return CT_GEOMETRY_ERR;
    }
    int amount = token[letters] - '0';
    char sign = token[letters + 1];
    if (sign != '+' && sign != '-') {
        return CT_GEOMETRY_ERR;
    }
    if (sign == '-') {
        if (amount == 0) {
            return CT_GEOMETRY_ERR;
        }
        amount = -amount;
    }
    int id = ct_clock_move_id(name, amount);
    if (id < 0) {
        return CT_GEOMETRY_ERR;
    }
    if (token[letters + 2] != '\0') {
        return CT_GEOMETRY_ERR;
    }
    *out = (ct_move){name, amount, 0, 0};
    return id;
}

static const ct_geometry ct_clock_driver = {
    .name = "clock",
    .sticker_count = clock_sticker_count,
    .move_count = clock_move_count,
    .move_facelets = clock_move_facelets,
    .move_facelets_state = NULL,
    .sticker_geometry = clock_sticker_geometry,
    .turn_axis = clock_turn_axis,
    .move_from_token = clock_move_from_token,
};

const ct_geometry *ct_geometry_clock(void) {
    return &ct_clock_driver;
}
