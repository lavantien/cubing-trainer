#include "framework.h"

#include "../src/render/renderer.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    float x;
    float y;
    float z;
} ct_f3;

static ct_f3 f3_sub(ct_f3 a, ct_f3 b) {
    ct_f3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

static ct_f3 f3_add(ct_f3 a, ct_f3 b) {
    ct_f3 r = {a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}

static ct_f3 f3_scale(ct_f3 a, float k) {
    ct_f3 r = {a.x * k, a.y * k, a.z * k};
    return r;
}

static float f3_dot(ct_f3 a, ct_f3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static ct_f3 f3_cross(ct_f3 a, ct_f3 b) {
    ct_f3 r = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    return r;
}

static float f3_len(ct_f3 a) {
    return sqrtf(f3_dot(a, a));
}

static bool f3_near(ct_f3 a, ct_f3 b, float eps) {
    return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps && fabsf(a.z - b.z) <= eps;
}

static ct_f3 quad_pos(const ct_sticker_quad *q, int i) {
    ct_f3 r = {q->pos[i].x, q->pos[i].y, q->pos[i].z};
    return r;
}

static ct_f3 quad_normal(const ct_sticker_quad *q) {
    ct_f3 n =
        f3_cross(f3_sub(quad_pos(q, 1), quad_pos(q, 0)), f3_sub(quad_pos(q, 3), quad_pos(q, 0)));
    return f3_scale(n, 1.0f / f3_len(n));
}

static ct_f3 quad_center(const ct_sticker_quad *q) {
    return f3_scale(
        f3_add(f3_add(quad_pos(q, 0), quad_pos(q, 2)), f3_add(quad_pos(q, 1), quad_pos(q, 3))),
        0.25f);
}

static float quad_area(const ct_sticker_quad *q) {
    return f3_len(
        f3_cross(f3_sub(quad_pos(q, 1), quad_pos(q, 0)), f3_sub(quad_pos(q, 3), quad_pos(q, 0))));
}

static bool color_eq(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static bool color_distinct(Color a, Color b) {
    return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
}

static float lattice_step(const ct_renderer *r) {
    return r->cube_size / (2.0f * (float)(r->n - 1));
}

static float outset_of(const ct_renderer *r) {
    return CT_RENDERER_OUTSET_FRACTION * 2.0f * lattice_step(r);
}

static int round_component(float v) {
    return (int)floorf(v + 0.5f);
}

static ct_v3 quad_lattice_center(const ct_sticker_quad *q, ct_f3 normal, const ct_renderer *r) {
    float s = lattice_step(r);
    ct_f3 c = f3_sub(quad_center(q), f3_scale(normal, outset_of(r)));
    ct_v3 out = {round_component(c.x / s), round_component(c.y / s), round_component(c.z / s)};
    return out;
}

static ct_v3 int_normal(ct_f3 normal) {
    ct_v3 out = {round_component(normal.x), round_component(normal.y), round_component(normal.z)};
    return out;
}

static ct_v3 face_normal_of(int face, int n) {
    ct_v3 center;
    ct_v3 normal;
    ct_sticker_world(n, face, 0, 0, &center, &normal);
    return normal;
}

static int facelet_at(const ct_cube *c, int face, int row, int col) {
    return c->facelet[face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col];
}

CT_SUITE(unit_renderer_draw_signature) {
    void (*pin)(const ct_renderer *, const ct_cube *, const ct_anim *, const ct_color_scheme *,
                Camera3D, double) = ct_renderer_draw;
    CT_ASSERT(pin == ct_renderer_draw);
}

CT_SUITE(unit_renderer_init) {
    ct_renderer r;
    ct_renderer_init(&r, 3);
    CT_ASSERT_EQ(r.n, 3);
    CT_ASSERT_DBL_EQ(r.cube_size, CT_RENDERER_DEFAULT_CUBE_SIZE, 0.0);
    CT_ASSERT_DBL_EQ(r.sticker_inset, CT_RENDERER_DEFAULT_INSET, 0.0);

    ct_renderer_init(&r, 1);
    CT_ASSERT_EQ(r.n, 2);
    ct_renderer_init(&r, 0);
    CT_ASSERT_EQ(r.n, 2);
    ct_renderer_init(&r, -7);
    CT_ASSERT_EQ(r.n, 2);
    ct_renderer_init(&r, CT_CUBE_MAX_N);
    CT_ASSERT_EQ(r.n, CT_CUBE_MAX_N);
    ct_renderer_init(&r, CT_CUBE_MAX_N + 5);
    CT_ASSERT_EQ(r.n, CT_CUBE_MAX_N);
}

CT_SUITE(unit_renderer_counts) {
    ct_cube c;
    ct_renderer r;
    ct_color_scheme cs = ct_color_scheme_default();
    static ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];

    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        ct_cube_init(&c, n);
        ct_renderer_init(&r, n);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), 6 * n * n);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, 6 * n * n), 6 * n * n);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, 6 * n * n - 1), 6 * n * n - 1);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, 5), 5);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, 1), 1);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, 0), 0);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, NULL, CT_RENDERER_MAX_QUADS), 0);
        CT_ASSERT_EQ(ct_renderer_quads(NULL, &c, &cs, quads, CT_RENDERER_MAX_QUADS), 0);
        CT_ASSERT_EQ(ct_renderer_quads(&r, NULL, &cs, quads, CT_RENDERER_MAX_QUADS), 0);
        CT_ASSERT_EQ(ct_renderer_quads(&r, &c, NULL, quads, CT_RENDERER_MAX_QUADS), 0);
    }

    ct_cube_init(&c, 4);
    ct_renderer_init(&r, 3);
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), 0);
    ct_renderer_init(&r, 4);
    quads[17].col = (Color){201, 202, 203, 204};
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, 17), 17);
    CT_ASSERT(color_eq(quads[17].col, (Color){201, 202, 203, 204}));

    for (int i = 0; i < 6 * 4 * 4; i++) {
        quads[i].col = (Color){210, 211, 212, 213};
    }
    c.facelet[0] = CT_CUBE_FACE_COUNT;
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), -1);
    c.facelet[0] = 255;
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), -1);
    CT_ASSERT(color_eq(quads[10].col, (Color){210, 211, 212, 213}));

    memset(&c, 0, sizeof c);
    c.n = 1;
    r.n = 1;
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), 0);
    c.n = CT_CUBE_MAX_N + 1;
    r.n = CT_CUBE_MAX_N + 1;
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), 0);
}

CT_SUITE(unit_renderer_geometry) {
    ct_cube c;
    ct_renderer r;
    ct_color_scheme cs = ct_color_scheme_default();
    static ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];

    int ns[2] = {3, 5};
    for (int ni = 0; ni < 2; ni++) {
        int n = ns[ni];
        ct_cube_init(&c, n);
        ct_renderer_init(&r, n);
        int count = ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS);
        CT_ASSERT_EQ(count, 6 * n * n);
        float s = lattice_step(&r);
        float half = s * (1.0f - r.sticker_inset);
        float outset = outset_of(&r);
        CT_ASSERT(outset > 0.0f);
        CT_ASSERT(outset < 2.0f * s);
        for (int i = 0; i < count; i++) {
            ct_f3 edge0 = f3_sub(quad_pos(&quads[i], 1), quad_pos(&quads[i], 0));
            ct_f3 edge1 = f3_sub(quad_pos(&quads[i], 3), quad_pos(&quads[i], 0));
            ct_f3 across = f3_sub(quad_pos(&quads[i], 2), quad_pos(&quads[i], 0));
            ct_f3 normal = quad_normal(&quads[i]);
            CT_ASSERT_DBL_EQ(f3_len(edge0), 2.0f * half, 1e-4);
            CT_ASSERT_DBL_EQ(f3_len(edge1), 2.0f * half, 1e-4);
            CT_ASSERT_DBL_EQ(f3_len(across), 2.0f * half * sqrtf(2.0f), 1e-4);
            CT_ASSERT_DBL_EQ(f3_dot(f3_cross(edge0, edge1), normal), f3_len(f3_cross(edge0, edge1)),
                             1e-4);
            CT_ASSERT_DBL_EQ(quad_area(&quads[i]), (2.0f * half) * (2.0f * half), 1e-4);

            ct_v3 normal_i = int_normal(normal);
            CT_ASSERT(fabsf(normal.x - (float)normal_i.x) < 1e-3f);
            CT_ASSERT(fabsf(normal.y - (float)normal_i.y) < 1e-3f);
            CT_ASSERT(fabsf(normal.z - (float)normal_i.z) < 1e-3f);
            ct_v3 lattice = quad_lattice_center(&quads[i], normal, &r);
            ct_cell cell = ct_world_sticker(lattice, normal_i, n);
            CT_ASSERT(cell.face >= 0);
            CT_ASSERT(cell.row >= 0);
            CT_ASSERT(cell.row < n);
            CT_ASSERT(cell.col >= 0);
            CT_ASSERT(cell.col < n);
            ct_v3 want_center;
            ct_v3 want_normal;
            ct_sticker_world(n, cell.face, cell.row, cell.col, &want_center, &want_normal);
            CT_ASSERT_EQ(lattice.x, want_center.x);
            CT_ASSERT_EQ(lattice.y, want_center.y);
            CT_ASSERT_EQ(lattice.z, want_center.z);
            CT_ASSERT_EQ(want_normal.x, normal_i.x);
            CT_ASSERT_EQ(want_normal.y, normal_i.y);
            CT_ASSERT_EQ(want_normal.z, normal_i.z);
            ct_f3 projected = f3_add(
                f3_scale((ct_f3){(float)want_center.x, (float)want_center.y, (float)want_center.z},
                         s),
                f3_scale((ct_f3){(float)want_normal.x, (float)want_normal.y, (float)want_normal.z},
                         outset));
            CT_ASSERT(f3_near(quad_center(&quads[i]), projected, 1e-4f));
            for (int k = 0; k < 4; k++) {
                CT_ASSERT(fabsf(f3_dot(f3_sub(quad_pos(&quads[i], k), projected), normal)) < 1e-4f);
            }
        }
    }

    ct_cube_init(&c, 3);
    ct_renderer_init(&r, 3);
    float s = lattice_step(&r);
    float inset_areas[3];
    float insets[3] = {0.0f, 0.3f, 0.6f};
    for (int k = 0; k < 3; k++) {
        r.sticker_inset = insets[k];
        int count = ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS);
        CT_ASSERT_EQ(count, 54);
        inset_areas[k] = quad_area(&quads[30]);
    }
    CT_ASSERT_DBL_EQ(inset_areas[0], (2.0f * s) * (2.0f * s), 1e-5);
    CT_ASSERT(inset_areas[0] > inset_areas[1]);
    CT_ASSERT(inset_areas[1] > inset_areas[2]);

    r.sticker_inset = -0.5f;
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), 54);
    CT_ASSERT_DBL_EQ(quad_area(&quads[0]), (2.0f * s) * (2.0f * s), 1e-5);
    r.sticker_inset = 3.0f;
    CT_ASSERT_EQ(ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS), 54);
    CT_ASSERT(quad_area(&quads[0]) > 0.0f);
}

CT_SUITE(unit_renderer_corners) {
    ct_cube c;
    ct_renderer r;
    ct_color_scheme cs = ct_color_scheme_default();
    static ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];
    int n = 5;
    ct_cube_init(&c, n);
    ct_renderer_init(&r, n);
    int count = ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS);
    CT_ASSERT_EQ(count, 6 * n * n);
    float half = lattice_step(&r) * (1.0f - r.sticker_inset);

    for (int i = 0; i < count; i += 7) {
        ct_f3 normal = quad_normal(&quads[i]);
        ct_f3 center = quad_center(&quads[i]);
        ct_f3 axes[2];
        int axis_count = 0;
        ct_f3 canonical[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        for (int a = 0; a < 3; a++) {
            if (fabsf(f3_dot(canonical[a], normal)) < 0.5f) {
                axes[axis_count] = canonical[a];
                axis_count++;
            }
        }
        CT_ASSERT_EQ(axis_count, 2);
        int signs[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
        ct_f3 expected[4];
        for (int k = 0; k < 4; k++) {
            expected[k] = f3_add(center, f3_add(f3_scale(axes[0], (float)signs[k][0] * half),
                                                f3_scale(axes[1], (float)signs[k][1] * half)));
        }
        for (int k = 0; k < 4; k++) {
            bool matched = false;
            for (int j = 0; j < 4; j++) {
                if (f3_near(quad_pos(&quads[i], k), expected[j], 1e-4f)) {
                    matched = true;
                }
            }
            CT_ASSERT(matched);
        }
    }
}

CT_SUITE(unit_renderer_colors) {
    ct_color_scheme cs = ct_color_scheme_default();
    CT_ASSERT(color_eq(cs.face[CT_FACE_U], WHITE));
    CT_ASSERT(color_eq(cs.face[CT_FACE_L], ORANGE));
    CT_ASSERT(color_eq(cs.face[CT_FACE_F], GREEN));
    CT_ASSERT(color_eq(cs.face[CT_FACE_R], RED));
    CT_ASSERT(color_eq(cs.face[CT_FACE_B], BLUE));
    CT_ASSERT(color_eq(cs.face[CT_FACE_D], YELLOW));
    for (int a = 0; a < CT_CUBE_FACE_COUNT; a++) {
        for (int b = a + 1; b < CT_CUBE_FACE_COUNT; b++) {
            CT_ASSERT(color_distinct(cs.face[a], cs.face[b]));
        }
    }

    ct_cube c;
    ct_renderer r;
    static ct_sticker_quad quads[CT_RENDERER_MAX_QUADS];
    int n = 4;
    ct_cube_init(&c, n);
    ct_renderer_init(&r, n);
    int count = ct_renderer_quads(&r, &c, &cs, quads, CT_RENDERER_MAX_QUADS);
    CT_ASSERT_EQ(count, 6 * n * n);
    for (int i = 0; i < count; i++) {
        ct_f3 normal = quad_normal(&quads[i]);
        ct_v3 normal_i = int_normal(normal);
        ct_v3 lattice = quad_lattice_center(&quads[i], normal, &r);
        ct_cell cell = ct_world_sticker(lattice, normal_i, n);
        CT_ASSERT(cell.face >= 0);
        CT_ASSERT(color_eq(quads[i].col, cs.face[facelet_at(&c, cell.face, cell.row, cell.col)]));
    }

    ct_color_scheme custom;
    Color palette[6] = {(Color){10, 0, 0, 255},  (Color){0, 20, 0, 255},  (Color){0, 0, 30, 255},
                        (Color){40, 40, 0, 255}, (Color){0, 50, 50, 255}, (Color){60, 0, 60, 255}};
    for (int f = 0; f < CT_CUBE_FACE_COUNT; f++) {
        custom.face[f] = palette[f];
    }
    ct_move scramble[4] = {
        {CT_FACE_R, 1, 1, 1}, {CT_FACE_U, 1, 2, 2}, {CT_FACE_F, 2, 2, 3}, {CT_FACE_D, 1, 1, 2}};
    for (int i = 0; i < 4; i++) {
        ct_apply(&c, scramble[i]);
    }
    CT_ASSERT(!ct_cube_solved(&c));
    count = ct_renderer_quads(&r, &c, &custom, quads, CT_RENDERER_MAX_QUADS);
    CT_ASSERT_EQ(count, 6 * n * n);
    for (int i = 0; i < count; i++) {
        ct_f3 normal = quad_normal(&quads[i]);
        ct_v3 normal_i = int_normal(normal);
        ct_v3 lattice = quad_lattice_center(&quads[i], normal, &r);
        ct_cell cell = ct_world_sticker(lattice, normal_i, n);
        CT_ASSERT(cell.face >= 0);
        CT_ASSERT(
            color_eq(quads[i].col, custom.face[facelet_at(&c, cell.face, cell.row, cell.col)]));
    }
}

CT_SUITE(unit_renderer_scheme_hex) {
    ct_color_scheme cs = ct_color_scheme_default();

    CT_ASSERT(ct_color_scheme_face_hex(&cs, CT_FACE_U, "102030"));
    CT_ASSERT(color_eq(cs.face[CT_FACE_U], (Color){0x10, 0x20, 0x30, 255}));
    CT_ASSERT(ct_color_scheme_face_hex(&cs, CT_FACE_L, "abcdef"));
    CT_ASSERT(color_eq(cs.face[CT_FACE_L], (Color){0xAB, 0xCD, 0xEF, 255}));
    CT_ASSERT(ct_color_scheme_face_hex(&cs, CT_FACE_F, "ABCDEF"));
    CT_ASSERT(color_eq(cs.face[CT_FACE_F], (Color){0xAB, 0xCD, 0xEF, 255}));
    CT_ASSERT(ct_color_scheme_face_hex(&cs, CT_FACE_R, "000000"));
    CT_ASSERT(color_eq(cs.face[CT_FACE_R], (Color){0, 0, 0, 255}));
    CT_ASSERT(ct_color_scheme_face_hex(&cs, CT_FACE_B, "ffffff"));
    CT_ASSERT(color_eq(cs.face[CT_FACE_B], (Color){255, 255, 255, 255}));
    CT_ASSERT(ct_color_scheme_face_hex(&cs, CT_FACE_D, "fF5800"));
    CT_ASSERT(color_eq(cs.face[CT_FACE_D], (Color){0xFF, 0x58, 0x00, 255}));

    static const char *const bad[] = {
        "",       "1",      "12",     "1234",    "12345",  "1234567", "12345678", "12345g",
        "1g3456", "g23456", "12345 ", " 123456", "12-456", "0x1234",  "123456\n",
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        ct_color_scheme before = cs;
        CT_ASSERT(!ct_color_scheme_face_hex(&cs, CT_FACE_U, bad[i]));
        CT_ASSERT(color_eq(cs.face[CT_FACE_U], before.face[CT_FACE_U]));
    }
    CT_ASSERT(!ct_color_scheme_face_hex(&cs, -1, "112233"));
    CT_ASSERT(!ct_color_scheme_face_hex(&cs, CT_CUBE_FACE_COUNT, "112233"));
    CT_ASSERT(!ct_color_scheme_face_hex(NULL, CT_FACE_U, "112233"));
    CT_ASSERT(!ct_color_scheme_face_hex(&cs, CT_FACE_U, NULL));
    CT_ASSERT(color_eq(cs.face[CT_FACE_U], (Color){0x10, 0x20, 0x30, 255}));
    CT_ASSERT(color_eq(cs.face[CT_FACE_L], (Color){0xAB, 0xCD, 0xEF, 255}));
    CT_ASSERT(color_eq(cs.face[CT_FACE_F], (Color){0xAB, 0xCD, 0xEF, 255}));
    CT_ASSERT(color_eq(cs.face[CT_FACE_R], (Color){0, 0, 0, 255}));
    CT_ASSERT(color_eq(cs.face[CT_FACE_B], (Color){255, 255, 255, 255}));
    CT_ASSERT(color_eq(cs.face[CT_FACE_D], (Color){0xFF, 0x58, 0x00, 255}));
    CT_ASSERT(ct_color_scheme_face_hex(&cs, CT_FACE_U, "999999"));
    CT_ASSERT(color_eq(cs.face[CT_FACE_U], (Color){0x99, 0x99, 0x99, 255}));

    for (int f = 0; f < CT_CUBE_FACE_COUNT; f++) {
        CT_ASSERT(ct_color_scheme_keys[f] != NULL);
        CT_ASSERT(ct_color_scheme_keys[f][0] == 'u');
    }
    CT_ASSERT_STR_EQ(ct_color_scheme_keys[CT_FACE_U], "ui.color_u");
    CT_ASSERT_STR_EQ(ct_color_scheme_keys[CT_FACE_L], "ui.color_l");
    CT_ASSERT_STR_EQ(ct_color_scheme_keys[CT_FACE_F], "ui.color_f");
    CT_ASSERT_STR_EQ(ct_color_scheme_keys[CT_FACE_R], "ui.color_r");
    CT_ASSERT_STR_EQ(ct_color_scheme_keys[CT_FACE_B], "ui.color_b");
    CT_ASSERT_STR_EQ(ct_color_scheme_keys[CT_FACE_D], "ui.color_d");
    for (int a = 0; a < CT_CUBE_FACE_COUNT; a++) {
        for (int b = a + 1; b < CT_CUBE_FACE_COUNT; b++) {
            CT_ASSERT(strcmp(ct_color_scheme_keys[a], ct_color_scheme_keys[b]) != 0);
        }
    }
}

CT_SUITE(unit_renderer_slab) {
    int ns[5] = {2, 3, 4, 5, 7};
    for (int ni = 0; ni < 5; ni++) {
        int n = ns[ni];
        int h = n - 1;
        for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
            ct_v3 normal = face_normal_of(face, n);
            for (int from = 1; from <= n; from++) {
                for (int to = from; to <= n; to++) {
                    int lo = h - 2 * (to - 1);
                    int hi = h - 2 * (from - 1);
                    int members = 0;
                    for (int f2 = 0; f2 < CT_CUBE_FACE_COUNT; f2++) {
                        for (int row = 0; row < n; row++) {
                            for (int col = 0; col < n; col++) {
                                ct_v3 center;
                                ct_v3 scratch;
                                ct_sticker_world(n, f2, row, col, &center, &scratch);
                                int u =
                                    center.x * normal.x + center.y * normal.y + center.z * normal.z;
                                bool want = u >= lo && u <= hi;
                                for (int quarter = 1; quarter <= CT_MOVE_QUARTERS - 1; quarter++) {
                                    ct_move m = {face, from, to, quarter};
                                    CT_ASSERT_EQ(ct_renderer_slab_contains(n, m, center), want);
                                }
                                members += want ? 1 : 0;
                            }
                        }
                    }
                    int layers = to - from + 1;
                    int expected_members =
                        4 * n * layers + (from == 1 ? n * n : 0) + (to == n ? n * n : 0);
                    CT_ASSERT_EQ(members, expected_members);
                }
            }
        }
    }
}

CT_SUITE(unit_renderer_slab_invalid) {
    ct_v3 center = {2, 2, 2};
    ct_move bad[8] = {
        {CT_FACE_U, 1, 1, 0}, {CT_FACE_U, 1, 1, CT_MOVE_QUARTERS},
        {CT_FACE_U, 0, 1, 1}, {CT_FACE_U, 1, 0, 1},
        {CT_FACE_U, 2, 1, 1}, {CT_FACE_U, 1, 8, 1},
        {-1, 1, 1, 1},        {CT_CUBE_FACE_COUNT, 1, 1, 1},
    };
    for (int n = 2; n <= CT_CUBE_MAX_N; n++) {
        for (int i = 0; i < 8; i++) {
            CT_ASSERT(!ct_renderer_slab_contains(n, bad[i], center));
        }
        ct_move ok = {CT_FACE_U, 1, 1, 1};
        CT_ASSERT(!ct_renderer_slab_contains(1, ok, center));
        CT_ASSERT(!ct_renderer_slab_contains(0, ok, center));
        CT_ASSERT(!ct_renderer_slab_contains(-3, ok, center));
        CT_ASSERT(!ct_renderer_slab_contains(CT_CUBE_MAX_N + 1, ok, center));
    }

    int n = 3;
    ct_move m = {CT_FACE_R, 1, 1, 1};
    ct_v3 centers[5] = {{2, 2, 2}, {2, 0, 0}, {2, -2, -2}, {0, 2, 0}, {-2, 2, 0}};
    bool want[5] = {true, true, true, false, false};
    for (int i = 0; i < 5; i++) {
        CT_ASSERT_EQ(ct_renderer_slab_contains(n, m, centers[i]), want[i]);
    }
    ct_move minner = {CT_FACE_R, 2, 3, 2};
    bool want_inner[5] = {false, false, false, true, true};
    for (int i = 0; i < 5; i++) {
        CT_ASSERT_EQ(ct_renderer_slab_contains(n, minner, centers[i]), want_inner[i]);
    }
}
