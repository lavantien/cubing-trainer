#include <stdio.h>
#include <string.h>

#include "raygui.h"
#include "raylib.h"
#include "ui/ui.h"
#include "ui/widgets.h"

enum {
    CT_STATS_SCR_MARGIN_SHIFT = 6,
    CT_STATS_SCR_ROW_SHIFT = 4,
    CT_STATS_SCR_LEFT_W_SHIFT = 2,
    CT_STATS_SCR_FORECAST_H_DIV = 3,
    CT_STATS_SCR_DECK_H_DIV = 5,
    CT_STATS_SCR_HEAT_H_DIV = 5,
    CT_STATS_SCR_HEAT_COLS = 8,
    CT_STATS_SCR_MARK_DIV = 8,
    CT_STATS_SCR_SUMMARY_LINES = 4,
    CT_STATS_SCR_ISO_DAY_MAX = (1 << 3) + 2,
    CT_STATS_SCR_LABEL_STEP = (1 << 2) + 1,
    CT_STATS_SCR_TEXT_SHIFT = 1,
    CT_STATS_SCR_LABEL_MAX = CT_STATS_STAMP_MAX + CT_STATS_DECK_MAX,
    CT_STATS_SCR_HOVER_MAX = CT_STATS_CASE_KEY_MAX + CT_STATS_SCR_LABEL_MAX
};

static ct_card ct_stats_scr_cards[CT_STATS_MAX_CASES];
static ct_review_row ct_stats_scr_rows[CT_STATS_REVIEW_ROWS_MAX];

static int ct_stats_view_deck_sections(ct_stats_view *v, const char *deck) {
    int rc = ct_repo_case_accuracy(v->db, deck, v->cases, CT_STATS_MAX_CASES);
    if (rc < 0) {
        return rc;
    }
    v->case_count = rc;
    if (ct_stats_heatmap_rank(v->cases, v->case_count, v->case_order, CT_STATS_MAX_CASES) < 0) {
        return CT_REPO_ERR_ARG;
    }
    int n_cards = ct_repo_card_list(v->db, deck, ct_stats_scr_cards, CT_STATS_MAX_CASES);
    if (n_cards < 0) {
        return n_cards;
    }
    int n_rows = 0;
    for (int i = 0; i < n_cards && n_rows < CT_STATS_REVIEW_ROWS_MAX; i++) {
        int got =
            ct_repo_reviews_for_card(v->db, ct_stats_scr_cards[i].id, ct_stats_scr_rows + n_rows,
                                     CT_STATS_REVIEW_ROWS_MAX - n_rows);
        if (got < 0) {
            return got;
        }
        n_rows += got;
    }
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    rc =
        ct_stats_retention(ct_stats_scr_rows, n_rows, &f, v->retention, CT_STATS_RETENTION_BUCKETS);
    if (rc < 0) {
        return CT_REPO_ERR_ARG;
    }
    v->retention_count = rc;
    v->retention_summary = ct_stats_retention_summary(v->retention, v->retention_count);
    return CT_REPO_OK;
}

int ct_stats_view_load(ct_stats_view *v, sqlite3 *db, const char *now_iso, const char *deck) {
    if (v == NULL || db == NULL || !ct_repo_text_ok(now_iso, CT_STATS_STAMP_MAX)) {
        return CT_REPO_ERR_ARG;
    }
    memset(v, 0, sizeof *v);
    v->db = db;
    v->deck_selected = -1;
    snprintf(v->now_iso, sizeof v->now_iso, "%s", now_iso);
    int rc = ct_repo_deck_stats(db, now_iso, v->decks, CT_STATS_MAX_DECKS);
    if (rc < 0) {
        return rc;
    }
    v->deck_count = rc;
    char from_day[CT_STATS_SCR_ISO_DAY_MAX + 1];
    snprintf(from_day, sizeof from_day, "%.*s", (int)CT_STATS_SCR_ISO_DAY_MAX, now_iso);
    rc = ct_repo_due_forecast(db, from_day, CT_STATS_FORECAST_DAYS, v->forecast,
                              CT_STATS_FORECAST_DAYS);
    if (rc < 0) {
        return rc;
    }
    v->forecast_count = rc;
    rc = ct_repo_sessions(db, v->sessions, CT_STATS_SESSIONS_MAX);
    if (rc < 0) {
        return rc;
    }
    v->session_count = rc;
    const char *pick = NULL;
    if (deck != NULL && deck[0] != '\0') {
        pick = deck;
    } else if (v->deck_count > 0) {
        pick = v->decks[0].deck;
    }
    if (pick == NULL) {
        return CT_REPO_OK;
    }
    return ct_stats_view_select_deck(v, pick);
}

int ct_stats_view_select_deck(ct_stats_view *v, const char *deck) {
    if (v == NULL || v->db == NULL || !ct_repo_text_ok(deck, CT_STATS_DECK_MAX)) {
        return CT_REPO_ERR_ARG;
    }
    int rc = ct_stats_view_deck_sections(v, deck);
    if (rc < 0) {
        return rc;
    }
    v->deck_selected = -1;
    for (int i = 0; i < v->deck_count; i++) {
        if (strcmp(v->decks[i].deck, deck) == 0) {
            v->deck_selected = i;
            break;
        }
    }
    return CT_REPO_OK;
}

int ct_stats_view_worst_cases(ct_stats_view *v, int n) {
    if (v == NULL || n < 0) {
        return 0;
    }
    return n < v->case_count ? n : v->case_count;
}

typedef struct {
    ct_stats_view view;
    bool opened;
    int status;
    int deck_scroll;
    int deck_active;
    int session_scroll;
    int session_active;
    char deck_labels[CT_STATS_MAX_DECKS][CT_STATS_DECK_MAX];
    char *deck_ptrs[CT_STATS_MAX_DECKS];
    char session_labels[CT_STATS_SESSIONS_MAX][CT_STATS_SCR_LABEL_MAX];
    char *session_ptrs[CT_STATS_SESSIONS_MAX];
} ct_stats_screen;

static ct_stats_screen ct_stats;

static const char *ct_stats_dir_tag(int direction) {
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

static const char *ct_stats_status_text(int status) {
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

static Rectangle ct_stats_rect(int x, int y, int w, int h) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    return r;
}

static void ct_stats_build_labels(void) {
    ct_stats_view *v = &ct_stats.view;
    for (int i = 0; i < v->deck_count; i++) {
        snprintf(ct_stats.deck_labels[i], sizeof ct_stats.deck_labels[i], "%s", v->decks[i].deck);
        ct_stats.deck_ptrs[i] = ct_stats.deck_labels[i];
    }
    for (int i = 0; i < v->session_count; i++) {
        const ct_session_row *s = &v->sessions[i];
        snprintf(ct_stats.session_labels[i], sizeof ct_stats.session_labels[i], "%.*s %s %dc",
                 (int)CT_STATS_SCR_ISO_DAY_MAX, s->started_at, s->deck, s->n_cards);
        ct_stats.session_ptrs[i] = ct_stats.session_labels[i];
    }
}

static void ct_stats_draw_forecast(ct_stats_view *v, int x, int y, int w, int h, int row_h,
                                   int margin) {
    GuiGroupBox(ct_stats_rect(x, y, w, h), "forecast");
    int inner_x = x + margin;
    int inner_y = y + margin + row_h;
    int inner_w = w - 2 * margin;
    int label_h = row_h >> CT_STATS_SCR_TEXT_SHIFT;
    int inner_h = h - (inner_y - y) - margin - label_h;
    char head[CT_STATS_SCR_LABEL_MAX];
    snprintf(head, sizeof head, "due next 30 days: %d",
             ct_stats_forecast_total(v->forecast, v->forecast_count));
    GuiLabel(ct_stats_rect(inner_x, y + margin, inner_w, row_h), head);
    Rectangle chart = {(float)inner_x, (float)inner_y, (float)inner_w, (float)inner_h};
    double values[CT_STATS_FORECAST_DAYS];
    ct_chart_bar bars[CT_STATS_FORECAST_DAYS];
    for (int i = 0; i < v->forecast_count; i++) {
        values[i] = (double)v->forecast[i].due_count;
    }
    int n = ct_chart_layout(chart, values, v->forecast_count, bars, CT_STATS_FORECAST_DAYS);
    int small = label_h;
    for (int i = 0; i < n; i++) {
        if (bars[i].bar.height > 0.0f) {
            DrawRectangleRec(bars[i].bar, SKYBLUE);
        }
        if (i % CT_STATS_SCR_LABEL_STEP != 0) {
            continue;
        }
        char label[CT_STATS_SCR_LABEL_STEP * 2];
        snprintf(label, sizeof label, "%d", (int)bars[i].label_day);
        DrawText(label, (int)bars[i].bar.x, inner_y + inner_h, small, GRAY);
    }
}

static void ct_stats_draw_heatmap(ct_stats_view *v, int x, int y, int w, int h, int row_h,
                                  int margin) {
    GuiGroupBox(ct_stats_rect(x, y, w, h), "case accuracy");
    int inner_x = x + margin;
    int inner_y = y + margin;
    int inner_w = w - 2 * margin;
    int inner_h = h - 2 * margin;
    int small = row_h >> CT_STATS_SCR_TEXT_SHIFT;
    if (v->case_count <= 0) {
        DrawText("no reviews yet", inner_x, inner_y, small, GRAY);
        return;
    }
    int cols = CT_STATS_SCR_HEAT_COLS;
    int rows = (v->case_count + cols - 1) / cols;
    Rectangle grid = {(float)inner_x, (float)inner_y, (float)inner_w, (float)inner_h};
    Vector2 mouse = GetMousePosition();
    int hover = ct_heatmap_pick(grid, cols, rows, mouse);
    Rectangle cell;
    for (int i = 0; i < v->case_count; i++) {
        if (ct_heatmap_cell(grid, cols, rows, i, &cell) != 0) {
            continue;
        }
        DrawRectangleRec(cell, ct_heatmap_color(v->cases[i].accuracy));
        if (i == hover) {
            DrawRectangleLines((int)cell.x, (int)cell.y, (int)cell.width, (int)cell.height,
                               RAYWHITE);
        }
    }
    if (hover < 0 || hover >= v->case_count) {
        return;
    }
    const ct_case_accuracy_row *row = &v->cases[hover];
    char buf[CT_STATS_SCR_HOVER_MAX];
    snprintf(buf, sizeof buf, "%s [%s] %ld of %ld correct", row->case_key,
             ct_stats_dir_tag(row->direction), row->correct, row->reviews);
    DrawText(buf, inner_x, y + h - margin - small, small, RAYWHITE);
}

static void ct_stats_draw_retention(ct_stats_view *v, int x, int y, int w, int h, int row_h,
                                    int margin) {
    GuiGroupBox(ct_stats_rect(x, y, w, h), "retention");
    int inner_x = x + margin;
    int inner_y = y + margin + row_h;
    int inner_w = w - 2 * margin;
    int inner_h = h - (inner_y - y) - margin;
    char head[CT_STATS_SCR_LABEL_MAX];
    snprintf(head, sizeof head, "reviews %.0f correct %.0f werr %.3f maxerr %.3f worst %ld",
             v->retention_summary.total_reviews, v->retention_summary.total_correct,
             v->retention_summary.weighted_error, v->retention_summary.max_abs_error,
             v->retention_summary.worst_bucket);
    GuiLabel(ct_stats_rect(inner_x, y + margin, inner_w, row_h), head);
    Rectangle chart = {(float)inner_x, (float)inner_y, (float)inner_w, (float)inner_h};
    double rates[CT_STATS_RETENTION_BUCKETS];
    ct_chart_bar bars[CT_STATS_RETENTION_BUCKETS];
    for (int i = 0; i < v->retention_count; i++) {
        rates[i] = v->retention[i].reviews > 0
                       ? (double)v->retention[i].actual_recall / (double)v->retention[i].reviews
                       : 0.0;
    }
    int n = ct_chart_layout(chart, rates, v->retention_count, bars, CT_STATS_RETENTION_BUCKETS);
    int mark = row_h / CT_STATS_SCR_MARK_DIV;
    if (mark < 1) {
        mark = 1;
    }
    bool has_prev = false;
    int prev_x = 0;
    int prev_y = 0;
    for (int i = 0; i < n; i++) {
        if (v->retention[i].reviews > 0 && bars[i].bar.height > 0.0f) {
            DrawRectangleRec(bars[i].bar, ORANGE);
        }
        if (v->retention[i].reviews == 0) {
            continue;
        }
        double px = (double)bars[i].bar.x + (double)bars[i].bar.width / 2.0;
        double py = (double)chart.y + (double)chart.height -
                    v->retention[i].predicted_recall * (double)chart.height;
        DrawRectangle((int)px - mark, (int)py - mark, mark * 2, mark * 2, GREEN);
        if (has_prev) {
            DrawLine(prev_x, prev_y, (int)px, (int)py, GREEN);
        }
        has_prev = true;
        prev_x = (int)px;
        prev_y = (int)py;
    }
}

static void ct_stats_draw_deck(ct_stats_view *v, int x, int y, int w, int h, int row_h,
                               int margin) {
    GuiGroupBox(ct_stats_rect(x, y, w, h), "deck");
    int inner_x = x + margin;
    int inner_y = y + margin;
    int inner_w = w - 2 * margin;
    if (v->deck_selected < 0 || v->deck_selected >= v->deck_count) {
        GuiLabel(ct_stats_rect(inner_x, inner_y, inner_w, row_h), "no deck selected");
        return;
    }
    const ct_deck_stats *d = &v->decks[v->deck_selected];
    char lines[CT_STATS_SCR_SUMMARY_LINES][CT_STATS_SCR_LABEL_MAX];
    snprintf(lines[0], sizeof lines[0], "cards %ld due %ld", d->total_cards, d->due_now);
    snprintf(lines[1], sizeof lines[1], "learning %ld review %ld", d->learning, d->review_state);
    snprintf(lines[2], sizeof lines[2], "avg stability %.2f", d->avg_stability);
    char *last = lines[CT_STATS_SCR_SUMMARY_LINES - 1];
    snprintf(last, sizeof lines[0], "avg difficulty %.2f", d->avg_difficulty);
    for (int i = 0; i < CT_STATS_SCR_SUMMARY_LINES; i++) {
        GuiLabel(ct_stats_rect(inner_x, inner_y + i * row_h, inner_w, row_h), lines[i]);
    }
}

void ct_screen_stats_update(ct_app *app) {
    if (app == NULL) {
        return;
    }
    if (!ct_stats.opened) {
        ct_stats.opened = true;
        char now[CT_STATS_STAMP_MAX];
        ct_now_utc_iso(now, sizeof now);
        ct_stats.status = ct_stats_view_load(&ct_stats.view, app->db, now, NULL);
        ct_stats_build_labels();
        ct_stats.deck_active = ct_stats.view.deck_selected;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        app->screen = CT_SCREEN_MENU;
    }
}

void ct_screen_stats_draw(ct_app *app) {
    ClearBackground(DARKGRAY);
    if (app == NULL) {
        return;
    }
    ct_stats_view *v = &ct_stats.view;
    int w = (int)app->cfg.window.width;
    int h = (int)app->cfg.window.height;
    int margin = w >> CT_STATS_SCR_MARGIN_SHIFT;
    int row_h = h >> CT_STATS_SCR_ROW_SHIFT;
    int status_h = row_h;
    int left_w = w >> CT_STATS_SCR_LEFT_W_SHIFT;
    int deck_h = h / CT_STATS_SCR_DECK_H_DIV;
    int left_x = margin;
    int deck_y = margin;
    int summary_y = deck_y + deck_h + margin;
    int summary_h = row_h * CT_STATS_SCR_SUMMARY_LINES + 2 * margin;
    int sessions_y = summary_y + summary_h + margin;
    int sessions_h = h - sessions_y - status_h - 2 * margin;
    int right_x = left_x + left_w + margin;
    int right_w = w - right_x - margin;
    int forecast_h = h / CT_STATS_SCR_FORECAST_H_DIV;
    int forecast_y = margin;
    int heat_h = h / CT_STATS_SCR_HEAT_H_DIV;
    int heat_y = forecast_y + forecast_h + margin;
    int retention_y = heat_y + heat_h + margin;
    int retention_h = h - retention_y - status_h - margin;

    int active = ct_stats.deck_active;
    if (GuiListViewEx(ct_stats_rect(left_x, deck_y, left_w, deck_h), ct_stats.deck_ptrs,
                      v->deck_count, &ct_stats.deck_scroll, &active, NULL) &&
        active >= 0 && active < v->deck_count) {
        ct_stats.deck_active = active;
        ct_stats.status = ct_stats_view_select_deck(v, v->decks[active].deck);
    }
    ct_stats_draw_deck(v, left_x, summary_y, left_w, summary_h, row_h, margin);
    GuiGroupBox(ct_stats_rect(left_x, sessions_y, left_w, sessions_h), "sessions");
    if (v->session_count > 0) {
        GuiListViewEx(ct_stats_rect(left_x + margin, sessions_y + margin, left_w - 2 * margin,
                                    sessions_h - 2 * margin),
                      ct_stats.session_ptrs, v->session_count, &ct_stats.session_scroll,
                      &ct_stats.session_active, NULL);
    }
    ct_stats_draw_forecast(v, right_x, forecast_y, right_w, forecast_h, row_h, margin);
    ct_stats_draw_heatmap(v, right_x, heat_y, right_w, heat_h, row_h, margin);
    ct_stats_draw_retention(v, right_x, retention_y, right_w, retention_h, row_h, margin);
    GuiStatusBar(ct_stats_rect(margin, h - status_h, w - 2 * margin, status_h),
                 ct_stats_status_text(ct_stats.status));
}
