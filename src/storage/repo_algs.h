#ifndef CT_STORAGE_REPO_ALGS_H
#define CT_STORAGE_REPO_ALGS_H

#include <sqlite3.h>

#include "config/config.h"
#include "storage/repo_cards.h"
#include "storage/repo_settings.h"

enum {
    CT_ALG_B1 = 1,
    CT_ALG_B8 = CT_ALG_B1 << 3,
    CT_ALG_B32 = CT_ALG_B1 << 5,
    CT_ALG_B64 = CT_ALG_B1 << 6,
    CT_ALG_B128 = CT_ALG_B64 << 1,
    CT_ALG_B256 = CT_ALG_B64 << 2,
    CT_ALG_PAIR_MAX = CT_ALG_B8,
    CT_ALG_SEQ_MAX = CT_ALG_B256,
    CT_ALG_SETUP_MAX = CT_ALG_B128,
    CT_ALG_INTER_MAX = CT_ALG_B64,
    CT_ALG_INSERT_MAX = CT_ALG_B128,
    CT_ALG_PARTS_MAX = CT_ALG_B256,
    CT_ALG_STAMP_MAX = CT_ALG_B32,
    CT_ALG_HASH_HEX_MAX = CT_ALG_B64 | CT_ALG_B1,
    CT_ALG_CANON_MAX = CT_ALG_B64 << 7,
    CT_ALG_CATEGORY_CHART = 0
};

typedef struct ct_generated_alg {
    long id;
    int orbit;
    char target_pair[CT_ALG_PAIR_MAX];
    int buffer_mode;
    int category;
    char sequence[CT_ALG_SEQ_MAX];
    int classification;
    char setup[CT_ALG_SETUP_MAX];
    char interchange[CT_ALG_INTER_MAX];
    char insertion[CT_ALG_INSERT_MAX];
    int qtm;
    double score;
    char score_parts_json[CT_ALG_PARTS_MAX];
    char created_at[CT_ALG_STAMP_MAX];
} ct_generated_alg;

int ct_repo_alg_put(sqlite3 *db, const ct_generated_alg *a, const char *params_hash);
int ct_repo_alg_get(sqlite3 *db, int orbit, const char *target_pair, int buffer_mode, int category,
                    const char *params_hash, ct_generated_alg *out);
int ct_repo_alg_count(sqlite3 *db, const char *params_hash);
int ct_repo_alg_list(sqlite3 *db, const char *params_hash, int orbit, ct_generated_alg *out,
                     int cap);
int ct_repo_alg_stale(sqlite3 *db, const char *params_hash, int *out_ids, int cap);
int ct_repo_alg_drop(sqlite3 *db, const char *params_hash);
int ct_repo_alg_drop_orbit(sqlite3 *db, const char *params_hash, int orbit);
void ct_params_hash(const ct_config *cfg, int scheme_id, ct_buffer_mode mode, int buffer_corner,
                    int buffer_edge, const ct_letter_map *map, char hex_out[CT_ALG_HASH_HEX_MAX]);

#endif
