#include "gen/targets.h"

#include <stddef.h>

static const int ct_face_normal[CT_TARGET_FACE_COUNT][CT_TARGET_AXIS_COUNT] = {
    {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 0, -1}, {0, -1, 0},
};

static const int ct_face_row_dir[CT_TARGET_FACE_COUNT][CT_TARGET_AXIS_COUNT] = {
    {0, 0, 1}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, 0, -1},
};

static const int ct_face_col_dir[CT_TARGET_FACE_COUNT][CT_TARGET_AXIS_COUNT] = {
    {1, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0},
};

static void ct_corner_grid(int pos, int *row, int *col) {
    switch (pos) {
    case 0:
        *row = 0;
        *col = 0;
        break;
    case 1:
        *row = 0;
        *col = 2;
        break;
    case 2:
        *row = 2;
        *col = 2;
        break;
    default:
        *row = 2;
        *col = 0;
        break;
    }
}

static void ct_edge_grid(int pos, int *row, int *col) {
    switch (pos) {
    case 0:
        *row = 0;
        *col = 1;
        break;
    case 1:
        *row = 1;
        *col = 2;
        break;
    case 2:
        *row = 2;
        *col = 1;
        break;
    default:
        *row = 1;
        *col = 0;
        break;
    }
}

static void ct_sticker_point(int sticker, bool corner, int point[CT_TARGET_AXIS_COUNT]) {
    int face = sticker / CT_TARGET_PER_FACE;
    int pos = sticker % CT_TARGET_PER_FACE;
    int row = 0;
    int col = 0;
    int row_off = 0;
    int col_off = 0;
    if (corner) {
        ct_corner_grid(pos, &row, &col);
        row_off = row == 2 ? 1 : -1;
        col_off = col == 2 ? 1 : -1;
    } else {
        ct_edge_grid(pos, &row, &col);
        row_off = row - 1;
        col_off = col - 1;
    }
    for (int axis = 0; axis < CT_TARGET_AXIS_COUNT; axis++) {
        point[axis] = ct_face_normal[face][axis] + ct_face_row_dir[face][axis] * row_off +
                      ct_face_col_dir[face][axis] * col_off;
    }
}

static int ct_corner_piece_from_point(const int point[CT_TARGET_AXIS_COUNT]) {
    int layer = point[1] > 0 ? 0 : 1;
    int back = point[2] < 0 ? 0 : 2;
    int left = point[0] < 0 ? 1 : 0;
    return layer * CT_TARGET_PER_FACE + back + left;
}

static int ct_edge_ring_slot(const int point[CT_TARGET_AXIS_COUNT]) {
    if (point[0] <= 0 && point[2] < 0) {
        return 0;
    }
    if (point[0] > 0 && point[2] <= 0) {
        return 1;
    }
    if (point[0] >= 0 && point[2] > 0) {
        return 2;
    }
    return CT_TARGET_PER_FACE - 1;
}

static int ct_edge_piece_from_point(const int point[CT_TARGET_AXIS_COUNT]) {
    int layer = point[1] > 0 ? 0 : (point[1] == 0 ? 1 : 2);
    return layer * CT_TARGET_PER_FACE + ct_edge_ring_slot(point);
}

int ct_corner_piece_of(int sticker) {
    if (sticker < 0 || sticker >= CT_TARGET_STICKERS) {
        return CT_TARGET_NONE;
    }
    int point[CT_TARGET_AXIS_COUNT];
    ct_sticker_point(sticker, true, point);
    return ct_corner_piece_from_point(point);
}

int ct_edge_piece_of(int sticker) {
    if (sticker < 0 || sticker >= CT_TARGET_STICKERS) {
        return CT_TARGET_NONE;
    }
    int point[CT_TARGET_AXIS_COUNT];
    ct_sticker_point(sticker, false, point);
    return ct_edge_piece_from_point(point);
}

typedef int (*ct_piece_of_fn)(int);

static int ct_enumerate_targets(int buffer_sticker, ct_piece_of_fn piece_of, ct_target_pair *out,
                                int cap) {
    if (buffer_sticker < 0 || buffer_sticker >= CT_TARGET_STICKERS) {
        return CT_TARGET_NONE;
    }
    if (cap < 0) {
        cap = 0;
    }
    int buffer_piece = piece_of(buffer_sticker);
    int total = 0;
    for (int t1 = 0; t1 < CT_TARGET_STICKERS; t1++) {
        int piece1 = piece_of(t1);
        if (piece1 == buffer_piece) {
            continue;
        }
        for (int t2 = 0; t2 < CT_TARGET_STICKERS; t2++) {
            int piece2 = piece_of(t2);
            if (piece2 == buffer_piece || piece2 == piece1) {
                continue;
            }
            if (out != NULL && total < cap) {
                out[total].t1 = t1;
                out[total].t2 = t2;
            }
            total++;
        }
    }
    return total;
}

int ct_targets_corner(int buffer_sticker, ct_target_pair *out, int cap) {
    return ct_enumerate_targets(buffer_sticker, ct_corner_piece_of, out, cap);
}

int ct_targets_edge(int buffer_sticker, ct_target_pair *out, int cap) {
    return ct_enumerate_targets(buffer_sticker, ct_edge_piece_of, out, cap);
}
