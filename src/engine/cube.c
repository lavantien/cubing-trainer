#include "engine/cube.h"

#include <string.h>

typedef struct {
    ct_v3 normal;
    ct_v3 row;
    ct_v3 col;
} ct_face_frame;

static const ct_face_frame ct_frames[CT_CUBE_FACE_COUNT] = {
    [CT_FACE_U] = {{0, 1, 0}, {0, 0, 1}, {1, 0, 0}},
    [CT_FACE_L] = {{-1, 0, 0}, {0, -1, 0}, {0, 0, 1}},
    [CT_FACE_F] = {{0, 0, 1}, {0, -1, 0}, {1, 0, 0}},
    [CT_FACE_R] = {{1, 0, 0}, {0, -1, 0}, {0, 0, -1}},
    [CT_FACE_B] = {{0, 0, -1}, {0, -1, 0}, {-1, 0, 0}},
    [CT_FACE_D] = {{0, -1, 0}, {0, 0, -1}, {1, 0, 0}},
};

static int cube_index(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int frame_axis(const ct_face_frame *fr) {
    if (fr->normal.x != 0) return 0;
    if (fr->normal.y != 0) return 1;
    return 2;
}

static int frame_sign(const ct_face_frame *fr) {
    if (fr->normal.x != 0) return fr->normal.x;
    if (fr->normal.y != 0) return fr->normal.y;
    return fr->normal.z;
}

void ct_sticker_world(int n, int face, int row, int col, ct_v3 *center, ct_v3 *normal) {
    const ct_face_frame *fr = &ct_frames[face];
    int h = n - 1;
    int along_row = 2 * row - h;
    int along_col = 2 * col - h;
    center->x = fr->normal.x * h + fr->row.x * along_row + fr->col.x * along_col;
    center->y = fr->normal.y * h + fr->row.y * along_row + fr->col.y * along_col;
    center->z = fr->normal.z * h + fr->row.z * along_row + fr->col.z * along_col;
    *normal = fr->normal;
}

ct_cell ct_world_sticker(ct_v3 center, ct_v3 normal, int n) {
    const ct_face_frame *fr = NULL;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        const ct_face_frame *candidate = &ct_frames[face];
        if (normal.x == candidate->normal.x
            && normal.y == candidate->normal.y
            && normal.z == candidate->normal.z) {
            fr = candidate;
            break;
        }
    }
    int h = n - 1;
    ct_cell cell = {-1, -1, -1};
    if (fr == NULL) return cell;
    int along_row = center.x * fr->row.x + center.y * fr->row.y + center.z * fr->row.z;
    int along_col = center.x * fr->col.x + center.y * fr->col.y + center.z * fr->col.z;
    cell.face = (int)(fr - ct_frames);
    cell.row = (along_row + h) / 2;
    cell.col = (along_col + h) / 2;
    return cell;
}

static ct_v3 v3_rot90(ct_v3 v, int axis) {
    ct_v3 r = v;
    if (axis == 0) {
        r.y = -v.z;
        r.z = v.y;
    } else if (axis == 1) {
        r.x = v.z;
        r.z = -v.x;
    } else {
        r.x = -v.y;
        r.y = v.x;
    }
    return r;
}

void ct_cube_init(ct_cube *c, int n) {
    if (n < 2) n = 2;
    if (n > CT_CUBE_MAX_N) n = CT_CUBE_MAX_N;
    c->n = n;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int i = 0; i < CT_CUBE_STRIDE * CT_CUBE_STRIDE; i++) {
            c->facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + i] = (uint8_t)face;
        }
    }
}

bool ct_cube_solved(const ct_cube *c) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < c->n; row++) {
            for (int col = 0; col < c->n; col++) {
                if (c->facelet[cube_index(face, row, col)] != face) return false;
            }
        }
    }
    return true;
}

bool ct_cube_equal(const ct_cube *a, const ct_cube *b) {
    return a->n == b->n && memcmp(a->facelet, b->facelet, sizeof a->facelet) == 0;
}

ct_move ct_move_inverse(ct_move m) {
    ct_move r = {m.face, m.depth_from, m.depth_to, CT_MOVE_QUARTERS - m.quarter};
    return r;
}

static bool move_legal(const ct_cube *c, ct_move m) {
    return m.face >= CT_FACE_U
        && m.face <= CT_FACE_D
        && m.quarter >= 1
        && m.quarter <= CT_MOVE_QUARTERS - 1
        && m.depth_from >= 1
        && m.depth_to >= m.depth_from
        && m.depth_to <= c->n;
}

void ct_apply(ct_cube *c, ct_move m) {
    if (!move_legal(c, m)) {
        return;
    }
    ct_cube src = *c;
    const ct_face_frame *fr = &ct_frames[m.face];
    int axis = frame_axis(fr);
    int sign = frame_sign(fr);
    int h = c->n - 1;
    int u_lo = h - 2 * (m.depth_to - 1);
    int u_hi = h - 2 * (m.depth_from - 1);
    int steps = ((-sign * m.quarter) % CT_MOVE_QUARTERS + CT_MOVE_QUARTERS) % CT_MOVE_QUARTERS;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < c->n; row++) {
            for (int col = 0; col < c->n; col++) {
                ct_v3 center;
                ct_v3 normal;
                ct_sticker_world(c->n, face, row, col, &center, &normal);
                int u = center.x * fr->normal.x + center.y * fr->normal.y + center.z * fr->normal.z;
                int rot = 0;
                if (u >= u_lo && u <= u_hi) rot = steps;
                for (int k = 0; k < rot; k++) {
                    center = v3_rot90(center, axis);
                    normal = v3_rot90(normal, axis);
                }
                ct_cell dst = ct_world_sticker(center, normal, c->n);
                c->facelet[cube_index(dst.face, dst.row, dst.col)] = src.facelet[cube_index(face, row, col)];
            }
        }
    }
}
