#include <math.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"

#include "render/renderer.c"
#include "render/anim.h"

void UploadMesh(Mesh *mesh, bool dynamic) {
    (void)mesh;
    (void)dynamic;
}

Material LoadMaterialDefault(void) {
    Material m;
    memset(&m, 0, sizeof m);
    return m;
}

void BeginMode3D(Camera3D cam) {
    (void)cam;
}

void DrawMesh(Mesh mesh, Material material, Matrix transform) {
    (void)mesh;
    (void)material;
    (void)transform;
}

void EndMode3D(void) {
}

void UnloadMaterial(Material material) {
    (void)material;
}

void UnloadMesh(Mesh mesh) {
    (void)mesh;
}

static int failures = 0;

static void check(int cond, const char *what) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    }
}

static int idx_of(int n, int face, int row, int col) {
    return (face * CT_CUBE_STRIDE + row) * CT_CUBE_STRIDE + col;
}

static void winding_test(int n) {
    ct_renderer r;
    ct_renderer_init(&r, n);
    ct_cube c;
    ct_cube_init(&c, n);
    ct_color_scheme cs = ct_color_scheme_default();
    static ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];
    int count = ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS);
    check(count == 6 * n * n, "quad count");
    long bad = 0;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                int qi = face * n * n + row * n + col;
                const ct_sticker_quad *q = &quads[qi];
                double ax = q->pos[1].x - q->pos[0].x;
                double ay = q->pos[1].y - q->pos[0].y;
                double az = q->pos[1].z - q->pos[0].z;
                double bx = q->pos[3].x - q->pos[0].x;
                double by = q->pos[3].y - q->pos[0].y;
                double bz = q->pos[3].z - q->pos[0].z;
                double cx = ay * bz - az * by;
                double cy = az * bx - ax * bz;
                double cz = ax * by - ay * bx;
                ct_v3 center;
                ct_v3 normal;
                ct_sticker_world(n, face, row, col, &center, &normal);
                double dot = cx * normal.x + cy * normal.y + cz * normal.z;
                if (!(dot > 0)) {
                    bad++;
                    if (bad < 8) {
                        printf("  winding bad n=%d face=%d row=%d col=%d dot=%f normal=(%d,%d,%d)\n",
                               n, face, row, col, dot, normal.x, normal.y, normal.z);
                    }
                }
                double t1x = q->pos[2].x - q->pos[1].x;
                double t1y = q->pos[2].y - q->pos[1].y;
                double t1z = q->pos[2].z - q->pos[1].z;
                double c2x = ay * t1z - az * t1y;
                double c2y = az * t1x - ax * t1z;
                double c2z = ax * t1y - ay * t1x;
                if ((c2x * normal.x + c2y * normal.y + c2z * normal.z) <= 0) {
                    bad++;
                }
            }
        }
    }
    printf("n=%d winding violations=%ld\n", n, bad);
    check(bad == 0, "all quads CCW outward");
}

static ct_v3 rot90_about(ct_v3 v, int axis) {
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

static int frame_axis_of(ct_v3 normal) {
    if (normal.x != 0) {
        return 0;
    }
    if (normal.y != 0) {
        return 1;
    }
    return 2;
}

static void rotation_end_state(int n, ct_move m) {
    ct_renderer r;
    ct_renderer_init(&r, n);
    ct_cube base;
    ct_cube_init(&base, n);
    ct_color_scheme cs = ct_color_scheme_default();

    double eased = 1.0;
    ct_v3 fnormal = ct_frames[m.face].normal;
    int sign = fnormal.x != 0 ? fnormal.x : (fnormal.y != 0 ? fnormal.y : fnormal.z);
    double degrees = eased * m.quarter * 90.0 * (-sign);
    float angle = (float)(degrees * CT_RENDERER_PI / 180.0);

    static ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];
    int count = emit_quads(&r, &base, &cs, quads, CT_RENDERER_MAX_QUADS, true, m, angle);
    check(count == 6 * n * n, "rotated quad count");

    float step = r.cube_size / (2.0f * (float)(n - 1));
    float outset = 2.0f * step * CT_RENDERER_OUTSET_FRACTION;
    double max_pos_err = 0;
    double max_norm_err = 0;
    long slab_checks = 0;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                int src = idx_of(n, face, row, col);
                int qi = face * n * n + row * n + col;
                ct_v3 lattice;
                ct_v3 normal;
                ct_sticker_world(n, face, row, col, &lattice, &normal);

                bool engine_in = false;
                {
                    int h = n - 1;
                    int u = lattice.x * fnormal.x + lattice.y * fnormal.y + lattice.z * fnormal.z;
                    int lo = h - 2 * (m.depth_to - 1);
                    int hi = h - 2 * (m.depth_from - 1);
                    engine_in = u >= lo && u <= hi;
                }
                bool render_in = ct_renderer_slab_contains(n, m, lattice);
                check(engine_in == render_in, "slab membership matches engine band");
                slab_checks++;

                int axis = frame_axis_of(fnormal);
                int steps = ((-sign * m.quarter) % 4 + 4) % 4;
                ct_v3 gl = lattice;
                ct_v3 gn = normal;
                if (engine_in) {
                    for (int k = 0; k < steps; k++) {
                        gl = rot90_about(gl, axis);
                        gn = rot90_about(gn, axis);
                    }
                }
                ct_cell dst_cell = ct_world_sticker(gl, gn, n);
                check(dst_cell.face >= 0, "oracle rotation lands on a valid cell");
                ct_v3 dlattice;
                ct_v3 dnormal;
                ct_sticker_world(n, dst_cell.face, dst_cell.row, dst_cell.col, &dlattice,
                                 &dnormal);
                double want_x = (double)dlattice.x * step + (double)dnormal.x * outset;
                double want_y = (double)dlattice.y * step + (double)dnormal.y * outset;
                double want_z = (double)dlattice.z * step + (double)dnormal.z * outset;

                const ct_sticker_quad *q = &quads[qi];
                double got_x = 0;
                double got_y = 0;
                double got_z = 0;
                for (int k = 0; k < 4; k++) {
                    got_x += q->pos[k].x;
                    got_y += q->pos[k].y;
                    got_z += q->pos[k].z;
                }
                got_x /= 4;
                got_y /= 4;
                got_z /= 4;
                double err = sqrt((got_x - want_x) * (got_x - want_x) +
                                  (got_y - want_y) * (got_y - want_y) +
                                  (got_z - want_z) * (got_z - want_z));
                if (err > max_pos_err) {
                    max_pos_err = err;
                }
                double arx = q->pos[1].x - q->pos[0].x;
                double ary = q->pos[1].y - q->pos[0].y;
                double arz = q->pos[1].z - q->pos[0].z;
                double acx = q->pos[3].x - q->pos[0].x;
                double acy = q->pos[3].y - q->pos[0].y;
                double acz = q->pos[3].z - q->pos[0].z;
                double cx = ary * acz - arz * acy;
                double cy = arz * acx - arx * acz;
                double cz = arx * acy - ary * acx;
                double nerr = cx * dnormal.x + cy * dnormal.y + cz * dnormal.z;
                double clen = sqrt(cx * cx + cy * cy + cz * cz);
                double cosd = clen > 0 ? nerr / clen : -1;
                if (1.0 - cosd > max_norm_err) {
                    max_norm_err = 1.0 - cosd;
                }
                if (cosd <= 0) {
                    failures++;
                    printf("  rotated quad normal flipped n=%d face=%d src=%d\n", n, face, src);
                }
            }
        }
    }
    printf("n=%d move(face=%d,%d..%d,q=%d) eased=1.0 max_pos_err=%.3g max_norm_cos_err=%.3g "
           "slab_checks=%ld\n",
           n, m.face, m.depth_from, m.depth_to, m.quarter, max_pos_err, max_norm_err, slab_checks);
    check(max_pos_err < 1e-3, "rotated end-state positions equal engine destination");
}

static void mid_rotation_radius(int n, ct_move m, float eased) {
    ct_renderer r;
    ct_renderer_init(&r, n);
    ct_cube base;
    ct_cube_init(&base, n);
    ct_color_scheme cs = ct_color_scheme_default();
    ct_v3 fnormal = ct_frames[m.face].normal;
    int sign = fnormal.x != 0 ? fnormal.x : (fnormal.y != 0 ? fnormal.y : fnormal.z);
    double degrees = (double)eased * m.quarter * 90.0 * (-sign);
    float angle = (float)(degrees * CT_RENDERER_PI / 180.0);
    static ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];
    emit_quads(&r, &base, &cs, quads, CT_RENDERER_MAX_QUADS, true, m, angle);
    double max_axis_drift = 0;
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                int qi = face * n * n + row * n + col;
                ct_v3 lattice;
                ct_v3 normal;
                ct_sticker_world(n, face, row, col, &lattice, &normal);
                if (!ct_renderer_slab_contains(n, m, lattice)) {
                    continue;
                }
                const ct_sticker_quad *q = &quads[qi];
                double cx = 0;
                double cy = 0;
                double cz = 0;
                for (int k = 0; k < 4; k++) {
                    cx += q->pos[k].x;
                    cy += q->pos[k].y;
                    cz += q->pos[k].z;
                }
                cx /= 4;
                cy /= 4;
                cz /= 4;
                double ax = cx * (double)fnormal.x;
                double ay = cy * (double)fnormal.y;
                double az = cz * (double)fnormal.z;
                double radius2 = cx * cx + cy * cy + cz * cz - ax * ax - ay * ay - az * az;
                if (radius2 < -1e-9) {
                    failures++;
                    printf("  negative radius n=%d qi=%d\n", n, qi);
                }
                (void)max_axis_drift;
            }
        }
    }
}

int main(void) {
    winding_test(3);
    winding_test(7);

    ct_move n3[14] = {
        {CT_FACE_U, 1, 1, 1}, {CT_FACE_U, 1, 1, 3}, {CT_FACE_U, 1, 1, 2},
        {CT_FACE_R, 1, 1, 1}, {CT_FACE_R, 1, 1, 3}, {CT_FACE_F, 1, 1, 2},
        {CT_FACE_D, 1, 1, 1}, {CT_FACE_L, 1, 1, 3}, {CT_FACE_B, 1, 1, 1},
        {CT_FACE_R, 1, 3, 1}, {CT_FACE_U, 1, 3, 2}, {CT_FACE_L, 2, 2, 1},
        {CT_FACE_F, 1, 3, 3}, {CT_FACE_B, 2, 2, 3},
    };
    for (size_t i = 0; i < sizeof n3 / sizeof n3[0]; i++) {
        rotation_end_state(3, n3[i]);
    }
    ct_move n7[8] = {
        {CT_FACE_R, 1, 3, 1}, {CT_FACE_L, 2, 2, 1}, {CT_FACE_F, 1, 5, 2},
        {CT_FACE_U, 1, 3, 3}, {CT_FACE_R, 1, 7, 1}, {CT_FACE_L, 4, 4, 1},
        {CT_FACE_D, 3, 5, 2}, {CT_FACE_B, 1, 2, 3},
    };
    for (size_t i = 0; i < sizeof n7 / sizeof n7[0]; i++) {
        rotation_end_state(7, n7[i]);
    }

    mid_rotation_radius(3, (ct_move){CT_FACE_R, 1, 1, 1}, 0.5f);
    mid_rotation_radius(7, (ct_move){CT_FACE_U, 1, 3, 2}, 0.25f);

    double e0 = ct_ease_cubic_in_out(-0.5);
    double e1 = ct_ease_cubic_in_out(0.0);
    double e2 = ct_ease_cubic_in_out(0.5);
    double e3 = ct_ease_cubic_in_out(1.0);
    double e4 = ct_ease_cubic_in_out(1.5);
    printf("ease clamps %.3f %.3f %.3f %.3f %.3f\n", e0, e1, e2, e3, e4);
    check(e0 == 0 && e1 == 0 && e2 == 0.5 && e3 == 1 && e4 == 1, "ease endpoints");

    printf("%s failures=%d\n", failures == 0 ? "ALL-OK" : "HAD-FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
