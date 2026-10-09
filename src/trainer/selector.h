#ifndef CT_TRAINER_SELECTOR_H
#define CT_TRAINER_SELECTOR_H

#include "config/config.h"

typedef struct {
    ct_event event;
    ct_method method;
    int list_id;
    int case_index;
} ct_selector;

int ct_selector_events(ct_event *out, int cap);
int ct_selector_methods(ct_event ev, ct_method *out, int cap);
int ct_selector_lists(ct_event ev, ct_method m, int *out_ids, ct_stage *out_stages,
                      ct_orbit *out_orbits, int cap);
int ct_selector_case_count(ct_event ev, ct_method m, int list_id);
int ct_selector_case_key(ct_event ev, ct_method m, int list_id, int index, char *out, unsigned cap);

#endif
