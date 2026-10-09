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

#include "engine/cube.h"
#include "render/anim.h"
#include "trainer/session.h"

#define CT_REVIEW_SPEED_MIN (1.0 / (double)CT_ANIM_SPEED_EPS_SCALE)
#define CT_UI_DEFAULT_DECK "unfiled"

typedef struct {
    sqlite3 *db;
    ct_session session;
    ct_card card;
    ct_cube cube;
    ct_anim anim;
    char alg_text[CT_CARD_ALG_MAX];
    int move_count;
    bool revealed;
    bool playing;
    double speed;
    int scrub_index;
    int status;
} ct_review_form;

int ct_review_form_begin(ct_review_form *f, sqlite3 *db, const char *deck, const char *now_iso,
                         long today_day, int limit);
int ct_review_form_load_case(ct_review_form *f);
void ct_review_form_reveal(ct_review_form *f);
int ct_review_form_grade(ct_review_form *f, ct_grade g, const char *now_iso);
void ct_review_form_play_pause(ct_review_form *f);
void ct_review_form_step(ct_review_form *f);
void ct_review_form_set_speed(ct_review_form *f, double speed);
int ct_review_form_scrub(ct_review_form *f, int move_index);
void ct_review_form_end(ct_review_form *f, const char *now_iso);

void ct_screen_review_update(ct_app *app);
void ct_screen_review_draw(ct_app *app);

#endif
