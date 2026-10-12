#include "ui/widgets.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "config/config.h"
#include "raygui.h"
#include "ui/ui.h"

static Font ct_ui_font_state;
static bool ct_ui_font_owned;

static int ct_ui_font_codepoints(int *out, int cap) {
    int n = 0;
    for (int c = CT_UI_FONT_ASCII_FIRST; c <= CT_UI_FONT_ASCII_LAST && n < cap; c++) {
        out[n++] = c;
    }
    for (int c = CT_UI_FONT_LATIN_FIRST; c <= CT_UI_FONT_LATIN_LAST && n < cap; c++) {
        out[n++] = c;
    }
    for (int c = CT_UI_FONT_VIET_FIRST; c <= CT_UI_FONT_VIET_LAST && n < cap; c++) {
        out[n++] = c;
    }
    return n;
}

static void ct_ui_theme_apply(void) {
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, (int)CT_UI_COLOR_BORDER);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, (int)CT_UI_COLOR_PANEL);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, (int)CT_UI_COLOR_TEXT);
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, (int)CT_UI_COLOR_ACCENT);
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, (int)CT_UI_COLOR_ACCENT_TINT);
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, (int)CT_UI_COLOR_TEXT);
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, (int)CT_UI_COLOR_ACCENT);
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, (int)CT_UI_COLOR_ACCENT);
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, (int)CT_UI_COLOR_PANEL);
    GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, (int)CT_UI_COLOR_BORDER);
    GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, (int)CT_UI_COLOR_PANEL_ACTIVE);
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, (int)CT_UI_COLOR_TEXT_DISABLED);
    GuiSetStyle(DEFAULT, LINE_COLOR, (int)CT_UI_COLOR_TEXT_DIM);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, (int)CT_UI_COLOR_BG);
    GuiSetStyle(DEFAULT, TEXT_SIZE, CT_UI_FONT_BODY);
    GuiSetStyle(DEFAULT, TEXT_SPACING, CT_UI_TEXT_SPACING);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, CT_UI_LIST_ITEM_H);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_SPACING, CT_UI_LIST_ITEM_GAP);
    GuiSetStyle(LISTVIEW, SCROLLBAR_WIDTH, CT_UI_SCROLL_W);
}

bool ct_ui_theme_load(const char *font_path) {
    if (ct_ui_font_owned) {
        ct_ui_theme_release();
    }
    ct_ui_font_state = GetFontDefault();
    ct_ui_font_owned = false;
    if (font_path != NULL && font_path[0] != '\0') {
        int codepoints[CT_UI_FONT_CODEPOINTS_MAX];
        int count = ct_ui_font_codepoints(codepoints, CT_UI_FONT_CODEPOINTS_MAX);
        Font loaded = LoadFontEx(font_path, CT_UI_FONT_BODY, codepoints, count);
        if (loaded.texture.id != 0 && loaded.glyphCount > 0) {
            ct_ui_font_state = loaded;
            ct_ui_font_owned = true;
            SetTextureFilter(ct_ui_font_state.texture, TEXTURE_FILTER_BILINEAR);
        } else {
            fprintf(stderr, "cubing-trainer: cannot load font %s, using the default font\n",
                    font_path);
            if (loaded.texture.id != 0) {
                UnloadFont(loaded);
            }
        }
    }
    if (ct_ui_font_state.texture.id > 0) {
        GuiSetFont(ct_ui_font_state);
    }
    ct_ui_theme_apply();
    return ct_ui_font_owned;
}

Font ct_ui_font(void) {
    if (ct_ui_font_state.texture.id == 0) {
        ct_ui_font_state = GetFontDefault();
    }
    return ct_ui_font_state;
}

void ct_ui_theme_release(void) {
    GuiSetFont(GetFontDefault());
    if (ct_ui_font_owned) {
        UnloadFont(ct_ui_font_state);
        ct_ui_font_owned = false;
    }
    ct_ui_font_state = GetFontDefault();
}

enum { CT_CHART_GAP_DIV = 4 };

int ct_chart_layout(Rectangle bounds, const double *values, int count, ct_chart_bar *out, int cap) {
    if (count < 0 || (count > 0 && values == NULL) || (cap > 0 && out == NULL) || cap < 0) {
        return -1;
    }
    int n = count < cap ? count : cap;
    if (n == 0) {
        return 0;
    }
    double unit = (double)bounds.width / (double)count;
    double gap = unit / (double)CT_CHART_GAP_DIV;
    double bar_w = unit - gap;
    double max = 0.0;
    for (int i = 0; i < count; i++) {
        if (isfinite(values[i]) && values[i] > max) {
            max = values[i];
        }
    }
    for (int i = 0; i < n; i++) {
        double value = isfinite(values[i]) && values[i] > 0.0 ? values[i] : 0.0;
        double h = max > 0.0 ? (double)bounds.height * value / max : 0.0;
        out[i].bar.x = (float)(bounds.x + unit * (double)i + gap / 2.0);
        out[i].bar.y = (float)(bounds.y + (double)bounds.height - h);
        out[i].bar.width = (float)bar_w;
        out[i].bar.height = (float)h;
        out[i].value = values[i];
        out[i].label_day = (double)i;
    }
    return n;
}

int ct_heatmap_cell(Rectangle bounds, int cols, int rows, int index, Rectangle *out) {
    if (cols <= 0 || rows <= 0 || index < 0 || index / cols >= rows || out == NULL) {
        return -1;
    }
    float cell_w = bounds.width / (float)cols;
    float cell_h = bounds.height / (float)rows;
    int col = index % cols;
    int row = index / cols;
    out->x = bounds.x + cell_w * (float)col;
    out->y = bounds.y + cell_h * (float)row;
    out->width = cell_w;
    out->height = cell_h;
    return 0;
}

int ct_heatmap_pick(Rectangle bounds, int cols, int rows, Vector2 point) {
    if (cols <= 0 || rows <= 0) {
        return -1;
    }
    if (point.x < bounds.x || point.y < bounds.y || point.x >= bounds.x + bounds.width ||
        point.y >= bounds.y + bounds.height) {
        return -1;
    }
    int col = (int)((point.x - bounds.x) * (float)cols / bounds.width);
    int row = (int)((point.y - bounds.y) * (float)rows / bounds.height);
    if (col >= cols) {
        col = cols - 1;
    }
    if (row >= rows) {
        row = rows - 1;
    }
    return row * cols + col;
}

Color ct_heatmap_color(double accuracy) {
    double t = accuracy;
    if (!isfinite(t) || t < 0.0) {
        t = 0.0;
    }
    if (t > 1.0) {
        t = 1.0;
    }
    double half = 1.0 / 2.0;
    double fr;
    double fg;
    if (t < half) {
        fr = 1.0;
        fg = t / half;
    } else {
        fr = (1.0 - t) / half;
        fg = 1.0;
    }
    Color c = {(unsigned char)(fr * (double)UCHAR_MAX), (unsigned char)(fg * (double)UCHAR_MAX), 0,
               UCHAR_MAX};
    return c;
}
