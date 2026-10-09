#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "raygui.h"
#include "raylib.h"
#include "ui/ui.h"

static const char CT_SQL_CARDS_ALL[] = "SELECT id FROM flashcard ORDER BY id";
static const char CT_CARDS_KEY_PREFIX[] = "new-";
static const char CT_CARDS_DEFAULT_DECK[] = "unfiled";

enum {
    CT_CARDS_MARGIN_SHIFT = 6,
    CT_CARDS_LIST_W_DIV = 3,
    CT_CARDS_FIELD_H_SHIFT = 4,
    CT_CARDS_LABEL_W_DIV = 8,
    CT_CARDS_TAG_MAX = 8,
    CT_CARDS_LABEL_MAX = CT_CARD_CASE_KEY_MAX + CT_CARDS_TAG_MAX,
    CT_CARDS_KEY_TRIES = (1 << 7),
    CT_CARDS_BTN_NEW = 0,
    CT_CARDS_BTN_EDIT,
    CT_CARDS_BTN_SAVE,
    CT_CARDS_BTN_CANCEL,
    CT_CARDS_BTN_DELETE,
    CT_CARDS_BTN_BACK,
    CT_CARDS_BTN_COUNT
};

static int ct_cards_list_all(sqlite3 *db, long *ids, int cap) {
    if (db == NULL || ids == NULL || cap < 0)
        return CT_REPO_ERR_ARG;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, CT_SQL_CARDS_ALL, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return CT_REPO_ERR_DB;
    }
    int n = 0;
    int step = SQLITE_OK;
    while (n < cap && (step = sqlite3_step(st)) == SQLITE_ROW) {
        ids[n++] = (long)sqlite3_column_int64(st, 0);
    }
    sqlite3_finalize(st);
    if (step != SQLITE_ROW && step != SQLITE_DONE)
        return CT_REPO_ERR_DB;
    return n;
}

static void ct_cards_form_refresh(ct_cards_form *f) {
    memset(&f->card, 0, sizeof f->card);
    if (f->db == NULL || f->selected < 0 || f->selected >= f->row_count)
        return;
    f->status = ct_repo_card_get(f->db, f->ids[f->selected], &f->card);
}

void ct_cards_form_open(ct_cards_form *f, sqlite3 *db, const char *deck) {
    if (f == NULL)
        return;
    memset(f, 0, sizeof *f);
    f->selected = -1;
    if (deck != NULL)
        snprintf(f->deck_filter, sizeof f->deck_filter, "%s", deck);
    ct_cards_form_reload(f, db);
    if (f->row_count > 0) {
        f->selected = 0;
        ct_cards_form_refresh(f);
    }
}

void ct_cards_form_reload(ct_cards_form *f, sqlite3 *db) {
    if (f == NULL)
        return;
    f->db = db;
    f->editing = false;
    f->dirty = false;
    int rc;
    if (db == NULL) {
        rc = CT_REPO_ERR_ARG;
    } else if (f->deck_filter[0] == '\0') {
        rc = ct_cards_list_all(db, f->ids, CT_CARDS_PAGE);
    } else {
        ct_card rows[CT_CARDS_PAGE];
        rc = ct_repo_card_list(db, f->deck_filter, rows, CT_CARDS_PAGE);
        if (rc >= 0) {
            for (int i = 0; i < rc; i++)
                f->ids[i] = rows[i].id;
        }
    }
    if (rc < 0) {
        f->row_count = 0;
        f->status = rc;
    } else {
        f->row_count = rc;
        f->status = CT_REPO_OK;
    }
    if (f->selected >= f->row_count)
        f->selected = f->row_count - 1;
    ct_cards_form_refresh(f);
}

int ct_cards_form_select(ct_cards_form *f, int row) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if (f->row_count <= 0) {
        f->selected = -1;
        f->status = CT_REPO_MISSING;
        return CT_REPO_MISSING;
    }
    if (row < 0)
        row = 0;
    if (row >= f->row_count)
        row = f->row_count - 1;
    f->selected = row;
    if (f->db == NULL) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    ct_cards_form_refresh(f);
    return f->status;
}

int ct_cards_form_begin_edit(ct_cards_form *f) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if (f->selected < 0 || f->selected >= f->row_count) {
        f->status = CT_REPO_MISSING;
        return CT_REPO_MISSING;
    }
    f->editing = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_cards_form_save(ct_cards_form *f, sqlite3 *db) {
    if (f == NULL || db == NULL)
        return CT_REPO_ERR_ARG;
    if (f->selected < 0 || f->selected >= f->row_count || f->card.id <= 0) {
        f->status = CT_REPO_MISSING;
        return CT_REPO_MISSING;
    }
    int rc = ct_repo_card_update(db, &f->card);
    f->status = rc;
    if (rc == CT_REPO_OK) {
        f->dirty = false;
        f->editing = false;
    }
    return rc;
}

int ct_cards_form_delete(ct_cards_form *f, sqlite3 *db) {
    if (f == NULL || db == NULL)
        return CT_REPO_ERR_ARG;
    if (f->selected < 0 || f->selected >= f->row_count || f->ids[f->selected] <= 0) {
        f->status = CT_REPO_MISSING;
        return CT_REPO_MISSING;
    }
    int rc = ct_repo_card_delete(db, f->ids[f->selected]);
    f->status = rc;
    if (rc == CT_REPO_OK)
        ct_cards_form_reload(f, db);
    return rc;
}

int ct_cards_form_create(ct_cards_form *f, sqlite3 *db, const char *deck) {
    if (f == NULL || db == NULL)
        return CT_REPO_ERR_ARG;
    if (!ct_repo_text_ok(deck, CT_CARD_DECK_MAX)) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    ct_card c;
    memset(&c, 0, sizeof c);
    snprintf(c.deck, sizeof c.deck, "%s", deck);
    c.direction = CT_DIR_FR;
    long seq = (long)f->row_count + 1;
    int rc = CT_REPO_CONFLICT;
    for (int i = 0; i < CT_CARDS_KEY_TRIES; i++) {
        snprintf(c.case_key, sizeof c.case_key, "%s%ld", CT_CARDS_KEY_PREFIX, seq);
        rc = ct_repo_card_insert(db, &c);
        if (rc != CT_REPO_CONFLICT)
            break;
        seq++;
    }
    f->status = rc;
    if (rc != CT_REPO_OK)
        return rc;
    ct_cards_form_reload(f, db);
    for (int i = 0; i < f->row_count; i++) {
        if (f->ids[i] == c.id) {
            f->selected = i;
            break;
        }
    }
    ct_cards_form_refresh(f);
    f->editing = true;
    f->dirty = false;
    return CT_REPO_OK;
}

void ct_cards_form_cancel(ct_cards_form *f) {
    if (f == NULL)
        return;
    f->editing = false;
    f->dirty = false;
    ct_cards_form_refresh(f);
}

typedef struct {
    ct_cards_form form;
    bool opened;
    bool deck_edit;
    int list_scroll;
    char labels[CT_CARDS_PAGE][CT_CARDS_LABEL_MAX];
    char *label_ptrs[CT_CARDS_PAGE];
} ct_cards_screen;

static ct_cards_screen ct_cards;

static const char *ct_cards_dir_tag(int direction) {
    switch (direction) {
    case CT_DIR_FR:
        return "fr";
    case CT_DIR_FL:
        return "fl";
    case CT_DIR_BR:
        return "br";
    case CT_DIR_BL:
        return "bl";
    default:
        return "--";
    }
}

static const char *ct_cards_status_text(int status) {
    switch (status) {
    case CT_REPO_OK:
        return "ok";
    case CT_REPO_MISSING:
        return "not found";
    case CT_REPO_CONFLICT:
        return "duplicate key";
    case CT_REPO_ERR_ARG:
        return "invalid input";
    case CT_REPO_ERR_DB:
        return "db error";
    default:
        return "unknown";
    }
}

static void ct_cards_build_labels(void) {
    for (int i = 0; i < ct_cards.form.row_count; i++) {
        ct_card c;
        if (ct_cards.form.db == NULL ||
            ct_repo_card_get(ct_cards.form.db, ct_cards.form.ids[i], &c) != CT_REPO_OK) {
            ct_cards.labels[i][0] = '\0';
        } else {
            snprintf(ct_cards.labels[i], sizeof ct_cards.labels[i], "%s [%s]", c.case_key,
                     ct_cards_dir_tag(c.direction));
        }
        ct_cards.label_ptrs[i] = ct_cards.labels[i];
    }
}

static void ct_cards_recompute_dirty(ct_cards_form *f) {
    f->dirty = false;
    if (!f->editing || f->db == NULL || f->card.id <= 0)
        return;
    ct_card cur;
    if (ct_repo_card_get(f->db, f->card.id, &cur) != CT_REPO_OK)
        return;
    f->dirty =
        strcmp(f->card.alg_3x3, cur.alg_3x3) != 0 || strcmp(f->card.alg_big, cur.alg_big) != 0 ||
        strcmp(f->card.alg_oh, cur.alg_oh) != 0 ||
        strcmp(f->card.front_json, cur.front_json) != 0 || strcmp(f->card.notes, cur.notes) != 0;
}

static Rectangle ct_cards_rect(int x, int y, int w, int h) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    return r;
}

static void ct_cards_field(int x, int y, int w, int label_w, int row_h, const char *label,
                           char *buf, size_t cap, bool edit) {
    GuiLabel(ct_cards_rect(x, y, label_w, row_h), label);
    GuiTextBox(ct_cards_rect(x + label_w, y, w - label_w, row_h), buf, (int)cap, edit);
}

typedef struct {
    const char *label;
    size_t offset;
    size_t cap;
} ct_cards_field_spec;

static const ct_cards_field_spec CT_CARDS_FIELDS[] = {
    {"alg 3x3", offsetof(ct_card, alg_3x3), CT_CARD_ALG_MAX},
    {"alg big", offsetof(ct_card, alg_big), CT_CARD_ALG_MAX},
    {"alg oh", offsetof(ct_card, alg_oh), CT_CARD_ALG_MAX},
    {"front", offsetof(ct_card, front_json), CT_CARD_FRONT_JSON_MAX},
    {"notes", offsetof(ct_card, notes), CT_CARD_NOTES_MAX},
};

enum { CT_CARDS_FIELD_COUNT = sizeof CT_CARDS_FIELDS / sizeof CT_CARDS_FIELDS[0] };

static void ct_cards_buttons(ct_app *app, int x, int y, int gap, int btn_w, int btn_h) {
    static const char *const CT_CARDS_BUTTONS[CT_CARDS_BTN_COUNT] = {"new",    "edit",   "save",
                                                                     "cancel", "delete", "back"};
    ct_cards_form *f = &ct_cards.form;
    for (int i = 0; i < CT_CARDS_BTN_COUNT; i++) {
        if (!GuiButton(ct_cards_rect(x + i * (btn_w + gap), y, btn_w, btn_h),
                       CT_CARDS_BUTTONS[i])) {
            continue;
        }
        switch (i) {
        case CT_CARDS_BTN_NEW:
            ct_cards_form_create(
                f, app->db, f->deck_filter[0] != '\0' ? f->deck_filter : CT_CARDS_DEFAULT_DECK);
            break;
        case CT_CARDS_BTN_EDIT:
            ct_cards_form_begin_edit(f);
            break;
        case CT_CARDS_BTN_SAVE:
            ct_cards_form_save(f, app->db);
            break;
        case CT_CARDS_BTN_CANCEL:
            ct_cards_form_cancel(f);
            break;
        case CT_CARDS_BTN_DELETE:
            ct_cards_form_delete(f, app->db);
            break;
        case CT_CARDS_BTN_BACK:
            app->screen = CT_SCREEN_MENU;
            break;
        default:
            break;
        }
    }
}

void ct_screen_cards_update(ct_app *app) {
    if (app == NULL)
        return;
    if (!ct_cards.opened) {
        ct_cards.opened = true;
        ct_cards_form_open(&ct_cards.form, app->db, "");
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        app->screen = CT_SCREEN_MENU;
    }
}

void ct_screen_cards_draw(ct_app *app) {
    ClearBackground(DARKGRAY);
    if (app == NULL)
        return;
    ct_cards_form *f = &ct_cards.form;
    int w = (int)app->cfg.window.width;
    int h = (int)app->cfg.window.height;
    int margin = w >> CT_CARDS_MARGIN_SHIFT;
    int row_h = h >> CT_CARDS_FIELD_H_SHIFT;
    int label_w = w / CT_CARDS_LABEL_W_DIV;
    int list_w = w / CT_CARDS_LIST_W_DIV - margin;
    int filter_y = margin;
    int list_y = filter_y + row_h + margin;
    int status_h = row_h;
    int list_h = h - list_y - status_h - 2 * margin;
    int editor_x = margin + list_w + margin;
    int editor_w = w - editor_x - margin;
    int editor_y = margin;
    int btn_h = row_h;
    int btn_y = h - status_h - margin - btn_h;
    int btn_w = (editor_w - (CT_CARDS_BTN_COUNT - 1) * margin) / CT_CARDS_BTN_COUNT;
    int editor_h = btn_y - margin - margin;
    int fields_y = editor_y + row_h + margin;

    GuiLabel(ct_cards_rect(margin, filter_y, label_w, row_h), "deck filter");
    if (GuiTextBox(ct_cards_rect(margin + label_w, filter_y, list_w - label_w, row_h),
                   f->deck_filter, (int)sizeof f->deck_filter, ct_cards.deck_edit)) {
        ct_cards.deck_edit = !ct_cards.deck_edit;
        if (!ct_cards.deck_edit)
            ct_cards_form_reload(f, app->db);
    }

    ct_cards_build_labels();
    int active = f->selected;
    if (GuiListViewEx(ct_cards_rect(margin, list_y, list_w, list_h), ct_cards.label_ptrs,
                      f->row_count, &ct_cards.list_scroll, &active, NULL)) {
        ct_cards_form_select(f, active);
    }

    GuiGroupBox(ct_cards_rect(editor_x, editor_y, editor_w, editor_h), "card");
    int field_step = row_h + margin;
    for (int i = 0; i < CT_CARDS_FIELD_COUNT; i++) {
        char *buf = (char *)&f->card + CT_CARDS_FIELDS[i].offset;
        ct_cards_field(editor_x, fields_y + i * field_step, editor_w, label_w, row_h,
                       CT_CARDS_FIELDS[i].label, buf, CT_CARDS_FIELDS[i].cap, f->editing);
    }
    ct_cards_recompute_dirty(f);

    ct_cards_buttons(app, editor_x, btn_y, margin, btn_w, btn_h);

    GuiStatusBar(ct_cards_rect(margin, h - status_h, w - 2 * margin, status_h),
                 ct_cards_status_text(f->status));
}
