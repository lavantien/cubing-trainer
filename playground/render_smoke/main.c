#include "engine/cube.h"
#include "render/anim.h"
#include "render/renderer.h"

#include "raylib.h"

enum {
    CT_SMOKE_WIDTH = 960,
    CT_SMOKE_HEIGHT = 540,
    CT_SMOKE_FPS = 60,
    CT_SMOKE_SECONDS = 3,
    CT_SMOKE_N = 3,
    CT_SMOKE_MOVES = 3
};

#define CT_SMOKE_MID_SHOT_AT 0.4
#define CT_SMOKE_MID_SHOT "build/render_smoke_mid.png"
#define CT_SMOKE_END_SHOT "build/render_smoke_end.png"

int main(void) {
    InitWindow(CT_SMOKE_WIDTH, CT_SMOKE_HEIGHT, "ct render smoke");
    SetTargetFPS(CT_SMOKE_FPS);

    ct_cube base;
    ct_cube_init(&base, CT_SMOKE_N);
    ct_renderer renderer;
    ct_renderer_init(&renderer, CT_SMOKE_N);
    ct_color_scheme scheme = ct_color_scheme_default();

    Camera3D camera = {
        .position = {4.5f, 4.5f, 4.5f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    ct_move script[CT_SMOKE_MOVES] = {
        {CT_FACE_R, 1, 1, 1},
        {CT_FACE_U, 1, 1, 1},
        {CT_FACE_R, 1, 1, 3},
    };
    ct_anim anim;
    ct_anim_init(&anim);
    for (int i = 0; i < CT_SMOKE_MOVES; i++) {
        ct_anim_push(&anim, script[i]);
    }
    double move_seconds = (double)CT_RENDERER_MOVE_MILLIS / 1000.0;

    double start = GetTime();
    bool mid_shot = false;
    bool end_shot = false;
    while (!WindowShouldClose() && GetTime() - start < (double)CT_SMOKE_SECONDS) {
        double elapsed = GetTime() - start;
        ct_anim_step_result step = ct_anim_update(&anim, GetFrameTime(), move_seconds);
        if (step.just_completed) {
            ct_apply(&base, step.move);
        }
        BeginDrawing();
        ClearBackground(RAYWHITE);
        ct_renderer_draw(&renderer, &base, &anim, &scheme, camera);
        EndDrawing();
        if (!mid_shot && elapsed >= CT_SMOKE_MID_SHOT_AT) {
            TakeScreenshot(CT_SMOKE_MID_SHOT);
            mid_shot = true;
        }
        if (!end_shot && elapsed >= (double)CT_SMOKE_SECONDS - 0.2) {
            TakeScreenshot(CT_SMOKE_END_SHOT);
            end_shot = true;
        }
    }

    CloseWindow();
    return 0;
}
