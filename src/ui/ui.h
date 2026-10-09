#ifndef CT_UI_H
#define CT_UI_H

#include "app/app.h"
#include "storage/repo_cards.h"

void ct_ui_update(ct_app *app);
void ct_ui_draw(ct_app *app);

enum { CT_CARDS_PAGE = (1 << 4) };

typedef struct {
    sqlite3 *db;
    ct_card card;
    char deck_filter[CT_CARD_DECK_MAX];
    long ids[CT_CARDS_PAGE];
    int row_count;
    int selected;
    bool editing;
    bool dirty;
    int status;
} ct_cards_form;

void ct_cards_form_open(ct_cards_form *f, sqlite3 *db, const char *deck);
void ct_cards_form_reload(ct_cards_form *f, sqlite3 *db);
int ct_cards_form_select(ct_cards_form *f, int row);
int ct_cards_form_begin_edit(ct_cards_form *f);
int ct_cards_form_save(ct_cards_form *f, sqlite3 *db);
int ct_cards_form_delete(ct_cards_form *f, sqlite3 *db);
int ct_cards_form_create(ct_cards_form *f, sqlite3 *db, const char *deck);
void ct_cards_form_cancel(ct_cards_form *f);

void ct_screen_cards_update(ct_app *app);
void ct_screen_cards_draw(ct_app *app);

#endif
