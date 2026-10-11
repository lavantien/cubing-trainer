#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "storage/repo_algs.h"

int main(void) {
    ct_config cfg = ct_config_defaults();
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        cfg.gen.penalty[i] = 1.0e300;
    }
    for (int f = 0; f < CT_FINGER_FACE_COUNT; f++) {
        for (int d = 0; d < CT_FINGER_DIR_COUNT; d++) {
            cfg.gen.finger_cost[f][d] = 1.0e300;
        }
    }
    cfg.gen.weight_len = 1.0e300;
    cfg.gen.weight_subset = 1.0e300;
    cfg.gen.weight_regrip = 1.0e300;
    cfg.gen.weight_finger = 1.0e300;
    ct_letter_map a;
    ct_letter_map b;
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        a.corner[i] = (char)('A' + i);
        a.edge[i] = (char)('A' + i);
        b.corner[i] = (char)('A' + (i + 1) % CT_LETTER_STICKERS);
        b.edge[i] = (char)('A' + (i + 1) % CT_LETTER_STICKERS);
    }
    char ha[CT_ALG_HASH_HEX_MAX];
    char hb[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, 2, 2, &a, ha);
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, 2, 2, &b, hb);
    printf("huge-double canonical: map A hash %.16s\n", ha);
    printf("huge-double canonical: map B hash %.16s\n", hb);
    printf("collision: %s\n", strcmp(ha, hb) == 0 ? "YES" : "no");
    ct_config sane = ct_config_defaults();
    ct_params_hash(&sane, 1, CT_BUFFER_FIXED, 2, 2, &a, ha);
    ct_params_hash(&sane, 1, CT_BUFFER_FIXED, 2, 2, &b, hb);
    printf("sane canonical: collision: %s\n", strcmp(ha, hb) == 0 ? "YES" : "no");
    return 0;
}
