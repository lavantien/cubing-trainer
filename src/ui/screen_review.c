#include <stdio.h>
#include <string.h>

#include "engine/notation.h"
#include "engine/transform.h"
#include "raygui.h"
#include "raylib.h"
#include "render/renderer.h"
#include "ui/ui.h"

static const double CT_REVIEW_SPEEDS[] = {1.0 / 2.0, 1.0, 2.0};

enum {
    CT_REVIEW_CUBE_N = 3,
    CT_REVIEW_MARGIN_SHIFT = 6,
    CT_REVIEW_PANE_W_DIV = 2,
    CT_REVIEW_FIELD_H_SHIFT = 5,
    CT_REVIEW_TEXT_SHIFT = 5,
    CT_REVIEW_GRADE_COUNT = CT_EASY,
    CT_REVIEW_PLAY_COUNT = 2,
    CT_REVIEW_SPEED_COUNT = (int)(sizeof CT_REVIEW_SPEEDS / sizeof CT_REVIEW_SPEEDS[0]),
    CT_REVIEW_ISO_FIELDS = 3,
    CT_REVIEW_LINE_MAX = CT_CARD_CASE_KEY_MAX + CT_CARD_DECK_MAX,
    CT_REVIEW_CAM_DIST = 4,
    CT_REVIEW_CAM_OFFS = 2,
    CT_REVIEW_FOV_DEG = CT_DECIMAL_BASE * 4
};

static const ct_grade CT_REVIEW_GRADES[CT_REVIEW_GRADE_COUNT] = {CT_AGAIN, CT_HARD, CT_GOOD,
                                                                 CT_EASY};
static const char *const CT_REVIEW_GRADE_LABELS[CT_REVIEW_GRADE_COUNT] = {"again", "hard", "good",
                                                                          "easy"};
static const char *const CT_REVIEW_SPEED_LABELS[CT_REVIEW_SPEED_COUNT] = {"0.5x", "1x", "2x"};

static ct_fsrs ct_review_fsrs;

typedef struct {
    ct_review_form form;
    ct_renderer renderer;
    ct_color_scheme scheme;
    Camera3D camera;
    bool opened;
} ct_review_screen;

static ct_review_screen ct_review;

static double ct_review_move_seconds(void) {
    return (double)CT_RENDERER_MOVE_MILLIS / (CT_DECIMAL_BASE * CT_DECIMAL_BASE * CT_DECIMAL_BASE);
}

static const char *ct_review_pick_alg(const ct_card *c) {
    if (c->alg_3x3[0] != '\0') {
        return c->alg_3x3;
    }
    if (c->alg_big[0] != '\0') {
        return c->alg_big;
    }
    return c->alg_oh;
}

static void ct_review_scramble(ct_cube *cube, const ct_move *moves, int count) {
    ct_cube_init(cube, CT_REVIEW_CUBE_N);
    ct_move inv[CT_ANIM_QUEUE_CAP];
    int n = ct_transform_inverse(moves, count, inv);
    for (int i = 0; i < n; i++) {
        ct_apply(cube, inv[i]);
    }
}

static void ct_review_push_queue(ct_review_form *f, const ct_move *moves, int count) {
    ct_anim_init(&f->anim);
    for (int i = 0; i < count; i++) {
        ct_anim_push(&f->anim, moves[i]);
    }
    ct_anim_set_speed(&f->anim, f->speed);
    ct_anim_pause(&f->anim);
    f->playing = false;
    f->scrub_index = 0;
}

static void ct_review_idle(ct_review_form *f) {
    memset(&f->card, 0, sizeof f->card);
    f->alg_text[0] = '\0';
    f->move_count = 0;
    ct_cube_init(&f->cube, CT_REVIEW_CUBE_N);
    ct_anim_init(&f->anim);
    f->revealed = false;
    f->playing = false;
    f->scrub_index = 0;
}

int ct_review_form_begin(ct_review_form *f, sqlite3 *db, const char *deck, const char *now_iso,
                         long today_day, int limit) {
    if (f == NULL) {
        return CT_SESSION_ERR_ARG;
    }
    ct_session_free(&f->session);
    memset(f, 0, sizeof *f);
    f->db = db;
    ct_config cfg = ct_config_defaults();
    ct_fsrs_init(&ct_review_fsrs, &cfg);
    int rc = ct_session_begin(&f->session, db, &ct_review_fsrs, deck, now_iso, today_day, limit);
    if (rc < 0) {
        f->status = rc;
        return rc;
    }
    f->status = CT_SESSION_OK;
    ct_review_form_load_case(f);
    return rc;
}

int ct_review_form_load_case(ct_review_form *f) {
    if (f == NULL) {
        return CT_SESSION_ERR_ARG;
    }
    ct_card card;
    memset(&card, 0, sizeof card);
    int rc = ct_session_current(&f->session, &card);
    f->speed = 1.0;
    ct_review_idle(f);
    if (rc != CT_SESSION_OK) {
        f->status = rc;
        return rc;
    }
    f->card = card;
    snprintf(f->alg_text, sizeof f->alg_text, "%s", ct_review_pick_alg(&f->card));
    ct_move moves[CT_ANIM_QUEUE_CAP];
    int n = ct_notation_parse(f->alg_text, CT_REVIEW_CUBE_N, moves, CT_ANIM_QUEUE_CAP);
    if (n < 0) {
        n = 0;
    }
    f->move_count = n;
    ct_review_scramble(&f->cube, moves, n);
    ct_review_push_queue(f, moves, n);
    f->status = CT_SESSION_OK;
    return CT_SESSION_OK;
}

void ct_review_form_reveal(ct_review_form *f) {
    if (f == NULL) {
        return;
    }
    f->revealed = true;
    ct_move moves[CT_ANIM_QUEUE_CAP];
    int n = ct_notation_parse(f->alg_text, CT_REVIEW_CUBE_N, moves, CT_ANIM_QUEUE_CAP);
    if (n < 0) {
        n = 0;
    }
    ct_review_scramble(&f->cube, moves, n);
    ct_review_push_queue(f, moves, n);
    ct_anim_resume(&f->anim);
    f->playing = true;
}

int ct_review_form_grade(ct_review_form *f, ct_grade g, const char *now_iso) {
    if (f == NULL) {
        return CT_SESSION_ERR_ARG;
    }
    int rc = ct_session_grade(&f->session, g, now_iso);
    f->status = rc;
    if (rc == CT_SESSION_OK) {
        ct_review_form_load_case(f);
    }
    return rc;
}

void ct_review_form_play_pause(ct_review_form *f) {
    if (f == NULL) {
        return;
    }
    if (f->playing) {
        ct_anim_pause(&f->anim);
        f->playing = false;
    } else {
        ct_anim_resume(&f->anim);
        f->playing = true;
    }
}

void ct_review_form_step(ct_review_form *f) {
    if (f == NULL) {
        return;
    }
    ct_anim_step_result r = ct_anim_step(&f->anim);
    if (r.just_completed) {
        ct_apply(&f->cube, r.move);
        f->scrub_index = f->move_count - f->anim.count;
    }
}

void ct_review_form_set_speed(ct_review_form *f, double speed) {
    if (f == NULL) {
        return;
    }
    if (speed < CT_REVIEW_SPEED_MIN) {
        speed = CT_REVIEW_SPEED_MIN;
    }
    f->speed = speed;
    ct_anim_set_speed(&f->anim, speed);
}

int ct_review_form_scrub(ct_review_form *f, int move_index) {
    if (f == NULL) {
        return CT_SESSION_ERR_ARG;
    }
    int upto = move_index;
    if (upto < 0) {
        upto = 0;
    }
    if (upto > f->move_count) {
        upto = f->move_count;
    }
    ct_move moves[CT_ANIM_QUEUE_CAP];
    int n = ct_notation_parse(f->alg_text, CT_REVIEW_CUBE_N, moves, CT_ANIM_QUEUE_CAP);
    if (n < 0) {
        n = 0;
    }
    ct_review_scramble(&f->cube, moves, n);
    ct_review_push_queue(f, moves, n);
    ct_move pre[CT_ANIM_QUEUE_CAP];
    int applied = ct_anim_prefix(&f->anim, upto, pre, CT_ANIM_QUEUE_CAP);
    for (int i = 0; i < applied; i++) {
        ct_apply(&f->cube, pre[i]);
    }
    for (int i = 0; i < upto; i++) {
        ct_anim_step(&f->anim);
    }
    f->scrub_index = upto;
    f->status = CT_SESSION_OK;
    return CT_SESSION_OK;
}

void ct_review_form_end(ct_review_form *f, const char *now_iso) {
    if (f == NULL) {
        return;
    }
    f->status = ct_session_end(&f->session, now_iso);
}

static int ct_review_iso_scan(const char *iso, long *y, int *m, int *d) {
    int parts[CT_REVIEW_ISO_FIELDS];
    int at = 0;
    for (int p = 0; p < CT_REVIEW_ISO_FIELDS; p++) {
        int v = 0;
        bool any = false;
        while (iso[at] >= '0' && iso[at] <= '9') {
            v = v * CT_DECIMAL_BASE + (iso[at] - '0');
            at++;
            any = true;
        }
        if (!any || (iso[at] != '-' && iso[at] != 'T')) {
            return CT_SESSION_ERR_ARG;
        }
        parts[p] = v;
        at++;
    }
    *y = parts[0];
    *m = parts[1];
    *d = parts[2];
    return CT_SESSION_OK;
}

static void ct_review_open(ct_app *app) {
    ct_review.opened = true;
    ct_renderer_init(&ct_review.renderer, CT_REVIEW_CUBE_N);
    ct_review.scheme = ct_color_scheme_default();
    float dist = (float)CT_REVIEW_CAM_DIST;
    float offs = (float)CT_REVIEW_CAM_OFFS;
    ct_review.camera.position = (Vector3){dist, dist, dist};
    ct_review.camera.target = (Vector3){offs, 0.0f, 0.0f};
    ct_review.camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    ct_review.camera.fovy = (float)CT_REVIEW_FOV_DEG;
    ct_review.camera.projection = CAMERA_PERSPECTIVE;
    char now[CT_SESSION_STAMP_MAX];
    ct_now_utc_iso(now, (unsigned)sizeof now);
    long y = 0;
    int m = 0;
    int d = 0;
    if (ct_review_iso_scan(now, &y, &m, &d) == CT_SESSION_OK) {
        ct_review_form_begin(&ct_review.form, app->db, CT_UI_DEFAULT_DECK, now,
                             ct_session_date_to_day(y, m, d), 0);
    }
}

void ct_screen_review_update(ct_app *app) {
    if (app == NULL) {
        return;
    }
    if (!ct_review.opened) {
        ct_review_open(app);
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        app->screen = CT_SCREEN_MENU;
        return;
    }
    ct_review_form *f = &ct_review.form;
    ct_anim_step_result r = ct_anim_update(&f->anim, app->fixed_dt, ct_review_move_seconds());
    if (r.just_completed) {
        ct_apply(&f->cube, r.move);
        f->scrub_index = f->move_count - f->anim.count;
        if (!ct_anim_active(&f->anim)) {
            ct_anim_pause(&f->anim);
            f->playing = false;
        }
    }
}

static Rectangle ct_review_rect(int x, int y, int w, int h) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    return r;
}

static const char *ct_review_status_text(int status) {
    switch (status) {
    case CT_SESSION_OK:
        return "ok";
    case CT_SESSION_ERR_ARG:
        return "invalid input";
    case CT_SESSION_ERR_DB:
        return "db error";
    case CT_SESSION_ERR_STATE:
        return "no card open";
    default:
        return "unknown";
    }
}

static void ct_review_row_buttons(int x, int y, int w, int btn_h, int count, int gap,
                                  const char *const *labels, void (*on_click)(int)) {
    int btn_w = (w - (count - 1) * gap) / count;
    for (int i = 0; i < count; i++) {
        if (GuiButton(ct_review_rect(x + i * (btn_w + gap), y, btn_w, btn_h), labels[i])) {
            on_click(i);
        }
    }
}

static void ct_review_on_grade(int i) {
    char now[CT_SESSION_STAMP_MAX];
    ct_now_utc_iso(now, (unsigned)sizeof now);
    ct_review_form_grade(&ct_review.form, CT_REVIEW_GRADES[i], now);
}

static void ct_review_on_play(int i) {
    ct_review_form *f = &ct_review.form;
    if (i == 0) {
        ct_review_form_play_pause(f);
    } else {
        ct_review_form_step(f);
    }
}

static void ct_review_on_speed(int i) {
    ct_review_form_set_speed(&ct_review.form, CT_REVIEW_SPEEDS[i]);
}

void ct_screen_review_draw(ct_app *app) {
    ClearBackground(DARKGRAY);
    if (app == NULL) {
        return;
    }
    ct_review_form *f = &ct_review.form;
    int w = (int)app->cfg.window.width;
    int h = (int)app->cfg.window.height;
    int margin = w >> CT_REVIEW_MARGIN_SHIFT;
    int pane_w = w / CT_REVIEW_PANE_W_DIV;
    int row_h = h >> CT_REVIEW_FIELD_H_SHIFT;
    int text_size = h >> CT_REVIEW_TEXT_SHIFT;
    int right_x = pane_w + margin;
    int right_w = w - right_x - margin;
    int step = row_h + margin;
    int y = margin;
    int status_h = row_h;

    BeginScissorMode(0, 0, pane_w, h);
    ct_renderer_draw(&ct_review.renderer, &f->cube, &f->anim, &ct_review.scheme, ct_review.camera);
    EndScissorMode();

    char line[CT_REVIEW_LINE_MAX];
    snprintf(line, sizeof line, "deck %s", f->card.deck);
    GuiLabel(ct_review_rect(right_x, y, right_w, row_h), line);
    y += step;
    snprintf(line, sizeof line, "case %s", f->card.case_key);
    GuiLabel(ct_review_rect(right_x, y, right_w, row_h), line);
    y += step;

    if (f->card.id == 0) {
        DrawText("no due cards", right_x, y, text_size, RAYWHITE);
        GuiStatusBar(ct_review_rect(margin, h - status_h, w - 2 * margin, status_h),
                     ct_review_status_text(f->status));
        return;
    }

    DrawText(f->revealed ? f->alg_text : "hidden", right_x, y, text_size, RAYWHITE);
    y += step;
    snprintf(line, sizeof line, "move %d of %d", f->scrub_index, f->move_count);
    GuiLabel(ct_review_rect(right_x, y, right_w, row_h), line);
    y += step;
    ct_review_row_buttons(right_x, y, right_w, row_h, CT_REVIEW_GRADE_COUNT, margin,
                          CT_REVIEW_GRADE_LABELS, ct_review_on_grade);
    y += step;
    const char *play_labels[CT_REVIEW_PLAY_COUNT] = {f->playing ? "pause" : "play", "step"};
    ct_review_row_buttons(right_x, y, right_w, row_h, CT_REVIEW_PLAY_COUNT, margin, play_labels,
                          ct_review_on_play);
    y += step;
    ct_review_row_buttons(right_x, y, right_w, row_h, CT_REVIEW_SPEED_COUNT, margin,
                          CT_REVIEW_SPEED_LABELS, ct_review_on_speed);
    y += step;
    float v = (float)f->scrub_index;
    int slider_h = row_h + row_h / CT_REVIEW_PANE_W_DIV;
    if (f->move_count > 0 && GuiSlider(ct_review_rect(right_x, y, right_w, slider_h), "scrub", NULL,
                                       &v, 0.0f, (float)f->move_count)) {
        ct_review_form_scrub(f, (int)v);
    }

    GuiStatusBar(ct_review_rect(margin, h - status_h, w - 2 * margin, status_h),
                 ct_review_status_text(f->status));
}
