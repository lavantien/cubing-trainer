#include "config/config.h"
#include "engine/cube.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

enum { SWEEP_CORNER_CAP = 378, SWEEP_EDGE_CAP = 440 };

static int sweep_raw(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int sweep_compact(int face, int row, int col) {
    return face * 9 + row * 3 + col;
}

static int sweep_sticker_compact(int orbit, int sticker) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return sweep_compact(face, row, col);
}

static int sweep_piece_of(int orbit, int sticker) {
    return orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(sticker) : ct_edge_piece_of(sticker);
}

static bool sweep_case_solved(int orbit, int buffer, int t1, int t2, const ct_move *seq, int len) {
    ct_cube c;
    memset(&c, 0, sizeof c);
    c.n = 3;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                c.facelet[sweep_raw(face, row, col)] = (uint8_t)sweep_compact(face, row, col);
            }
        }
    }
    for (int i = 0; i < len; i++) {
        ct_apply(&c, seq[i]);
    }
    int cb = sweep_sticker_compact(orbit, buffer);
    int c1 = sweep_sticker_compact(orbit, t1);
    int c2 = sweep_sticker_compact(orbit, t2);
    uint8_t compact_state[54];
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                compact_state[sweep_compact(face, row, col)] = c.facelet[sweep_raw(face, row, col)];
            }
        }
    }
    if (compact_state[c1] != (uint8_t)cb || compact_state[c2] != (uint8_t)c1 ||
        compact_state[cb] != (uint8_t)c2) {
        return false;
    }
    int pb = sweep_piece_of(orbit, buffer);
    int p1 = sweep_piece_of(orbit, t1);
    int p2 = sweep_piece_of(orbit, t2);
    for (int q = 0; q < 54; q++) {
        if (compact_state[q] == (uint8_t)q) {
            continue;
        }
        bool on_pieces = false;
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            int p = sweep_piece_of(orbit, s);
            if ((p == pb || p == p1 || p == p2) && sweep_sticker_compact(orbit, s) == q) {
                on_pieces = true;
            }
        }
        if (!on_pieces) {
            return false;
        }
    }
    return true;
}

static void sweep_run(const char *label, int orbit, int buffer, int cap) {
    static ct_target_pair pairs[SWEEP_EDGE_CAP];
    int count = orbit == CT_ORBIT_CORNER ? ct_targets_corner(buffer, pairs, cap)
                                         : ct_targets_edge(buffer, pairs, cap);
    ct_config cfg = ct_config_defaults();
    int direct = 0;
    int setup = 0;
    int fallback = 0;
    int miss = 0;
    int invalid = 0;
    int qtm_direct_total = 0;
    int qtm_max = 0;
    int direct_max = 0;
    for (int i = 0; i < count; i++) {
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(orbit, buffer, pairs[i].t1, pairs[i].t2, &cfg, &res);
        if (rc != 0) {
            invalid++;
            continue;
        }
        if (!res.found) {
            miss++;
            if (miss <= 10) {
                printf("%s miss: t1=%d t2=%d\n", label, pairs[i].t1, pairs[i].t2);
            }
            continue;
        }
        if (!sweep_case_solved(orbit, buffer, pairs[i].t1, pairs[i].t2, res.seq, res.len)) {
            printf("%s WRONG ALG: t1=%d t2=%d\n", label, pairs[i].t1, pairs[i].t2);
        }
        if (res.qtm > qtm_max) {
            qtm_max = res.qtm;
        }
        if (res.used_fallback) {
            fallback++;
        } else if (res.used_setup) {
            setup++;
        } else {
            direct++;
            qtm_direct_total += res.qtm;
            if (res.qtm > direct_max) {
                direct_max = res.qtm;
            }
        }
    }
    printf("%s: total=%d direct=%d setup=%d fallback=%d miss=%d invalid=%d "
           "direct_avg_qtm=%.2f max_qtm=%d\n",
           label, count, direct, setup, fallback, miss, invalid,
           direct > 0 ? (double)qtm_direct_total / direct : 0.0, qtm_max);
    printf("%s: direct_max_qtm=%d\n", label, direct_max);
}

int main(void) {
    clock_t start = clock();
    int size = ct_synth_init();
    double build_seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("table size=%d build=%.3fs\n", size, build_seconds);
    if (size < 0) {
        return 1;
    }
    start = clock();
    sweep_run("corner378", CT_ORBIT_CORNER, 0, SWEEP_CORNER_CAP);
    sweep_run("edge440", CT_ORBIT_EDGE, 2, SWEEP_EDGE_CAP);
    printf("sweep seconds=%.3f\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    return 0;
}
