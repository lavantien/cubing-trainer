#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/synth.h"
#include "gen/targets.h"

static const char *face_chars = "ULFRBD";

static const char *const corner_letter[6][4] = {
    {"A", "B", "C", "D"}, {"E", "F", "G", "H"}, {"I", "J", "K", "L"},
    {"M", "N", "O", "P"}, {"Q", "R", "S", "T"}, {"U", "V", "W", "X"},
};

static const char *const edge_letter[6][4] = {
    {"A", "B", "C", "D"}, {"E", "F", "G", "H"}, {"I", "J", "K", "L"},
    {"M", "N", "O", "P"}, {"Q", "R", "S", "T"}, {"U", "V", "W", "X"},
};

static int ring_slot(int normal_face, ct_v3 center) {
    ct_v3 r_vec;
    ct_v3 u_vec;
    if (normal_face == 0) {
        r_vec = (ct_v3){1, 0, 0};
        u_vec = (ct_v3){0, 0, -1};
    } else if (normal_face == 5) {
        r_vec = (ct_v3){1, 0, 0};
        u_vec = (ct_v3){0, 0, 1};
    } else if (normal_face == 1) {
        r_vec = (ct_v3){0, 0, 1};
        u_vec = (ct_v3){0, 1, 0};
    } else if (normal_face == 3) {
        r_vec = (ct_v3){0, 0, -1};
        u_vec = (ct_v3){0, 1, 0};
    } else if (normal_face == 2) {
        r_vec = (ct_v3){1, 0, 0};
        u_vec = (ct_v3){0, 1, 0};
    } else {
        r_vec = (ct_v3){-1, 0, 0};
        u_vec = (ct_v3){0, 1, 0};
    }
    int rc = center.x * r_vec.x + center.y * r_vec.y + center.z * r_vec.z;
    int uc = center.x * u_vec.x + center.y * u_vec.y + center.z * u_vec.z;
    if (rc != 0 && uc != 0) {
        if (rc < 0 && uc > 0) {
            return 0;
        }
        if (rc > 0 && uc > 0) {
            return 1;
        }
        if (rc > 0 && uc < 0) {
            return 2;
        }
        return 3;
    }
    if (rc == 0) {
        return uc > 0 ? 0 : 2;
    }
    return rc > 0 ? 1 : 3;
}

static int speffz_fails = 0;

static void speffz_check(void) {
    for (int orbit = 0; orbit < 2; orbit++) {
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int face = -1;
            int row = -1;
            int col = -1;
            ct_sticker_cell(orbit, s, &face, &row, &col);
            ct_v3 center;
            ct_v3 normal;
            ct_sticker_world(3, face, row, col, &center, &normal);
            int slot = ring_slot(face, center);
            const char *want = orbit == 0 ? corner_letter[face][slot] : edge_letter[face][slot];
            char got = (char)('A' + s);
            if (want[0] != got) {
                printf("SPEFFZ_FAIL orbit=%d sticker=%d face=%d cell=(%d,%d) center=(%d,%d,%d)"
                       " want=%s got=%c\n",
                       orbit, s, face, row, col, center.x, center.y, center.z, want, got);
                speffz_fails++;
            }
        }
    }
    printf("speffz_fails=%d\n", speffz_fails);
}

static void dump_moves(const char *tag, const ct_move *seq, int len) {
    printf("%s len=%d moves=", tag, len);
    for (int i = 0; i < len; i++) {
        printf("[%d,%d,%d,%d]", seq[i].face, seq[i].depth_from, seq[i].depth_to, seq[i].quarter);
        if (i + 1 < len) {
            printf(" ");
        }
    }
    printf("\n");
}

static const char *const fixture_algs[] = {
    "L F' L B2 L' F L B2 L2",
    "R2 B2 R F R' B2 R F' R",
    "R B L B' R' B L' B'",
    "U' B D' B' U B D B'",
    "U B' D' B U' B' D B",
    "U F E F' U' F E' F'",
    "U F E' F' U' F E F'",
    "R' E2 R U' R' E2 R U",
    "L F' L' S' L F L' S",
    "D M2 D' R2 D M2 D' R2",
};

static void fixture_dump(void) {
    for (unsigned i = 0; i < sizeof fixture_algs / sizeof fixture_algs[0]; i++) {
        ct_move seq[32];
        int n = ct_notation_parse(fixture_algs[i], 3, seq, 32);
        if (n < 0) {
            printf("FIXTURE_PARSE_FAIL idx=%u alg=%s rc=%d\n", i, fixture_algs[i], n);
            continue;
        }
        dump_moves("fixture", seq, n);
    }
}

static unsigned long rng_state = 0x9e3779b97f4a7c15UL;

static unsigned long next_u64(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

int main(void) {
    speffz_check();
    fixture_dump();
    if (ct_synth_init() < 0) {
        printf("SYNTH_INIT_FAIL\n");
        return 1;
    }
    printf("table_size=%d\n", ct_synth_table_size());
    ct_config cfg = ct_config_defaults();
    ct_target_pair pairs[CT_TARGET_STICKERS * CT_TARGET_STICKERS];
    int corner_total = ct_targets_corner(0, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
    int edge_total = ct_targets_edge(2, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
    printf("corner_targets=%d edge_targets=%d\n", corner_total, edge_total);
    for (int orbit = 0; orbit < 2; orbit++) {
        int total = orbit == 0 ? ct_targets_corner(0, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS)
                               : ct_targets_edge(2, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
        for (int k = 0; k < total; k++) {
            ct_synth_result res;
            int rc = ct_synth_target(orbit, orbit == 0 ? 0 : 2, pairs[k].t1, pairs[k].t2, &cfg,
                                     &res);
            if (rc != 0 || !res.found) {
                printf("SYNTH_MISS orbit=%d t1=%d t2=%d rc=%d\n", orbit, pairs[k].t1, pairs[k].t2,
                       rc);
                continue;
            }
            printf("SYNTH orbit=%d t1=%d t2=%d setup=%d fallback=%d ", orbit, pairs[k].t1,
                   pairs[k].t2, res.used_setup, res.used_fallback);
            dump_moves("alg", res.seq, res.len);
        }
    }
    return 0;
}
