#ifndef CT_TARGETS_H
#define CT_TARGETS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ct_target_face {
    CT_TARGET_FACE_U = 0,
    CT_TARGET_FACE_L,
    CT_TARGET_FACE_F,
    CT_TARGET_FACE_R,
    CT_TARGET_FACE_B,
    CT_TARGET_FACE_D
};

enum ct_target_orbit {
    CT_TARGET_ORBIT_CORNER = 0,
    CT_TARGET_ORBIT_EDGE = 1,
};

enum ct_target_const {
    CT_TARGET_NONE = -1,
    CT_TARGET_AXIS_COUNT = 3,
    CT_TARGET_CORNER_STICKERS_PER_PIECE = 3,
    CT_TARGET_EDGE_STICKERS_PER_PIECE = 2,
    CT_TARGET_FACE_COUNT = 6,
    CT_TARGET_PER_FACE = 4,
    CT_TARGET_STICKERS = CT_TARGET_FACE_COUNT * CT_TARGET_PER_FACE,
    CT_TARGET_CORNER_PIECES = CT_TARGET_STICKERS / CT_TARGET_CORNER_STICKERS_PER_PIECE,
    CT_TARGET_EDGE_PIECES = CT_TARGET_STICKERS / CT_TARGET_EDGE_STICKERS_PER_PIECE
};

typedef struct ct_target_pair {
    int t1;
    int t2;
} ct_target_pair;

int ct_corner_piece_of(int sticker);
int ct_edge_piece_of(int sticker);
void ct_sticker_cell(int orbit, int sticker, int *face, int *row, int *col);
int ct_targets_corner(int buffer_sticker, ct_target_pair *out, int cap);
int ct_targets_edge(int buffer_sticker, ct_target_pair *out, int cap);

#ifdef __cplusplus
}
#endif

#endif
