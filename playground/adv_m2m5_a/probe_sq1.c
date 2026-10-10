#include <stdio.h>
#include <string.h>

#include "engine/geometry.h"
#include "engine/geometry_clock.h"
#include "engine/geometry_sq1.h"

static const char *const sq1_bad[] = {
    "(1,0)x", "x(1,0)", "(1,0", "1,0)", "(,)", "(,0)", "(1,)", "()", "( 1,0)",
    "(+1,0)", "(01,0)", "(00,0)", "(0,01)", "(1,0))", "((1,0)", "(1, 0)", "(1,0) ",
    " (1,0)", "(1,0))x", "/", "//", "(0,0)", "(7,0)", "(-6,0)", "(0,-6)", "(0,7)",
    "(1000000000000,0)", "(1,1000000000000)", "(2147483648,0)", "(4294967296,0)",
    "(99999999999999999999,0)", "(-99999999999,0)", "(2147483647,0)", "(6,6)",
    "(+0,+0)", "(1,0)x)", "(1,-0)", "(-0,0)",
};

static const char *const clock_bad[] = {
    "UR6-", "UR7+", "UR0-", "UR", "UR+", "UR++", "U+", "ALL", "ALL6-", "y", "y2 ",
    "Y2", "URR5+", "UR5", "UR5+x", "xUR5+", "ALL7+", "U6-", "U0-", "", "ur5+",
    "UR5", "U-5", "5+", "UR05+", "UR5 ", " UR5+", "UL5+ ", "y22", "y2x",
};

static int sq1_direction_demo(void) {
    const ct_geometry *g = ct_geometry_sq1();
    ct_sq1_state s;
    ct_sq1_state_solved(&s);
    int id = ct_sq1_move_id(1, 0);
    if (ct_sq1_apply(&s, id) != 0) {
        return -1;
    }
    uint8_t perm[CT_SQ1_STICKERS];
    if (g->move_facelets(g, id, CT_GEOMETRY_SINGLE_N, perm) != 0) {
        return -2;
    }
    uint8_t facelet_dst_src[CT_SQ1_STICKERS];
    uint8_t facelet_src_dst[CT_SQ1_STICKERS];
    uint8_t next[CT_SQ1_STICKERS];
    for (int i = 0; i < CT_SQ1_STICKERS; i++) {
        facelet_dst_src[i] = (uint8_t)i;
        facelet_src_dst[i] = (uint8_t)i;
    }
    for (int i = 0; i < CT_SQ1_STICKERS; i++) {
        next[perm[i]] = facelet_src_dst[i];
    }
    memcpy(facelet_src_dst, next, sizeof next);
    for (int i = 0; i < CT_SQ1_STICKERS; i++) {
        next[i] = facelet_dst_src[perm[i]];
    }
    memcpy(facelet_dst_src, next, sizeof next);
    static const uint8_t solved[CT_SQ1_LAYERS * CT_SQ1_UNITS] = {
        0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 15,
    };
    int match_src_dst = 0;
    int match_dst_src = 0;
    for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
        for (int u = 0; u < CT_SQ1_UNITS; u++) {
            int cell = layer * CT_SQ1_UNITS + u;
            if (s.units[cell] != solved[facelet_src_dst[cell]]) {
                match_src_dst = -1;
            }
            if (s.units[cell] != solved[facelet_dst_src[cell]]) {
                match_dst_src = -1;
            }
        }
    }
    printf("sq1 perm convention: perm[src]=dst matches state: %d, perm[dst]=src matches: %d\n",
           match_src_dst == 0, match_dst_src == 0);
    int nxn_match = -1;
    const ct_geometry *nxn = ct_geometry_nxn();
    uint8_t nxn_perm[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE];
    ct_move u = {0, 1, 1, 1};
    extern int ct_sticker_perm(ct_move m, int n, uint8_t perm[]);
    if (nxn->move_facelets(nxn, 0, 3, nxn_perm) == 0 && ct_sticker_perm(u, 3, nxn_perm) == 0) {
        nxn_match = 0;
    }
    printf("nxn driver agrees with ct_sticker_perm (perm[dst]=src): %d\n", nxn_match == 0);
    return 0;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const ct_geometry *sq = ct_geometry_sq1();
    const ct_geometry *ck = ct_geometry_clock();
    printf("== sq1 tokens ==\n");
    for (unsigned i = 0; i < sizeof sq1_bad / sizeof sq1_bad[0]; i++) {
        ct_move m = {9, 9, 9, 9};
        bool overflow_token = strlen(sq1_bad[i]) > 8;
        if (overflow_token && (argc < 2 || strcmp(argv[1], "overflow") != 0)) {
            printf("sq1 token=[%s] skipped (run with overflow arg)\n", sq1_bad[i]);
            continue;
        }
        int id = sq->move_from_token(sq, sq1_bad[i], CT_GEOMETRY_SINGLE_N, &m);
        printf("sq1 token=[%s] id=%d m=(%d,%d,%d,%d)\n", sq1_bad[i], id, m.face, m.depth_from,
               m.depth_to, m.quarter);
    }
    printf("== clock tokens ==\n");
    for (unsigned i = 0; i < sizeof clock_bad / sizeof clock_bad[0]; i++) {
        ct_move m = {9, 9, 9, 9};
        int id = ck->move_from_token(ck, clock_bad[i], CT_GEOMETRY_SINGLE_N, &m);
        printf("clock token=[%s] id=%d m=(%d,%d,%d,%d)\n", clock_bad[i], id, m.face, m.depth_from,
               m.depth_to, m.quarter);
    }
    printf("== clock state ==\n");
    ct_clock_state cs;
    ct_clock_state_solved(&cs);
    int id_all6 = ct_clock_move_id(CT_CLOCK_ALL, 6);
    ct_clock_apply(&cs, id_all6);
    printf("after ALL6+: ");
    for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
        printf("%d ", cs.clocks[i]);
    }
    printf("flipped=%d\n", cs.flipped);
    ct_clock_apply(&cs, ct_clock_move_id(CT_CLOCK_ALL, -5));
    printf("after ALL5-:  ");
    for (int i = 0; i < CT_CLOCK_CLOCKS; i++) {
        printf("%d ", cs.clocks[i]);
    }
    printf("\n");
    ct_clock_apply(&cs, CT_CLOCK_Y2);
    ct_clock_apply(&cs, CT_CLOCK_Y2);
    printf("after y2x2 solved=%d\n", memcmp(&cs, &(ct_clock_state){0}, sizeof cs) == 0);
    int rc = sq1_direction_demo();
    printf("direction_demo rc=%d\n", rc);
    return 0;
}
