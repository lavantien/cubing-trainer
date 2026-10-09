#include "framework.h"
#include "gen/targets.h"

#include <stdbool.h>

static const int corner_expected[CT_TARGET_STICKERS] = {
    1, 0, 2, 3, 1, 3, 7, 5, 3, 2, 6, 7, 2, 0, 4, 6, 0, 1, 5, 4, 7, 6, 4, 5,
};

static const int edge_expected[CT_TARGET_STICKERS] = {
    0, 1, 2, 3, 3, 7, 11, 4, 2, 6, 10, 7, 1, 5, 9, 6, 0, 4, 8, 5, 10, 9, 8, 11,
};

static bool pair_present(const ct_target_pair *pairs, int n, int t1, int t2) {
    for (int i = 0; i < n; i++) {
        if (pairs[i].t1 == t1 && pairs[i].t2 == t2) {
            return true;
        }
    }
    return false;
}

CT_SUITE(unit_targets) {
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        CT_ASSERT_EQ(ct_corner_piece_of(s), corner_expected[s]);
        CT_ASSERT_EQ(ct_edge_piece_of(s), edge_expected[s]);
    }
    CT_ASSERT_EQ(ct_corner_piece_of(-1), CT_TARGET_NONE);
    CT_ASSERT_EQ(ct_corner_piece_of(CT_TARGET_STICKERS), CT_TARGET_NONE);
    CT_ASSERT_EQ(ct_edge_piece_of(-5), CT_TARGET_NONE);
    CT_ASSERT_EQ(ct_edge_piece_of(999), CT_TARGET_NONE);

    int corner_pieces[CT_TARGET_CORNER_PIECES] = {0};
    int edge_pieces[CT_TARGET_EDGE_PIECES] = {0};
    unsigned corner_buffer_mask = 0;
    unsigned edge_buffer_mask = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        corner_pieces[ct_corner_piece_of(s)]++;
        edge_pieces[ct_edge_piece_of(s)]++;
        if (ct_corner_piece_of(s) == ct_corner_piece_of(0)) {
            corner_buffer_mask |= 1u << s;
        }
        if (ct_edge_piece_of(s) == ct_edge_piece_of(2)) {
            edge_buffer_mask |= 1u << s;
        }
    }
    for (int p = 0; p < CT_TARGET_CORNER_PIECES; p++) {
        CT_ASSERT_EQ(corner_pieces[p], 3);
    }
    for (int p = 0; p < CT_TARGET_EDGE_PIECES; p++) {
        CT_ASSERT_EQ(edge_pieces[p], 2);
    }
    CT_ASSERT_EQ(corner_buffer_mask, (1u << 0) | (1u << 4) | (1u << 17));
    CT_ASSERT_EQ(edge_buffer_mask, (1u << 2) | (1u << 8));

    static ct_target_pair corner_pairs[378];
    static ct_target_pair edge_pairs[440];
    CT_ASSERT_EQ(ct_targets_corner(0, NULL, 0), 378);
    CT_ASSERT_EQ(ct_targets_edge(2, NULL, 0), 440);
    CT_ASSERT_EQ(ct_targets_corner(0, corner_pairs, 378), 378);
    CT_ASSERT_EQ(ct_targets_edge(2, edge_pairs, 440), 440);

    int corner_t1[CT_TARGET_STICKERS] = {0};
    int edge_t1[CT_TARGET_STICKERS] = {0};
    for (int i = 0; i < 378; i++) {
        CT_ASSERT(corner_pairs[i].t1 >= 0 && corner_pairs[i].t1 < CT_TARGET_STICKERS);
        CT_ASSERT(corner_pairs[i].t2 >= 0 && corner_pairs[i].t2 < CT_TARGET_STICKERS);
        CT_ASSERT(ct_corner_piece_of(corner_pairs[i].t1) != ct_corner_piece_of(0));
        CT_ASSERT(ct_corner_piece_of(corner_pairs[i].t2) != ct_corner_piece_of(0));
        CT_ASSERT(ct_corner_piece_of(corner_pairs[i].t1) != ct_corner_piece_of(corner_pairs[i].t2));
        corner_t1[corner_pairs[i].t1]++;
    }
    for (int i = 0; i < 440; i++) {
        CT_ASSERT(edge_pairs[i].t1 >= 0 && edge_pairs[i].t1 < CT_TARGET_STICKERS);
        CT_ASSERT(edge_pairs[i].t2 >= 0 && edge_pairs[i].t2 < CT_TARGET_STICKERS);
        CT_ASSERT(ct_edge_piece_of(edge_pairs[i].t1) != ct_edge_piece_of(2));
        CT_ASSERT(ct_edge_piece_of(edge_pairs[i].t2) != ct_edge_piece_of(2));
        CT_ASSERT(ct_edge_piece_of(edge_pairs[i].t1) != ct_edge_piece_of(edge_pairs[i].t2));
        edge_t1[edge_pairs[i].t1]++;
    }
    int corner_t1_distinct = 0;
    int edge_t1_distinct = 0;
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int corner_want = ct_corner_piece_of(s) == ct_corner_piece_of(0) ? 0 : 18;
        int edge_want = ct_edge_piece_of(s) == ct_edge_piece_of(2) ? 0 : 20;
        CT_ASSERT_EQ(corner_t1[s], corner_want);
        CT_ASSERT_EQ(edge_t1[s], edge_want);
        if (corner_t1[s] > 0) {
            corner_t1_distinct++;
        }
        if (edge_t1[s] > 0) {
            edge_t1_distinct++;
        }
    }
    CT_ASSERT_EQ(corner_t1_distinct, 21);
    CT_ASSERT_EQ(edge_t1_distinct, 22);

    for (int i = 1; i < 378; i++) {
        bool ok = corner_pairs[i].t1 > corner_pairs[i - 1].t1 ||
                  (corner_pairs[i].t1 == corner_pairs[i - 1].t1 &&
                   corner_pairs[i].t2 > corner_pairs[i - 1].t2);
        CT_ASSERT(ok);
    }
    for (int i = 1; i < 440; i++) {
        bool ok =
            edge_pairs[i].t1 > edge_pairs[i - 1].t1 ||
            (edge_pairs[i].t1 == edge_pairs[i - 1].t1 && edge_pairs[i].t2 > edge_pairs[i - 1].t2);
        CT_ASSERT(ok);
    }

    static ct_target_pair alt_corner[378];
    static ct_target_pair alt_edge[440];
    CT_ASSERT_EQ(ct_targets_corner(2, alt_corner, 378), 378);
    CT_ASSERT_EQ(ct_targets_edge(16, alt_edge, 440), 440);
    for (int i = 0; i < 378; i++) {
        CT_ASSERT(alt_corner[i].t1 != 2 && alt_corner[i].t1 != 9 && alt_corner[i].t1 != 12);
        CT_ASSERT(alt_corner[i].t2 != 2 && alt_corner[i].t2 != 9 && alt_corner[i].t2 != 12);
    }
    for (int i = 0; i < 440; i++) {
        CT_ASSERT(alt_edge[i].t1 != 0 && alt_edge[i].t1 != 16);
        CT_ASSERT(alt_edge[i].t2 != 0 && alt_edge[i].t2 != 16);
    }
    CT_ASSERT_EQ(alt_corner[0].t1, 0);
    CT_ASSERT_EQ(alt_corner[0].t2, 1);
    CT_ASSERT(pair_present(alt_corner, 378, 0, 3));
    CT_ASSERT(!pair_present(alt_corner, 378, 0, 9));
    CT_ASSERT(!pair_present(alt_corner, 378, 0, 4));
    CT_ASSERT_EQ(alt_edge[0].t1, 1);
    CT_ASSERT_EQ(alt_edge[0].t2, 2);
    CT_ASSERT(pair_present(alt_edge, 440, 1, 3));
    CT_ASSERT(!pair_present(alt_edge, 440, 1, 12));
    CT_ASSERT(!pair_present(alt_edge, 440, 5, 11));

    static ct_target_pair capped[8];
    for (int i = 0; i < 8; i++) {
        capped[i].t1 = -99;
        capped[i].t2 = -99;
    }
    CT_ASSERT_EQ(ct_targets_corner(0, capped, 5), 378);
    for (int i = 0; i < 5; i++) {
        CT_ASSERT_EQ(capped[i].t1, corner_pairs[i].t1);
        CT_ASSERT_EQ(capped[i].t2, corner_pairs[i].t2);
    }
    CT_ASSERT_EQ(capped[5].t1, -99);
    CT_ASSERT_EQ(capped[5].t2, -99);
    for (int i = 0; i < 8; i++) {
        capped[i].t1 = -99;
        capped[i].t2 = -99;
    }
    CT_ASSERT_EQ(ct_targets_edge(2, capped, 1), 440);
    CT_ASSERT_EQ(capped[0].t1, edge_pairs[0].t1);
    CT_ASSERT_EQ(capped[0].t2, edge_pairs[0].t2);
    CT_ASSERT_EQ(capped[1].t1, -99);
    CT_ASSERT_EQ(capped[1].t2, -99);
    for (int i = 0; i < 8; i++) {
        capped[i].t1 = -99;
        capped[i].t2 = -99;
    }
    CT_ASSERT_EQ(ct_targets_corner(0, capped, 0), 378);
    CT_ASSERT_EQ(capped[0].t1, -99);
    CT_ASSERT_EQ(capped[0].t2, -99);
    CT_ASSERT_EQ(ct_targets_corner(0, capped, -7), 378);
    CT_ASSERT_EQ(capped[0].t1, -99);
    CT_ASSERT_EQ(ct_targets_edge(2, NULL, -7), 440);
    CT_ASSERT_EQ(ct_targets_edge(2, NULL, 0), 440);

    CT_ASSERT_EQ(ct_targets_corner(-1, NULL, 0), CT_TARGET_NONE);
    CT_ASSERT_EQ(ct_targets_corner(CT_TARGET_STICKERS, capped, 5), CT_TARGET_NONE);
    CT_ASSERT_EQ(ct_targets_edge(-1, NULL, 0), CT_TARGET_NONE);
    CT_ASSERT_EQ(ct_targets_edge(CT_TARGET_STICKERS, NULL, 0), CT_TARGET_NONE);
}
