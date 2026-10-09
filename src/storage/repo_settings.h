#ifndef CT_STORAGE_REPO_SETTINGS_H
#define CT_STORAGE_REPO_SETTINGS_H

#include <sqlite3.h>

#define CT_LETTER_FACES 6
#define CT_LETTER_PER_FACE 4
#define CT_LETTER_STICKERS (CT_LETTER_FACES * CT_LETTER_PER_FACE)
#define CT_LETTER_FIRST 'A'
#define CT_LETTER_LAST (CT_LETTER_FIRST + CT_LETTER_STICKERS - 1)

typedef struct {
    char corner[CT_LETTER_STICKERS];
    char edge[CT_LETTER_STICKERS];
} ct_letter_map;

int ct_repo_settings_get(sqlite3 *db, const char *key, char *out, unsigned cap);
int ct_repo_settings_set(sqlite3 *db, const char *key, const char *value);

int ct_letter_scheme_create(sqlite3 *db, const char *name, int *id);
int ct_letter_scheme_default_id(sqlite3 *db);
int ct_letter_scheme_set_default(sqlite3 *db, int scheme_id);
int ct_letter_map_load(sqlite3 *db, int scheme_id, ct_letter_map *out);
int ct_letter_map_set(sqlite3 *db, int scheme_id, int orbit, int sticker, char letter);
int ct_letter_seed_speffz(sqlite3 *db);

#endif
