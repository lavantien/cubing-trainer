#ifndef CT_SYNTH_H
#define CT_SYNTH_H

#include "config/config.h"
#include "engine/cube.h"
#include "gen/targets.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CT_SYNTH_ONE = 1,
    CT_SYNTH_TWO = CT_SYNTH_ONE + CT_SYNTH_ONE,
    CT_SYNTH_FOUR = CT_SYNTH_TWO * CT_SYNTH_TWO,
    CT_SYNTH_EIGHT = CT_SYNTH_FOUR * CT_SYNTH_TWO,
    CT_SYNTH_CYCLE_LEN = CT_SYNTH_ONE + CT_SYNTH_TWO,
    CT_SYNTH_N = CT_SYNTH_CYCLE_LEN,
    CT_SYNTH_CELLS = CT_CUBE_FACE_COUNT * CT_SYNTH_N * CT_SYNTH_N,
    CT_SYNTH_KEY_FIELDS = CT_SYNTH_CYCLE_LEN + CT_SYNTH_ONE,
    CT_SYNTH_INSERT_LEN = CT_SYNTH_CYCLE_LEN + CT_SYNTH_TWO,
    CT_SYNTH_TPL_MAX = CT_SYNTH_CYCLE_LEN * CT_SYNTH_FOUR,
    CT_SYNTH_SETUP_MAX = CT_SYNTH_EIGHT,
    CT_SYNTH_FALLBACK_DEPTH_MAX = CT_SYNTH_SETUP_MAX + CT_SYNTH_TPL_MAX,
    CT_SYNTH_FALLBACK_SIDE = CT_SYNTH_EIGHT * CT_SYNTH_EIGHT * CT_SYNTH_EIGHT,
    CT_SYNTH_FALLBACK_NODES = CT_SYNTH_FALLBACK_SIDE * CT_SYNTH_FALLBACK_SIDE,
    CT_SYNTH_FALLBACK_HASH = CT_SYNTH_FALLBACK_NODES * CT_SYNTH_TWO + CT_SYNTH_ONE,
    CT_SYNTH_CORNER_KEYS =
            (CT_TARGET_CORNER_PIECES * (CT_TARGET_CORNER_PIECES - CT_SYNTH_ONE) *
             (CT_TARGET_CORNER_PIECES - CT_SYNTH_TWO) / (CT_SYNTH_CYCLE_LEN * CT_SYNTH_TWO)) *
        CT_TARGET_CORNER_STICKERS_PER_PIECE * CT_TARGET_CORNER_STICKERS_PER_PIECE *
        CT_TARGET_CORNER_STICKERS_PER_PIECE * CT_SYNTH_TWO,
    CT_SYNTH_EDGE_KEYS =
            (CT_TARGET_EDGE_PIECES * (CT_TARGET_EDGE_PIECES - CT_SYNTH_ONE) *
             (CT_TARGET_EDGE_PIECES - CT_SYNTH_TWO) / (CT_SYNTH_CYCLE_LEN * CT_SYNTH_TWO)) *
        CT_TARGET_EDGE_STICKERS_PER_PIECE * CT_TARGET_EDGE_STICKERS_PER_PIECE *
        CT_TARGET_EDGE_STICKERS_PER_PIECE * CT_SYNTH_TWO,
    CT_SYNTH_KEY_MAX = CT_SYNTH_CORNER_KEYS + CT_SYNTH_EDGE_KEYS,
    CT_SYNTH_HASH_MAX = CT_SYNTH_KEY_MAX * CT_SYNTH_TWO + CT_SYNTH_ONE,
};

#define CT_SYNTH_MAX_MOVES (CT_SYNTH_FALLBACK_DEPTH_MAX * CT_SYNTH_TWO)

typedef struct {
    ct_move seq[CT_SYNTH_MAX_MOVES];
    int len;
    int qtm;
    double subset_penalty;
    bool found;
    bool used_setup;
    bool used_fallback;
} ct_synth_result;

int ct_synth_init(void);
int ct_synth_table_size(void);
int ct_synth_table_entry(int index, int key[CT_SYNTH_KEY_FIELDS], ct_move *out, int cap);
int ct_synth_target(int orbit, int buffer, int t1, int t2, const ct_config *cfg,
                    ct_synth_result *out);

#ifdef __cplusplus
}
#endif

#endif
