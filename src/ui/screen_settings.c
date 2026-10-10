#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raygui.h"
#include "raylib.h"
#include "render/renderer.h"
#include "ui/ui.h"

static const char CT_SETTINGS_KEY_RETENTION[] = "srs.request_retention";
static const char CT_SETTINGS_KEY_SECONDS[] = "anim.move_seconds";
static const char CT_SETTINGS_KEY_MOVES[] = "gen.preferred_moves";
static const char CT_SETTINGS_KEY_BUFFER_CORNER[] = "bld.buffer_corner";
static const char CT_SETTINGS_KEY_BUFFER_EDGE[] = "bld.buffer_edge";

static const char *const CT_SETTINGS_COLOR_DEFAULTS[CT_SETTINGS_FACE_COUNT] = {
    "FFFFFF", "FF5800", "009B48", "B71234", "0046AD", "FFD500",
};

static const char CT_SETTINGS_FACE_NAMES[CT_SETTINGS_FACE_COUNT][2] = {
    "U", "L", "F", "R", "B", "D",
};

enum {
    CT_SETTINGS_ORBITS = 2,
    CT_SETTINGS_HEX_SHIFT = 4,
    CT_SETTINGS_HEX_ALPHA = ('9' - '0') + 1,
    CT_SETTINGS_TAB_GENERAL = 0,
    CT_SETTINGS_TAB_LETTERS,
    CT_SETTINGS_TAB_BLD,
    CT_SETTINGS_TAB_COUNT = 3,
    CT_SETTINGS_MARGIN_SHIFT = 5,
    CT_SETTINGS_ROW_H_SHIFT = 4,
    CT_SETTINGS_LABEL_W_DIV = 8,
    CT_SETTINGS_GRID_COLS = CT_LETTER_FACES,
    CT_SETTINGS_GRID_ROWS = CT_LETTER_PER_FACE,
    CT_SETTINGS_MOVE_COLS = CT_LETTER_FACES,
    CT_SETTINGS_BTN_COUNT = 2,
    CT_SETTINGS_STATUS_MAX = (1 << 6),
};

static char *ct_settings_orbit(ct_settings_form *f, int orbit) {
    if (orbit == CT_ORBIT_CORNER)
        return f->staged.corner;
    if (orbit == CT_ORBIT_EDGE)
        return f->staged.edge;
    return NULL;
}

static int ct_settings_hex_nibble(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + CT_SETTINGS_HEX_ALPHA;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + CT_SETTINGS_HEX_ALPHA;
    return -1;
}

static bool ct_settings_parse_color(const char *text, unsigned char out[CT_SETTINGS_CHANNELS]) {
    if (strlen(text) != 2 * (size_t)CT_SETTINGS_CHANNELS)
        return false;
    for (int ch = 0; ch < CT_SETTINGS_CHANNELS; ch++) {
        int off = 2 * ch;
        int hi = ct_settings_hex_nibble(text[off]);
        int lo = ct_settings_hex_nibble(text[off + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out[ch] = (unsigned char)((hi << CT_SETTINGS_HEX_SHIFT) | lo);
    }
    return true;
}

static void ct_settings_load_cfg(ct_settings_form *f) {
    ct_config cfg = ct_config_defaults();
    char buf[CT_SETTINGS_TEXT_MAX];
    if (ct_repo_settings_get(f->db, CT_SETTINGS_KEY_RETENTION, buf, sizeof buf) == 0)
        (void)ct_config_set(&cfg, CT_SETTINGS_KEY_RETENTION, buf);
    if (ct_repo_settings_get(f->db, CT_SETTINGS_KEY_SECONDS, buf, sizeof buf) == 0)
        (void)ct_config_set(&cfg, CT_SETTINGS_KEY_SECONDS, buf);
    if (ct_repo_settings_get(f->db, CT_SETTINGS_KEY_MOVES, buf, sizeof buf) == 0)
        (void)ct_config_set(&cfg, CT_SETTINGS_KEY_MOVES, buf);
    f->retention = cfg.fsrs.request_retention;
    f->move_seconds = cfg.anim.move_seconds;
    f->move_mask =
        cfg.gen.preferred_moves == 0 ? CT_GEN_DEFAULT_PREFERRED_MOVES : cfg.gen.preferred_moves;
}

static int ct_settings_load_buffer(sqlite3 *db, const char *key, int fallback) {
    char buf[CT_SETTINGS_TEXT_MAX];
    if (ct_repo_settings_get(db, key, buf, sizeof buf) != 0)
        return fallback;
    errno = 0;
    char *end = NULL;
    long v = strtol(buf, &end, CT_DECIMAL_BASE);
    if (end == buf || end[0] != '\0' || errno == ERANGE)
        return fallback;
    if (v < 0 || v >= CT_LETTER_STICKERS)
        return fallback;
    return (int)v;
}

static void ct_settings_load_colors(ct_settings_form *f) {
    char buf[CT_SETTINGS_TEXT_MAX];
    for (int face = 0; face < CT_SETTINGS_FACE_COUNT; face++) {
        const char *src = CT_SETTINGS_COLOR_DEFAULTS[face];
        if (ct_repo_settings_get(f->db, ct_color_scheme_keys[face], buf, sizeof buf) == 0 &&
            ct_settings_parse_color(buf, f->colors[face])) {
            continue;
        }
        if (!ct_settings_parse_color(src, f->colors[face]))
            memset(f->colors[face], 0, CT_SETTINGS_CHANNELS);
    }
}

void ct_settings_form_open(ct_settings_form *f, sqlite3 *db) {
    if (f == NULL)
        return;
    memset(f, 0, sizeof *f);
    f->db = db;
    if (db == NULL) {
        f->status = CT_REPO_ERR_ARG;
        return;
    }
    int scheme = ct_letter_scheme_default_id(db);
    if (scheme <= 0) {
        if (ct_letter_seed_speffz(db) != 0) {
            f->status = CT_REPO_ERR_DB;
            return;
        }
        scheme = ct_letter_scheme_default_id(db);
    }
    if (scheme <= 0) {
        f->status = CT_REPO_ERR_DB;
        return;
    }
    f->scheme_id = scheme;
    if (ct_letter_map_load(db, scheme, &f->map) < 0) {
        f->status = CT_REPO_ERR_DB;
        return;
    }
    f->staged = f->map;
    ct_settings_load_cfg(f);
    f->buffer_corner =
        ct_settings_load_buffer(db, CT_SETTINGS_KEY_BUFFER_CORNER, CT_3STYLE_CORNER_BUFFER);
    f->buffer_edge =
        ct_settings_load_buffer(db, CT_SETTINGS_KEY_BUFFER_EDGE, CT_3STYLE_EDGE_BUFFER);
    ct_settings_load_colors(f);
    f->dirty = false;
    f->status = CT_REPO_OK;
}

int ct_settings_form_stage_letter(ct_settings_form *f, int orbit, int sticker, char letter) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    char *arr = ct_settings_orbit(f, orbit);
    if (arr == NULL || sticker < 0 || sticker >= CT_LETTER_STICKERS) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if (letter < CT_LETTER_FIRST || letter > CT_LETTER_LAST) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        if (i != sticker && arr[i] == letter) {
            f->status = CT_REPO_CONFLICT;
            return CT_REPO_CONFLICT;
        }
    }
    if (arr[sticker] == letter) {
        f->status = CT_REPO_OK;
        return CT_REPO_OK;
    }
    arr[sticker] = letter;
    f->dirty = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_settings_form_swap_letter(ct_settings_form *f, int orbit, int sticker, char letter) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    char *arr = ct_settings_orbit(f, orbit);
    if (arr == NULL || sticker < 0 || sticker >= CT_LETTER_STICKERS) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if (letter < CT_LETTER_FIRST || letter > CT_LETTER_LAST) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    int holder = -1;
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        if (i != sticker && arr[i] == letter) {
            holder = i;
        }
    }
    if (holder < 0 || arr[sticker] == letter) {
        return ct_settings_form_stage_letter(f, orbit, sticker, letter);
    }
    char freed = arr[sticker];
    arr[sticker] = letter;
    arr[holder] = freed;
    f->dirty = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_settings_form_stage_color(ct_settings_form *f, int face, int channel, int value) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if (face < 0 || face >= CT_SETTINGS_FACE_COUNT || channel < 0 ||
        channel >= CT_SETTINGS_CHANNELS || value < 0 || value > CT_SETTINGS_COLOR_MAX) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    f->colors[face][channel] = (unsigned char)value;
    f->dirty = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_settings_form_stage_move(ct_settings_form *f, unsigned bit) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if ((bit & CT_MOVE_ROTATION_MASK) != 0) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    bool known = false;
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        if ((1u << i) == bit)
            known = true;
    }
    if (!known) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if ((f->move_mask & ~bit) == 0) {
        f->status = CT_REPO_CONFLICT;
        return CT_REPO_CONFLICT;
    }
    f->move_mask ^= bit;
    f->dirty = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_settings_form_stage_retention(ct_settings_form *f, double v) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if (!isfinite(v)) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if (v < CT_FSRS_RETENTION_MIN)
        v = CT_FSRS_RETENTION_MIN;
    if (v > CT_FSRS_RETENTION_MAX)
        v = CT_FSRS_RETENTION_MAX;
    f->retention = v;
    f->dirty = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_settings_form_stage_move_seconds(ct_settings_form *f, double v) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if (!isfinite(v)) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if (v < CT_ANIM_MIN_MOVE_SECONDS)
        v = CT_ANIM_MIN_MOVE_SECONDS;
    f->move_seconds = v;
    f->dirty = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

int ct_settings_form_stage_buffer(ct_settings_form *f, int orbit, int sticker) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if (orbit != CT_ORBIT_CORNER && orbit != CT_ORBIT_EDGE) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if (sticker < 0 || sticker >= CT_LETTER_STICKERS) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    if (orbit == CT_ORBIT_CORNER)
        f->buffer_corner = sticker;
    else
        f->buffer_edge = sticker;
    f->dirty = true;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

static void ct_settings_format_moves(unsigned mask, char *out, size_t cap) {
    size_t n = 0;
    for (int i = 0; i < CT_FACE_COUNT && n + 1 < cap; i++) {
        if ((CT_MOVE_ROTATION_MASK & (1u << i)) != 0)
            continue;
        char c = ct_move_face_char(1u << i);
        if (c != '\0' && (mask & (1u << i)) != 0)
            out[n++] = c;
    }
    out[n] = '\0';
}

int ct_settings_form_save(ct_settings_form *f) {
    if (f == NULL)
        return CT_REPO_ERR_ARG;
    if (f->db == NULL || f->scheme_id <= 0) {
        f->status = CT_REPO_ERR_ARG;
        return CT_REPO_ERR_ARG;
    }
    sqlite3 *db = f->db;
    if (sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK) {
        f->status = CT_REPO_ERR_DB;
        return CT_REPO_ERR_DB;
    }
    int rc = CT_REPO_OK;
    for (int orbit = CT_ORBIT_CORNER; orbit <= CT_ORBIT_EDGE && rc == CT_REPO_OK; orbit++) {
        const char *arr = orbit == CT_ORBIT_CORNER ? f->staged.corner : f->staged.edge;
        for (int sticker = 0; sticker < CT_LETTER_STICKERS && rc == CT_REPO_OK; sticker++) {
            if (arr[sticker] == '\0')
                continue;
            if (ct_letter_map_set(db, f->scheme_id, orbit, sticker, arr[sticker]) != 0)
                rc = CT_REPO_ERR_DB;
        }
    }
    char buf[CT_SETTINGS_TEXT_MAX];
    if (rc == CT_REPO_OK) {
        snprintf(buf, sizeof buf, "%.6f", f->retention);
        if (ct_repo_settings_set(db, CT_SETTINGS_KEY_RETENTION, buf) != 0)
            rc = CT_REPO_ERR_DB;
    }
    if (rc == CT_REPO_OK) {
        snprintf(buf, sizeof buf, "%.6f", f->move_seconds);
        if (ct_repo_settings_set(db, CT_SETTINGS_KEY_SECONDS, buf) != 0)
            rc = CT_REPO_ERR_DB;
    }
    if (rc == CT_REPO_OK) {
        ct_settings_format_moves(f->move_mask, buf, sizeof buf);
        if (ct_repo_settings_set(db, CT_SETTINGS_KEY_MOVES, buf) != 0)
            rc = CT_REPO_ERR_DB;
    }
    if (rc == CT_REPO_OK) {
        snprintf(buf, sizeof buf, "%d", f->buffer_corner);
        if (ct_repo_settings_set(db, CT_SETTINGS_KEY_BUFFER_CORNER, buf) != 0)
            rc = CT_REPO_ERR_DB;
    }
    if (rc == CT_REPO_OK) {
        snprintf(buf, sizeof buf, "%d", f->buffer_edge);
        if (ct_repo_settings_set(db, CT_SETTINGS_KEY_BUFFER_EDGE, buf) != 0)
            rc = CT_REPO_ERR_DB;
    }
    for (int face = 0; face < CT_SETTINGS_FACE_COUNT && rc == CT_REPO_OK; face++) {
        snprintf(buf, sizeof buf, "%02X%02X%02X", (unsigned)f->colors[face][0],
                 (unsigned)f->colors[face][1], (unsigned)f->colors[face][2]);
        if (ct_repo_settings_set(db, ct_color_scheme_keys[face], buf) != 0)
            rc = CT_REPO_ERR_DB;
    }
    if (rc != CT_REPO_OK) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        f->status = rc;
        return rc;
    }
    if (sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        f->status = CT_REPO_ERR_DB;
        return CT_REPO_ERR_DB;
    }
    f->map = f->staged;
    f->dirty = false;
    f->status = CT_REPO_OK;
    return CT_REPO_OK;
}

void ct_settings_form_cancel(ct_settings_form *f) {
    if (f == NULL)
        return;
    ct_settings_form_open(f, f->db);
}

typedef struct {
    ct_settings_form form;
    bool opened;
    int tab;
    int tab_scroll;
    int sel_orbit;
    int sel_sticker;
    char letter_buf[2];
    bool letter_edit;
    char retention_buf[CT_SETTINGS_TEXT_MAX];
    bool retention_edit;
    char seconds_buf[CT_SETTINGS_TEXT_MAX];
    bool seconds_edit;
    int chan[CT_SETTINGS_FACE_COUNT][CT_SETTINGS_CHANNELS];
    unsigned chan_edit;
    char labels[CT_SETTINGS_ORBITS][CT_LETTER_STICKERS][2];
    int rejected;
} ct_settings_screen;

static ct_settings_screen ct_settings;

static const char *ct_settings_status_text(int status) {
    switch (status) {
    case CT_REPO_OK:
        return "ok";
    case CT_REPO_MISSING:
        return "not found";
    case CT_REPO_CONFLICT:
        return "duplicate letter or empty subset";
    case CT_REPO_ERR_ARG:
        return "invalid input";
    case CT_REPO_ERR_DB:
        return "db error";
    default:
        return "unknown";
    }
}

static Rectangle ct_settings_rect(int x, int y, int w, int h) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    return r;
}

static Rectangle ct_settings_cell_rect(int x, int y, int cell_w, int cell_h, int sticker) {
    int col = sticker / CT_SETTINGS_GRID_COLS;
    int row = sticker % CT_SETTINGS_GRID_COLS;
    return ct_settings_rect(x + col * cell_w, y + row * cell_h, cell_w, cell_h);
}

static void ct_settings_canon_double(char *buf, size_t cap, double v) {
    snprintf(buf, cap, "%.6f", v);
}

static void ct_settings_apply_double(const char *text, bool retention) {
    ct_settings_form *f = &ct_settings.form;
    errno = 0;
    char *end = NULL;
    double v = strtod(text, &end);
    if (end == text || end[0] != '\0' || errno == ERANGE)
        return;
    if (retention)
        ct_settings_form_stage_retention(f, v);
    else
        ct_settings_form_stage_move_seconds(f, v);
}

static void ct_settings_build_labels(void) {
    for (int orbit = 0; orbit < CT_SETTINGS_ORBITS; orbit++) {
        const char *arr = orbit == CT_ORBIT_CORNER ? ct_settings.form.staged.corner
                                                   : ct_settings.form.staged.edge;
        for (int i = 0; i < CT_LETTER_STICKERS; i++) {
            ct_settings.labels[orbit][i][0] = arr[i] != '\0' ? arr[i] : '-';
            ct_settings.labels[orbit][i][1] = '\0';
        }
    }
}

static void ct_settings_draw_general(ct_app *app, int x, int y, int w, int h) {
    ct_settings_form *f = &ct_settings.form;
    int row_h = app->cfg.window.height >> CT_SETTINGS_ROW_H_SHIFT;
    int label_w = w / CT_SETTINGS_LABEL_W_DIV;
    int step = row_h + (w >> CT_SETTINGS_MARGIN_SHIFT);

    GuiLabel(ct_settings_rect(x, y, label_w, row_h), "retention");
    if (GuiTextBox(ct_settings_rect(x + label_w, y, w - label_w, row_h), ct_settings.retention_buf,
                   (int)sizeof ct_settings.retention_buf, ct_settings.retention_edit)) {
        ct_settings.retention_edit = !ct_settings.retention_edit;
        if (!ct_settings.retention_edit) {
            ct_settings_apply_double(ct_settings.retention_buf, true);
            ct_settings_canon_double(ct_settings.retention_buf, sizeof ct_settings.retention_buf,
                                     f->retention);
        }
    }
    if (!ct_settings.retention_edit)
        ct_settings_canon_double(ct_settings.retention_buf, sizeof ct_settings.retention_buf,
                                 f->retention);

    int row2 = y + step;
    GuiLabel(ct_settings_rect(x, row2, label_w, row_h), "move seconds");
    if (GuiTextBox(ct_settings_rect(x + label_w, row2, w - label_w, row_h), ct_settings.seconds_buf,
                   (int)sizeof ct_settings.seconds_buf, ct_settings.seconds_edit)) {
        ct_settings.seconds_edit = !ct_settings.seconds_edit;
        if (!ct_settings.seconds_edit) {
            ct_settings_apply_double(ct_settings.seconds_buf, false);
            ct_settings_canon_double(ct_settings.seconds_buf, sizeof ct_settings.seconds_buf,
                                     f->move_seconds);
        }
    }
    if (!ct_settings.seconds_edit)
        ct_settings_canon_double(ct_settings.seconds_buf, sizeof ct_settings.seconds_buf,
                                 f->move_seconds);

    int gy = row2 + step;
    int gh = y + h - gy;
    GuiGroupBox(ct_settings_rect(x, gy, w, gh), "preferred moves");
    int pad = w >> CT_SETTINGS_MARGIN_SHIFT;
    int grid_w = w - 2 * pad;
    int grid_h = gh - 2 * pad;
    int cell_w = grid_w / CT_SETTINGS_MOVE_COLS;
    int cell_h = grid_h / CT_SETTINGS_ORBITS;
    int shown = 0;
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        unsigned bit = 1u << i;
        if ((bit & CT_MOVE_ROTATION_MASK) != 0)
            continue;
        int col = shown % CT_SETTINGS_MOVE_COLS;
        int row = shown / CT_SETTINGS_MOVE_COLS;
        shown++;
        bool on = (f->move_mask & bit) != 0;
        char label[2] = {ct_move_face_char(bit), '\0'};
        if (GuiToggle(
                ct_settings_rect(x + pad + col * cell_w, gy + pad + row * cell_h, cell_w, cell_h),
                label, &on) != 0)
            ct_settings_form_stage_move(f, bit);
    }
}

static void ct_settings_draw_orbit_grid(ct_settings_form *f, int orbit, int x, int y, int w, int h,
                                        bool buffer_mode) {
    int cell_w = w / CT_SETTINGS_GRID_COLS;
    int cell_h = h / CT_SETTINGS_GRID_ROWS;
    int selected = buffer_mode ? (orbit == CT_ORBIT_CORNER ? f->buffer_corner : f->buffer_edge)
                               : (orbit == ct_settings.sel_orbit ? ct_settings.sel_sticker : -1);
    for (int sticker = 0; sticker < CT_LETTER_STICKERS; sticker++) {
        Rectangle r = ct_settings_cell_rect(x, y, cell_w, cell_h, sticker);
        if (GuiButton(r, ct_settings.labels[orbit][sticker])) {
            if (buffer_mode) {
                ct_settings_form_stage_buffer(f, orbit, sticker);
            } else {
                ct_settings.sel_orbit = orbit;
                ct_settings.sel_sticker = sticker;
                const char *arr = orbit == CT_ORBIT_CORNER ? f->staged.corner : f->staged.edge;
                ct_settings.letter_buf[0] = arr[sticker];
                ct_settings.letter_edit = false;
            }
        }
        if (sticker == selected)
            DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, GREEN);
        else if (!buffer_mode && ct_settings.letter_buf[0] >= CT_LETTER_FIRST &&
                 ct_settings.letter_buf[0] <= CT_LETTER_LAST &&
                 ct_settings.labels[orbit][sticker][0] == ct_settings.letter_buf[0])
            DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, MAROON);
    }
}

static void ct_settings_draw_letters(ct_app *app, int x, int y, int w, int h) {
    ct_settings_form *f = &ct_settings.form;
    int row_h = app->cfg.window.height >> CT_SETTINGS_ROW_H_SHIFT;
    int label_w = w / CT_SETTINGS_LABEL_W_DIV;
    int margin = w >> CT_SETTINGS_MARGIN_SHIFT;

    GuiLabel(ct_settings_rect(x, y, label_w, row_h), "letter");
    if (GuiTextBox(ct_settings_rect(x + label_w, y, label_w, row_h), ct_settings.letter_buf,
                   (int)sizeof ct_settings.letter_buf, ct_settings.letter_edit)) {
        ct_settings.letter_edit = !ct_settings.letter_edit;
        if (!ct_settings.letter_edit && ct_settings.letter_buf[0] >= CT_LETTER_FIRST &&
            ct_settings.letter_buf[0] <= CT_LETTER_LAST)
            ct_settings_form_swap_letter(f, ct_settings.sel_orbit, ct_settings.sel_sticker,
                                         ct_settings.letter_buf[0]);
    }
    GuiLabel(ct_settings_rect(x + 2 * label_w, y, w - 2 * label_w, row_h),
             "click a cell then type a letter, a taken letter swaps cells");

    int grid_y = y + row_h + margin;
    int grid_h = (h - row_h - margin - margin) / CT_SETTINGS_ORBITS;
    int pad = margin;
    GuiGroupBox(ct_settings_rect(x, grid_y, w, grid_h), "corners");
    ct_settings_draw_orbit_grid(f, CT_ORBIT_CORNER, x + pad, grid_y + pad, w - 2 * pad,
                                grid_h - 2 * pad, false);
    int edge_y = grid_y + grid_h + margin;
    GuiGroupBox(ct_settings_rect(x, edge_y, w, grid_h), "edges");
    ct_settings_draw_orbit_grid(f, CT_ORBIT_EDGE, x + pad, edge_y + pad, w - 2 * pad,
                                grid_h - 2 * pad, false);
}

static void ct_settings_draw_bld(ct_app *app, int x, int y, int w, int h) {
    ct_settings_form *f = &ct_settings.form;
    int row_h = app->cfg.window.height >> CT_SETTINGS_ROW_H_SHIFT;
    int margin = w >> CT_SETTINGS_MARGIN_SHIFT;
    int half_w = (w - margin) / CT_SETTINGS_ORBITS;
    int grid_h = (h - margin - row_h - margin - margin) / CT_SETTINGS_ORBITS;
    int pad = margin;

    GuiGroupBox(ct_settings_rect(x, y, half_w, grid_h), "corner buffer");
    ct_settings_draw_orbit_grid(f, CT_ORBIT_CORNER, x + pad, y + pad, half_w - 2 * pad,
                                grid_h - 2 * pad, true);
    int edge_x = x + half_w + margin;
    GuiGroupBox(ct_settings_rect(edge_x, y, half_w, grid_h), "edge buffer");
    ct_settings_draw_orbit_grid(f, CT_ORBIT_EDGE, edge_x + pad, y + pad, half_w - 2 * pad,
                                grid_h - 2 * pad, true);

    int color_y = y + grid_h + margin;
    int color_h = h - (color_y - y);
    GuiGroupBox(ct_settings_rect(x, color_y, w, color_h), "colors");
    int face_w = (w - 2 * pad) / CT_SETTINGS_FACE_COUNT;
    int inner_y = color_y + pad;
    int inner_h = color_h - 2 * pad;
    int swatch_h = inner_h / CT_SETTINGS_ORBITS;
    int box_h = inner_h - swatch_h - margin;
    for (int face = 0; face < CT_SETTINGS_FACE_COUNT; face++) {
        int fx = x + pad + face * face_w;
        GuiLabel(ct_settings_rect(fx, inner_y, face_w, row_h), CT_SETTINGS_FACE_NAMES[face]);
        Color c = {f->colors[face][0], f->colors[face][1], f->colors[face][2],
                   CT_SETTINGS_COLOR_MAX};
        DrawRectangle(fx, inner_y + row_h, face_w - margin, swatch_h - row_h, c);
        for (int ch = 0; ch < CT_SETTINGS_CHANNELS; ch++) {
            int idx = face * CT_SETTINGS_CHANNELS + ch;
            bool edit = (ct_settings.chan_edit & (1u << idx)) != 0;
            if (!edit)
                ct_settings.chan[face][ch] = f->colors[face][ch];
            int by = inner_y + swatch_h;
            if (GuiValueBox(ct_settings_rect(fx, by + ch * box_h, face_w - margin, box_h), NULL,
                            &ct_settings.chan[face][ch], 0, CT_SETTINGS_COLOR_MAX, edit) != 0) {
                ct_settings.chan_edit ^= 1u << idx;
                if (!(ct_settings.chan_edit & (1u << idx))) {
                    ct_settings_form_stage_color(f, face, ch, ct_settings.chan[face][ch]);
                    ct_settings.chan[face][ch] = f->colors[face][ch];
                }
            }
        }
    }
}

void ct_screen_settings_update(ct_app *app) {
    if (app == NULL)
        return;
    if (!ct_settings.opened) {
        ct_settings.opened = true;
        ct_settings.rejected = 0;
        ct_settings_form_open(&ct_settings.form, app->db);
        ct_settings.sel_orbit = CT_ORBIT_CORNER;
        ct_settings.sel_sticker = 0;
        ct_settings.letter_buf[0] = ct_settings.form.staged.corner[0];
    }
    if (IsKeyPressed(KEY_ESCAPE))
        app->screen = CT_SCREEN_MENU;
}

void ct_screen_settings_draw(ct_app *app) {
    ClearBackground(DARKGRAY);
    if (app == NULL)
        return;
    ct_settings_form *f = &ct_settings.form;
    int w = (int)app->cfg.window.width;
    int h = (int)app->cfg.window.height;
    int margin = w >> CT_SETTINGS_MARGIN_SHIFT;
    int row_h = h >> CT_SETTINGS_ROW_H_SHIFT;
    int status_h = row_h;
    int btn_h = row_h;
    int btn_y = h - status_h - margin - btn_h;
    int btn_w = (w - 2 * margin - margin) / CT_SETTINGS_BTN_COUNT;
    int content_x = margin;
    int content_y = margin + row_h + margin;
    int content_w = w - 2 * margin;
    int content_h = btn_y - margin - content_y;

    ct_settings_build_labels();
    GuiTabBar(ct_settings_rect(margin, margin, content_w, row_h), "general;letters;bld",
              &ct_settings.tab_scroll, &ct_settings.tab);
    if (ct_settings.tab < CT_SETTINGS_TAB_GENERAL || ct_settings.tab >= CT_SETTINGS_TAB_COUNT)
        ct_settings.tab = CT_SETTINGS_TAB_GENERAL;

    switch (ct_settings.tab) {
    case CT_SETTINGS_TAB_LETTERS:
        ct_settings_draw_letters(app, content_x, content_y, content_w, content_h);
        break;
    case CT_SETTINGS_TAB_BLD:
        ct_settings_draw_bld(app, content_x, content_y, content_w, content_h);
        break;
    default:
        ct_settings_draw_general(app, content_x, content_y, content_w, content_h);
        break;
    }

    if (GuiButton(ct_settings_rect(margin, btn_y, btn_w, btn_h), "save")) {
        if (ct_settings_form_save(f) == CT_REPO_OK) {
            int rc = ct_repo_settings_merge_rejected(app->db, &app->cfg, &ct_settings.rejected);
            if (rc != 0)
                ct_settings.rejected = 0;
        }
    }
    if (GuiButton(ct_settings_rect(margin + btn_w + margin, btn_y, btn_w, btn_h), "cancel"))
        ct_settings_form_cancel(f);
    char status_buf[CT_SETTINGS_STATUS_MAX];
    if (ct_settings.rejected > 0) {
        snprintf(status_buf, sizeof status_buf, "%s, %d settings rows rejected",
                 ct_settings_status_text(f->status), ct_settings.rejected);
    } else {
        snprintf(status_buf, sizeof status_buf, "%s", ct_settings_status_text(f->status));
    }
    GuiStatusBar(ct_settings_rect(margin, h - status_h, content_w, status_h), status_buf);
}
