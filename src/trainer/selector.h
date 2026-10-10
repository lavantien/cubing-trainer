#ifndef CT_TRAINER_SELECTOR_H
#define CT_TRAINER_SELECTOR_H

#include <sqlite3.h>

#include "config/config.h"
#include "packs/packs.h"
#include "storage/repo_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ct_selector_source {
    CT_SELECTOR_CFOP = 0,
    CT_SELECTOR_CHART,
    CT_SELECTOR_PACK
} ct_selector_source;

enum { CT_SELECTOR_LISTS_PER_PAIR_MAX = 5 };

typedef struct {
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
} ct_selector_list;

typedef struct {
    ct_event event;
    ct_method method;
    int list_id;
    int case_index;
} ct_selector;

typedef struct {
    ct_letter_map map;
    int buffer_corner;
    int buffer_edge;
} ct_selector_scheme;

int ct_selector_scheme_load(sqlite3 *db, ct_selector_scheme *out);
int ct_selector_events(ct_event *out, int cap);
int ct_selector_methods(ct_event ev, ct_method *out, int cap);
int ct_selector_lists(ct_event ev, ct_method m, ct_selector_list *out, int cap);
int ct_selector_case_count(sqlite3 *db, const ct_selector_scheme *scheme, ct_event ev, ct_method m,
                           int list_id);
int ct_selector_case_key(sqlite3 *db, const ct_selector_scheme *scheme, ct_event ev, ct_method m,
                         int list_id, int index, char *out, unsigned cap);
int ct_selector_case_pair(const ct_selector_scheme *scheme, ct_event ev, ct_method m, int list_id,
                          int index, int *t1, int *t2);
int ct_selector_pack_case(sqlite3 *db, ct_event ev, ct_method m, int list_id, int index,
                          ct_pack_entry *out);

#ifdef __cplusplus
}
#endif

#endif
