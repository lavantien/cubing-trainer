#ifndef CT_UI_WIDGETS_H
#define CT_UI_WIDGETS_H

#include "raylib.h"

typedef struct {
    Rectangle bar;
    double value;
    double label_day;
} ct_chart_bar;

int ct_chart_layout(Rectangle bounds, const double *values, int count, ct_chart_bar *out, int cap);
int ct_heatmap_cell(Rectangle bounds, int cols, int rows, int index, Rectangle *out);
int ct_heatmap_pick(Rectangle bounds, int cols, int rows, Vector2 point);
Color ct_heatmap_color(double accuracy);

#endif
