#include <stdio.h>
#include <string.h>

#include "gen/classify.h"
#include "raygui.h"
#include "raylib.h"
#include "ui/ui.h"
#include "ui/widgets.h"

enum {
    CT_TRAINER_GRID_COLS = CT_LETTER_FACES,
    CT_TRAINER_GRID_ROWS = CT_LETTER_PER_FACE,
    CT_TRAINER_KEY_MAX = CT_CARD_CASE_KEY_MAX,
    CT_TRAINER_EVENTS_MAX = CT_EVENT_FTO + 1,
    CT_TRAINER_CASES_MAX = CT_LETTER_STICKERS * CT_LETTER_STICKERS,
    CT_TRAINER_HALF = 2,
    CT_TRAINER_DETAIL_COL = 3,
    CT_TRAINER_DECOMP_STEP = CT_UI_FONT_BODY + CT_UI_ROW_GAP / 2,
    CT_TRAINER_MEMO_GAP = 8,
    CT_TRAINER_MEMO_DECOMP_RESERVE = CT_TRAINER_DECOMP_LINES * CT_TRAINER_DECOMP_STEP
};

static const char *const CT_TRAINER_EVENT_NAMES[] = {
    "3x3", "2x2", "3bld", "4bld",  "5bld",     "mbld", "fmc",   "oh",  "4x4",
    "5x5", "6x6", "7x7",  "clock", "megaminx", "pyra", "skewb", "sq1", "fto",
};

static const char *const CT_TRAINER_METHOD_NAMES[] = {
    "cfop", "3style", "old pochmann", "m2",        "u2",  "r2",  "lbl",     "l4e",
    "4lll", "sarah",  "yau",          "reduction", "cll", "l4t", "concept",
};

static const char *const CT_TRAINER_STAGE_NAMES[] = {
    "pll", "oll", "f2l",  "cll",   "cubeshape", "co",     "eo",      "cp",
    "ep",  "l4e", "4lll", "sarah", "l4t",       "parity", "concept",
};

static const char *ct_trainer_event_name(ct_event ev) {
    unsigned long i = (unsigned long)ev;
    unsigned long count = sizeof CT_TRAINER_EVENT_NAMES / sizeof CT_TRAINER_EVENT_NAMES[0];
    return i < count ? CT_TRAINER_EVENT_NAMES[i] : "?";
}

static const char *ct_trainer_method_name(ct_method m) {
    unsigned long i = (unsigned long)m;
    unsigned long count = sizeof CT_TRAINER_METHOD_NAMES / sizeof CT_TRAINER_METHOD_NAMES[0];
    return i < count ? CT_TRAINER_METHOD_NAMES[i] : "?";
}

static const char *ct_trainer_stage_name(ct_stage st) {
    unsigned long i = (unsigned long)st;
    unsigned long count = sizeof CT_TRAINER_STAGE_NAMES / sizeof CT_TRAINER_STAGE_NAMES[0];
    return i < count ? CT_TRAINER_STAGE_NAMES[i] : "?";
}

static void ct_trainer_memo_clear_case(ct_trainer_form *f) {
    f->memo_saved_phoneme[0] = '\0';
    f->memo_saved_image[0] = '\0';
    f->memo_phoneme[0] = '\0';
    f->memo_image[0] = '\0';
}

static void ct_trainer_memo_reload_rows(ct_trainer_form *f);

static void ct_trainer_form_reload(ct_trainer_form *f) {
    f->list_count = ct_selector_lists(f->sel.event, f->sel.method, f->lists, CT_TRAINER_LISTS_MAX);
    f->case_count = 0;
    f->group_count = 0;
    for (int g = 0; g < CT_SELECTOR_GROUPS_MAX; g++) {
        f->group_counts[g] = 0;
    }
    f->cell = CT_TRAINER_CELL_NONE;
    f->sel.case_index = -1;
    f->front[0] = '\0';
    f->alg_text[0] = '\0';
    f->pack_status = CT_TRAINER_NO_DETAIL;
    f->alg_status = CT_TRAINER_NO_DETAIL;
    f->memo_row_count = 0;
    ct_trainer_memo_clear_case(f);
    if (f->list_count <= 0) {
        f->status = CT_REPO_ERR_DB;
        return;
    }
    if (f->sel.list_id >= f->list_count) {
        f->sel.list_id = 0;
    }
    int n = ct_selector_case_count(f->db, &f->scheme, f->sel.event, f->sel.method, f->sel.list_id);
    f->case_count = n > 0 ? n : 0;
    for (int i = 0; i < f->case_count; i++) {
        int group = ct_selector_case_group(f->db, f->sel.event, f->sel.method, f->sel.list_id, i);
        if (group < 0) {
            continue;
        }
        f->group_counts[group]++;
        if (group + 1 > f->group_count) {
            f->group_count = group + 1;
        }
    }
    ct_trainer_memo_reload_rows(f);
    f->status = CT_REPO_OK;
}

static int ct_trainer_form_refresh_inputs(ct_trainer_form *f) {
    if (ct_repo_settings_merge(f->db, &f->cfg) != 0) {
        return CT_REPO_ERR_DB;
    }
    if (ct_selector_scheme_load(f->db, &f->scheme) != 0) {
        return CT_REPO_ERR_DB;
    }
    f->scheme_id = ct_letter_scheme_default_id(f->db);
    return CT_REPO_OK;
}

int ct_trainer_form_open(ct_trainer_form *f, sqlite3 *db) {
    if (f == NULL || db == NULL) {
        return CT_REPO_ERR_ARG;
    }
    memset(f, 0, sizeof *f);
    f->db = db;
    f->cfg = ct_config_defaults();
    if (ct_trainer_form_refresh_inputs(f) != CT_REPO_OK) {
        f->status = CT_REPO_ERR_DB;
        return CT_REPO_ERR_DB;
    }
    f->sel.event = CT_EVENT_3X3;
    ct_method methods[CT_TRAINER_LISTS_MAX];
    int method_count = ct_selector_methods(f->sel.event, methods, CT_TRAINER_LISTS_MAX);
    if (method_count <= 0) {
        f->status = CT_REPO_ERR_DB;
        return CT_REPO_ERR_DB;
    }
    f->sel.method = methods[0];
    f->sel.list_id = 0;
    ct_trainer_form_reload(f);
    return f->status;
}

int ct_trainer_form_select_event(ct_trainer_form *f, ct_event ev) {
    if (f == NULL || f->db == NULL) {
        return CT_REPO_ERR_ARG;
    }
    ct_method methods[CT_TRAINER_LISTS_MAX];
    int method_count = ct_selector_methods(ev, methods, CT_TRAINER_LISTS_MAX);
    if (method_count <= 0) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    f->sel.event = ev;
    f->sel.method = methods[0];
    f->sel.list_id = 0;
    ct_trainer_form_reload(f);
    return f->status;
}

int ct_trainer_form_select_method(ct_trainer_form *f, ct_method m) {
    if (f == NULL || f->db == NULL) {
        return CT_REPO_ERR_ARG;
    }
    if (ct_selector_lists(f->sel.event, m, NULL, 0) <= 0) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    f->sel.method = m;
    f->sel.list_id = 0;
    ct_trainer_form_reload(f);
    return f->status;
}

int ct_trainer_form_select_list(ct_trainer_form *f, int list_id) {
    if (f == NULL || f->db == NULL) {
        return CT_REPO_ERR_ARG;
    }
    if (list_id < 0 || list_id >= f->list_count) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    f->sel.list_id = list_id;
    ct_trainer_form_reload(f);
    return f->status;
}

static int ct_trainer_list_source(const ct_trainer_form *f, ct_selector_source *source) {
    if (f->sel.list_id < 0 || f->sel.list_id >= f->list_count) {
        return CT_REPO_ERR_DB;
    }
    *source = f->lists[f->sel.list_id].source;
    return CT_REPO_OK;
}

static void ct_trainer_memo_reload_rows(ct_trainer_form *f) {
    f->memo_row_count = 0;
    ct_selector_source source;
    if (f->db == NULL || ct_trainer_list_source(f, &source) != CT_REPO_OK ||
        source != CT_SELECTOR_CHART) {
        return;
    }
    int n =
        ct_memo_list(f->db, f->lists[f->sel.list_id].orbit, f->memo_rows, CT_TRAINER_MEMO_ROWS_MAX);
    f->memo_row_count = n > 0 ? n : 0;
}

static void ct_trainer_memo_load_case(ct_trainer_form *f) {
    ct_trainer_memo_clear_case(f);
    ct_selector_source source;
    if (f->db == NULL || ct_trainer_list_source(f, &source) != CT_REPO_OK ||
        source != CT_SELECTOR_CHART) {
        return;
    }
    ct_memo_row row;
    if (ct_memo_get(f->db, f->lists[f->sel.list_id].orbit, f->front, &row) != CT_REPO_OK) {
        return;
    }
    snprintf(f->memo_saved_phoneme, sizeof f->memo_saved_phoneme, "%s", row.phoneme);
    snprintf(f->memo_saved_image, sizeof f->memo_saved_image, "%s", row.image);
    snprintf(f->memo_phoneme, sizeof f->memo_phoneme, "%s", row.phoneme);
    snprintf(f->memo_image, sizeof f->memo_image, "%s", row.image);
}

int ct_trainer_form_select_case(ct_trainer_form *f, int index) {
    if (f == NULL || f->db == NULL) {
        return CT_REPO_ERR_ARG;
    }
    if (index < 0 || index >= f->case_count) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if (ct_trainer_form_refresh_inputs(f) != CT_REPO_OK) {
        f->status = CT_REPO_ERR_DB;
        return CT_REPO_ERR_DB;
    }
    f->sel.case_index = index;
    f->front[0] = '\0';
    f->alg_text[0] = '\0';
    f->pack_status = CT_TRAINER_NO_DETAIL;
    f->alg_status = CT_TRAINER_NO_DETAIL;
    ct_trainer_memo_clear_case(f);
    ct_selector_source source;
    if (ct_trainer_list_source(f, &source) != CT_REPO_OK) {
        f->status = CT_REPO_ERR_DB;
        return CT_REPO_ERR_DB;
    }
    if (source == CT_SELECTOR_PACK) {
        ct_pack_entry e;
        int rc =
            ct_selector_pack_case(f->db, f->sel.event, f->sel.method, f->sel.list_id, index, &e);
        f->pack_status = rc == 0 ? CT_PACK_OK : CT_PACK_ERR_DB;
        if (rc != 0) {
            f->status = CT_REPO_MISSING;
            return CT_REPO_MISSING;
        }
        snprintf(f->front, sizeof f->front, "%s", e.front_json);
        snprintf(f->alg_text, sizeof f->alg_text, "%s", e.alg);
        f->status = CT_REPO_OK;
        return CT_REPO_OK;
    }
    int rc = ct_selector_case_key(f->db, &f->scheme, f->sel.event, f->sel.method, f->sel.list_id,
                                  index, f->front, (unsigned)sizeof f->front);
    if (rc != 0) {
        f->front[0] = '\0';
        f->status = CT_REPO_ERR_DB;
        return CT_REPO_ERR_DB;
    }
    if (source == CT_SELECTOR_CHART) {
        char hash[CT_ALG_HASH_HEX_MAX];
        ct_params_hash(&f->cfg, f->scheme_id, CT_BUFFER_FIXED, f->scheme.buffer_corner,
                       f->scheme.buffer_edge, &f->scheme.map, hash);
        int arc = ct_repo_alg_get(f->db, f->lists[f->sel.list_id].orbit, f->front, CT_BUFFER_FIXED,
                                  CT_TRAINER_ALG_CATEGORY, hash, &f->alg);
        f->alg_status = arc;
        if (arc == CT_REPO_OK) {
            snprintf(f->alg_text, sizeof f->alg_text, "%s", f->alg.sequence);
        }
    }
    ct_trainer_memo_load_case(f);
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_trainer_form_memo_save(ct_trainer_form *f) {
    if (f == NULL || f->db == NULL) {
        return CT_REPO_ERR_ARG;
    }
    ct_selector_source source;
    if (ct_trainer_list_source(f, &source) != CT_REPO_OK || source != CT_SELECTOR_CHART ||
        f->sel.case_index < 0 || f->front[0] == '\0') {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    int rc = ct_memo_set(f->db, f->lists[f->sel.list_id].orbit, f->front, f->memo_phoneme,
                         f->memo_image);
    f->status = rc;
    if (rc != CT_REPO_OK) {
        return rc;
    }
    ct_trainer_memo_load_case(f);
    ct_trainer_memo_reload_rows(f);
    return CT_REPO_OK;
}

const ct_memo_list_row *ct_trainer_form_memo_find(const ct_trainer_form *f, const char *pair) {
    if (f == NULL || pair == NULL) {
        return NULL;
    }
    int lo = 0;
    int hi = f->memo_row_count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(f->memo_rows[mid].pair, pair);
        if (cmp == 0) {
            return &f->memo_rows[mid];
        }
        if (cmp < 0) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return NULL;
}

int ct_trainer_form_select_cell(ct_trainer_form *f, int sticker) {
    if (f == NULL) {
        return CT_REPO_ERR_ARG;
    }
    ct_selector_source source;
    if (ct_trainer_list_source(f, &source) != CT_REPO_OK || source != CT_SELECTOR_CHART ||
        sticker < 0 || sticker >= CT_LETTER_STICKERS) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    f->cell = sticker;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_trainer_form_clear_cell(ct_trainer_form *f) {
    if (f == NULL) {
        return CT_REPO_ERR_ARG;
    }
    f->cell = CT_TRAINER_CELL_NONE;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

static int ct_trainer_cell_matches(const ct_trainer_form *f, int index) {
    int t1 = -1;
    int t2 = -1;
    if (ct_selector_case_pair(&f->scheme, f->sel.event, f->sel.method, f->sel.list_id, index, &t1,
                              &t2) != 0) {
        return 0;
    }
    return t1 == f->cell || t2 == f->cell;
}

int ct_trainer_form_visible_count(const ct_trainer_form *f) {
    if (f == NULL || f->case_count <= 0) {
        return 0;
    }
    ct_selector_source source;
    if (ct_trainer_list_source(f, &source) != CT_REPO_OK || source != CT_SELECTOR_CHART ||
        f->cell == CT_TRAINER_CELL_NONE) {
        return f->case_count;
    }
    int total = 0;
    for (int i = 0; i < f->case_count; i++) {
        if (ct_trainer_cell_matches(f, i)) {
            total++;
        }
    }
    return total;
}

int ct_trainer_form_nth_visible(const ct_trainer_form *f, int visible) {
    if (f == NULL || visible < 0) {
        return -1;
    }
    ct_selector_source source;
    if (ct_trainer_list_source(f, &source) != CT_REPO_OK || source != CT_SELECTOR_CHART ||
        f->cell == CT_TRAINER_CELL_NONE) {
        return visible < f->case_count ? visible : -1;
    }
    int seen = 0;
    for (int i = 0; i < f->case_count; i++) {
        if (!ct_trainer_cell_matches(f, i)) {
            continue;
        }
        if (seen == visible) {
            return i;
        }
        seen++;
    }
    return -1;
}

int ct_trainer_decomp(const ct_trainer_form *f, char (*lines)[CT_TRAINER_DECOMP_MAX], int cap) {
    if (f == NULL || lines == NULL || cap <= 0) {
        return 0;
    }
    if (f->sel.case_index < 0 || f->alg_status != CT_REPO_OK || f->alg.sequence[0] == '\0') {
        return 0;
    }
    int n = 0;
    if (n < cap && f->alg.setup[0] != '\0') {
        snprintf(lines[n++], CT_TRAINER_DECOMP_MAX, "setup %s", f->alg.setup);
    }
    if (n < cap && f->alg.interchange[0] != '\0') {
        snprintf(lines[n++], CT_TRAINER_DECOMP_MAX, "interchange %s", f->alg.interchange);
    }
    if (n < cap && f->alg.insertion[0] != '\0') {
        snprintf(lines[n++], CT_TRAINER_DECOMP_MAX, "insertion %s", f->alg.insertion);
    }
    if (n < cap) {
        snprintf(lines[n++], CT_TRAINER_DECOMP_MAX, "class %s",
                 ct_classification_label((ct_classification)f->alg.classification));
    }
    return n;
}

typedef struct {
    ct_trainer_form form;
    bool opened;
    int event_scroll;
    int method_scroll;
    int list_scroll;
    int case_scroll;
    int case_active;
    bool memo_edit_phoneme;
    bool memo_edit_image;
    char event_labels[CT_TRAINER_EVENTS_MAX][CT_TRAINER_KEY_MAX];
    char *event_ptrs[CT_TRAINER_EVENTS_MAX];
    char method_labels[CT_TRAINER_LISTS_MAX][CT_TRAINER_KEY_MAX];
    char *method_ptrs[CT_TRAINER_LISTS_MAX];
    char list_labels[CT_TRAINER_LISTS_MAX][CT_TRAINER_KEY_MAX];
    char *list_ptrs[CT_TRAINER_LISTS_MAX];
    char case_labels[CT_TRAINER_CASES_MAX][CT_TRAINER_KEY_MAX];
    char *case_ptrs[CT_TRAINER_CASES_MAX];
} ct_trainer_screen;

static ct_trainer_screen ct_trainer;

ct_trainer_form *ct_screen_trainer_form(void) {
    return &ct_trainer.form;
}

static void ct_trainer_build_labels(ct_trainer_screen *s) {
    ct_trainer_form *f = &s->form;
    int events = ct_selector_events(NULL, 0);
    for (int i = 0; i < events && i < CT_TRAINER_EVENTS_MAX; i++) {
        snprintf(s->event_labels[i], sizeof s->event_labels[i], "%s",
                 ct_trainer_event_name((ct_event)i));
        s->event_ptrs[i] = s->event_labels[i];
    }
    ct_method methods[CT_TRAINER_LISTS_MAX];
    int method_count = ct_selector_methods(f->sel.event, methods, CT_TRAINER_LISTS_MAX);
    for (int i = 0; i < method_count; i++) {
        snprintf(s->method_labels[i], sizeof s->method_labels[i], "%s",
                 ct_trainer_method_name(methods[i]));
        s->method_ptrs[i] = s->method_labels[i];
    }
    for (int i = 0; i < f->list_count; i++) {
        snprintf(s->list_labels[i], sizeof s->list_labels[i], "%s",
                 ct_trainer_stage_name(f->lists[i].stage));
        s->list_ptrs[i] = s->list_labels[i];
    }
    int visible = ct_trainer_form_visible_count(f);
    s->case_active = -1;
    for (int v = 0; v < visible && v < CT_TRAINER_CASES_MAX; v++) {
        int index = ct_trainer_form_nth_visible(f, v);
        if (index < 0 || ct_selector_case_key(f->db, &f->scheme, f->sel.event, f->sel.method,
                                              f->sel.list_id, index, s->case_labels[v],
                                              (unsigned)sizeof s->case_labels[v]) != 0) {
            s->case_labels[v][0] = '\0';
        } else {
            const ct_memo_list_row *row = ct_trainer_form_memo_find(f, s->case_labels[v]);
            if (row != NULL && row->image[0] != '\0') {
                char key[CT_TRAINER_KEY_MAX];
                snprintf(key, sizeof key, "%s", s->case_labels[v]);
                snprintf(s->case_labels[v], sizeof s->case_labels[v], "%s %s", key, row->image);
            }
        }
        s->case_ptrs[v] = s->case_labels[v];
        if (index == f->sel.case_index) {
            s->case_active = v;
        }
    }
}

static void ct_trainer_on_event(int active) {
    if (active >= 0 && active < CT_TRAINER_EVENTS_MAX) {
        ct_trainer_form_select_event(&ct_trainer.form, (ct_event)active);
    }
}

static void ct_trainer_on_method(int active) {
    ct_method methods[CT_TRAINER_LISTS_MAX];
    int method_count =
        ct_selector_methods(ct_trainer.form.sel.event, methods, CT_TRAINER_LISTS_MAX);
    if (active >= 0 && active < method_count) {
        ct_trainer_form_select_method(&ct_trainer.form, methods[active]);
    }
}

static void ct_trainer_on_list(int active) {
    if (active >= 0) {
        ct_trainer_form_select_list(&ct_trainer.form, active);
    }
}

static void ct_trainer_on_case(int active) {
    int index = ct_trainer_form_nth_visible(&ct_trainer.form, active);
    if (index >= 0) {
        ct_trainer_form_select_case(&ct_trainer.form, index);
    }
}

void ct_screen_trainer_update(ct_app *app) {
    if (app == NULL) {
        return;
    }
    if (!ct_trainer.opened || ct_trainer.form.db != app->db) {
        ct_trainer.opened = true;
        ct_trainer_form_open(&ct_trainer.form, app->db);
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        app->screen = CT_SCREEN_MENU;
    }
}

static Rectangle ct_trainer_rect(int x, int y, int w, int h) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    return r;
}

static void ct_trainer_draw_grid(ct_trainer_form *f, Rectangle bounds) {
    GuiGroupBox(bounds, "letters");
    const char *letters =
        f->lists[f->sel.list_id].orbit == CT_ORBIT_EDGE ? f->scheme.map.edge : f->scheme.map.corner;
    int buffer = f->lists[f->sel.list_id].orbit == CT_ORBIT_EDGE ? f->scheme.buffer_edge
                                                                 : f->scheme.buffer_corner;
    Font font = ct_ui_font();
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        Rectangle cell;
        if (ct_heatmap_cell(bounds, CT_TRAINER_GRID_COLS, CT_TRAINER_GRID_ROWS, i, &cell) != 0) {
            continue;
        }
        if (i == f->cell) {
            DrawRectangleRec(cell, GetColor(CT_UI_COLOR_ACCENT_TINT));
        } else if (i == buffer) {
            DrawRectangleRec(cell, GetColor(CT_UI_COLOR_PANEL_ACTIVE));
        }
        if (i == f->cell) {
            DrawRectangleLinesEx(cell, (float)CT_UI_CELL_FOCUS_LINE, GetColor(CT_UI_COLOR_ACCENT));
        } else if (i == buffer) {
            DrawRectangleLinesEx(cell, (float)CT_UI_CELL_FOCUS_LINE, GetColor(CT_UI_COLOR_WARN));
        } else {
            DrawRectangleLines((int)cell.x, (int)cell.y, (int)cell.width, (int)cell.height,
                               GetColor(CT_UI_COLOR_BORDER));
        }
        char label[2] = {letters[i], '\0'};
        Color tint = GetColor(CT_UI_COLOR_TEXT);
        if (i == f->cell) {
            tint = GetColor(CT_UI_COLOR_ACCENT);
        } else if (i == buffer) {
            tint = GetColor(CT_UI_COLOR_WARN);
        }
        float size = cell.height / CT_TRAINER_HALF;
        float tw = MeasureTextEx(font, label, size, CT_UI_TEXT_SPACING).x;
        Vector2 pos = {cell.x + (cell.width - tw) / CT_TRAINER_HALF,
                       cell.y + (cell.height - size) / CT_TRAINER_HALF};
        DrawTextEx(font, label, pos, size, CT_UI_TEXT_SPACING, tint);
    }
    Vector2 mouse = GetMousePosition();
    int picked = ct_heatmap_pick(bounds, CT_TRAINER_GRID_COLS, CT_TRAINER_GRID_ROWS, mouse);
    if (picked >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (picked == f->cell) {
            ct_trainer_form_clear_cell(f);
        } else {
            ct_trainer_form_select_cell(f, picked);
        }
    }
}

void ct_screen_trainer_draw(ct_app *app) {
    ClearBackground(GetColor(CT_UI_COLOR_BG));
    if (app == NULL) {
        return;
    }
    ct_trainer_form *f = &ct_trainer.form;
    int w = (int)app->cfg.window.width;
    int h = (int)app->cfg.window.height;
    int margin = CT_UI_MARGIN;
    int row_h = CT_UI_ROW_H;
    float text_size = (float)CT_UI_FONT_BODY;
    int col_w = w / CT_UI_COL_DIV - margin;
    int events = ct_selector_events(NULL, 0);
    ct_method methods[CT_TRAINER_LISTS_MAX];
    int method_count = ct_selector_methods(f->sel.event, methods, CT_TRAINER_LISTS_MAX);
    int col_h = h - margin - CT_UI_STATUS_H - margin;
    int list_y = margin;
    int stage_y = list_y;
    int stage_h = row_h + margin;
    int chart = f->list_count > 0 && f->lists[f->sel.list_id].source == CT_SELECTOR_CHART;
    int grid_h = row_h * CT_TRAINER_GRID_ROWS + CT_UI_ROW_GAP;
    int grid_y = stage_y + stage_h + CT_UI_ROW_GAP;
    int case_y = chart ? grid_y + grid_h + CT_UI_ROW_GAP : stage_y + stage_h + CT_UI_ROW_GAP;
    int case_h = h - case_y - CT_UI_STATUS_H - margin;
    int detail_x = margin + CT_TRAINER_DETAIL_COL * (col_w + margin);
    int detail_w = w - detail_x - margin;
    int detail_h = h - 2 * margin - CT_UI_STATUS_H;
    int detail_pad = margin;
    int line1_y = margin + row_h;
    int line2_y = line1_y + row_h;
    int front_y = line2_y + row_h + CT_UI_ROW_GAP;
    int alg_y = front_y + row_h;
    int decomp_y = alg_y + row_h + CT_UI_ROW_GAP;

    ct_trainer_build_labels(&ct_trainer);
    if (f->list_count <= 0 || f->db == NULL) {
        GuiStatusBar(ct_trainer_rect(margin, h - CT_UI_STATUS_H, w - 2 * margin, CT_UI_STATUS_H),
                     "no lists for this event");
        return;
    }
    int event_active = (int)f->sel.event;
    if (GuiListViewEx(ct_trainer_rect(margin, list_y, col_w, col_h), ct_trainer.event_ptrs, events,
                      &ct_trainer.event_scroll, &event_active, NULL)) {
        ct_trainer_on_event(event_active);
    }
    int method_active = -1;
    for (int i = 0; i < method_count; i++) {
        if (methods[i] == f->sel.method) {
            method_active = i;
        }
    }
    if (GuiListViewEx(ct_trainer_rect(margin + col_w + margin, list_y, col_w, col_h),
                      ct_trainer.method_ptrs, method_count, &ct_trainer.method_scroll,
                      &method_active, NULL)) {
        ct_trainer_on_method(method_active);
    }
    int list_active = f->sel.list_id;
    if (GuiListViewEx(ct_trainer_rect(margin + 2 * (col_w + margin), stage_y, col_w, stage_h),
                      ct_trainer.list_ptrs, f->list_count, &ct_trainer.list_scroll, &list_active,
                      NULL)) {
        ct_trainer_on_list(list_active);
    }
    if (chart) {
        ct_trainer_draw_grid(f,
                             ct_trainer_rect(margin + 2 * (col_w + margin), grid_y, col_w, grid_h));
    }
    int visible = ct_trainer_form_visible_count(f);
    if (visible > CT_TRAINER_CASES_MAX) {
        visible = CT_TRAINER_CASES_MAX;
    }
    if (GuiListViewEx(ct_trainer_rect(margin + 2 * (col_w + margin), case_y, col_w, case_h),
                      ct_trainer.case_ptrs, visible, &ct_trainer.case_scroll,
                      &ct_trainer.case_active, NULL)) {
        ct_trainer_on_case(ct_trainer.case_active);
    }

    GuiGroupBox(ct_trainer_rect(detail_x, margin, detail_w, detail_h), "case");
    char line[CT_TRAINER_KEY_MAX];
    snprintf(line, sizeof line, "event %s method %s", ct_trainer_event_name(f->sel.event),
             ct_trainer_method_name(f->sel.method));
    GuiLabel(ct_trainer_rect(detail_x + detail_pad, line1_y, detail_w - 2 * detail_pad, row_h),
             line);
    if (f->group_count > 1) {
        int written =
            snprintf(line, sizeof line, "list %s cases %d |",
                     ct_trainer_stage_name(f->lists[f->sel.list_id].stage), f->case_count);
        for (int g = 0; g < f->group_count && g < CT_SELECTOR_GROUPS_MAX; g++) {
            if (written < 0 || (unsigned)written >= sizeof line) {
                break;
            }
            written += snprintf(line + written, sizeof line - (unsigned)written, " %s %d",
                                ct_selector_group_label(g), f->group_counts[g]);
        }
    } else {
        snprintf(line, sizeof line, "list %s cases %d",
                 ct_trainer_stage_name(f->lists[f->sel.list_id].stage), f->case_count);
    }
    GuiLabel(ct_trainer_rect(detail_x + detail_pad, line2_y, detail_w - 2 * detail_pad, row_h),
             line);
    Font font = ct_ui_font();
    Vector2 detail_pos = {(float)(detail_x + detail_pad), 0.0f};
    if (f->sel.case_index >= 0) {
        detail_pos.y = (float)front_y;
        DrawTextEx(font, f->front, detail_pos, text_size, CT_UI_TEXT_SPACING,
                   GetColor(CT_UI_COLOR_TEXT));
        detail_pos.y = (float)alg_y;
        DrawTextEx(font, f->alg_text[0] != '\0' ? f->alg_text : "no alg", detail_pos, text_size,
                   CT_UI_TEXT_SPACING,
                   f->alg_text[0] != '\0' ? GetColor(CT_UI_COLOR_TEXT)
                                          : GetColor(CT_UI_COLOR_TEXT_DIM));
    } else {
        detail_pos.y = (float)front_y;
        DrawTextEx(font, "no case selected", detail_pos, text_size, CT_UI_TEXT_SPACING,
                   GetColor(CT_UI_COLOR_TEXT_DIM));
    }
    char decomp[CT_TRAINER_DECOMP_LINES][CT_TRAINER_DECOMP_MAX];
    int decomp_count = ct_trainer_decomp(f, decomp, CT_TRAINER_DECOMP_LINES);
    for (int i = 0; i < decomp_count; i++) {
        detail_pos.y = (float)(decomp_y + i * CT_TRAINER_DECOMP_STEP);
        DrawTextEx(font, decomp[i], detail_pos, text_size, CT_UI_TEXT_SPACING,
                   GetColor(CT_UI_COLOR_TEXT_DIM));
    }
    int back_y = h - CT_UI_STATUS_H - margin - row_h;
    if (chart && f->sel.case_index >= 0) {
        int memo_y = decomp_y + CT_TRAINER_MEMO_DECOMP_RESERVE + CT_TRAINER_MEMO_GAP;
        int memo_h = back_y - CT_UI_ROW_GAP - memo_y;
        int memo_pad = CT_UI_MEMO_PAD;
        int memo_w = detail_w - 2 * memo_pad;
        GuiGroupBox(ct_trainer_rect(detail_x, memo_y, detail_w, memo_h), "memo");
        char memo_line[CT_MEMO_TEXT_MAX + CT_TRAINER_KEY_MAX];
        int row1_y = memo_y + memo_pad;
        int row2_y = row1_y + CT_UI_MEMO_ROW_H;
        snprintf(memo_line, sizeof memo_line, "phoneme %s", f->memo_saved_phoneme);
        GuiLabel(ct_trainer_rect(detail_x + memo_pad, row1_y, memo_w, CT_UI_MEMO_ROW_H), memo_line);
        snprintf(memo_line, sizeof memo_line, "image %s", f->memo_saved_image);
        GuiLabel(ct_trainer_rect(detail_x + memo_pad, row2_y, memo_w, CT_UI_MEMO_ROW_H), memo_line);
        int box_y = row2_y + CT_UI_MEMO_ROW_H + CT_TRAINER_MEMO_GAP;
        int box_w = (memo_w - CT_UI_MEMO_FIELD_GAP) / 2;
        if (GuiTextBox(ct_trainer_rect(detail_x + memo_pad, box_y, box_w, CT_UI_MEMO_FIELD_H),
                       f->memo_phoneme, (int)sizeof f->memo_phoneme,
                       ct_trainer.memo_edit_phoneme)) {
            ct_trainer.memo_edit_phoneme = !ct_trainer.memo_edit_phoneme;
        }
        if (GuiTextBox(ct_trainer_rect(detail_x + memo_pad + box_w + CT_UI_MEMO_FIELD_GAP, box_y,
                                       box_w, CT_UI_MEMO_FIELD_H),
                       f->memo_image, (int)sizeof f->memo_image, ct_trainer.memo_edit_image)) {
            ct_trainer.memo_edit_image = !ct_trainer.memo_edit_image;
        }
        int save_y = box_y + CT_UI_MEMO_FIELD_H + CT_TRAINER_MEMO_GAP;
        if (GuiButton(ct_trainer_rect(detail_x + memo_pad, save_y, memo_w, CT_UI_MEMO_FIELD_H),
                      "save memo")) {
            ct_trainer.memo_edit_phoneme = false;
            ct_trainer.memo_edit_image = false;
            ct_trainer_form_memo_save(f);
        }
    }
    if (GuiButton(ct_trainer_rect(detail_x, back_y, detail_w, row_h), "back")) {
        app->screen = CT_SCREEN_MENU;
    }
    GuiStatusBar(ct_trainer_rect(margin, h - CT_UI_STATUS_H, w - 2 * margin, CT_UI_STATUS_H),
                 f->status == CT_REPO_OK ? "ok" : "error");
}
