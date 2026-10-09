#ifndef CT_RENDERER_H
#define CT_RENDERER_H

#include "raylib.h"

#include "config/config.h"
#include "engine/cube.h"
#include "render/anim.h"

enum {
    CT_RENDERER_MAX_QUADS = CT_CUBE_FACE_COUNT * CT_CUBE_MAX_N * CT_CUBE_MAX_N,
    CT_MESH_BUFFER_POSITION = 0,
    CT_MESH_BUFFER_COLOR = 3,
    CT_MESH_VERTS_PER_QUAD = 6
};

typedef struct {
    Color face[CT_CUBE_FACE_COUNT];
} ct_color_scheme;

ct_color_scheme ct_color_scheme_default(void);

typedef struct {
    Vector3 pos[4];
    Color col;
} ct_sticker_quad;

typedef struct {
    int n;
    float cube_size;
    float sticker_inset;
} ct_renderer;

void ct_renderer_init(ct_renderer *r, int n);
int ct_renderer_quads(const ct_renderer *r, const ct_cube *c, const ct_color_scheme *cs,
                      ct_sticker_quad *out, int cap);
bool ct_renderer_slab_contains(int n, ct_move m, ct_v3 center);
void ct_renderer_draw(const ct_renderer *r, const ct_cube *base, const ct_anim *anim,
                      const ct_color_scheme *cs, Camera3D cam, double move_seconds);

#endif
