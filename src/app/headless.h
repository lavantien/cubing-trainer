#ifndef CT_HEADLESS_H
#define CT_HEADLESS_H

enum {
    CT_HEADLESS_WIDTH = 960,
    CT_HEADLESS_HEIGHT = 540,
    CT_HEADLESS_OK = 0,
    CT_HEADLESS_E_ARG = -1,
    CT_HEADLESS_E_SIZE = -2,
    CT_HEADLESS_E_ALG = -3,
    CT_HEADLESS_E_WINDOW = -4,
    CT_HEADLESS_E_WRITE = -5
};

enum { CT_HEADLESS_SCENE_SOLVED, CT_HEADLESS_SCENE_SCRAMBLE, CT_HEADLESS_SCENE_MID };

#define CT_HEADLESS_SCRAMBLE_ALG "R U R' U R U2 R'"
#define CT_HEADLESS_MID_MOVE "R"
#define CT_HEADLESS_MID_PROGRESS 0.5

int ct_headless_scene_id(const char *name);
int ct_headless_render_scene(int scene, int n, const char *alg, const char *out_path);

#endif
