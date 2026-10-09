#ifndef CT_TRAINER_CFOP_H
#define CT_TRAINER_CFOP_H

#include <sqlite3.h>

#include "config/config.h"

int ct_cfop_case_count(ct_stage stage);
int ct_cfop_case_key(ct_stage stage, int index, char *out, unsigned cap);
int ct_cfop_case_direction(ct_stage stage, int index);
int ct_cfop_deck_name(ct_stage stage, char *out, unsigned cap);
int ct_cfop_populate(sqlite3 *db, ct_stage stage);

#endif
