#include "render/renderer.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

const char *const ct_color_scheme_keys[CT_CUBE_FACE_COUNT] = {
    "ui.color_u", "ui.color_l", "ui.color_f", "ui.color_r", "ui.color_b", "ui.color_d",
};

enum {
    ct_hex_digits_per_channel = 2,
    ct_hex_channels = 3,
    ct_hex_len = ct_hex_digits_per_channel * ct_hex_channels,
    ct_hex_nibble_shift = 4,
    ct_hex_alpha = '9' - '0' + 1,
    ct_hex_alpha_opaque = UCHAR_MAX
};

static int ct_hex_nibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + ct_hex_alpha;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + ct_hex_alpha;
    }
    return -1;
}

bool ct_color_scheme_face_hex(ct_color_scheme *scheme, int face, const char *hex) {
    if (scheme == NULL || hex == NULL || face < 0 || face >= CT_CUBE_FACE_COUNT) {
        return false;
    }
    if (strlen(hex) != (size_t)ct_hex_len) {
        return false;
    }
    unsigned char rgb[ct_hex_channels];
    for (int ch = 0; ch < ct_hex_channels; ch++) {
        size_t off = (size_t)ch * (size_t)ct_hex_digits_per_channel;
        int hi = ct_hex_nibble(hex[off]);
        int lo = ct_hex_nibble(hex[off + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        rgb[ch] = (unsigned char)((hi << ct_hex_nibble_shift) | lo);
    }
    scheme->face[face] = (Color){rgb[0], rgb[1], rgb[2], ct_hex_alpha_opaque};
    return true;
}

static float clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

ct_color_scheme ct_color_scheme_default(void) {
    ct_color_scheme scheme;
    scheme.face[CT_FACE_U] = WHITE;
    scheme.face[CT_FACE_L] = ORANGE;
    scheme.face[CT_FACE_F] = GREEN;
    scheme.face[CT_FACE_R] = RED;
    scheme.face[CT_FACE_B] = BLUE;
    scheme.face[CT_FACE_D] = YELLOW;
    return scheme;
}

void ct_renderer_init(ct_renderer *r, int n) {
    if (n < CT_CUBE_MIN_N) {
        n = CT_CUBE_MIN_N;
    }
    if (n > CT_CUBE_MAX_N) {
        n = CT_CUBE_MAX_N;
    }
    r->n = n;
    r->cube_size = CT_RENDERER_DEFAULT_CUBE_SIZE;
    r->sticker_inset = CT_RENDERER_DEFAULT_INSET;
}

bool ct_renderer_slab_contains(int n, ct_move m, ct_v3 center) {
    if (!ct_move_legal(&m, n)) {
        return false;
    }
    ct_v3 normal = ct_frames[m.face].normal;
    int u = center.x * normal.x + center.y * normal.y + center.z * normal.z;
    int h = n - 1;
    int lo = h - 2 * (m.depth_to - 1);
    int hi = h - 2 * (m.depth_from - 1);
    return u >= lo && u <= hi;
}

static Vector3 v3_rotate(Vector3 v, Vector3 axis, float rad) {
    float c = cosf(rad);
    float s = sinf(rad);
    float d = axis.x * v.x + axis.y * v.y + axis.z * v.z;
    Vector3 cross = {axis.y * v.z - axis.z * v.y, axis.z * v.x - axis.x * v.z,
                     axis.x * v.y - axis.y * v.x};
    Vector3 r;
    r.x = v.x * c + cross.x * s + axis.x * d * (1.0f - c);
    r.y = v.y * c + cross.y * s + axis.y * d * (1.0f - c);
    r.z = v.z * c + cross.z * s + axis.z * d * (1.0f - c);
    return r;
}

static int emit_quads(const ct_renderer *r, const ct_cube *c, const ct_color_scheme *cs,
                      ct_sticker_quad *out, int cap, bool rotate, ct_move m, float angle_rad) {
    if (r == NULL || c == NULL || cs == NULL || out == NULL || cap <= 0 || r->n != c->n) {
        return 0;
    }
    if (c->n < 2 || c->n > CT_CUBE_MAX_N) {
        return 0;
    }
    float inset = clampf(r->sticker_inset, 0.0f, CT_RENDERER_INSET_MAX);
    float step = r->cube_size / (2.0f * (float)(c->n - 1));
    float half = step * (1.0f - inset);
    float outset = 2.0f * step * CT_RENDERER_OUTSET_FRACTION;
    Vector3 axis = {0};
    if (rotate) {
        ct_v3 normal = ct_frames[m.face].normal;
        if (normal.x != 0) {
            axis = (Vector3){1.0f, 0.0f, 0.0f};
        } else if (normal.y != 0) {
            axis = (Vector3){0.0f, 1.0f, 0.0f};
        } else {
            axis = (Vector3){0.0f, 0.0f, 1.0f};
        }
    }
    int count = 0;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        const ct_face_frame *fr = &ct_frames[face];
        for (int row = 0; row < c->n; row++) {
            for (int col = 0; col < c->n; col++) {
                if (count >= cap) {
                    return count;
                }
                int value =
                    c->facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col];
                if (value >= CT_CUBE_FACE_COUNT) {
                    return -1;
                }
                ct_v3 lattice;
                ct_v3 normal;
                ct_sticker_world(c->n, face, row, col, &lattice, &normal);
                Vector3 center = {(float)lattice.x * step + (float)fr->normal.x * outset,
                                  (float)lattice.y * step + (float)fr->normal.y * outset,
                                  (float)lattice.z * step + (float)fr->normal.z * outset};
                Vector3 tangent_row = {(float)fr->row.x, (float)fr->row.y, (float)fr->row.z};
                Vector3 tangent_col = {(float)fr->col.x, (float)fr->col.y, (float)fr->col.z};
                if (rotate && ct_renderer_slab_contains(c->n, m, lattice)) {
                    center = v3_rotate(center, axis, angle_rad);
                    tangent_row = v3_rotate(tangent_row, axis, angle_rad);
                    tangent_col = v3_rotate(tangent_col, axis, angle_rad);
                }
                ct_sticker_quad *q = &out[count];
                static const float corner_signs[CT_QUAD_CORNER_COUNT][2] = {
                    {-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
                for (int k = 0; k < CT_QUAD_CORNER_COUNT; k++) {
                    float sx = corner_signs[k][0];
                    float sy = corner_signs[k][1];
                    q->pos[k].x = center.x + tangent_row.x * half * sx + tangent_col.x * half * sy;
                    q->pos[k].y = center.y + tangent_row.y * half * sx + tangent_col.y * half * sy;
                    q->pos[k].z = center.z + tangent_row.z * half * sx + tangent_col.z * half * sy;
                }
                q->col = cs->face[value];
                count++;
            }
        }
    }
    return count;
}

int ct_renderer_quads(const ct_renderer *r, const ct_cube *c, const ct_color_scheme *cs,
                      ct_sticker_quad *out, int cap) {
    ct_move none = {0};
    return emit_quads(r, c, cs, out, cap, false, none, 0.0f);
}

void ct_renderer_draw(const ct_renderer *r, const ct_cube *base, const ct_anim *anim,
                      const ct_color_scheme *cs, Camera3D cam, double move_seconds) {
    if (r == NULL || base == NULL || cs == NULL) {
        return;
    }
    ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];
    bool rotate = false;
    ct_move m = {0};
    float angle_rad = 0.0f;
    if (anim != NULL && ct_anim_active(anim)) {
        m = anim->moves[anim->head];
        if (ct_move_legal(&m, r->n)) {
            double eased = ct_ease_cubic_in_out(anim->elapsed / move_seconds);
            ct_v3 normal = ct_frames[m.face].normal;
            int sign = (normal.x != 0) ? normal.x : ((normal.y != 0) ? normal.y : normal.z);
            double degrees =
                eased * (double)m.quarter * CT_RENDERER_RIGHT_ANGLE_DEGREES * (double)(-sign);
            angle_rad = (float)(degrees * CT_RENDERER_PI / CT_RENDERER_STRAIGHT_ANGLE_DEGREES);
            rotate = true;
        }
    }
    int count = emit_quads(r, base, cs, quads, CT_RENDERER_MAX_QUADS, rotate, m, angle_rad);
    if (count <= 0) {
        return;
    }
    int vcount = count * CT_MESH_VERTS_PER_QUAD;
    float *verts = malloc(sizeof(float) * (size_t)vcount * CT_MESH_FLOATS_PER_VERT);
    unsigned char *cols = malloc(sizeof(unsigned char) * (size_t)vcount * CT_MESH_BYTES_PER_COLOR);
    if (verts == NULL || cols == NULL) {
        free(verts);
        free(cols);
        return;
    }
    static const int corner_order[CT_MESH_VERTS_PER_QUAD] = {
        CT_QUAD_CORNER_ROW_MIN_COL_MIN, CT_QUAD_CORNER_ROW_MAX_COL_MIN,
        CT_QUAD_CORNER_ROW_MAX_COL_MAX, CT_QUAD_CORNER_ROW_MIN_COL_MIN,
        CT_QUAD_CORNER_ROW_MAX_COL_MAX, CT_QUAD_CORNER_ROW_MIN_COL_MAX};
    int vi = 0;
    int ci = 0;
    for (int i = 0; i < count; i++) {
        const ct_sticker_quad *q = &quads[i];
        for (int k = 0; k < CT_MESH_VERTS_PER_QUAD; k++) {
            Vector3 p = q->pos[corner_order[k]];
            verts[vi++] = p.x;
            verts[vi++] = p.y;
            verts[vi++] = p.z;
            cols[ci++] = q->col.r;
            cols[ci++] = q->col.g;
            cols[ci++] = q->col.b;
            cols[ci++] = q->col.a;
        }
    }
    Mesh mesh = {0};
    mesh.vertexCount = vcount;
    mesh.triangleCount = count * 2;
    mesh.vertices = verts;
    mesh.colors = cols;
    UploadMesh(&mesh, true);
    Matrix identity = {.m0 = 1.0f, .m5 = 1.0f, .m10 = 1.0f, .m15 = 1.0f};
    Material material = LoadMaterialDefault();
    BeginMode3D(cam);
    DrawMesh(mesh, material, identity);
    EndMode3D();
    UnloadMaterial(material);
    UnloadMesh(mesh);
}
