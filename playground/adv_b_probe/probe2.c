#include "engine/cube.h"
#include "gen/interchange.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t a[294];
    uint8_t b[294];
    printf("== slice duplication matrix at n=3 ==\n");
    for (int q = 1; q <= 3; q++) {
        ct_sticker_perm((ct_move){CT_FACE_L, 2, 2, q}, 3, a);
        ct_sticker_perm((ct_move){CT_FACE_R, 2, 2, q}, 3, b);
        printf("q=%d: {L,2,2,%d} vs {R,2,2,%d} identical=%d\n", q, q, q,
               memcmp(a, b, 294) == 0);
    }
    ct_sticker_perm((ct_move){CT_FACE_L, 2, 2, 3}, 3, a);
    ct_sticker_perm((ct_move){CT_FACE_R, 2, 2, 1}, 3, b);
    printf("cross: {L,2,2,3} vs {R,2,2,1} identical=%d\n", memcmp(a, b, 294) == 0);
    ct_sticker_perm((ct_move){CT_FACE_L, 2, 2, 1}, 3, a);
    ct_sticker_perm((ct_move){CT_FACE_R, 2, 2, 3}, 3, b);
    printf("cross: {L,2,2,1} vs {R,2,2,3} identical=%d\n", memcmp(a, b, 294) == 0);

    printf("== distinct permutations among the 54 n=3 candidates ==\n");
    static ct_move cands[CT_INTERCHANGE_MAX_CANDIDATES];
    static uint8_t perms[CT_INTERCHANGE_MAX_CANDIDATES][294];
    int n = 3;
    int count = ct_candidate_moves(n, cands, CT_INTERCHANGE_MAX_CANDIDATES);
    for (int i = 0; i < count; i++) {
        ct_sticker_perm(cands[i], n, perms[i]);
    }
    int distinct = 0;
    for (int i = 0; i < count; i++) {
        bool first = true;
        for (int j = 0; j < i; j++) {
            if (memcmp(perms[i], perms[j], 294) == 0) {
                first = false;
                break;
            }
        }
        if (first) {
            distinct++;
        } else {
            for (int j = 0; j < i; j++) {
                if (memcmp(perms[i], perms[j], 294) == 0) {
                    printf("dup: ");
                    ct_move m = cands[i];
                    ct_move o = cands[j];
                    printf("{%d,%d,%d,%d} == {%d,%d,%d,%d}\n", m.face, m.depth_from, m.depth_to,
                           m.quarter, o.face, o.depth_from, o.depth_to, o.quarter);
                    break;
                }
            }
        }
    }
    printf("candidates=%d distinct permutations=%d\n", count, distinct);

    printf("== exact interchange count for edge (2,22) at n=3 ==\n");
    ct_move res[CT_INTERCHANGE_MAX_CANDIDATES];
    int cnt = ct_interchanges_for(CT_TARGET_ORBIT_EDGE, 2, 22, 3, res, CT_INTERCHANGE_MAX_CANDIDATES);
    printf("cnt=%d:", cnt);
    for (int i = 0; i < cnt; i++) {
        printf(" {%d,%d,%d,%d}", res[i].face, res[i].depth_from, res[i].depth_to, res[i].quarter);
    }
    printf("\n");
    int cdup = 0;
    for (int i = 0; i < cnt; i++) {
        ct_sticker_perm(res[i], 3, a);
        for (int j = 0; j < i; j++) {
            ct_sticker_perm(res[j], 3, b);
            if (memcmp(a, b, 294) == 0) {
                cdup++;
            }
        }
    }
    printf("pairwise duplicate perms among results: %d\n", cdup);
    return 0;
}
