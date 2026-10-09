#include "app/headless.h"

#include <string.h>

#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "render/anim.h"
#include "render/renderer.h"

#include "raylib.h"

#define CT_HEADLESS_CAM_X 4.5f
#define CT_HEADLESS_CAM_Y 4.5f
#define CT_HEADLESS_CAM_Z 4.5f
#define CT_HEADLESS_CAM_FOVY 45.0f
#define CT_HEADLESS_WINDOW_TITLE "cubing-trainer headless"

static Camera3D headless_camera(void) {
    Camera3D cam;
    cam.position.x = CT_HEADLESS_CAM_X;
    cam.position.y = CT_HEADLESS_CAM_Y;
    cam.position.z = CT_HEADLESS_CAM_Z;
    cam.target.x = 0.0f;
    cam.target.y = 0.0f;
    cam.target.z = 0.0f;
    cam.up.x = 0.0f;
    cam.up.y = 1.0f;
    cam.up.z = 0.0f;
    cam.fovy = CT_HEADLESS_CAM_FOVY;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}

static double headless_move_seconds(void) {
    return (double)CT_RENDERER_MOVE_MILLIS /
           (CT_DECIMAL_BASE * CT_DECIMAL_BASE * CT_DECIMAL_BASE);
}

int ct_headless_scene_id(const char *name) {
    if (name == NULL) {
        return CT_HEADLESS_E_ARG;
    }
    if (strcmp(name, "solved") == 0) {
        return CT_HEADLESS_SCENE_SOLVED;
    }
    if (strcmp(name, "scramble") == 0) {
        return CT_HEADLESS_SCENE_SCRAMBLE;
    }
    if (strcmp(name, "mid") == 0) {
        return CT_HEADLESS_SCENE_MID;
    }
    return CT_HEADLESS_E_ARG;
}

static int apply_alg(ct_cube *cube, const char *alg) {
    ct_move moves[CT_ANIM_QUEUE_CAP];
    int count = ct_notation_parse(alg, cube->n, moves, CT_ANIM_QUEUE_CAP);
    if (count < 0) {
        return CT_HEADLESS_E_ALG;
    }
    for (int i = 0; i < count; i++) {
        ct_apply(cube, moves[i]);
    }
    return CT_HEADLESS_OK;
}

int ct_headless_render_scene(int scene, int n, const char *alg, const char *out_path) {
    if (out_path == NULL || out_path[0] == '\0') {
        return CT_HEADLESS_E_ARG;
    }
    if (scene != CT_HEADLESS_SCENE_SOLVED && scene != CT_HEADLESS_SCENE_SCRAMBLE &&
        scene != CT_HEADLESS_SCENE_MID) {
        return CT_HEADLESS_E_ARG;
    }
    if (n < 2 || n > CT_CUBE_MAX_N) {
        return CT_HEADLESS_E_SIZE;
    }

    ct_cube cube;
    ct_cube_init(&cube, n);
    ct_anim anim;
    ct_anim_init(&anim);
    const ct_anim *anim_view = NULL;

    if (scene == CT_HEADLESS_SCENE_SCRAMBLE) {
        const char *scramble = (alg != NULL && alg[0] != '\0') ? alg : CT_HEADLESS_SCRAMBLE_ALG;
        int rc = apply_alg(&cube, scramble);
        if (rc != CT_HEADLESS_OK) {
            return rc;
        }
    }

    if (scene == CT_HEADLESS_SCENE_MID) {
        if (alg != NULL && alg[0] != '\0') {
            int rc = apply_alg(&cube, alg);
            if (rc != CT_HEADLESS_OK) {
                return rc;
            }
        }
        ct_move move;
        int rc = ct_notation_parse_one(CT_HEADLESS_MID_MOVE, n, &move);
        if (rc != 0) {
            return CT_HEADLESS_E_ALG;
        }
        ct_anim_push(&anim, move);
        anim.elapsed = CT_HEADLESS_MID_PROGRESS * headless_move_seconds();
        anim_view = &anim;
    }

    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(CT_HEADLESS_WIDTH, CT_HEADLESS_HEIGHT, CT_HEADLESS_WINDOW_TITLE);
    if (!IsWindowReady()) {
        return CT_HEADLESS_E_WINDOW;
    }

    RenderTexture2D target = LoadRenderTexture(CT_HEADLESS_WIDTH, CT_HEADLESS_HEIGHT);
    ct_renderer renderer;
    ct_renderer_init(&renderer, n);
    ct_color_scheme scheme = ct_color_scheme_default();
    Camera3D cam = headless_camera();

    BeginTextureMode(target);
    ClearBackground(RAYWHITE);
    ct_renderer_draw(&renderer, &cube, anim_view, &scheme, cam, headless_move_seconds());
    EndTextureMode();

    Image frame = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&frame);
    bool written = ExportImage(frame, out_path);
    UnloadImage(frame);
    UnloadRenderTexture(target);
    CloseWindow();

    return written ? CT_HEADLESS_OK : CT_HEADLESS_E_WRITE;
}
