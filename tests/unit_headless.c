#include <stdio.h>

#include "../src/app/headless.h"
#include "../src/config/config.h"
#include "../src/engine/cube.h"
#include "framework.h"

#define CT_HEADLESS_OUT_DIR "build/test_headless"

CT_SUITE(headless_scene_id) {
    CT_ASSERT_EQ(ct_headless_scene_id(NULL), CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_scene_id(""), CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_scene_id("solved"), CT_HEADLESS_SCENE_SOLVED);
    CT_ASSERT_EQ(ct_headless_scene_id("scramble"), CT_HEADLESS_SCENE_SCRAMBLE);
    CT_ASSERT_EQ(ct_headless_scene_id("mid"), CT_HEADLESS_SCENE_MID);
    CT_ASSERT_EQ(ct_headless_scene_id("Solved"), CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_scene_id("solvedx"), CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_scene_id("mids"), CT_HEADLESS_E_ARG);
}

CT_SUITE(headless_render_guards) {
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SOLVED, 3, NULL, NULL),
                 CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SOLVED, 3, NULL, ""),
                 CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_render_scene(99, 3, NULL, "out.png"), CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_render_scene(-1, 3, NULL, "out.png"), CT_HEADLESS_E_ARG);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SOLVED, 1, NULL, "out.png"),
                 CT_HEADLESS_E_SIZE);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SOLVED, CT_CUBE_MAX_N + 1, NULL,
                                          "out.png"),
                 CT_HEADLESS_E_SIZE);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SCRAMBLE, 3, "zz q", "out.png"),
                 CT_HEADLESS_E_ALG);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_MID, 3, "7R", "out.png"),
                 CT_HEADLESS_E_ALG);
}

CT_SUITE(headless_render_smoke) {
    const char *solved_png = CT_HEADLESS_OUT_DIR "/solved.png";
    const char *scramble_png = CT_HEADLESS_OUT_DIR "/scramble.png";
    const char *mid_png = CT_HEADLESS_OUT_DIR "/mid.png";
    const char *big_png = CT_HEADLESS_OUT_DIR "/big.png";
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SOLVED, 2, NULL, solved_png),
                 CT_HEADLESS_OK);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SCRAMBLE, 3, NULL, scramble_png),
                 CT_HEADLESS_OK);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_MID, 3, NULL, mid_png),
                 CT_HEADLESS_OK);
    CT_ASSERT_EQ(ct_headless_render_scene(CT_HEADLESS_SCENE_SOLVED, CT_CUBE_MAX_N, NULL, big_png),
                 CT_HEADLESS_OK);
    const char *pngs[] = {solved_png, scramble_png, mid_png, big_png};
    for (unsigned i = 0; i < sizeof pngs / sizeof pngs[0]; i++) {
        FILE *f = fopen(pngs[i], "rb");
        CT_ASSERT(f != NULL);
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            CT_ASSERT(ftell(f) > 1000);
            fclose(f);
            CT_ASSERT_EQ(remove(pngs[i]), 0);
        }
    }
}
