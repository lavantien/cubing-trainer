#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "engine/cube.h"
#include "render/anim.h"
#include "render/renderer.h"

#include "raylib.h"

enum {
    CT_SPIKE_W = 960,
    CT_SPIKE_H = 540,
    CT_SPIKE_N = 3
};

static const char *HIDDEN_ORIENTATION_PNG = "build/headless_hidden_orientation.png";
static const char *HIDDEN_CUBE_PNG = "build/headless_hidden_cube.png";

static uint32_t pixel_at(const Image *img, int x, int y) {
    const unsigned char *p = (const unsigned char *)img->data;
    uint32_t offset = (uint32_t)(y * img->width + x) * 4;
    return ((uint32_t)p[offset] << 24) | ((uint32_t)p[offset + 1] << 16) |
           ((uint32_t)p[offset + 2] << 8) | (uint32_t)p[offset + 3];
}

static uint64_t image_digest(const Image *img) {
    uint64_t h = 1469598103934665603ULL;
    const unsigned char *p = (const unsigned char *)img->data;
    for (uint32_t i = 0; i < (uint32_t)img->width * (uint32_t)img->height * 4; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static bool grab_frame(RenderTexture2D rt, Image *out) {
    *out = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(out);
    return out->data != NULL && out->width == CT_SPIKE_W && out->height == CT_SPIKE_H;
}

static bool render_orientation(RenderTexture2D rt, Image *out) {
    BeginTextureMode(rt);
    ClearBackground(BLACK);
    DrawRectangle(0, 0, CT_SPIKE_W, CT_SPIKE_H / 2, RED);
    DrawRectangle(0, CT_SPIKE_H / 2, CT_SPIKE_W, CT_SPIKE_H / 2, BLUE);
    EndTextureMode();
    return grab_frame(rt, out);
}

static bool render_cube(RenderTexture2D rt, Image *out) {
    ct_cube cube;
    ct_cube_init(&cube, CT_SPIKE_N);
    ct_renderer renderer;
    ct_renderer_init(&renderer, CT_SPIKE_N);
    ct_color_scheme scheme = ct_color_scheme_default();
    Camera3D camera = {
        .position = {4.5f, 4.5f, 4.5f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    BeginTextureMode(rt);
    ClearBackground(RAYWHITE);
    ct_renderer_draw(&renderer, &cube, NULL, &scheme, camera);
    EndTextureMode();
    return grab_frame(rt, out);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(CT_SPIKE_W, CT_SPIKE_H, "ct hidden spike");
    RenderTexture2D rt = LoadRenderTexture(CT_SPIKE_W, CT_SPIKE_H);

    Image orientation;
    if (!render_orientation(rt, &orientation)) {
        fprintf(stderr, "FAIL orientation readback\n");
        UnloadRenderTexture(rt);
        CloseWindow();
        return 1;
    }
    uint32_t top = pixel_at(&orientation, CT_SPIKE_W / 2, 10);
    uint32_t bottom = pixel_at(&orientation, CT_SPIKE_W / 2, CT_SPIKE_H - 10);
    printf("orientation top=%08x bottom=%08x\n", top, bottom);
    ExportImage(orientation, HIDDEN_ORIENTATION_PNG);
    UnloadImage(orientation);
    bool top_red = top == 0xe62937ff;
    bool bottom_blue = bottom == 0x0079f1ff;
    if (!top_red || !bottom_blue) {
        fprintf(stderr, "FAIL orientation colors\n");
        UnloadRenderTexture(rt);
        CloseWindow();
        return 1;
    }

    Image first;
    Image second;
    if (!render_cube(rt, &first) || !render_cube(rt, &second)) {
        fprintf(stderr, "FAIL cube readback\n");
        UnloadRenderTexture(rt);
        CloseWindow();
        return 1;
    }
    uint64_t d1 = image_digest(&first);
    uint64_t d2 = image_digest(&second);
    ExportImage(first, HIDDEN_CUBE_PNG);
    printf("cube digest first=%016llx second=%016llx\n", (unsigned long long)d1,
           (unsigned long long)d2);
    UnloadImage(first);
    UnloadImage(second);
    UnloadRenderTexture(rt);
    CloseWindow();
    if (d1 != d2) {
        fprintf(stderr, "FAIL nondeterministic\n");
        return 1;
    }
    printf("SPIKE PASS hidden window render texture readback ok\n");
    return 0;
}
