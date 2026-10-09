#ifndef CT_PACKS_SEED_REGISTRY_H
#define CT_PACKS_SEED_REGISTRY_H

#include <sqlite3.h>

#include "config/config.h"

typedef struct {
    const char *file;
    ct_event event;
    ct_method method;
    ct_stage stage;
} ct_pack_seed_reg;

extern const ct_pack_seed_reg ct_pack_seed_registry[];

int ct_pack_seed_registry_count(void);
int ct_pack_seed_registered(sqlite3 *db, const char *dir);

#endif
