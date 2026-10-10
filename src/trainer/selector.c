#include "trainer/selector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gen/targets.h"
#include "trainer/cfop.h"

enum {
    CT_SELECTOR_EVENT_COUNT = CT_EVENT_FTO + 1,
    CT_SELECTOR_METHOD_COUNT = CT_METHOD_CONCEPT + 1,
    CT_SELECTOR_PAIR_KEY_LEN = 3,
    CT_SELECTOR_PACK_ROWS_MAX = (1 << 8),
    CT_SELECTOR_SETTING_MAX = (1 << 5),
    CT_SELECTOR_CHART_ROWS_MAX = CT_TARGET_STICKERS * CT_TARGET_STICKERS
};

static const struct {
    ct_event event;
    ct_method method;
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
} ct_selector_table[] = {
    {CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_PLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_OLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_3X3, CT_METHOD_CFOP, CT_STAGE_F2L, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_PLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_OLL, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_OH, CT_METHOD_CFOP, CT_STAGE_F2L, CT_ORBIT_CORNER, CT_SELECTOR_CFOP},
    {CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_4BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_4BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_5BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_5BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_CHART},
    {CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_CHART},
    {CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT, CT_ORBIT_EDGE, CT_SELECTOR_PACK},
    {CT_EVENT_4BLD, CT_METHOD_U2, CT_STAGE_CONCEPT, CT_ORBIT_CENTER, CT_SELECTOR_PACK},
    {CT_EVENT_4BLD, CT_METHOD_R2, CT_STAGE_CONCEPT, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_5BLD, CT_METHOD_U2, CT_STAGE_CONCEPT, CT_ORBIT_CENTER, CT_SELECTOR_PACK},
    {CT_EVENT_5BLD, CT_METHOD_R2, CT_STAGE_CONCEPT, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_5X5, CT_METHOD_REDUCTION, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_6X6, CT_METHOD_REDUCTION, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_7X7, CT_METHOD_REDUCTION, CT_STAGE_PARITY, CT_ORBIT_WING, CT_SELECTOR_PACK},
    {CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_PYRAMINX, CT_METHOD_LBL, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH, CT_ORBIT_CORNER,
     CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO, CT_ORBIT_EDGE, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP, CT_ORBIT_EDGE, CT_SELECTOR_PACK},
    {CT_EVENT_MEGAMINX, CT_METHOD_4LLL, CT_STAGE_4LLL, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
    {CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, CT_ORBIT_CORNER, CT_SELECTOR_PACK},
};

static const struct {
    const char *key;
    int fallback;
} ct_selector_buffer_keys[] = {
    {"bld.buffer_corner", CT_3STYLE_CORNER_BUFFER},
    {"bld.buffer_edge", CT_3STYLE_EDGE_BUFFER},
};

static ct_pack_entry ct_selector_pack_rows[CT_SELECTOR_PACK_ROWS_MAX];
static ct_target_pair ct_selector_chart_rows[CT_SELECTOR_CHART_ROWS_MAX];

static int ct_selector_event_ok(ct_event ev) {
    return (int)ev >= 0 && (int)ev < CT_SELECTOR_EVENT_COUNT;
}

static int ct_selector_method_ok(ct_method m) {
    return (int)m >= 0 && (int)m < CT_SELECTOR_METHOD_COUNT;
}

static int ct_selector_pair_ok(ct_event ev, ct_method m) {
    if (!ct_selector_event_ok(ev) || !ct_selector_method_ok(m)) {
        return 0;
    }
    for (unsigned long i = 0; i < sizeof ct_selector_table / sizeof ct_selector_table[0]; i++) {
        if (ct_selector_table[i].event == ev && ct_selector_table[i].method == m) {
            return 1;
        }
    }
    return 0;
}

static int ct_selector_cap(int cap) {
    return cap < 0 ? 0 : cap;
}

static int ct_selector_buffer_of(const ct_selector_scheme *scheme, ct_orbit orbit) {
    return orbit == CT_ORBIT_EDGE ? scheme->buffer_edge : scheme->buffer_corner;
}

static int ct_selector_list_def(ct_event ev, ct_method m, int list_id, ct_stage *stage,
                                ct_orbit *orbit, ct_selector_source *source) {
    int seen = 0;
    for (unsigned long i = 0; i < sizeof ct_selector_table / sizeof ct_selector_table[0]; i++) {
        if (ct_selector_table[i].event != ev || ct_selector_table[i].method != m) {
            continue;
        }
        if (seen == list_id) {
            *stage = ct_selector_table[i].stage;
            *orbit = ct_selector_table[i].orbit;
            *source = ct_selector_table[i].source;
            return 0;
        }
        seen++;
    }
    return CT_SELECTOR_ERR;
}

static int ct_selector_list_count(ct_event ev, ct_method m) {
    int total = 0;
    for (unsigned long i = 0; i < sizeof ct_selector_table / sizeof ct_selector_table[0]; i++) {
        if (ct_selector_table[i].event == ev && ct_selector_table[i].method == m) {
            total++;
        }
    }
    return total;
}

static int ct_selector_parse_buffer(const char *text, int fallback) {
    char *end = NULL;
    long v = strtol(text, &end, CT_DECIMAL_BASE);
    if (end == text || end[0] != '\0' || v < 0 || v >= CT_LETTER_STICKERS) {
        return fallback;
    }
    return (int)v;
}

int ct_selector_scheme_load(sqlite3 *db, ct_selector_scheme *out) {
    if (db == NULL || out == NULL) {
        return CT_SELECTOR_ERR;
    }
    int scheme = ct_letter_scheme_default_id(db);
    if (scheme <= 0) {
        if (ct_letter_seed_speffz(db) != 0) {
            return CT_SELECTOR_ERR;
        }
        scheme = ct_letter_scheme_default_id(db);
    }
    if (scheme <= 0 || ct_letter_map_load(db, scheme, &out->map) < 0) {
        return CT_SELECTOR_ERR;
    }
    out->buffer_corner = ct_selector_buffer_keys[0].fallback;
    out->buffer_edge = ct_selector_buffer_keys[1].fallback;
    char value[CT_SELECTOR_SETTING_MAX];
    for (unsigned long i = 0;
         i < sizeof ct_selector_buffer_keys / sizeof ct_selector_buffer_keys[0]; i++) {
        if (ct_repo_settings_get(db, ct_selector_buffer_keys[i].key, value,
                                 (unsigned)sizeof value) == 0) {
            int v = ct_selector_parse_buffer(value, ct_selector_buffer_keys[i].fallback);
            if (i == 0) {
                out->buffer_corner = v;
            } else {
                out->buffer_edge = v;
            }
        }
    }
    return 0;
}

int ct_selector_events(ct_event *out, int cap) {
    cap = ct_selector_cap(cap);
    int total = 0;
    for (ct_event ev = CT_EVENT_3X3; (int)ev < CT_SELECTOR_EVENT_COUNT; ev++) {
        if (out != NULL && total < cap) {
            out[total] = ev;
        }
        total++;
    }
    return total;
}

int ct_selector_methods(ct_event ev, ct_method *out, int cap) {
    cap = ct_selector_cap(cap);
    if (!ct_selector_event_ok(ev)) {
        return 0;
    }
    int total = 0;
    for (unsigned long i = 0; i < sizeof ct_selector_table / sizeof ct_selector_table[0]; i++) {
        if (ct_selector_table[i].event != ev) {
            continue;
        }
        int duplicate = 0;
        for (unsigned long j = 0; j < i; j++) {
            if (ct_selector_table[j].event == ev &&
                ct_selector_table[j].method == ct_selector_table[i].method) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate) {
            continue;
        }
        if (out != NULL && total < cap) {
            out[total] = ct_selector_table[i].method;
        }
        total++;
    }
    return total;
}

int ct_selector_lists(ct_event ev, ct_method m, ct_selector_list *out, int cap) {
    cap = ct_selector_cap(cap);
    if (!ct_selector_pair_ok(ev, m)) {
        return 0;
    }
    int total = 0;
    for (unsigned long i = 0; i < sizeof ct_selector_table / sizeof ct_selector_table[0]; i++) {
        if (ct_selector_table[i].event != ev || ct_selector_table[i].method != m) {
            continue;
        }
        if (out != NULL && total < cap) {
            out[total].stage = ct_selector_table[i].stage;
            out[total].orbit = ct_selector_table[i].orbit;
            out[total].source = ct_selector_table[i].source;
        }
        total++;
    }
    return total;
}

int ct_selector_case_count(sqlite3 *db, const ct_selector_scheme *scheme, ct_event ev, ct_method m,
                           int list_id) {
    if (!ct_selector_pair_ok(ev, m) || list_id < 0 || list_id >= ct_selector_list_count(ev, m)) {
        return CT_SELECTOR_ERR;
    }
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
    if (ct_selector_list_def(ev, m, list_id, &stage, &orbit, &source) != 0) {
        return CT_SELECTOR_ERR;
    }
    if (source == CT_SELECTOR_CFOP) {
        return ct_cfop_case_count(stage);
    }
    if (source == CT_SELECTOR_CHART) {
        if (scheme == NULL) {
            return CT_SELECTOR_ERR;
        }
        if (orbit == CT_ORBIT_EDGE) {
            return ct_targets_edge(scheme->buffer_edge, NULL, 0);
        }
        return ct_targets_corner(scheme->buffer_corner, NULL, 0);
    }
    return ct_pack_count(db, ev, m, stage);
}

static int ct_selector_chart_pair(ct_orbit orbit, int buffer, int index, ct_target_pair *out) {
    int total;
    if (orbit == CT_ORBIT_EDGE) {
        total = ct_targets_edge(buffer, ct_selector_chart_rows, CT_SELECTOR_CHART_ROWS_MAX);
    } else {
        total = ct_targets_corner(buffer, ct_selector_chart_rows, CT_SELECTOR_CHART_ROWS_MAX);
    }
    if (total <= 0 || index < 0 || index >= total) {
        return CT_SELECTOR_ERR;
    }
    *out = ct_selector_chart_rows[index];
    return 0;
}

int ct_selector_case_key(sqlite3 *db, const ct_selector_scheme *scheme, ct_event ev, ct_method m,
                         int list_id, int index, char *out, unsigned cap) {
    int count = ct_selector_case_count(db, scheme, ev, m, list_id);
    if (count <= 0 || index < 0 || index >= count || out == NULL) {
        return CT_SELECTOR_ERR;
    }
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
    if (ct_selector_list_def(ev, m, list_id, &stage, &orbit, &source) != 0) {
        return CT_SELECTOR_ERR;
    }
    if (source == CT_SELECTOR_CFOP) {
        return ct_cfop_case_key(stage, index, out, cap);
    }
    if (source == CT_SELECTOR_CHART) {
        if (scheme == NULL || cap < (unsigned)CT_SELECTOR_PAIR_KEY_LEN) {
            return CT_SELECTOR_ERR;
        }
        ct_target_pair pair;
        int buffer = ct_selector_buffer_of(scheme, orbit);
        if (ct_selector_chart_pair(orbit, buffer, index, &pair) != 0) {
            return CT_SELECTOR_ERR;
        }
        const char *letters = orbit == CT_ORBIT_EDGE ? scheme->map.edge : scheme->map.corner;
        out[0] = letters[pair.t1];
        out[1] = letters[pair.t2];
        out[2] = '\0';
        return 0;
    }
    ct_pack_entry e;
    if (ct_selector_pack_case(db, ev, m, list_id, index, &e) != 0) {
        return CT_SELECTOR_ERR;
    }
    if (strlen(e.case_key) >= cap) {
        return CT_SELECTOR_ERR;
    }
    snprintf(out, cap, "%s", e.case_key);
    return 0;
}

int ct_selector_case_pair(const ct_selector_scheme *scheme, ct_event ev, ct_method m, int list_id,
                          int index, int *t1, int *t2) {
    if (scheme == NULL || t1 == NULL || t2 == NULL) {
        return CT_SELECTOR_ERR;
    }
    if (!ct_selector_pair_ok(ev, m) || list_id < 0 || list_id >= ct_selector_list_count(ev, m)) {
        return CT_SELECTOR_ERR;
    }
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
    if (ct_selector_list_def(ev, m, list_id, &stage, &orbit, &source) != 0 ||
        source != CT_SELECTOR_CHART) {
        return CT_SELECTOR_ERR;
    }
    ct_target_pair pair;
    int buffer = ct_selector_buffer_of(scheme, orbit);
    if (ct_selector_chart_pair(orbit, buffer, index, &pair) != 0) {
        return CT_SELECTOR_ERR;
    }
    *t1 = pair.t1;
    *t2 = pair.t2;
    return 0;
}

int ct_selector_pack_case(sqlite3 *db, ct_event ev, ct_method m, int list_id, int index,
                          ct_pack_entry *out) {
    if (!ct_selector_pair_ok(ev, m) || list_id < 0 || list_id >= ct_selector_list_count(ev, m) ||
        index < 0 || out == NULL) {
        return CT_SELECTOR_ERR;
    }
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
    if (ct_selector_list_def(ev, m, list_id, &stage, &orbit, &source) != 0 ||
        source != CT_SELECTOR_PACK) {
        return CT_SELECTOR_ERR;
    }
    int cap = index + 1;
    if (cap > CT_SELECTOR_PACK_ROWS_MAX) {
        return CT_SELECTOR_ERR;
    }
    int rows = ct_pack_list(db, ev, m, stage, ct_selector_pack_rows, cap);
    if (rows != cap) {
        return CT_SELECTOR_ERR;
    }
    *out = ct_selector_pack_rows[index];
    return 0;
}

static const char *const ct_selector_group_prefixes[CT_SELECTOR_GROUPS_MAX] = {"flip.", "twist.",
                                                                               "parity.", "ltct."};
static const char *const ct_selector_group_labels[CT_SELECTOR_GROUPS_MAX] = {"flip", "twist",
                                                                             "parity", "ltct"};

int ct_selector_case_group(sqlite3 *db, ct_event ev, ct_method m, int list_id, int index) {
    if (db == NULL || !ct_selector_pair_ok(ev, m) || list_id < 0 ||
        list_id >= ct_selector_list_count(ev, m) || index < 0) {
        return CT_SELECTOR_ERR;
    }
    ct_stage stage;
    ct_orbit orbit;
    ct_selector_source source;
    if (ct_selector_list_def(ev, m, list_id, &stage, &orbit, &source) != 0 ||
        source != CT_SELECTOR_PACK) {
        return CT_SELECTOR_ERR;
    }
    ct_pack_entry e;
    if (ct_selector_pack_case(db, ev, m, list_id, index, &e) != 0) {
        return CT_SELECTOR_ERR;
    }
    for (int g = 0; g < CT_SELECTOR_GROUPS_MAX; g++) {
        size_t len = strlen(ct_selector_group_prefixes[g]);
        if (strncmp(e.case_key, ct_selector_group_prefixes[g], len) == 0) {
            return g;
        }
    }
    return 0;
}

const char *ct_selector_group_label(int group) {
    if (group < 0 || group >= CT_SELECTOR_GROUPS_MAX) {
        return NULL;
    }
    return ct_selector_group_labels[group];
}
