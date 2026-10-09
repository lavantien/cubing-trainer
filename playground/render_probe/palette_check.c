#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>

enum { PALETTE_SIZE = 6, TOLERANCE = 24 };

static const int palette[PALETTE_SIZE][3] = {
    {255, 255, 255}, {255, 161, 0}, {0, 228, 48}, {230, 41, 55}, {0, 121, 241}, {253, 249, 0},
};

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: palette_check <png>\n");
        return 2;
    }
    Image img = LoadImage(argv[1]);
    if (img.data == NULL) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 2;
    }
    Image formatted = ImageCopy(img);
    ImageFormat(&formatted, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    unsigned char *px = formatted.data;
    long counts[PALETTE_SIZE] = {0};
    long background = 0;
    long unknown = 0;
    for (int y = 0; y < formatted.height; y++) {
        for (int x = 0; x < formatted.width; x++) {
            unsigned char *p = px + ((size_t)y * formatted.width + x) * 4;
            if (p[0] >= 240 && p[1] >= 240 && p[2] >= 240) {
                background++;
                continue;
            }
            int matched = -1;
            for (int c = 0; c < PALETTE_SIZE; c++) {
                int dr = abs((int)p[0] - palette[c][0]);
                int dg = abs((int)p[1] - palette[c][1]);
                int db = abs((int)p[2] - palette[c][2]);
                if (dr <= TOLERANCE && dg <= TOLERANCE && db <= TOLERANCE) {
                    matched = c;
                    break;
                }
            }
            if (matched < 0) {
                unknown++;
                if (unknown <= 10) {
                    fprintf(stderr, "unknown pixel %d,%d rgb %d,%d,%d\n", x, y, p[0], p[1], p[2]);
                }
            } else {
                counts[matched]++;
            }
        }
    }
    printf("size=%dx%d background=%ld unknown=%ld\n", formatted.width, formatted.height, background,
           unknown);
    for (int c = 0; c < PALETTE_SIZE; c++) {
        printf("palette[%d]=%ld\n", c, counts[c]);
    }
    UnloadImage(formatted);
    UnloadImage(img);
    return unknown == 0 ? 0 : 1;
}
