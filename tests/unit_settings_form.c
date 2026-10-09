#include <math.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../src/ui/ui.h"
#include "framework.h"

static sqlite3 *form_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static bool is_bijection(const char *arr) {
    bool seen[CT_LETTER_STICKERS] = {false};
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        if (arr[i] < CT_LETTER_FIRST || arr[i] > CT_LETTER_LAST) {
            return false;
        }
        int idx = arr[i] - CT_LETTER_FIRST;
        if (seen[idx]) {
            return false;
        }
        seen[idx] = true;
    }
    return true;
}

static void assert_identity_map(const ct_letter_map *m) {
    for (int i = 0; i < CT_LETTER_STICKERS; i++) {
        CT_ASSERT_EQ(m->corner[i], (char)(CT_LETTER_FIRST + i));
        CT_ASSERT_EQ(m->edge[i], (char)(CT_LETTER_FIRST + i));
    }
}

static void assert_default_colors(const ct_settings_form *f) {
    static const unsigned char want[CT_SETTINGS_FACE_COUNT][CT_SETTINGS_CHANNELS] = {
        {0xFF, 0xFF, 0xFF}, {0xFF, 0x58, 0x00}, {0x00, 0x9B, 0x48},
        {0xB7, 0x12, 0x34}, {0x00, 0x46, 0xAD}, {0xFF, 0xD5, 0x00},
    };
    for (int face = 0; face < CT_SETTINGS_FACE_COUNT; face++) {
        for (int ch = 0; ch < CT_SETTINGS_CHANNELS; ch++) {
            CT_ASSERT_EQ(f->colors[face][ch], want[face][ch]);
        }
    }
}

CT_SUITE(settings_form_open) {
    sqlite3 *db = form_db();
    ct_config def = ct_config_defaults();
    ct_settings_form f;
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT_EQ(f.db, db);
    CT_ASSERT_EQ(f.scheme_id, ct_letter_scheme_default_id(db));
    CT_ASSERT(f.scheme_id > 0);
    assert_identity_map(&f.map);
    assert_identity_map(&f.staged);
    CT_ASSERT_DBL_EQ(f.retention, def.fsrs.request_retention, 1e-12);
    CT_ASSERT_DBL_EQ(f.move_seconds, def.anim.move_seconds, 1e-12);
    CT_ASSERT_EQ(f.move_mask, CT_GEN_DEFAULT_PREFERRED_MOVES);
    CT_ASSERT_EQ(f.buffer_corner, 0);
    CT_ASSERT_EQ(f.buffer_edge, 2);
    assert_default_colors(&f);
    CT_ASSERT(!f.dirty);

    ct_settings_form_open(&f, NULL);
    CT_ASSERT_EQ(f.status, CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(f.scheme_id, 0);
    ct_db_close(db);
}

CT_SUITE(settings_form_open_stored) {
    sqlite3 *db = form_db();
    CT_ASSERT_EQ(ct_letter_seed_speffz(db), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "srs.request_retention", "0.85"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "anim.move_seconds", "0.5"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "gen.preferred_moves", "FU"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_corner", "5"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "7"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "ui.color_u", "112233"), 0);

    ct_settings_form f;
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.retention, 0.85, 1e-12);
    CT_ASSERT_DBL_EQ(f.move_seconds, 0.5, 1e-12);
    CT_ASSERT_EQ(f.move_mask, (unsigned)(CT_MOVE_BIT_U | CT_MOVE_BIT_F));
    CT_ASSERT_EQ(f.buffer_corner, 5);
    CT_ASSERT_EQ(f.buffer_edge, 7);
    CT_ASSERT_EQ(f.colors[0][0], 0x11);
    CT_ASSERT_EQ(f.colors[0][1], 0x22);
    CT_ASSERT_EQ(f.colors[0][2], 0x33);
    CT_ASSERT_EQ(f.colors[1][0], 0xFF);
    CT_ASSERT_EQ(f.colors[5][2], 0x00);
    ct_db_close(db);
}

CT_SUITE(settings_form_open_fallbacks) {
    sqlite3 *db = form_db();
    ct_config def = ct_config_defaults();
    CT_ASSERT_EQ(ct_repo_settings_set(db, "srs.request_retention", "abc"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "anim.move_seconds", "-3"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "gen.preferred_moves", "Q!"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_corner", "99"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_edge", "xyz"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "ui.color_d", "GGGGGG"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "ui.color_f", "12345"), 0);

    ct_settings_form f;
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.retention, def.fsrs.request_retention, 1e-12);
    CT_ASSERT_DBL_EQ(f.move_seconds, CT_ANIM_MIN_MOVE_SECONDS, 1e-12);
    CT_ASSERT_EQ(f.move_mask, CT_GEN_DEFAULT_PREFERRED_MOVES);
    CT_ASSERT_EQ(f.buffer_corner, 0);
    CT_ASSERT_EQ(f.buffer_edge, 2);
    assert_default_colors(&f);

    CT_ASSERT_EQ(ct_repo_settings_set(db, "gen.preferred_moves", ""), 0);
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(f.move_mask, CT_GEN_DEFAULT_PREFERRED_MOVES);
    ct_db_close(db);
}

CT_SUITE(settings_form_letters) {
    sqlite3 *db = form_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);

    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 1, f.staged.corner[0]),
                 CT_REPO_CONFLICT);
    CT_ASSERT_EQ(f.status, CT_REPO_CONFLICT);
    CT_ASSERT(!f.dirty);
    CT_ASSERT_EQ(f.staged.corner[1], 'B');

    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_EDGE, 5, f.staged.edge[5]), CT_REPO_OK);
    CT_ASSERT(!f.dirty);

    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 0, 'Y'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 0, 'a'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 0, '\0'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CENTER, 0, 'A'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, CT_LETTER_STICKERS, 'A'),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, -1, 'A'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(NULL, CT_ORBIT_CORNER, 0, 'A'), CT_REPO_ERR_ARG);
    CT_ASSERT(!f.dirty);

    int scheme = f.scheme_id;
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, CT_ORBIT_CORNER, 3, 'A'), 0);
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(f.staged.corner[3], 'A');
    CT_ASSERT(!is_bijection(f.staged.corner));

    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 3, 'D'), CT_REPO_OK);
    CT_ASSERT(f.dirty);
    CT_ASSERT_EQ(f.staged.corner[3], 'D');
    CT_ASSERT(is_bijection(f.staged.corner));
    assert_identity_map(&f.staged);

    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, CT_ORBIT_CORNER, 3, 'A'), 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, CT_ORBIT_CORNER, 4, 'A'), 0);
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 3, 'E'), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 4, 'D'), CT_REPO_OK);
    CT_ASSERT_EQ(f.staged.corner[3], 'E');
    CT_ASSERT_EQ(f.staged.corner[4], 'D');
    CT_ASSERT(is_bijection(f.staged.corner));

    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_EDGE, CT_LETTER_STICKERS - 1, 'A'),
                 CT_REPO_CONFLICT);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_EDGE, 2, 'C'), CT_REPO_OK);
    ct_db_close(db);
}

CT_SUITE(settings_form_swap) {
    sqlite3 *db = form_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);

    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 0, 'A'), CT_REPO_OK);
    CT_ASSERT(!f.dirty);

    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 1, 'C'), CT_REPO_CONFLICT);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 1, 'C'), CT_REPO_OK);
    CT_ASSERT(f.dirty);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT_EQ(f.staged.corner[1], 'C');
    CT_ASSERT_EQ(f.staged.corner[2], 'B');
    CT_ASSERT(is_bijection(f.staged.corner));
    CT_ASSERT(is_bijection(f.staged.edge));

    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 1, 'B'), CT_REPO_OK);
    assert_identity_map(&f.staged);

    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 1, 'Y'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 1, 'a'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 1, '\0'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CENTER, 0, 'A'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, CT_LETTER_STICKERS, 'A'),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, -1, 'A'), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_swap_letter(NULL, CT_ORBIT_CORNER, 0, 'A'), CT_REPO_ERR_ARG);
    assert_identity_map(&f.staged);

    f.staged.corner[3] = '\0';
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 3, 'A'), CT_REPO_OK);
    CT_ASSERT_EQ(f.staged.corner[3], 'A');
    CT_ASSERT_EQ(f.staged.corner[0], '\0');

    int scheme = f.scheme_id;
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, CT_ORBIT_CORNER, 3, 'A'), 0);
    ct_settings_form_open(&f, db);
    CT_ASSERT(!is_bijection(f.staged.corner));
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_CORNER, 3, 'D'), CT_REPO_OK);
    CT_ASSERT_EQ(f.staged.corner[3], 'D');
    assert_identity_map(&f.staged);

    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, CT_ORBIT_EDGE, 5, 'E'), 0);
    ct_settings_form_open(&f, db);
    CT_ASSERT(!is_bijection(f.staged.edge));
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_EDGE, 5, 'F'), CT_REPO_OK);
    CT_ASSERT_EQ(f.staged.edge[5], 'F');
    CT_ASSERT(is_bijection(f.staged.edge));
    CT_ASSERT_EQ(ct_settings_form_swap_letter(&f, CT_ORBIT_EDGE, 5, 'A'), CT_REPO_OK);
    CT_ASSERT_EQ(f.staged.edge[5], 'A');
    CT_ASSERT_EQ(f.staged.edge[0], 'F');
    CT_ASSERT(is_bijection(f.staged.edge));
    ct_db_close(db);
}

CT_SUITE(settings_form_colors) {
    sqlite3 *db = form_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);

    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, 0, CT_SETTINGS_COLOR_MAX), CT_REPO_OK);
    CT_ASSERT(f.dirty);
    CT_ASSERT_EQ(f.colors[0][0], CT_SETTINGS_COLOR_MAX);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, 1, 0), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, CT_SETTINGS_FACE_COUNT - 1,
                                              CT_SETTINGS_CHANNELS - 1, CT_SETTINGS_COLOR_MAX),
                 CT_REPO_OK);

    unsigned char before[CT_SETTINGS_FACE_COUNT][CT_SETTINGS_CHANNELS];
    memcpy(before, f.colors, sizeof before);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, 0, -1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, 0, CT_SETTINGS_COLOR_MAX + 1),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, CT_SETTINGS_FACE_COUNT, 0, 1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, CT_SETTINGS_CHANNELS, 1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_color(NULL, 0, 0, 1), CT_REPO_ERR_ARG);
    CT_ASSERT(memcmp(f.colors, before, sizeof before) == 0);
    ct_db_close(db);
}

CT_SUITE(settings_form_moves) {
    sqlite3 *db = form_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);
    unsigned base = CT_GEN_DEFAULT_PREFERRED_MOVES;

    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_F), CT_REPO_OK);
    CT_ASSERT(f.dirty);
    CT_ASSERT_EQ(f.move_mask, base | CT_MOVE_BIT_F);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_F), CT_REPO_OK);
    CT_ASSERT_EQ(f.move_mask, base);

    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_U), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_D), CT_REPO_OK);
    CT_ASSERT_EQ(f.move_mask, CT_MOVE_BIT_R);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_R), CT_REPO_CONFLICT);
    CT_ASSERT_EQ(f.status, CT_REPO_CONFLICT);
    CT_ASSERT_EQ(f.move_mask, CT_MOVE_BIT_R);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_Z), CT_REPO_OK);
    CT_ASSERT_EQ(f.move_mask, CT_MOVE_BIT_R | CT_MOVE_BIT_Z);

    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, 0), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, 1u << 20), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_U | CT_MOVE_BIT_D), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_move(NULL, CT_MOVE_BIT_U), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(f.move_mask, CT_MOVE_BIT_R | CT_MOVE_BIT_Z);
    ct_db_close(db);
}

CT_SUITE(settings_form_clamps) {
    sqlite3 *db = form_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);

    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, 0.5), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.retention, CT_FSRS_RETENTION_MIN, 1e-12);
    CT_ASSERT(f.dirty);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, 2.0), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.retention, CT_FSRS_RETENTION_MAX, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, 0.85), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.retention, 0.85, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, NAN), CT_REPO_ERR_ARG);
    CT_ASSERT_DBL_EQ(f.retention, 0.85, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(NULL, 0.9), CT_REPO_ERR_ARG);

    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(&f, -5.0), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.move_seconds, CT_ANIM_MIN_MOVE_SECONDS, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(&f, 0.0), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.move_seconds, CT_ANIM_MIN_MOVE_SECONDS, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(&f, 0.75), CT_REPO_OK);
    CT_ASSERT_DBL_EQ(f.move_seconds, 0.75, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(&f, NAN), CT_REPO_ERR_ARG);
    CT_ASSERT_DBL_EQ(f.move_seconds, 0.75, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(NULL, 0.5), CT_REPO_ERR_ARG);
    ct_db_close(db);
}

CT_SUITE(settings_form_buffers) {
    sqlite3 *db = form_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);

    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_CORNER, CT_LETTER_STICKERS - 1),
                 CT_REPO_OK);
    CT_ASSERT(f.dirty);
    CT_ASSERT_EQ(f.buffer_corner, CT_LETTER_STICKERS - 1);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_EDGE, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.buffer_edge, 0);

    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_CORNER, CT_LETTER_STICKERS),
                 CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_EDGE, -1), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_CENTER, 5), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_WING, 5), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(NULL, CT_ORBIT_CORNER, 0), CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(f.buffer_corner, CT_LETTER_STICKERS - 1);
    CT_ASSERT_EQ(f.buffer_edge, 0);
    ct_db_close(db);
}

CT_SUITE(settings_form_save) {
    sqlite3 *db = form_db();
    ct_settings_form f;
    ct_settings_form_open(&f, db);
    int scheme = f.scheme_id;
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, CT_ORBIT_CORNER, 3, 'A'), 0);
    CT_ASSERT_EQ(ct_letter_map_set(db, scheme, CT_ORBIT_CORNER, 4, 'A'), 0);
    ct_settings_form_open(&f, db);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 3, 'E'), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 4, 'D'), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, 0.85), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move_seconds(&f, 0.5), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_F), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_CORNER, 4), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_EDGE, 9), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, 0, 0x11), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, 1, 0x22), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 0, 2, 0x33), CT_REPO_OK);

    CT_ASSERT_EQ(ct_settings_form_save(&f), CT_REPO_OK);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT(!f.dirty);
    CT_ASSERT_EQ(f.map.corner[3], 'E');
    CT_ASSERT_EQ(f.map.corner[4], 'D');

    char val[CT_SETTINGS_TEXT_MAX];
    CT_ASSERT_EQ(ct_repo_settings_get(db, "srs.request_retention", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "0.850000");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "anim.move_seconds", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "0.500000");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "gen.preferred_moves", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "UDRF");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "bld.buffer_corner", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "4");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "bld.buffer_edge", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "9");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "ui.color_u", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "112233");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "ui.color_l", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "FF5800");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "ui.color_f", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "009B48");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "ui.color_r", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "B71234");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "ui.color_b", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "0046AD");
    CT_ASSERT_EQ(ct_repo_settings_get(db, "ui.color_d", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "FFD500");

    ct_letter_map m;
    CT_ASSERT_EQ(ct_letter_map_load(db, scheme, &m), CT_LETTER_STICKERS * 2);
    CT_ASSERT_EQ(m.corner[3], 'E');
    CT_ASSERT_EQ(m.corner[4], 'D');
    CT_ASSERT(is_bijection(m.corner));

    ct_settings_form back;
    ct_settings_form_open(&back, db);
    CT_ASSERT_EQ(back.status, CT_REPO_OK);
    CT_ASSERT_DBL_EQ(back.retention, 0.85, 1e-12);
    CT_ASSERT_DBL_EQ(back.move_seconds, 0.5, 1e-12);
    CT_ASSERT_EQ(back.move_mask,
                 (unsigned)(CT_MOVE_BIT_U | CT_MOVE_BIT_D | CT_MOVE_BIT_R | CT_MOVE_BIT_F));
    CT_ASSERT_EQ(back.buffer_corner, 4);
    CT_ASSERT_EQ(back.buffer_edge, 9);
    CT_ASSERT_EQ(back.colors[0][0], 0x11);
    CT_ASSERT_EQ(back.colors[0][1], 0x22);
    CT_ASSERT_EQ(back.colors[0][2], 0x33);
    CT_ASSERT_EQ(back.colors[1][0], 0xFF);
    CT_ASSERT_EQ(back.staged.corner[3], 'E');
    CT_ASSERT_EQ(back.staged.corner[4], 'D');

    ct_settings_form_open(&back, db);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&back, 0.7), CT_REPO_OK);
    CT_ASSERT_EQ(sqlite3_exec(db, "DELETE FROM letter_scheme", NULL, NULL, NULL), SQLITE_OK);
    CT_ASSERT_EQ(ct_settings_form_save(&back), CT_REPO_ERR_DB);
    CT_ASSERT_EQ(back.status, CT_REPO_ERR_DB);
    CT_ASSERT(back.dirty);
    CT_ASSERT_EQ(ct_repo_settings_get(db, "srs.request_retention", val, sizeof val), 0);
    CT_ASSERT_STR_EQ(val, "0.850000");

    CT_ASSERT_EQ(ct_settings_form_save(NULL), CT_REPO_ERR_ARG);
    back.db = NULL;
    CT_ASSERT_EQ(ct_settings_form_save(&back), CT_REPO_ERR_ARG);
    ct_db_close(db);
}

CT_SUITE(settings_form_cancel) {
    sqlite3 *db = form_db();
    CT_ASSERT_EQ(ct_repo_settings_set(db, "srs.request_retention", "0.900000"), 0);
    CT_ASSERT_EQ(ct_repo_settings_set(db, "bld.buffer_corner", "6"), 0);

    ct_settings_form f;
    ct_settings_form_open(&f, db);
    CT_ASSERT_DBL_EQ(f.retention, 0.9, 1e-12);
    CT_ASSERT_EQ(ct_settings_form_stage_retention(&f, 0.7), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_color(&f, 5, 2, 1), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_move(&f, CT_MOVE_BIT_L), CT_REPO_OK);
    CT_ASSERT_EQ(ct_settings_form_stage_buffer(&f, CT_ORBIT_EDGE, CT_LETTER_STICKERS - 1),
                 CT_REPO_OK);
    CT_ASSERT(f.dirty);

    ct_settings_form_cancel(&f);
    CT_ASSERT_EQ(f.status, CT_REPO_OK);
    CT_ASSERT(!f.dirty);
    CT_ASSERT_DBL_EQ(f.retention, 0.9, 1e-12);
    assert_default_colors(&f);
    CT_ASSERT_EQ(f.move_mask, CT_GEN_DEFAULT_PREFERRED_MOVES);
    CT_ASSERT_EQ(f.buffer_corner, 6);
    CT_ASSERT_EQ(f.buffer_edge, 2);
    assert_identity_map(&f.staged);

    ct_settings_form_cancel(NULL);
    ct_db_close(db);
}

CT_SUITE(settings_form_guards) {
    sqlite3 *db = form_db();
    ct_settings_form_open(NULL, db);
    ct_settings_form_cancel(NULL);
    ct_settings_form f;
    ct_settings_form_open(&f, NULL);
    CT_ASSERT_EQ(f.status, CT_REPO_ERR_ARG);
    CT_ASSERT_EQ(ct_settings_form_stage_letter(&f, CT_ORBIT_CORNER, 0, 'A'), CT_REPO_OK);
    CT_ASSERT(f.dirty);
    CT_ASSERT_EQ(ct_settings_form_save(&f), CT_REPO_ERR_ARG);
    ct_db_close(db);
}
