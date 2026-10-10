#include <stdio.h>
#include <string.h>

#include "engine/cube.h"
#include "gen/interchange.h"
#include "gen/targets.h"

static int piece_of(int orbit, int sticker) {
    return orbit == 0 ? ct_corner_piece_of(sticker) : ct_edge_piece_of(sticker);
}

static int raw_index(int n, int compact) {
    int f = compact / (n * n);
    int rest = compact % (n * n);
    int r = rest / n;
    int col = rest % n;
    return f * CT_CUBE_STRIDE * CT_CUBE_STRIDE + r * CT_CUBE_STRIDE + col;
}

static void cells_of_piece(int n, int orbit, int piece, int cells[], int *count) {
    *count = 0;
    for (int s = 0; s < 24; s++) {
        if (piece_of(orbit, s) != piece) {
            continue;
        }
        int f;
        int r;
        int c;
        ct_sticker_cell(orbit, s, &f, &r, &c);
        cells[(*count)++] = f * n * n + r * n + c;
    }
}

static ct_cube base_cube(int n) {
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = n;
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < n; r++) {
            for (int col = 0; col < n; col++) {
                c.facelet[f * CT_CUBE_STRIDE * CT_CUBE_STRIDE + r * CT_CUBE_STRIDE + col] =
                    (uint8_t)(f * n * n + r * n + col);
            }
        }
    }
    return c;
}

static int pieces_swap(int n, ct_move m, int orbit, int x, int y) {
    ct_cube c = base_cube(n);
    ct_apply(&c, m);
    int inv[6 * 7 * 7];
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < n; r++) {
            for (int col = 0; col < n; col++) {
                int p = f * CT_CUBE_STRIDE * CT_CUBE_STRIDE + r * CT_CUBE_STRIDE + col;
                inv[c.facelet[p]] = f * n * n + r * n + col;
            }
        }
    }
    int cx[8];
    int cy[8];
    int nx;
    int ny;
    cells_of_piece(n, orbit, piece_of(orbit, x), cx, &nx);
    cells_of_piece(n, orbit, piece_of(orbit, y), cy, &ny);
    int x_to_y = 0;
    int y_to_x = 0;
    for (int i = 0; i < nx; i++) {
        int landed = inv[cx[i]];
        for (int j = 0; j < ny; j++) {
            if (landed == cy[j]) {
                x_to_y++;
            }
        }
    }
    for (int i = 0; i < ny; i++) {
        int landed = inv[cy[i]];
        for (int j = 0; j < nx; j++) {
            if (landed == cx[j]) {
                y_to_x++;
            }
        }
    }
    return x_to_y == nx && y_to_x == ny;
}

int main(void) {
    ct_move cands[CT_INTERCHANGE_MAX_CANDIDATES];
    int count = ct_candidate_moves(3, cands, CT_INTERCHANGE_MAX_CANDIDATES);
    printf("candidates=%d\n", count);
    long fires = 0;
    long genuine = 0;
    long same_piece_fires = 0;
    long mismatches = 0;
    int corner02 = -1;
    for (int orbit = 0; orbit < 2; orbit++) {
        for (int x = 0; x < 24; x++) {
            for (int y = 0; y < 24; y++) {
                if (x == y) {
                    continue;
                }
                ct_move res[8];
                int n = ct_interchanges_for(orbit, x, y, 3, res, 8);
                if (n < 0) {
                    printf("error at orbit=%d x=%d y=%d\n", orbit, x, y);
                    return 1;
                }
                for (int i = 0; i < n; i++) {
                    fires++;
                    if (piece_of(orbit, x) == piece_of(orbit, y)) {
                        same_piece_fires++;
                    }
                    if (pieces_swap(3, res[i], orbit, x, y)) {
                        genuine++;
                    } else {
                        mismatches++;
                        if (mismatches <= 5) {
                            printf("mismatch orbit=%d x=%d y=%d move=%d %d %d %d\n", orbit, x, y,
                                   res[i].face, res[i].depth_from, res[i].depth_to, res[i].quarter);
                        }
                    }
                }
                if (orbit == 0 && x == 0 && y == 2) {
                    corner02 = n;
                }
            }
        }
    }
    printf("fires=%ld genuine_piece_swaps=%ld mismatches=%ld same_piece_fires=%ld corner02=%d\n",
           fires, genuine, mismatches, same_piece_fires, corner02);

    int n4 = ct_interchanges_for(0, 0, 2, 4, cands, CT_INTERCHANGE_MAX_CANDIDATES);
    printf("n4_corner_sticker02_moves=%d\n", n4);
    return 0;
}
