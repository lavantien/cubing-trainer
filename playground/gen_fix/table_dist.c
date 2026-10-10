#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/classify.h"
#include "gen/synth.h"

#include <stdio.h>
#include <string.h>

static int dist_of(int cell) {
    for (int s = 0; s < CT_TARGET_STICKERS; s++) {
        int face = -1;
        int row = -1;
        int col = -1;
        ct_sticker_cell(CT_ORBIT_CORNER, s, &face, &row, &col);
        if (face * 9 + row * 3 + col == cell) {
            return 0;
        }
        ct_sticker_cell(CT_ORBIT_EDGE, s, &face, &row, &col);
        if (face * 9 + row * 3 + col == cell) {
            return 1;
        }
    }
    return -1;
}

int main(void) {
    if (ct_synth_init() < 0) {
        printf("init fail\n");
        return 1;
    }
    int size = ct_synth_table_size();
    printf("table=%d\n", size);
    int counts[2][20] = {0};
    int cls_counts[2][CT_CLS_UNKNOWN + 1] = {0};
    for (int i = 0; i < size; i++) {
        int key[CT_SYNTH_KEY_FIELDS];
        ct_move seq[CT_SYNTH_TPL_MAX];
        int len = ct_synth_table_entry(i, key, seq, CT_SYNTH_TPL_MAX);
        int orbit = dist_of(key[0]);
        if (len < 20) {
            counts[orbit][len]++;
        }
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        ct_classify(seq, len, &r);
        cls_counts[orbit][r.cls]++;
    }
    for (int o = 0; o < 2; o++) {
        printf("orbit%d lens:", o);
        for (int l = 0; l < 20; l++) {
            if (counts[o][l]) {
                printf(" %d:%d", l, counts[o][l]);
            }
        }
        printf("\n");
    }
    for (int o = 0; o < 2; o++) {
        printf("orbit%d classes:", o);
        for (int c = 0; c <= CT_CLS_UNKNOWN; c++) {
            if (cls_counts[o][c]) {
                printf(" %s:%d", ct_classification_label((ct_classification)c), cls_counts[o][c]);
            }
        }
        printf("\n");
    }
    return 0;
}
