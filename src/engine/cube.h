#ifndef CT_CUBE_H
#define CT_CUBE_H

#include <stdbool.h>
#include <stdint.h>

enum {
    CT_CUBE_FACE_COUNT = 6,
    CT_CUBE_MAX_N = 7,
    CT_CUBE_STRIDE = CT_CUBE_MAX_N,
    CT_MOVE_QUARTERS = 4
};

enum {
    CT_FACE_U = 0,
    CT_FACE_L = 1,
    CT_FACE_F = 2,
    CT_FACE_R = 3,
    CT_FACE_B = 4,
    CT_FACE_D = 5
};

typedef struct {
    int n;
    uint8_t facelet[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE];
} ct_cube;

typedef struct {
    int face;
    int depth_from;
    int depth_to;
    int quarter;
} ct_move;

void ct_cube_init(ct_cube *c, int n);
bool ct_cube_solved(const ct_cube *c);
bool ct_cube_equal(const ct_cube *a, const ct_cube *b);
void ct_apply(ct_cube *c, ct_move m);
ct_move ct_move_inverse(ct_move m);

typedef struct {
    int x;
    int y;
    int z;
} ct_v3;

typedef struct {
    int face;
    int row;
    int col;
} ct_cell;

void ct_sticker_world(int n, int face, int row, int col, ct_v3 *center, ct_v3 *normal);
ct_cell ct_world_sticker(ct_v3 center, ct_v3 normal, int n);

#endif
