#include "trainer/selector.h"

#include <stddef.h>

#include "gen/targets.h"
#include "trainer/cfop.h"

enum {
    CT_SELECTOR_EVENT_COUNT = CT_EVENT_FTO + 1,
    CT_SELECTOR_METHOD_COUNT = CT_METHOD_CONCEPT + 1,
    CT_SELECTOR_CORNER_BUFFER = 0,
    CT_SELECTOR_EDGE_BUFFER = 2,
    CT_SELECTOR_PAIR_KEY_LEN = 3
};

static const struct {
    ct_event event;
    ct_method method;
} ct_event_methods[] = {
    {CT_EVENT_3X3, CT_METHOD_CFOP},      {CT_EVENT_2X2, CT_METHOD_CLL},
    {CT_EVENT_3BLD, CT_METHOD_3STYLE},   {CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN},
    {CT_EVENT_3BLD, CT_METHOD_M2},       {CT_EVENT_4BLD, CT_METHOD_3STYLE},
    {CT_EVENT_4BLD, CT_METHOD_U2},       {CT_EVENT_4BLD, CT_METHOD_R2},
    {CT_EVENT_5BLD, CT_METHOD_3STYLE},   {CT_EVENT_5BLD, CT_METHOD_U2},
    {CT_EVENT_5BLD, CT_METHOD_R2},       {CT_EVENT_MULTI_BLD, CT_METHOD_3STYLE},
    {CT_EVENT_OH, CT_METHOD_CFOP},       {CT_EVENT_4X4, CT_METHOD_YAU},
    {CT_EVENT_5X5, CT_METHOD_REDUCTION}, {CT_EVENT_6X6, CT_METHOD_REDUCTION},
    {CT_EVENT_7X7, CT_METHOD_REDUCTION}, {CT_EVENT_CLOCK, CT_METHOD_LBL},
    {CT_EVENT_MEGAMINX, CT_METHOD_4LLL}, {CT_EVENT_PYRAMINX, CT_METHOD_L4E},
    {CT_EVENT_PYRAMINX, CT_METHOD_LBL},  {CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE},
    {CT_EVENT_SQUARE1, CT_METHOD_LBL},   {CT_EVENT_FTO, CT_METHOD_L4T_REDUX},
};

static const struct {
    ct_method method;
    ct_stage stage;
    ct_orbit orbit;
} ct_lists[] = {
    {CT_METHOD_CFOP, CT_STAGE_PLL, CT_ORBIT_CORNER},
    {CT_METHOD_CFOP, CT_STAGE_OLL, CT_ORBIT_CORNER},
    {CT_METHOD_CFOP, CT_STAGE_F2L, CT_ORBIT_CORNER},
    {CT_METHOD_CFOP, CT_STAGE_CROSS, CT_ORBIT_CORNER},
    {CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
    {CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE},
    {CT_METHOD_3STYLE, CT_STAGE_PARITY, CT_ORBIT_CORNER},
    {CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_EDGE},
    {CT_METHOD_3STYLE, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
    {CT_METHOD_CLL, CT_STAGE_CLL, CT_ORBIT_CORNER},
    {CT_METHOD_YAU, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
    {CT_METHOD_YAU, CT_STAGE_PARITY, CT_ORBIT_WING},
    {CT_METHOD_REDUCTION, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
    {CT_METHOD_4LLL, CT_STAGE_4LLL, CT_ORBIT_CORNER},
    {CT_METHOD_L4E, CT_STAGE_L4E, CT_ORBIT_CORNER},
    {CT_METHOD_LBL, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
    {CT_METHOD_LBL, CT_STAGE_CUBESHAPE, CT_ORBIT_CORNER},
    {CT_METHOD_LBL, CT_STAGE_CO, CT_ORBIT_CORNER},
    {CT_METHOD_LBL, CT_STAGE_EO, CT_ORBIT_EDGE},
    {CT_METHOD_LBL, CT_STAGE_CP, CT_ORBIT_CORNER},
    {CT_METHOD_LBL, CT_STAGE_EP, CT_ORBIT_EDGE},
    {CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH, CT_ORBIT_CORNER},
    {CT_METHOD_L4T_REDUX, CT_STAGE_L4T, CT_ORBIT_CORNER},
    {CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT, CT_ORBIT_CORNER},
    {CT_METHOD_M2, CT_STAGE_CONCEPT, CT_ORBIT_EDGE},
    {CT_METHOD_U2, CT_STAGE_CONCEPT, CT_ORBIT_CENTER},
    {CT_METHOD_R2, CT_STAGE_CONCEPT, CT_ORBIT_WING},
};

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
    for (unsigned long i = 0; i < sizeof ct_event_methods / sizeof ct_event_methods[0]; i++) {
        if (ct_event_methods[i].event == ev && ct_event_methods[i].method == m) {
            return 1;
        }
    }
    return 0;
}

static int ct_selector_cap(int cap) {
    return cap < 0 ? 0 : cap;
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
    for (unsigned long i = 0; i < sizeof ct_event_methods / sizeof ct_event_methods[0]; i++) {
        if (ct_event_methods[i].event != ev) {
            continue;
        }
        if (out != NULL && total < cap) {
            out[total] = ct_event_methods[i].method;
        }
        total++;
    }
    return total;
}

int ct_selector_lists(ct_event ev, ct_method m, int *out_ids, ct_stage *out_stages,
                      ct_orbit *out_orbits, int cap) {
    cap = ct_selector_cap(cap);
    if (!ct_selector_pair_ok(ev, m)) {
        return 0;
    }
    int total = 0;
    for (unsigned long i = 0; i < sizeof ct_lists / sizeof ct_lists[0]; i++) {
        if (ct_lists[i].method != m) {
            continue;
        }
        if (out_ids != NULL && total < cap) {
            out_ids[total] = total;
        }
        if (out_stages != NULL && total < cap) {
            out_stages[total] = ct_lists[i].stage;
        }
        if (out_orbits != NULL && total < cap) {
            out_orbits[total] = ct_lists[i].orbit;
        }
        total++;
    }
    return total;
}

static void ct_selector_list_def(ct_method m, int list_id, ct_stage *stage, ct_orbit *orbit) {
    int seen = 0;
    for (unsigned long i = 0; i < sizeof ct_lists / sizeof ct_lists[0]; i++) {
        if (ct_lists[i].method != m) {
            continue;
        }
        if (seen == list_id) {
            *stage = ct_lists[i].stage;
            *orbit = ct_lists[i].orbit;
            return;
        }
        seen++;
    }
}

int ct_selector_case_count(ct_event ev, ct_method m, int list_id) {
    int lists = ct_selector_lists(ev, m, NULL, NULL, NULL, 0);
    if (lists <= 0 || list_id < 0 || list_id >= lists) {
        return -1;
    }
    if (m == CT_METHOD_CFOP) {
        ct_stage stage;
        ct_orbit orbit;
        ct_selector_list_def(m, list_id, &stage, &orbit);
        if (stage == CT_STAGE_CROSS) {
            return 0;
        }
        return ct_cfop_case_count(stage);
    }
    if (m == CT_METHOD_3STYLE) {
        if (list_id == 0) {
            return ct_targets_corner(CT_SELECTOR_CORNER_BUFFER, NULL, 0);
        }
        if (list_id == 1) {
            return ct_targets_edge(CT_SELECTOR_EDGE_BUFFER, NULL, 0);
        }
    }
    return 0;
}

static int ct_selector_pair_key(int list_id, int index, char *out, unsigned cap) {
    if (cap < CT_SELECTOR_PAIR_KEY_LEN) {
        return -1;
    }
    ct_target_pair pairs[CT_TARGET_STICKERS * CT_TARGET_STICKERS];
    int total;
    if (list_id == 0) {
        total = ct_targets_corner(CT_SELECTOR_CORNER_BUFFER, pairs,
                                  (int)(sizeof pairs / sizeof pairs[0]));
    } else {
        total =
            ct_targets_edge(CT_SELECTOR_EDGE_BUFFER, pairs, (int)(sizeof pairs / sizeof pairs[0]));
    }
    if (index < 0 || index >= total) {
        return -1;
    }
    out[0] = (char)('A' + pairs[index].t1);
    out[1] = (char)('A' + pairs[index].t2);
    out[2] = '\0';
    return 0;
}

int ct_selector_case_key(ct_event ev, ct_method m, int list_id, int index, char *out,
                         unsigned cap) {
    int count = ct_selector_case_count(ev, m, list_id);
    if (count <= 0 || index < 0 || index >= count || out == NULL) {
        return -1;
    }
    if (m == CT_METHOD_CFOP) {
        ct_stage stage;
        ct_orbit orbit;
        ct_selector_list_def(m, list_id, &stage, &orbit);
        return ct_cfop_case_key(stage, index, out, cap);
    }
    if (m == CT_METHOD_3STYLE) {
        return ct_selector_pair_key(list_id, index, out, cap);
    }
    return -1;
}
