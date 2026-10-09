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

static const char *ORIENTATION_PNG = "build/headless_spike_orientation.png";
static const char *CUBE_PNG = "build/headless_spike_cube.png";

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

static bool render_orientation_frame(Image *out) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawRectangle(0, 0, CT_SPIKE_W, CT_SPIKE_H / 2, RED);
    DrawRectangle(0, CT_SPIKE_H / 2, CT_SPIKE_W, CT_SPIKE_H / 2, BLUE);
    EndDrawing();
    *out = LoadImageFromScreen();
    ImageFlipVertical(out);
    return out->data != NULL && out->width == CT_SPIKE_W && out->height == CT_SPIKE_H;
}

static bool render_cube_frame(Image *out) {
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
    BeginDrawing();
    ClearBackground(RAYWHITE);
    ct_renderer_draw(&renderer, &cube, NULL, &scheme, camera);
    EndDrawing();
    *out = LoadImageFromScreen();
    ImageFlipVertical(out);
    return out->data != NULL;
}

static bool render_builtins_frame(Image *out) {
    Camera3D camera = {
        .position = {4.5f, 4.5f, 4.5f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);
    DrawCube((Vector3){0.0f, 0.0f, 0.0f}, 2.0f, 2.0f, 2.0f, RED);
    DrawCubeWires((Vector3){0.0f, 0.0f, 0.0f}, 2.0f, 2.0f, 2.0f, BLACK);
    DrawCube((Vector3){-1.2f, -1.2f, 3.0f}, 0.8f, 0.8f, 0.8f, BLUE);
    EndMode3D();
    EndDrawing();
    *out = LoadImageFromScreen();
    ImageFlipVertical(out);
    return out->data != NULL;
}

int main(void) {
    InitWindow(CT_SPIKE_W, CT_SPIKE_H, "ct headless spike");

    Image builtins;
    if (render_builtins_frame(&builtins)) {
        ExportImage(builtins, "build/headless_spike_builtins.png");
        UnloadImage(builtins);
    } else {
        fprintf(stderr, "FAIL builtins readback\n");
    }

    BeginDrawing();
    ClearBackground(MAGENTA);
    EndDrawing();
    Image clear_probe = LoadImageFromScreen();
    printf("clear probe center=%08x corner=%08x\n", pixel_at(&clear_probe, CT_SPIKE_W / 2, CT_SPIKE_H / 2),
           pixel_at(&clear_probe, 5, 5));
    UnloadImage(clear_probe);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawRectangle(0, 0, CT_SPIKE_W, CT_SPIKE_H, RED);
    EndDrawing();
    Image rect_probe = LoadImageFromScreen();
    printf("rect probe center=%08x corner=%08x\n", pixel_at(&rect_probe, CT_SPIKE_W / 2, CT_SPIKE_H / 2),
           pixel_at(&rect_probe, 5, 5));
    UnloadImage(rect_probe);

    Image orientation;
    if (!render_orientation_frame(&orientation)) {
        fprintf(stderr, "FAIL orientation readback\n");
        CloseWindow();
        return 1;
    }
    uint32_t top = pixel_at(&orientation, CT_SPIKE_W / 2, 10);
    uint32_t bottom = pixel_at(&orientation, CT_SPIKE_W / 2, CT_SPIKE_H - 10);
    printf("orientation top=%08x bottom=%08x\n", top, bottom);
    bool top_red = ((top >> 24) & 0xff) > 200 && ((top >> 16) & 0xff) < 80 &&
                   ((top >> 8) & 0xff) < 80;
    bool bottom_blue = ((bottom >> 24) & 0xff) < 80 && ((bottom >> 8) & 0xff) > 200;
    ExportImage(orientation, ORIENTATION_PNG);
    for (int y = 0; y < CT_SPIKE_H; y += CT_SPIKE_H / 10) {
        uint32_t mid = pixel_at(&orientation, CT_SPIKE_W / 2, y);
        printf("probe y=%d %08x\n", y, mid);
    }
    if (!top_red || !bottom_blue) {
        fprintf(stderr, "WARN orientation colors off (shapes path)\n");
    }
    UnloadImage(orientation);

    Image first;
    if (!render_cube_frame(&first)) {
        fprintf(stderr, "FAIL cube readback\n");
        CloseWindow();
        return 1;
    }
    uint64_t digest_first = image_digest(&first);
    ExportImage(first, CUBE_PNG);
    UnloadImage(first);

    Image second;
    if (!render_cube_frame(&second)) {
        fprintf(stderr, "FAIL cube second readback\n");
        CloseWindow();
        return 1;
    }
    uint64_t digest_second = image_digest(&second);
    UnloadImage(second);

    printf("cube digest first=%016llx second=%016llx\n", (unsigned long long)digest_first,
           (unsigned long long)digest_second);
    if (digest_first != digest_second) {
        fprintf(stderr, "FAIL nondeterministic cube frames\n");
        CloseWindow();
        return 1;
    }

    CloseWindow();
    printf("SPIKE PASS software memory platform windowless render readback ok\n");
    return 0;
}
