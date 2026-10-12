#include "app/shots.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app/paths.h"
#include "raygui.h"
#include "raylib.h"
#include "ui/ui.h"

static const char CT_SHOTS_WINDOW_TITLE[] = "cubing-trainer shots";

static bool ct_shots_window_open = false;

static int ct_shots_args_ok(const ct_app *app, const char *out_path) {
    if (app == NULL || !app->running) {
        return CT_SHOTS_E_ARG;
    }
    if (out_path == NULL || out_path[0] == '\0') {
        return CT_SHOTS_E_ARG;
    }
    return CT_SHOTS_OK;
}

static int ct_shots_window(ct_app *app) {
    if (!ct_shots_window_open) {
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(CT_WINDOW_WIDTH, CT_WINDOW_HEIGHT, CT_SHOTS_WINDOW_TITLE);
        if (!IsWindowReady()) {
            return CT_SHOTS_E_WINDOW;
        }
        char font_path[CT_APP_EXE_PATH_MAX];
        if (ct_font_path(font_path, (unsigned)sizeof font_path) != 0) {
            font_path[0] = '\0';
        }
        ct_ui_theme_load(font_path);
        ct_shots_window_open = true;
    }
    app->cfg.window.width = CT_WINDOW_WIDTH;
    app->cfg.window.height = CT_WINDOW_HEIGHT;
    return CT_SHOTS_OK;
}

static int ct_shots_mkdirs(const char *out_path) {
    char dir[CT_APP_EXE_PATH_MAX];
    size_t len = strlen(out_path);
    if (len == 0 || len >= sizeof dir) {
        return CT_SHOTS_E_ARG;
    }
    memcpy(dir, out_path, len + 1);
    char *last = strrchr(dir, '/');
    char *last_win = strrchr(dir, '\\');
    if (last_win != NULL && (last == NULL || last_win > last)) {
        last = last_win;
    }
    if (last == NULL || last == dir || last[1] == '\0') {
        return CT_SHOTS_OK;
    }
    *last = '\0';
    return ct_mkdirs(dir) == 0 ? CT_SHOTS_OK : CT_SHOTS_E_WRITE;
}

static int ct_shots_render(ct_app *app, const char *out_path) {
    int rc = ct_shots_mkdirs(out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    RenderTexture2D target = LoadRenderTexture(CT_WINDOW_WIDTH, CT_WINDOW_HEIGHT);
    BeginTextureMode(target);
    ct_ui_draw(app);
    EndTextureMode();
    Image frame = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&frame);
    bool written = ExportImage(frame, out_path);
    UnloadImage(frame);
    UnloadRenderTexture(target);
    return written ? CT_SHOTS_OK : CT_SHOTS_E_WRITE;
}

static int ct_shots_show(ct_app *app, ct_ui_screen screen, const char *out_path) {
    int rc = ct_shots_window(app);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    app->screen = screen;
    ct_ui_update(app);
    return ct_shots_render(app, out_path);
}

void ct_shots_close(void) {
    if (ct_shots_window_open) {
        ct_ui_theme_release();
        CloseWindow();
        ct_shots_window_open = false;
    }
}

int ct_shots_menu(ct_app *app, const char *out_path) {
    int rc = ct_shots_args_ok(app, out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    return ct_shots_show(app, CT_SCREEN_MENU, out_path);
}

int ct_shots_trainer(ct_app *app, ct_event ev, ct_method m, int list_id, int case_index,
                     const char *out_path) {
    int rc = ct_shots_args_ok(app, out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    rc = ct_shots_window(app);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    app->screen = CT_SCREEN_TRAINER;
    ct_ui_update(app);
    ct_trainer_form *f = ct_screen_trainer_form();
    if (f == NULL || f->db == NULL) {
        return CT_SHOTS_E_STATE;
    }
    if (ct_trainer_form_select_event(f, ev) != CT_REPO_OK ||
        ct_trainer_form_select_method(f, m) != CT_REPO_OK ||
        ct_trainer_form_select_list(f, list_id) != CT_REPO_OK) {
        return CT_SHOTS_E_STATE;
    }
    if (case_index != CT_TRAINER_CELL_NONE &&
        ct_trainer_form_select_case(f, case_index) != CT_REPO_OK) {
        return CT_SHOTS_E_STATE;
    }
    return ct_shots_render(app, out_path);
}

int ct_shots_chart(ct_app *app, ct_event ev, ct_method m, int list_id, int cell,
                   const char *out_path) {
    int rc = ct_shots_args_ok(app, out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    rc = ct_shots_window(app);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    app->screen = CT_SCREEN_TRAINER;
    ct_ui_update(app);
    ct_trainer_form *f = ct_screen_trainer_form();
    if (f == NULL || f->db == NULL) {
        return CT_SHOTS_E_STATE;
    }
    if (ct_trainer_form_select_event(f, ev) != CT_REPO_OK ||
        ct_trainer_form_select_method(f, m) != CT_REPO_OK ||
        ct_trainer_form_select_list(f, list_id) != CT_REPO_OK ||
        ct_trainer_form_select_cell(f, cell) != CT_REPO_OK) {
        return CT_SHOTS_E_STATE;
    }
    return ct_shots_render(app, out_path);
}

int ct_shots_review(ct_app *app, bool revealed, const char *out_path) {
    int rc = ct_shots_args_ok(app, out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    rc = ct_shots_window(app);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    app->screen = CT_SCREEN_REVIEW;
    ct_ui_update(app);
    ct_review_form *f = ct_screen_review_form();
    if (f == NULL || f->db == NULL) {
        return CT_SHOTS_E_STATE;
    }
    ct_review_form_load_case(f);
    if (revealed) {
        ct_review_form_reveal(f);
    }
    return ct_shots_render(app, out_path);
}

int ct_shots_cards(ct_app *app, const char *deck, bool editing, const char *out_path) {
    int rc = ct_shots_args_ok(app, out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    if (deck == NULL || deck[0] == '\0') {
        return CT_SHOTS_E_ARG;
    }
    rc = ct_shots_window(app);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    app->screen = CT_SCREEN_CARDS;
    ct_ui_update(app);
    ct_cards_form *f = ct_screen_cards_form();
    if (f == NULL || f->db == NULL) {
        return CT_SHOTS_E_STATE;
    }
    snprintf(f->deck_filter, sizeof f->deck_filter, "%s", deck);
    ct_cards_form_reload(f, app->db);
    if (ct_cards_form_select(f, 0) != CT_REPO_OK) {
        return CT_SHOTS_E_STATE;
    }
    if (editing) {
        if (ct_cards_form_begin_edit(f) != CT_REPO_OK) {
            return CT_SHOTS_E_STATE;
        }
    }
    return ct_shots_render(app, out_path);
}

int ct_shots_settings(ct_app *app, int tab, const char *out_path) {
    int rc = ct_shots_args_ok(app, out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    if (tab < CT_SETTINGS_TAB_GENERAL || tab >= CT_SETTINGS_TAB_COUNT) {
        return CT_SHOTS_E_ARG;
    }
    rc = ct_shots_window(app);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    app->screen = CT_SCREEN_SETTINGS;
    ct_ui_update(app);
    ct_screen_settings_select_tab(tab);
    return ct_shots_render(app, out_path);
}

int ct_shots_stats(ct_app *app, const char *out_path) {
    int rc = ct_shots_args_ok(app, out_path);
    if (rc != CT_SHOTS_OK) {
        return rc;
    }
    return ct_shots_show(app, CT_SCREEN_STATS, out_path);
}
