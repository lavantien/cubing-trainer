#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    if (ct_synth_init() <= 0) {
        printf("init failed\n");
        return 1;
    }
    printf("size=%d\n", ct_synth_table_size());
    for (int idx = 0; idx < 3; idx++) {
        int key[CT_SYNTH_KEY_FIELDS];
        ct_move seq[CT_SYNTH_TPL_MAX];
        int len = ct_synth_table_entry(idx, key, seq, CT_SYNTH_TPL_MAX);
        printf("entry %d len=%d key=%d,%d,%d,%d\n", idx, len, key[0], key[1], key[2], key[3]);
        for (int i = 0; i < len; i++) {
            printf("  %d: f=%d d=%d-%d q=%d\n", i, seq[i].face, seq[i].depth_from, seq[i].depth_to, seq[i].quarter);
        }
    }
    int key[CT_SYNTH_KEY_FIELDS];
    ct_move seq[CT_SYNTH_TPL_MAX];
    printf("entry-1=%d entry+size=%d entry-size-bad-out=%d\n",
           ct_synth_table_entry(-1, key, seq, CT_SYNTH_TPL_MAX),
           ct_synth_table_entry(ct_synth_table_size(), key, seq, CT_SYNTH_TPL_MAX),
           ct_synth_table_entry(0, key, NULL, CT_SYNTH_TPL_MAX));
    return 0;
}
