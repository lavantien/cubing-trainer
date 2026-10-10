#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "storage/repo_algs.h"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    ct_config base = ct_config_defaults();
    ct_config penalty_changed = ct_config_defaults();
    ct_config finger_changed = ct_config_defaults();
    ct_config fsrs_changed = ct_config_defaults();
    ct_config subset_changed = ct_config_defaults();
    penalty_changed.gen.penalty[4] = 9.5;
    finger_changed.gen.finger_cost[4][1] = 9.5;
    fsrs_changed.fsrs.w[0] = 1.5;
    subset_changed.gen.preferred_moves = base.gen.preferred_moves & ~(unsigned)CT_MOVE_BIT_U;
    char h_base[CT_ALG_HASH_HEX_MAX];
    char h_pen[CT_ALG_HASH_HEX_MAX];
    char h_fin[CT_ALG_HASH_HEX_MAX];
    char h_fsrs[CT_ALG_HASH_HEX_MAX];
    char h_sub[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&base, 1, CT_BUFFER_FIXED, h_base);
    ct_params_hash(&penalty_changed, 1, CT_BUFFER_FIXED, h_pen);
    ct_params_hash(&finger_changed, 1, CT_BUFFER_FIXED, h_fin);
    ct_params_hash(&fsrs_changed, 1, CT_BUFFER_FIXED, h_fsrs);
    ct_params_hash(&subset_changed, 1, CT_BUFFER_FIXED, h_sub);
    printf("base          =%s\n", h_base);
    printf("penalty_F 9.5 =%s same=%d\n", h_pen, strcmp(h_base, h_pen) == 0);
    printf("fingerF/ccw9.5=%s same=%d\n", h_fin, strcmp(h_base, h_fin) == 0);
    printf("srs.w0 1.5    =%s same=%d\n", h_fsrs, strcmp(h_base, h_fsrs) == 0);
    printf("subset no U   =%s same=%d\n", h_sub, strcmp(h_base, h_sub) == 0);
    return 0;
}
