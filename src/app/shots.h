#ifndef CT_APP_SHOTS_H
#define CT_APP_SHOTS_H

#include <stdbool.h>

#include "app/app.h"

enum {
    CT_SHOTS_OK = 0,
    CT_SHOTS_E_ARG = -1,
    CT_SHOTS_E_STATE = -2,
    CT_SHOTS_E_WINDOW = -3,
    CT_SHOTS_E_WRITE = -4
};

int ct_shots_menu(ct_app *app, const char *out_path);
int ct_shots_trainer(ct_app *app, ct_event ev, ct_method m, int list_id, int case_index,
                     const char *out_path);
int ct_shots_chart(ct_app *app, ct_event ev, ct_method m, int list_id, int cell,
                   const char *out_path);
int ct_shots_review(ct_app *app, bool revealed, const char *out_path);
int ct_shots_cards(ct_app *app, const char *deck, bool editing, const char *out_path);
int ct_shots_settings(ct_app *app, int tab, const char *out_path);
int ct_shots_stats(ct_app *app, const char *out_path);
void ct_shots_close(void);

#endif
