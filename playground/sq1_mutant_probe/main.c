#include "engine/geometry_sq1.h"
#include "framework.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    ct_sq1_state state;
    char alg[512];
} walk_entry;

static const char *next_probe_token(const char *cursor, char *out, unsigned cap) {
    while (*cursor == ' ') {
        cursor++;
    }
    if (*cursor == '\0') {
        return NULL;
    }
    unsigned len = 0;
    if (cursor[0] != '/') {
        while (cursor[len] != '\0' && cursor[len] != ' ' && cursor[len] != '/') {
            len++;
        }
    } else {
        len = 1;
    }
    if (len >= cap) {
        return NULL;
    }
    memcpy(out, cursor, len);
    out[len] = '\0';
    return cursor + len;
}

static int cut_flags(const ct_sq1_state *s, int layer, int *cut0, int *cut6) {
    const uint8_t *u = s->units + layer * CT_SQ1_UNITS;
    *cut0 = u[0] == u[CT_SQ1_UNITS - 1];
    *cut6 = u[CT_SQ1_CUT_HALF - 1] == u[CT_SQ1_CUT_HALF];
    return 0;
}

static int replay(const char *alg) {
    ct_sq1_state s;
    ct_sq1_state_solved(&s);
    char tok[64];
    const char *cursor = alg;
    while ((cursor = next_probe_token(cursor, tok, sizeof tok)) != NULL) {
        ct_move m;
        int id = ct_geometry_sq1()->move_from_token(ct_geometry_sq1(), tok, CT_GEOMETRY_SINGLE_N, &m);
        if (id < 0) {
            printf("replay: token [%s] rejected\n", tok);
            return 1;
        }
        if (ct_sq1_apply(&s, id) != 0) {
            printf("replay: token [%s] apply failed\n", tok);
            return 1;
        }
    }
    int t0 = 0;
    int t6 = 0;
    int b0 = 0;
    int b6 = 0;
    cut_flags(&s, 0, &t0, &t6);
    cut_flags(&s, 1, &b0, &b6);
    printf("replay t0=%d t6=%d b0=%d b6=%d can=%d bottom0=%d flipped=%d\n", t0, t6, b0, b6,
           ct_sq1_can_slash(&s), s.units[CT_SQ1_UNITS], (int)s.slice_flipped);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        return replay(argv[1]);
    }
    long nonadjacent_equal = 0;
    long single_cut_illegal = 0;
    long top_legal_bottom_illegal = 0;
    long bottom_illegal_top_legal = 0;
    long alias_states = 0;
    long walks = 200000;
    char alias_alg[512] = {0};
    char split_alg[512] = {0};
    ct_sfc64 rng;
    ct_sfc64_init(&rng, 20261010u);
    for (long w = 0; w < walks; w++) {
        ct_sq1_state s;
        ct_sq1_state_solved(&s);
        char alg[512] = "";
        int steps = 4 + (int)ct_sfc64_range(&rng, 10);
        for (int k = 0; k < steps; k++) {
            int id;
            if (ct_sq1_can_slash(&s) && ct_sfc64_range(&rng, 5) == 0) {
                id = CT_SQ1_SLASH;
                strcat(alg, "/");
            } else {
                int x = (int)ct_sfc64_range(&rng, 12) - 5;
                int y = (int)ct_sfc64_range(&rng, 12) - 5;
                if (x == 0 && y == 0) {
                    y = 1;
                }
                id = ct_sq1_move_id(x, y);
                char tok[32];
                snprintf(tok, sizeof tok, "(%d,%d) ", x, y);
                strcat(alg, tok);
            }
            if (ct_sq1_apply(&s, id) != 0) {
                break;
            }
        }
        for (int layer = 0; layer < CT_SQ1_LAYERS; layer++) {
            const uint8_t *u = s.units + layer * CT_SQ1_UNITS;
            static const int pairs[][2] = {{0, 10}, {1, 11}, {4, 6}};
            for (size_t p = 0; p < sizeof pairs / sizeof pairs[0]; p++) {
                if (u[pairs[p][0]] == u[pairs[p][1]]) {
                    nonadjacent_equal++;
                }
            }
        }
        int t0 = 0;
        int t6 = 0;
        int b0 = 0;
        int b6 = 0;
        cut_flags(&s, 0, &t0, &t6);
        cut_flags(&s, 1, &b0, &b6);
        if (t0 == 0 && b0 != 0) {
            top_legal_bottom_illegal++;
        }
        if (b0 == 0 && t0 != 0) {
            bottom_illegal_top_legal++;
        }
        if (t0 == 0 && t6 == 0 && b0 != b6) {
            single_cut_illegal++;
            if (split_alg[0] == '\0') {
                printf("and_kill_witness alg=[%s] t0=%d t6=%d b0=%d b6=%d can=%d\n", alg, t0, t6,
                       b0, b6, ct_sq1_can_slash(&s));
                snprintf(split_alg, sizeof split_alg, "%s", alg);
            }
        }
        if (s.units[CT_SQ1_UNITS] == 0 && !s.slice_flipped && ct_sq1_can_slash(&s) == 1) {
            alias_states++;
            if (alias_alg[0] == '\0') {
                snprintf(alias_alg, sizeof alias_alg, "%s", alg);
            }
        }
    }
    printf("walks=%ld\n", walks);
    printf("nonadjacent_equal=%ld\n", nonadjacent_equal);
    printf("single_cut_illegal=%ld\n", single_cut_illegal);
    printf("top_legal_bottom_illegal=%ld alg=[%s]\n", top_legal_bottom_illegal, split_alg);
    printf("bottom_illegal_top_legal=%ld\n", bottom_illegal_top_legal);
    printf("alias_states=%ld alg=[%s]\n", alias_states, alias_alg);
    return 0;
}
