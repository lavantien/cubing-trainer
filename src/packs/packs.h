#ifndef CT_PACKS_PACKS_H
#define CT_PACKS_PACKS_H

#include <sqlite3.h>

#include "config/config.h"
#include "packs/provenance.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CT_PACK_CASE_KEY_MAX = CT_B64,
    CT_PACK_ALG_MAX = CT_B8 * CT_B32,
    CT_PACK_FRONT_JSON_MAX = CT_B16 * CT_B32,
    CT_PACK_URL_MAX = CT_B8 * CT_B32,
    CT_PACK_AT_MAX = CT_B32,
    CT_PACK_HASH_MAX = CT_SHA_HEX_MAX,
    CT_PACK_CANON_MAX = CT_B16 * CT_B128
};

typedef enum ct_pack_rc {
    CT_PACK_OK = 0,
    CT_PACK_MISSING = 1,
    CT_PACK_ERR_ARG = -1,
    CT_PACK_ERR_DB = -2
} ct_pack_rc;

typedef struct ct_pack_entry {
    ct_event event;
    ct_method method;
    ct_stage stage;
    char case_key[CT_PACK_CASE_KEY_MAX];
    char alg[CT_PACK_ALG_MAX];
    char front_json[CT_PACK_FRONT_JSON_MAX];
    char source_url[CT_PACK_URL_MAX];
    char source_at[CT_PACK_AT_MAX];
} ct_pack_entry;

int ct_pack_upsert(sqlite3 *db, const ct_pack_entry *e);
int ct_pack_get(sqlite3 *db, ct_event ev, ct_method m, ct_stage st, const char *case_key,
                ct_pack_entry *out);
int ct_pack_count(sqlite3 *db, ct_event ev, ct_method m, ct_stage st);
int ct_pack_list(sqlite3 *db, ct_event ev, ct_method m, ct_stage st, ct_pack_entry *out, int cap);
int ct_pack_content_hash(const ct_pack_entry *e, char hex_out[CT_PACK_HASH_MAX]);

#ifdef __cplusplus
}
#endif

#endif
