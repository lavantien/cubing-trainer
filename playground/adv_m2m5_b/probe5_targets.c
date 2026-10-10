#include <stdio.h>
#include <string.h>

#include "gen/targets.h"

int main(void) {
    int corner_hist[64] = {0};
    int edge_hist[64] = {0};
    int distinct_corner = 0;
    int distinct_edge = 0;
    for (int s = 0; s < 24; s++) {
        int pc = ct_corner_piece_of(s);
        int pe = ct_edge_piece_of(s);
        if (pc < 0 || pc >= 64 || pe < 0 || pe >= 64) {
            printf("out of range piece sticker=%d corner=%d edge=%d\n", s, pc, pe);
            return 1;
        }
        if (corner_hist[pc] == 0) {
            distinct_corner++;
        }
        if (edge_hist[pe] == 0) {
            distinct_edge++;
        }
        corner_hist[pc]++;
        edge_hist[pe]++;
    }
    int corner_ok = distinct_corner == 8;
    int edge_ok = distinct_edge == 12;
    for (int i = 0; i < 64; i++) {
        if (corner_hist[i] != 0 && corner_hist[i] != 3) {
            corner_ok = 0;
        }
        if (edge_hist[i] != 0 && edge_hist[i] != 2) {
            edge_ok = 0;
        }
    }
    printf("corner_pieces=%d all_size3=%d edge_pieces=%d all_size2=%d\n", distinct_corner, corner_ok,
           distinct_edge, edge_ok);

    int buffer_corner_piece = ct_corner_piece_of(0);
    int buffer_edge_piece = ct_edge_piece_of(2);
    int buf_corner_members = 0;
    int buf_edge_members = 0;
    for (int s = 0; s < 24; s++) {
        if (ct_corner_piece_of(s) == buffer_corner_piece) {
            buf_corner_members++;
        }
        if (ct_edge_piece_of(s) == buffer_edge_piece) {
            buf_edge_members++;
        }
    }
    printf("buffer0_members=%d buffer2_members=%d\n", buf_corner_members, buf_edge_members);

    static ct_target_pair cp[24 * 24];
    static ct_target_pair ep[24 * 24];
    int nc = ct_targets_corner(0, cp, 24 * 24);
    int ne = ct_targets_edge(2, ep, 24 * 24);
    printf("corner_targets=%d expect=378 edge_targets=%d expect=440\n", nc, ne);

    static char seen_c[24][24];
    static char seen_e[24][24];
    int bad_c = 0;
    int bad_e = 0;
    int asymmetric_c = 0;
    int asymmetric_e = 0;
    for (int i = 0; i < nc; i++) {
        seen_c[cp[i].t1][cp[i].t2] = 1;
        if (ct_corner_piece_of(cp[i].t1) == ct_corner_piece_of(cp[i].t2) ||
            ct_corner_piece_of(cp[i].t1) == buffer_corner_piece ||
            ct_corner_piece_of(cp[i].t2) == buffer_corner_piece) {
            bad_c++;
        }
    }
    for (int i = 0; i < ne; i++) {
        seen_e[ep[i].t1][ep[i].t2] = 1;
        if (ct_edge_piece_of(ep[i].t1) == ct_edge_piece_of(ep[i].t2) ||
            ct_edge_piece_of(ep[i].t1) == buffer_edge_piece ||
            ct_edge_piece_of(ep[i].t2) == buffer_edge_piece) {
            bad_e++;
        }
    }
    for (int a = 0; a < 24; a++) {
        for (int b = 0; b < 24; b++) {
            if (seen_c[a][b] != seen_c[b][a]) {
                asymmetric_c++;
            }
            if (seen_e[a][b] != seen_e[b][a]) {
                asymmetric_e++;
            }
        }
    }
    printf("buffer_or_same_piece_violations corner=%d edge=%d order_asymmetry corner=%d edge=%d\n",
           bad_c, bad_e, asymmetric_c, asymmetric_e);

    int cap_probe = ct_targets_corner(0, NULL, 0);
    printf("null_out_corner_total=%d\n", cap_probe);
    return 0;
}
