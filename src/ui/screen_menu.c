#include <stddef.h>

#include "raygui.h"
#include "raylib.h"
#include "ui/ui.h"

typedef struct {
    const char *label;
    int key;
    ct_ui_screen screen;
} ct_menu_entry;

static const ct_menu_entry CT_MENU_ENTRIES[] = {
    {"trainers", KEY_ONE, CT_SCREEN_TRAINER},   {"cards", KEY_TWO, CT_SCREEN_CARDS},
    {"review", KEY_THREE, CT_SCREEN_REVIEW},    {"stats", KEY_FOUR, CT_SCREEN_STATS},
    {"settings", KEY_FIVE, CT_SCREEN_SETTINGS},
};

void ct_ui_update(ct_app *app) {
    if (app->screen == CT_SCREEN_TRAINER) {
        ct_screen_trainer_update(app);
        return;
    }
    if (app->screen == CT_SCREEN_CARDS) {
        ct_screen_cards_update(app);
        return;
    }
    if (app->screen == CT_SCREEN_REVIEW) {
        ct_screen_review_update(app);
        return;
    }
    if (app->screen == CT_SCREEN_SETTINGS) {
        ct_screen_settings_update(app);
        return;
    }
    if (app->screen == CT_SCREEN_STATS) {
        ct_screen_stats_update(app);
        return;
    }
    if (app->screen != CT_SCREEN_MENU) {
        app->screen = CT_SCREEN_MENU;
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        app->running = false;
        return;
    }
    for (size_t i = 0; i < sizeof CT_MENU_ENTRIES / sizeof CT_MENU_ENTRIES[0]; i++) {
        if (IsKeyPressed(CT_MENU_ENTRIES[i].key)) {
            app->screen = CT_MENU_ENTRIES[i].screen;
            return;
        }
    }
}

void ct_ui_draw(ct_app *app) {
    if (app->screen == CT_SCREEN_TRAINER) {
        ct_screen_trainer_draw(app);
        return;
    }
    if (app->screen == CT_SCREEN_CARDS) {
        ct_screen_cards_draw(app);
        return;
    }
    if (app->screen == CT_SCREEN_REVIEW) {
        ct_screen_review_draw(app);
        return;
    }
    if (app->screen == CT_SCREEN_SETTINGS) {
        ct_screen_settings_draw(app);
        return;
    }
    if (app->screen == CT_SCREEN_STATS) {
        ct_screen_stats_draw(app);
        return;
    }
    ClearBackground(GetColor(CT_UI_COLOR_BG));
    int w = (int)app->cfg.window.width;
    int h = (int)app->cfg.window.height;
    const char *title = "cubing trainer";
    Font font = ct_ui_font();
    float title_size = (float)CT_UI_FONT_TITLE;
    float title_w = MeasureTextEx(font, title, title_size, CT_UI_TEXT_SPACING).x;
    Vector2 title_pos = {(float)w / 2.0f - title_w / 2.0f, (float)h / 8.0f};
    DrawTextEx(font, title, title_pos, title_size, CT_UI_TEXT_SPACING, GetColor(CT_UI_COLOR_TEXT));
    size_t count = sizeof CT_MENU_ENTRIES / sizeof CT_MENU_ENTRIES[0];
    int btn_w = w / 4;
    int btn_h = CT_UI_BTN_H;
    int step = btn_h + CT_UI_BTN_GAP;
    int total = (int)(count + 1) * step - CT_UI_BTN_GAP;
    int x = (w - btn_w) / 2;
    int y = h / 2 - total / 2 + h / 8;
    for (size_t i = 0; i < count; i++) {
        Rectangle entry_rect = {(float)x, (float)(y + (int)i * step), (float)btn_w, (float)btn_h};
        if (GuiButton(entry_rect, CT_MENU_ENTRIES[i].label)) {
            app->screen = CT_MENU_ENTRIES[i].screen;
        }
    }
    Rectangle quit_rect = {(float)x, (float)(y + (int)count * step), (float)btn_w, (float)btn_h};
    if (GuiButton(quit_rect, "quit")) {
        app->running = false;
    }
}
