#ifndef CT_STORAGE_REPO_CARDS_H
#define CT_STORAGE_REPO_CARDS_H

#include <sqlite3.h>
#include <string.h>

#define CT_CARD_STAMP_MAX (1 << 5)
#define CT_CARD_DECK_MAX (1 << 6)
#define CT_CARD_CASE_KEY_MAX (1 << 6)
#define CT_CARD_ALG_MAX (1 << 8)
#define CT_CARD_FRONT_JSON_MAX (1 << 9)
#define CT_CARD_NOTES_MAX (1 << 9)
#define CT_CASE_KEY_FORBIDDEN 0x20
#define CT_CASE_KEY_DELETE 0x7F

typedef enum ct_repo_rc {
    CT_REPO_OK = 0,
    CT_REPO_MISSING = 1,
    CT_REPO_CONFLICT = 2,
    CT_REPO_ERR_ARG = -1,
    CT_REPO_ERR_DB = -2
} ct_repo_rc;

typedef struct ct_card {
    long id;
    char deck[CT_CARD_DECK_MAX];
    char case_key[CT_CARD_CASE_KEY_MAX];
    int direction;
    char front_json[CT_CARD_FRONT_JSON_MAX];
    char alg_3x3[CT_CARD_ALG_MAX];
    char alg_big[CT_CARD_ALG_MAX];
    char alg_oh[CT_CARD_ALG_MAX];
    char notes[CT_CARD_NOTES_MAX];
    char created_at[CT_CARD_STAMP_MAX];
    char updated_at[CT_CARD_STAMP_MAX];
} ct_card;

typedef struct ct_srs_row {
    double stability;
    double difficulty;
    char due[CT_CARD_STAMP_MAX];
    char last_review[CT_CARD_STAMP_MAX];
    int reps;
    int lapses;
    int state;
} ct_srs_row;

static inline int ct_repo_text_ok(const char *s, unsigned max) {
    return s != NULL && memchr(s, '\0', max) != NULL && s[0] != '\0';
}

void ct_now_utc_iso(char *out, unsigned cap);
int ct_repo_card_insert(sqlite3 *db, ct_card *c);
int ct_repo_card_update(sqlite3 *db, const ct_card *c);
int ct_repo_card_get(sqlite3 *db, long id, ct_card *out);
int ct_repo_card_get_by_key(sqlite3 *db, const char *deck, const char *case_key, int direction,
                            ct_card *out);
int ct_repo_card_delete(sqlite3 *db, long id);
int ct_repo_card_list(sqlite3 *db, const char *deck, ct_card *out, int cap);
int ct_repo_srs_get(sqlite3 *db, long card_id, ct_srs_row *out);
int ct_repo_srs_put(sqlite3 *db, long card_id, const ct_srs_row *row);

#endif
