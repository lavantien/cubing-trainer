#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "engine/cube.h"
#include "gen/score.h"
#include "storage/repo_algs.h"

static int diff_hash(const char *label, const ct_config *base, const char *base_hash,
                     const ct_config *mutated) {
    char h[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(mutated, 1, CT_BUFFER_FIXED, h);
    int changed = strcmp(base_hash, h) != 0;
    printf("%-28s changed=%d\n", label, changed);
    return changed;
}

int main(void) {
    ct_config cfg = ct_config_defaults();
    char base[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&cfg, 1, CT_BUFFER_FIXED, base);

    ct_config m = cfg;
    ct_config_set(&m, "gen.penalty_R", "9.5");
    int penalty_changed = diff_hash("gen.penalty_R", &cfg, base, &m);

    m = cfg;
    ct_config_set(&m, "gen.penalty_U", "7.25");
    int penalty2_changed = diff_hash("gen.penalty_U", &cfg, base, &m);

    m = cfg;
    ct_config_set(&m, "gen.finger_cost_R_cw", "3.75");
    int finger_changed = diff_hash("gen.finger_cost_R_cw", &cfg, base, &m);

    m = cfg;
    ct_config_set(&m, "srs.w3", "16.5");
    int fsrs_changed = diff_hash("srs.w3", &cfg, base, &m);

    m = cfg;
    ct_config_set(&m, "srs.request_retention", "0.95");
    int retention_changed = diff_hash("srs.request_retention", &cfg, base, &m);

    ct_move seq[3] = {{2, 1, 1, 1}, {4, 1, 1, 2}, {3, 1, 1, 3}};
    ct_score_parts a;
    ct_score_parts b;
    ct_score_alg(seq, 3, &cfg, &a);
    ct_config m2 = cfg;
    ct_config_set(&m2, "gen.penalty_F", "9.5");
    ct_score_alg(seq, 3, &m2, &b);
    printf("score default=%.4f penaltyF9.5=%.4f subset_default=%.2f subset_mut=%.2f\n", a.total,
           b.total, a.subset_penalty, b.subset_penalty);

    printf("VERDICT penalty_in_hash=%d finger_in_hash=%d fsrs_in_hash=%d retention_in_hash=%d\n",
           penalty_changed, penalty2_changed, finger_changed, fsrs_changed);
    (void)retention_changed;
    return 0;
}
