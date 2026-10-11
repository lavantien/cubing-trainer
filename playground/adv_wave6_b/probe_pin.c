#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/classify.h"
#include "gen/score.h"
#include "gen/synth.h"
#include "gen/targets.h"
#include "storage/db.h"
#include "storage/repo_algs.h"
#include "storage/repo_settings.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { PIN_N = 3, PIN_CELLS = 54 };

static const char PIN_SEQ[] = "U' R2 Uw R2 Uw' F2 Uw R2 Uw' F2 R2 U";

static int raw_cell(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int compact_cell(int face, int row, int col) {
    return face * PIN_N * PIN_N + row * PIN_N + col;
}

static int sticker_cell(int orbit, int sticker) {
    int face = -1;
    int row = -1;
    int col = -1;
    ct_sticker_cell(orbit, sticker, &face, &row, &col);
    return compact_cell(face, row, col);
}

static int piece_of(int orbit, int sticker) {
    return orbit == CT_ORBIT_CORNER ? ct_corner_piece_of(sticker) : ct_edge_piece_of(sticker);
}

int main(void) {
    if (ct_synth_init() < 0) {
        printf("synth init failed\n");
        return 1;
    }
    ct_config cfg = ct_config_defaults();
    const int buffer = CT_3STYLE_CORNER_BUFFER;
    const int t1 = 0;
    const int t2 = 1;
    ct_synth_result res;
    memset(&res, 0, sizeof res);
    int rc = ct_synth_target(CT_ORBIT_CORNER, buffer, t1, t2, &cfg, &res);
    printf("synth_rc %d found %d\n", rc, res.found ? 1 : 0);
    if (rc != 0 || !res.found) {
        printf("SYNTH_MISS\n");
        return 1;
    }
    char text[256];
    if (ct_notation_format(res.seq, res.len, PIN_N, text, (unsigned)sizeof text) < 0) {
        printf("format failed\n");
        return 1;
    }
    printf("sequence %s\n", text);
    printf("pin_match %s\n", strcmp(text, PIN_SEQ) == 0 ? "yes" : "NO");

    ct_score_parts parts;
    memset(&parts, 0, sizeof parts);
    if (ct_score_alg(res.seq, res.len, &cfg, &parts) != CT_SCORE_OK) {
        printf("score failed\n");
        return 1;
    }
    printf("qtm %d pin_18 %s total %.6f subset %.6f regrip %d finger %.6f\n", parts.qtm,
           parts.qtm == 18 ? "yes" : "NO", parts.total, parts.subset_penalty, parts.regrips,
           parts.finger_penalty);

    ct_classify_result cls;
    memset(&cls, 0, sizeof cls);
    if (ct_classify(res.seq, res.len, &cls) != CT_CLASSIFY_OK) {
        printf("classify failed\n");
        return 1;
    }
    printf("classification %s pin_perspecial %s\n", ct_classification_label(cls.cls),
           strcmp(ct_classification_label(cls.cls), "per special") == 0 ? "yes" : "NO");

    ct_cube cube;
    memset(&cube, 0, sizeof cube);
    cube.n = PIN_N;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < PIN_N; row++) {
            for (int col = 0; col < PIN_N; col++) {
                cube.facelet[raw_cell(face, row, col)] =
                    (uint8_t)compact_cell(face, row, col);
            }
        }
    }
    for (int i = 0; i < res.len; i++) {
        ct_apply(&cube, res.seq[i]);
    }
    uint8_t state[PIN_CELLS];
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < PIN_N; row++) {
            for (int col = 0; col < PIN_N; col++) {
                state[compact_cell(face, row, col)] = cube.facelet[raw_cell(face, row, col)];
            }
        }
    }
    int cb = sticker_cell(CT_ORBIT_CORNER, buffer);
    int c1 = sticker_cell(CT_ORBIT_CORNER, t1);
    int c2 = sticker_cell(CT_ORBIT_CORNER, t2);
    printf("cycle_cells cb %d c1 %d c2 %d\n", cb, c1, c2);
    printf("cycle_state c1_holds_cb %d c2_holds_c1 %d cb_holds_c2 %d\n", state[c1] == cb,
           state[c2] == c1, state[cb] == c2);

    bool on_pieces[PIN_CELLS] = {false};
    int key_stickers[3];
    key_stickers[0] = buffer;
    key_stickers[1] = t1;
    key_stickers[2] = t2;
    for (int i = 0; i < 3; i++) {
        int piece = piece_of(CT_ORBIT_CORNER, key_stickers[i]);
        for (int s = 0; s < CT_TARGET_STICKERS; s++) {
            if (piece_of(CT_ORBIT_CORNER, s) == piece) {
                on_pieces[sticker_cell(CT_ORBIT_CORNER, s)] = true;
            }
        }
    }
    int stray = 0;
    for (int q = 0; q < PIN_CELLS; q++) {
        if (state[q] != (uint8_t)q && !on_pieces[q]) {
            stray++;
            printf("stray_cell %d holds %d\n", q, state[q]);
        }
    }
    printf("stray_cells %d\n", stray);

    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK) {
        printf("db open failed\n");
        return 1;
    }
    if (ct_db_migrate(db, "db/migrations") != CT_REPO_OK || ct_letter_seed_speffz(db) != 0) {
        printf("db setup failed\n");
        return 1;
    }
    ct_letter_map map;
    int scheme = ct_letter_scheme_default_id(db);
    int letters = ct_letter_map_load(db, scheme, &map);
    printf("scheme %d letters %d corner0 %c edge2 %c\n", scheme, letters, map.corner[0],
           map.edge[2]);
    ct_config merged = ct_config_defaults();
    ct_repo_settings_merge(db, &merged);
    char hash[CT_ALG_HASH_HEX_MAX];
    ct_params_hash(&merged, scheme, CT_BUFFER_FIXED, CT_3STYLE_CORNER_BUFFER,
                   CT_3STYLE_EDGE_BUFFER, &map, hash);
    printf("current_default_hash %.16s\n", hash);
    sqlite3_close(db);
    return 0;
}
