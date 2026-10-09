#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/headless.h"

static int parse_int(const char *text, int *out) {
    if (text == NULL || *text == '\0') {
        return -1;
    }
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return -1;
    }
    *out = (int)value;
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s <scene> <n> [alg] <out.png>\n", argv0);
    fprintf(stderr, "scenes: solved scramble mid\n");
    fprintf(stderr, "example: %s scramble 3 \"%s\" out.png\n", argv0, CT_HEADLESS_SCRAMBLE_ALG);
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 5) {
        usage(argv[0]);
        return 1;
    }
    int scene = ct_headless_scene_id(argv[1]);
    if (scene < 0) {
        fprintf(stderr, "error: unknown scene '%s'\n", argv[1]);
        usage(argv[0]);
        return 1;
    }
    int n = 0;
    if (parse_int(argv[2], &n) != 0) {
        fprintf(stderr, "error: bad cube size '%s'\n", argv[2]);
        return 1;
    }
    const char *alg = NULL;
    const char *out_path = NULL;
    if (argc == 4) {
        out_path = argv[3];
    } else {
        alg = argv[3];
        out_path = argv[4];
    }
    int rc = ct_headless_render_scene(scene, n, alg, out_path);
    if (rc != CT_HEADLESS_OK) {
        fprintf(stderr, "error: ct_headless_render_scene failed with %d\n", rc);
        return 1;
    }
    printf("%s\n", out_path);
    return 0;
}
