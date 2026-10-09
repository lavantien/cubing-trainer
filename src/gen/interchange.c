#include "gen/interchange.h"
#include "gen/targets.h"

#include <stdbool.h>
#include <string.h>

enum {
    CT_PERM_MIN_N = 2,
    CT_PERM_MAX_N = 6,
    CT_ORBIT_MIN_N = 3,
};

static int perm_raw(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int perm_compact(int n, int face, int row, int col) {
    return face * n * n + row * n + col;
}

int ct_sticker_perm(ct_move m, int n,
                    uint8_t perm[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE]) {
    if (n < CT_PERM_MIN_N || n > CT_PERM_MAX_N || !ct_move_legal(&m, n)) {
        return -1;
    }
    memset(perm, 0xFF, (size_t)(CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE));
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = n;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                c.facelet[perm_raw(face, row, col)] = (uint8_t)perm_compact(n, face, row, col);
            }
        }
    }
    ct_apply(&c, m);
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                perm[c.facelet[perm_raw(face, row, col)]] =
                    (uint8_t)perm_compact(n, face, row, col);
            }
        }
    }
    return 0;
}

int ct_candidate_moves(int n, ct_move *out, int cap) {
    if (cap < 0) {
        cap = 0;
    }
    if (n < CT_PERM_MIN_N || n > CT_CUBE_MAX_N) {
        return 0;
    }
    int half = (n + 1) / 2;
    int total = 0;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int from = 1; from <= half; from++) {
            for (int to = from; to <= half; to++) {
                for (int quarter = 1; quarter < CT_MOVE_QUARTERS; quarter++) {
                    if (out != NULL && total < cap) {
                        out[total] = (ct_move){face, from, to, quarter};
                    }
                    total++;
                }
            }
        }
    }
    return total;
}

int ct_interchanges_for(int orbit, int x, int y, int n, ct_move *out, int cap) {
    if (cap < 0) {
        cap = 0;
    }
    if ((orbit != CT_TARGET_ORBIT_CORNER && orbit != CT_TARGET_ORBIT_EDGE) || x < 0 ||
        x >= CT_TARGET_STICKERS || y < 0 || y >= CT_TARGET_STICKERS || x == y ||
        n < CT_ORBIT_MIN_N || n > CT_PERM_MAX_N) {
        return -1;
    }
    int xf = 0;
    int xr = 0;
    int xc = 0;
    int yf = 0;
    int yr = 0;
    int yc = 0;
    ct_sticker_cell(orbit, x, &xf, &xr, &xc);
    ct_sticker_cell(orbit, y, &yf, &yr, &yc);
    int cx = perm_compact(n, xf, xr, xc);
    int cy = perm_compact(n, yf, yr, yc);
    ct_move cands[CT_INTERCHANGE_MAX_CANDIDATES];
    int count = ct_candidate_moves(n, cands, CT_INTERCHANGE_MAX_CANDIDATES);
    uint8_t perm[CT_CUBE_FACE_COUNT * CT_CUBE_STRIDE * CT_CUBE_STRIDE];
    int total = 0;
    for (int i = 0; i < count; i++) {
        if (ct_sticker_perm(cands[i], n, perm) != 0) {
            return -1;
        }
        if (perm[cx] == (uint8_t)cy && perm[cy] == (uint8_t)cx) {
            if (out != NULL && total < cap) {
                out[total] = cands[i];
            }
            total++;
        }
    }
    return total;
}
