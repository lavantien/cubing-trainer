#ifndef CT_UI_H
#define CT_UI_H

#include <limits.h>

#include "app/app.h"
#include "engine/cube.h"
#include "render/anim.h"
#include "stats/stats.h"
#include "storage/repo_cards.h"
#include "storage/repo_settings.h"
#include "trainer/session.h"

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

enum {
    CT_SETTINGS_FACE_COUNT = CT_LETTER_FACES,
    CT_SETTINGS_CHANNELS = 3,
    CT_SETTINGS_COLOR_MAX = UCHAR_MAX,
    CT_SETTINGS_BUFFER_CORNER_DEFAULT = 0,
    CT_SETTINGS_BUFFER_EDGE_DEFAULT = 2,
    CT_SETTINGS_TEXT_MAX = (1 << 5),
};

typedef struct {
    sqlite3 *db;
    int scheme_id;
    ct_letter_map map;
    ct_letter_map staged;
    unsigned move_mask;
    double retention;
    double move_seconds;
    int buffer_corner;
    int buffer_edge;
    unsigned char colors[CT_SETTINGS_FACE_COUNT][CT_SETTINGS_CHANNELS];
    bool dirty;
    int status;
} ct_settings_form;

void ct_settings_form_open(ct_settings_form *f, sqlite3 *db);
int ct_settings_form_stage_letter(ct_settings_form *f, int orbit, int sticker, char letter);
int ct_settings_form_swap_letter(ct_settings_form *f, int orbit, int sticker, char letter);
int ct_settings_form_stage_color(ct_settings_form *f, int face, int channel, int value);
int ct_settings_form_stage_move(ct_settings_form *f, unsigned bit);
int ct_settings_form_stage_retention(ct_settings_form *f, double v);
int ct_settings_form_stage_move_seconds(ct_settings_form *f, double v);
int ct_settings_form_stage_buffer(ct_settings_form *f, int orbit, int sticker);
int ct_settings_form_save(ct_settings_form *f);
void ct_settings_form_cancel(ct_settings_form *f);

void ct_screen_settings_update(ct_app *app);
void ct_screen_settings_draw(ct_app *app);

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

enum {
    CT_STATS_MAX_DECKS = CT_STATS_DECK_MAX,
    CT_STATS_FORECAST_DAYS = (1 << 5) - 2,
    CT_STATS_MAX_CASES = (1 << 6),
    CT_STATS_RETENTION_BUCKETS = (1 << 3) + 2,
    CT_STATS_REVIEW_ROWS_MAX = (1 << 8),
    CT_STATS_SESSIONS_MAX = (1 << 4)
};

typedef struct {
    sqlite3 *db;
    ct_deck_stats decks[CT_STATS_MAX_DECKS];
    int deck_count;
    int deck_selected;
    ct_due_forecast_row forecast[CT_STATS_FORECAST_DAYS];
    int forecast_count;
    ct_case_accuracy_row cases[CT_STATS_MAX_CASES];
    int case_count;
    int case_order[CT_STATS_MAX_CASES];
    ct_retention_bucket retention[CT_STATS_RETENTION_BUCKETS];
    int retention_count;
    ct_retention_summary retention_summary;
    ct_session_row sessions[CT_STATS_SESSIONS_MAX];
    int session_count;
    char now_iso[CT_STATS_STAMP_MAX];
} ct_stats_view;

int ct_stats_view_load(ct_stats_view *v, sqlite3 *db, const char *now_iso, const char *deck);
int ct_stats_view_select_deck(ct_stats_view *v, const char *deck);
int ct_stats_view_worst_cases(ct_stats_view *v, int n);

void ct_screen_stats_update(ct_app *app);
void ct_screen_stats_draw(ct_app *app);

#endif
