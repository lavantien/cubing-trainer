#include "config/config.h"
#include "engine/notation.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    if (ct_synth_init() < 0) {
        printf("SYNTH_INIT_FAIL\n");
        return 1;
    }
    ct_config cfg = ct_config_defaults();
    static ct_target_pair pairs[CT_TARGET_STICKERS * CT_TARGET_STICKERS];
    int corner_total =
        ct_targets_corner(CT_3STYLE_CORNER_BUFFER, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
    int edge_total =
        ct_targets_edge(CT_3STYLE_EDGE_BUFFER, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
    printf("corner_buffer=%d edge_buffer=%d corner_targets=%d edge_targets=%d\n",
           CT_3STYLE_CORNER_BUFFER, CT_3STYLE_EDGE_BUFFER, corner_total, edge_total);
    printf("orbit\tbuffer\tt1\tt2\tsetup\tfallback\tlen\tqtm\tnotation\tmoves\n");
    for (int orbit = CT_ORBIT_CORNER; orbit <= CT_ORBIT_EDGE; orbit++) {
        int buffer = orbit == CT_ORBIT_CORNER ? CT_3STYLE_CORNER_BUFFER : CT_3STYLE_EDGE_BUFFER;
        int total =
            ct_targets_corner(buffer, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
        if (orbit == CT_ORBIT_EDGE) {
            total = ct_targets_edge(buffer, pairs, CT_TARGET_STICKERS * CT_TARGET_STICKERS);
        }
        for (int k = 0; k < total; k++) {
            ct_synth_result res;
            memset(&res, 0, sizeof res);
            int rc = ct_synth_target(orbit, buffer, pairs[k].t1, pairs[k].t2, &cfg, &res);
            if (rc != 0 || !res.found) {
                printf("SYNTH_MISS\t%d\t%d\t%d\t%d\trc=%d\n", orbit, buffer, pairs[k].t1,
                       pairs[k].t2, rc);
                continue;
            }
            char text[256];
            if (ct_notation_format(res.seq, res.len, 3, text, (unsigned)sizeof text) < 0) {
                printf("FMT_FAIL\t%d\t%d\t%d\t%d\n", orbit, buffer, pairs[k].t1, pairs[k].t2);
                continue;
            }
            printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t", orbit, buffer, pairs[k].t1, pairs[k].t2,
                   res.used_setup ? 1 : 0, res.used_fallback ? 1 : 0, res.len, res.qtm, text);
            for (int i = 0; i < res.len; i++) {
                printf("[%d,%d,%d,%d]", res.seq[i].face, res.seq[i].depth_from, res.seq[i].depth_to,
                       res.seq[i].quarter);
            }
            printf("\n");
        }
    }
    return 0;
}
