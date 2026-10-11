#include "config/config.h"
#include "storage/repo_algs.h"
#include "storage/repo_settings.h"

#include <stdio.h>
#include <string.h>

static void identity_map(ct_letter_map *m) {
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        m->corner[i] = (char)(CT_LETTER_FIRST + i);
        m->edge[i] = (char)(CT_LETTER_FIRST + i);
    }
}

static void hash_of(const ct_config *cfg, int scheme, ct_buffer_mode mode, int bc, int be,
                    const ct_letter_map *m, char out[CT_ALG_HASH_HEX_MAX]) {
    ct_params_hash(cfg, scheme, mode, bc, be, m, out);
}

static int differs(const char *a, const char *b) {
    return strcmp(a, b) != 0;
}

int main(void) {
    ct_config cfg = ct_config_defaults();
    ct_letter_map map;
    identity_map(&map);
    char base[CT_ALG_HASH_HEX_MAX];
    char alt[CT_ALG_HASH_HEX_MAX];
    int fails = 0;

    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, base);
    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("deterministic %s\n", differs(base, alt) ? "FAIL" : "ok");

    hash_of(&cfg, 2, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("scheme_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    hash_of(&cfg, 1, CT_BUFFER_FLOATING, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map,
            alt);
    printf("mode_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER + 1, CT_3STYLE_EDGE_BUFFER, &map,
            alt);
    printf("buffer_corner_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER + 3, &map,
            alt);
    printf("buffer_edge_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config mask = cfg;
    mask.gen.preferred_moves |= (unsigned)CT_MOVE_BIT_B;
    hash_of(&mask, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("mask_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config pen = cfg;
    pen.gen.penalty[4] += 1.0;
    hash_of(&pen, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("penalty_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config fin = cfg;
    fin.gen.finger_cost[2][1] += 0.25;
    hash_of(&fin, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("finger_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config depth = cfg;
    depth.gen.setup_bfs_depth += 1;
    hash_of(&depth, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("setup_depth_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config fb = cfg;
    fb.gen.fallback_depth += 1;
    hash_of(&fb, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("fallback_depth_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config cand = cfg;
    cand.gen.candidate_max += 1;
    hash_of(&cand, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("candidate_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config wl = cfg;
    wl.gen.weight_len += 0.5;
    hash_of(&wl, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("weight_len_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_letter_map map2;
    identity_map(&map2);
    map2.corner[7] = 'Z';
    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map2, alt);
    printf("map_corner_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_letter_map map3;
    identity_map(&map3);
    map3.edge[23] = 'A';
    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map3, alt);
    printf("map_edge_sensitive %s\n", differs(base, alt) ? "ok" : "FAIL");

    ct_config fsrs = cfg;
    fsrs.fsrs.w[3] += 0.5;
    fsrs.fsrs.request_retention = 0.83;
    fsrs.anim.move_seconds += 0.1;
    fsrs.window.target_fps += 7;
    hash_of(&fsrs, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map, alt);
    printf("irrelevant_inert %s\n", differs(base, alt) ? "FAIL" : "ok");

    const char *big = "1.7e308";
    ct_config via_api = ct_config_defaults();
    int api_bad = 0;
    if (ct_config_set(&via_api, "gen.penalty_U", big) != CT_CFG_OK)
        api_bad = 1;
    if (ct_config_set(&via_api, "gen.finger_cost_U_cw", big) != CT_CFG_OK)
        api_bad = 1;
    if (ct_config_set(&via_api, "gen.weight_len", big) != CT_CFG_OK)
        api_bad = 1;
    printf("settings_api_accepts_1p7e308 %s\n", api_bad ? "no" : "yes");

    ct_config extreme = ct_config_defaults();
    for (int i = 0; i < CT_FACE_COUNT; i++)
        extreme.gen.penalty[i] = 1.7e308;
    for (int f = 0; f < CT_FINGER_FACE_COUNT; f++)
        for (int d = 0; d < CT_FINGER_DIR_COUNT; d++)
            extreme.gen.finger_cost[f][d] = 1.7e308;
    extreme.gen.weight_len = 1.7e308;
    extreme.gen.weight_subset = 1.7e308;
    extreme.gen.weight_regrip = 1.7e308;
    extreme.gen.weight_finger = 1.7e308;

    char est[512];
    int n = snprintf(est, sizeof est, "%d|%d|%d|%d|%x|", 1, 0, 2, 2,
                     (unsigned)extreme.gen.preferred_moves);
    long total = n;
    for (int i = 0; i < CT_FACE_COUNT; i++)
        total += 316 + (i > 0 ? 1 : 0);
    for (int k = 0; k < CT_FINGER_FACE_COUNT * CT_FINGER_DIR_COUNT; k++)
        total += 316 + (k > 0 ? 1 : 0);
    total += 3 * 11 + 4 * (316 + 1) + 7;
    total += 1 + CT_LETTER_STICKERS + 1 + CT_LETTER_STICKERS;
    printf("modeled_worst_case_bytes %ld canon_max %d\n", total, CT_ALG_CANON_MAX);
    (void)est;

    ct_letter_map map_a;
    ct_letter_map map_b;
    identity_map(&map_a);
    identity_map(&map_b);
    map_b.corner[11] = 'Y';
    map_b.corner[12] = 'L';
    char ha[CT_ALG_HASH_HEX_MAX];
    char hb[CT_ALG_HASH_HEX_MAX];
    hash_of(&extreme, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map_a,
            ha);
    hash_of(&extreme, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map_b,
            hb);
    printf("extreme_map_aliasing %s\n", differs(ha, hb) ? "none" : "ALIASED");

    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map_a, ha);
    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &map_b, hb);
    printf("default_map_aliasing %s\n", differs(ha, hb) ? "none" : "ALIASED");

    ct_letter_map holed;
    memset(&holed, 0, sizeof holed);
    hash_of(&cfg, 1, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, &holed, alt);
    printf("holed_map_differs %s\n", differs(base, alt) ? "ok" : "FAIL");

    char empty[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, 2, 2, NULL, empty);
    printf("null_map_hash_len %d\n", (int)strlen(empty));
    (void)fails;
    return 0;
}
