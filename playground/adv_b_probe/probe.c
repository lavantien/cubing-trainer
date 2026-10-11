#include "engine/cube.h"
#include "gen/interchange.h"
#include "gen/targets.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int comp(int n, int f, int r, int c) {
    return f * n * n + r * n + c;
}

static void print_move(ct_move m) {
    printf("{face=%d from=%d to=%d q=%d}", m.face, m.depth_from, m.depth_to, m.quarter);
}

int main(void) {
    uint8_t perm[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE];
    uint8_t other[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE];

    printf("== 1. direction probe: single quarter R on n=3 ==\n");
    ct_move r1 = {CT_FACE_R, 1, 1, 1};
    printf("ret=%d\n", ct_sticker_perm(r1, 3, perm));
    printf("perm[U(0,2)]=%d : src->dst expects 42 (B(2,0)), dst->src expects 20 (F(0,2))\n",
           perm[comp(3, CT_FACE_U, 0, 2)]);
    printf("perm[F(0,2)]=%d : src->dst expects 2 (U(0,2)), dst->src expects 47 (D(0,2))\n",
           perm[comp(3, CT_FACE_F, 0, 2)]);
    printf("perm[B(2,0)]=%d : src->dst expects 47 (D(0,2)), dst->src expects 2 (U(0,2))\n",
           perm[comp(3, CT_FACE_B, 2, 0)]);

    printf("== 2. direction probe: single quarter U on n=3 ==\n");
    ct_move u1 = {CT_FACE_U, 1, 1, 1};
    ct_sticker_perm(u1, 3, perm);
    printf("perm[F(0,0)]=%d : src->dst expects 9 (L(0,0)), dst->src expects 11 (L(0,2))\n",
           perm[comp(3, CT_FACE_F, 0, 0)]);
    printf("perm[L(0,0)]=%d : src->dst expects 18 (B(0,2))\n", perm[comp(3, CT_FACE_L, 0, 0)]);

    printf("== 3. interchanges_for n sweep, corner pair (0,2) ==\n");
    ct_move res[CT_INTERCHANGE_MAX_CANDIDATES];
    for (int n = 3; n <= 6; n++) {
        int cnt = ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, n, res,
                                      CT_INTERCHANGE_MAX_CANDIDATES);
        printf("n=%d cnt=%d:", n, cnt);
        for (int i = 0; i < cnt && i < 12; i++) {
            printf(" ");
            print_move(res[i]);
        }
        printf("\n");
    }

    printf("== 4. interchanges_for n sweep, edge pair (1,21) ==\n");
    for (int n = 3; n <= 6; n++) {
        int cnt = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 1, 21, n, res,
                                      CT_INTERCHANGE_MAX_CANDIDATES);
        printf("n=%d cnt=%d:", n, cnt);
        for (int i = 0; i < cnt && i < 12; i++) {
            printf(" ");
            print_move(res[i]);
        }
        printf("\n");
    }

    printf("== 5. what cells do corner/edge stickers map to at n=4,5,6 ==\n");
    int stickers[] = {0, 1, 2, 3, 9, 21};
    for (int i = 0; i < 6; i++) {
        int f = 0;
        int r = 0;
        int c = 0;
        ct_sticker_cell(CT_TARGET_ORBIT_CORNER, stickers[i], &f, &r, &c);
        printf("corner sticker %d -> face %d row %d col %d |", stickers[i], f, r, c);
        for (int n = 3; n <= 6; n++) {
            printf(" n=%d: is_edge_cell_of_face=%d is_center_region=%d", n,
                   (r == 0 || r == n - 1) != (c == 0 || c == n - 1),
                   (r >= 1 && r <= n - 2 && c >= 1 && c <= n - 2));
        }
        printf("\n");
    }

    printf("== 6. duplicate candidate permutations at odd n ==\n");
    ct_move m_l2 = {CT_FACE_L, 2, 2, 2};
    ct_move m_r2 = {CT_FACE_R, 2, 2, 2};
    ct_sticker_perm(m_l2, 3, perm);
    ct_sticker_perm(m_r2, 3, other);
    printf("n=3 {L,2,2,2} vs {R,2,2,2} identical perm: %d\n", memcmp(perm, other, 294) == 0);
    ct_move m_l1 = {CT_FACE_L, 2, 2, 1};
    ct_move m_r1s = {CT_FACE_R, 2, 2, 1};
    ct_sticker_perm(m_l1, 3, perm);
    ct_sticker_perm(m_r1s, 3, other);
    printf("n=3 {L,2,2,1} vs {R,2,2,1} identical perm: %d\n", memcmp(perm, other, 294) == 0);
    ct_move l5 = {CT_FACE_L, 3, 3, 2};
    ct_move r5 = {CT_FACE_R, 3, 3, 2};
    ct_sticker_perm(l5, 5, perm);
    ct_sticker_perm(r5, 5, other);
    printf("n=5 {L,3,3,2} vs {R,3,3,2} identical perm: %d\n", memcmp(perm, other, 294) == 0);
    ct_move l4 = {CT_FACE_L, 2, 2, 2};
    ct_move r4 = {CT_FACE_R, 2, 2, 2};
    ct_sticker_perm(l4, 4, perm);
    ct_sticker_perm(r4, 4, other);
    printf("n=4 {L,2,2,2} vs {R,2,2,2} identical perm: %d\n", memcmp(perm, other, 294) == 0);

    printf("== 7. semantic check n=5 edge interchange (1,21): which cells swap ==\n");
    {
        int f1 = 0;
        int r1c = 0;
        int c1 = 0;
        int f2 = 0;
        int r2c = 0;
        int c2 = 0;
        ct_sticker_cell(CT_TARGET_ORBIT_EDGE, 1, &f1, &r1c, &c1);
        ct_sticker_cell(CT_TARGET_ORBIT_EDGE, 21, &f2, &r2c, &c2);
        ct_v3 cen;
        ct_v3 nor;
        ct_sticker_world(5, f1, r1c, c1, &cen, &nor);
        printf("edge sticker 1 at n=5 -> (%d,%d,%d) center=(%d,%d,%d) normal=(%d,%d,%d)\n", f1, r1c,
               c1, cen.x, cen.y, cen.z, nor.x, nor.y, nor.z);
        ct_sticker_world(5, f2, r2c, c2, &cen, &nor);
        printf("edge sticker 21 at n=5 -> (%d,%d,%d) center=(%d,%d,%d) normal=(%d,%d,%d)\n", f2,
               r2c, c2, cen.x, cen.y, cen.z, nor.x, nor.y, nor.z);
        int nonzero = 0;
        for (int a = 0; a < CT_TARGET_STICKERS; a++) {
            for (int b = a + 1; b < CT_TARGET_STICKERS; b++) {
                int cnt = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, a, b, 5, res,
                                              CT_INTERCHANGE_MAX_CANDIDATES);
                if (cnt > 0) {
                    nonzero++;
                    if (nonzero <= 4) {
                        printf("edge pair (%d,%d) n=5 cnt=%d first=", a, b, cnt);
                        print_move(res[0]);
                        printf("\n");
                    }
                }
                cnt = ct_interchanges_for(CT_TARGET_ORBIT_CORNER, a, b, 5, res,
                                          CT_INTERCHANGE_MAX_CANDIDATES);
                if (cnt > 0 && nonzero < 100 && a == 0) {
                    printf("corner pair (%d,%d) n=5 cnt=%d\n", a, b, cnt);
                }
            }
        }
        printf("edge pairs with nonzero interchanges at n=5: %d of 276\n", nonzero);
    }

    printf("== 8. rejection and cap re-checks ==\n");
    printf("x==y: %d\n", ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 5, 5, 3, res, 54));
    printf("orbit 2: %d\n", ct_interchanges_for(2, 0, 2, 3, res, 54));
    printf("n=2: %d\n", ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 2, res, 54));
    printf("n=7: %d\n", ct_interchanges_for(CT_TARGET_ORBIT_CORNER, 0, 2, 7, res, 54));
    printf("perm q=0: %d\n", ct_sticker_perm((ct_move){CT_FACE_R, 1, 1, 0}, 3, perm));
    printf("perm q=4: %d\n", ct_sticker_perm((ct_move){CT_FACE_R, 1, 1, 4}, 3, perm));
    printf("perm face 6: %d\n", ct_sticker_perm((ct_move){6, 1, 1, 1}, 3, perm));
    printf("perm face -1: %d\n", ct_sticker_perm((ct_move){-1, 1, 1, 1}, 3, perm));
    printf("perm depth_from 0: %d\n", ct_sticker_perm((ct_move){CT_FACE_R, 0, 1, 1}, 3, perm));
    printf("perm from>to: %d\n", ct_sticker_perm((ct_move){CT_FACE_R, 2, 1, 1}, 3, perm));
    printf("perm depth_to 4: %d\n", ct_sticker_perm((ct_move){CT_FACE_R, 1, 4, 1}, 3, perm));
    printf("perm n=5: %d\n", ct_sticker_perm((ct_move){CT_FACE_R, 1, 1, 1}, 5, perm));
    printf("perm n=6: %d\n", ct_sticker_perm((ct_move){CT_FACE_R, 1, 1, 1}, 6, perm));
    printf("whole cube {U,1,3,2} n=3: %d\n", ct_sticker_perm((ct_move){CT_FACE_U, 1, 3, 2}, 3, perm));
    bool bij = true;
    bool seen[294] = {false};
    for (int i = 0; i < 54; i++) {
        if (seen[perm[i]]) {
            bij = false;
        }
        seen[perm[i]] = true;
    }
    printf("whole cube perm bijection over 54: %d perm[U(0,0)]=%d\n", bij,
           perm[comp(3, CT_FACE_U, 0, 0)]);

    printf("== 9. cap partial write integrity n=5 edge (1,21) ==\n");
    ct_move guard[8];
    for (int i = 0; i < 8; i++) {
        guard[i] = (ct_move){-9, -9, -9, -9};
    }
    int cnt = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 1, 21, 5, guard, 2);
    printf("cnt=%d guard[2].face=%d guard[3].face=%d\n", cnt, guard[2].face, guard[3].face);
    int cnt0 = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 1, 21, 5, NULL, 0);
    printf("null/0 cnt=%d equal=%d\n", cnt0, cnt0 == cnt);
    return 0;
}
