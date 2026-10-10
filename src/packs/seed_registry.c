#include "packs/seed_registry.h"

#include <dirent.h>
#include <stdio.h>

#include "config/config.h"
#include "packs/packs.h"

const ct_pack_seed_reg ct_pack_seed_registry[] = {
    {"bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY},
    {"bld_m2.tsv", CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT},
    {"bld_oldpoch.tsv", CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT},
    {"clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT},
    {"fmc_concepts.tsv", CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT},
    {"fto_l4t_redux.tsv", CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T},
    {"mbld_concepts.tsv", CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT},
    {"megaminx_4lll.tsv", CT_EVENT_MEGAMINX, CT_METHOD_4LLL, CT_STAGE_4LLL},
    {"pyraminx_l4e.tsv", CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E},
    {"skewb_sarah.tsv", CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH},
    {"sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO},
    {"sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP},
    {"sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE},
    {"sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO},
    {"sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP},
    {"twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL},
};

enum { CT_PACK_SEED_COUNT = (int)(sizeof ct_pack_seed_registry / sizeof ct_pack_seed_registry[0]) };

int ct_pack_seed_registry_count(void) {
    return CT_PACK_SEED_COUNT;
}

int ct_pack_seed_registered(sqlite3 *db, const char *dir) {
    if (db == NULL || dir == NULL || dir[0] == '\0')
        return -1;
    DIR *d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "cubing-trainer: no event pack seed directory at %s\n", dir);
        return -1;
    }
    closedir(d);
    int failed = 0;
    char path[CT_APP_EXE_PATH_MAX];
    for (int i = 0; i < CT_PACK_SEED_COUNT; i++) {
        const ct_pack_seed_reg *reg = &ct_pack_seed_registry[i];
        int written = snprintf(path, sizeof path, "%s/%s", dir, reg->file);
        if (written < 0 || (unsigned)written >= sizeof path) {
            fprintf(stderr, "cubing-trainer: event pack seed path for %s is too long\n", reg->file);
            failed++;
            continue;
        }
        int rc = ct_pack_seed_file(db, path, reg->event, reg->method, reg->stage);
        if (rc < 0) {
            fprintf(stderr, "cubing-trainer: cannot seed event pack %s (%d)\n", reg->file, rc);
            failed++;
        }
    }
    return failed;
}
