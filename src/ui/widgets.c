#include "ui/widgets.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

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
