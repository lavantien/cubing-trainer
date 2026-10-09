#include "engine/transform.h"

#include <stddef.h>

static const int ct_mirror_face[CT_CUBE_FACE_COUNT] = {
    [CT_FACE_U] = CT_FACE_U, [CT_FACE_L] = CT_FACE_R, [CT_FACE_F] = CT_FACE_F,
    [CT_FACE_R] = CT_FACE_L, [CT_FACE_B] = CT_FACE_B, [CT_FACE_D] = CT_FACE_D,
};

static const int ct_y_face[CT_CUBE_FACE_COUNT] = {
    [CT_FACE_U] = CT_FACE_U, [CT_FACE_L] = CT_FACE_B, [CT_FACE_F] = CT_FACE_L,
    [CT_FACE_R] = CT_FACE_F, [CT_FACE_B] = CT_FACE_R, [CT_FACE_D] = CT_FACE_D,
};

static bool move_valid(ct_move m) {
    return m.face >= CT_FACE_U && m.face <= CT_FACE_D && m.quarter >= 1 &&
           m.quarter <= CT_MOVE_QUARTERS - 1 && m.depth_from >= 1 && m.depth_to >= m.depth_from;
}

static bool moves_valid(const ct_move *in, int count) {
    for (int i = 0; i < count; i++)
        if (!move_valid(in[i]))
            return false;
    return true;
}

int ct_transform_inverse(const ct_move *in, int count, ct_move *out) {
    if (count < 0 || (count > 0 && (in == NULL || out == NULL)) || !moves_valid(in, count))
        return -1;
    for (int i = 0; i < count; i++) {
        ct_move m = in[count - 1 - i];
        m.quarter = CT_MOVE_QUARTERS - m.quarter;
        out[i] = m;
    }
    return count;
}

int ct_transform_mirror(const ct_move *in, int count, ct_move *out) {
    if (count < 0 || (count > 0 && (in == NULL || out == NULL)) || !moves_valid(in, count))
        return -1;
    for (int i = 0; i < count; i++) {
        ct_move m = in[i];
        m.face = ct_mirror_face[m.face];
        m.quarter = CT_MOVE_QUARTERS - m.quarter;
        out[i] = m;
    }
    return count;
}

int ct_transform_rotate_y(const ct_move *in, int count, int quarter_turns, ct_move *out) {
    if (count < 0 || (count > 0 && (in == NULL || out == NULL)) || !moves_valid(in, count))
        return -1;
    int k = ((quarter_turns % CT_MOVE_QUARTERS) + CT_MOVE_QUARTERS) % CT_MOVE_QUARTERS;
    for (int i = 0; i < count; i++) {
        ct_move m = in[i];
        for (int step = 0; step < k; step++)
            m.face = ct_y_face[m.face];
        out[i] = m;
    }
    return count;
}

int ct_transform_conjugate(const ct_move *setup, int setup_count, const ct_move *inner,
                           int inner_count, ct_move *out, int cap) {
    if (setup_count < 0 || inner_count < 0 || cap < 0)
        return -1;
    if ((setup_count > 0 && setup == NULL) || (inner_count > 0 && inner == NULL))
        return -1;
    if (!moves_valid(setup, setup_count) || !moves_valid(inner, inner_count))
        return -1;
    int total = 2 * setup_count + inner_count;
    if (total > cap || (total > 0 && out == NULL))
        return -1;
    for (int i = 0; i < setup_count; i++)
        out[i] = setup[i];
    for (int i = 0; i < inner_count; i++)
        out[setup_count + i] = inner[i];
    ct_transform_inverse(setup, setup_count, out + setup_count + inner_count);
    return total;
}

int ct_transform_simplify(const ct_move *in, int count, ct_move *out, int cap) {
    if (count < 0 || count > CT_TRANSFORM_ASSEMBLY_MAX || cap < 0 ||
        (count > 0 && (in == NULL || out == NULL)) || !moves_valid(in, count))
        return -1;
    ct_move stack[CT_TRANSFORM_ASSEMBLY_MAX];
    int cnt = 0;
    for (int i = 0; i < count; i++) {
        ct_move m = in[i];
        int quarter = m.quarter;
        while (cnt > 0 && quarter != 0 && stack[cnt - 1].face == m.face &&
               stack[cnt - 1].depth_from == m.depth_from && stack[cnt - 1].depth_to == m.depth_to) {
            quarter = (stack[cnt - 1].quarter + quarter) % CT_MOVE_QUARTERS;
            cnt--;
        }
        if (quarter != 0) {
            stack[cnt] = m;
            stack[cnt].quarter = quarter;
            cnt++;
        }
    }
    if (cnt > cap)
        return -1;
    for (int i = 0; i < cnt; i++)
        out[i] = stack[i];
    return cnt;
}
