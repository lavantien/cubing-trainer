#include "engine/cube.h"
#include "engine/notation.h"

#include <stdio.h>
#include <string.h>

enum { PROBE_N = 3, PROBE_CELLS = 54, PROBE_ALG_MAX = 32 };

typedef struct {
    const char *orbit;
    const char *cycle;
    const char *alg;
} probe_case;

static const probe_case probe_cases[] = {
    {"corner", "(URB UBL ULF)", "L F' L B2 L' F L B2 L2"},
    {"corner", "(URB UBL UFR)", "R2 B2 R F R' B2 R F' R"},
    {"corner", "(URB UBL FRU)", "R B L B' R' B L' B'"},
    {"corner", "(URB UBL FLD)", "U' B D' B' U B D B'"},
    {"corner", "(URB BRD UBL)", "U B' D' B U' B' D B"},
    {"edge", "(UR UF FD)", "U F E F' U' F E' F'"},
    {"edge", "(UR UF BR)", "U F E' F' U' F E F'"},
    {"edge", "(UR UF BL)", "R' E2 R U' R' E2 R U"},
    {"edge", "(UR UF LU)", "L F' L' S' L F L' S"},
    {"edge", "(UR UF DR)", "D M2 D' R2 D M2 D' R2"},
};

static int probe_raw(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int probe_compact(int face, int row, int col) {
    return face * PROBE_N * PROBE_N + row * PROBE_N + col;
}

static void probe_identity(ct_cube *c) {
    memset(c, 0, sizeof *c);
    c->n = PROBE_N;
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < PROBE_N; row++) {
            for (int col = 0; col < PROBE_N; col++) {
                c->facelet[probe_raw(face, row, col)] = (uint8_t)probe_compact(face, row, col);
            }
        }
    }
}

static void probe_sticker_faces(int cell, char *names) {
    int face = cell / (PROBE_N * PROBE_N);
    int row = (cell % (PROBE_N * PROBE_N)) / PROBE_N;
    int col = cell % PROBE_N;
    ct_v3 center;
    ct_v3 normal;
    ct_sticker_world(PROBE_N, face, row, col, &center, &normal);
    static const char axis_letters[3][2] = {{'L', 'R'}, {'D', 'U'}, {'B', 'F'}};
    int coords[3] = {center.x / 2, center.y / 2, center.z / 2};
    int normal_axis = normal.x != 0 ? 0 : (normal.y != 0 ? 1 : 2);
    int pos = 0;
    names[pos++] = axis_letters[normal_axis][coords[normal_axis] > 0 ? 1 : 0];
    for (int axis = 0; axis < 3; axis++) {
        if (axis != normal_axis) {
            names[pos++] = axis_letters[axis][coords[axis] > 0 ? 1 : 0];
        }
    }
    names[pos] = '\0';
}

static int probe_face_of(char c) {
    switch (c) {
    case 'U':
        return CT_FACE_U;
    case 'D':
        return CT_FACE_D;
    case 'L':
        return CT_FACE_L;
    case 'R':
        return CT_FACE_R;
    case 'F':
        return CT_FACE_F;
    case 'B':
        return CT_FACE_B;
    default:
        return -1;
    }
}

static int probe_name_cell(const char *name) {
    int want[CT_CUBE_FACE_COUNT] = {0, 0, 0, 0, 0, 0};
    int len = (int)strlen(name);
    if (len < 2 || len > 3) {
        return -1;
    }
    for (int i = 0; i < len; i++) {
        int face = probe_face_of(name[i]);
        if (face < 0) {
            return -1;
        }
        want[face]++;
    }
    for (int cell = 0; cell < PROBE_CELLS; cell++) {
        int face = cell / (PROBE_N * PROBE_N);
        int row = (cell % (PROBE_N * PROBE_N)) / PROBE_N;
        int col = cell % PROBE_N;
        ct_v3 center;
        ct_v3 normal;
        ct_sticker_world(PROBE_N, face, row, col, &center, &normal);
        if ((normal.x != 0) + (normal.y != 0) + (normal.z != 0) != 1) {
            continue;
        }
        int coords[3] = {center.x / 2, center.y / 2, center.z / 2};
        int own_face = probe_face_of(name[0]);
        if (own_face != face) {
            continue;
        }
        int have[CT_CUBE_FACE_COUNT] = {0, 0, 0, 0, 0, 0};
        for (int axis = 0; axis < 3; axis++) {
            if (coords[axis] == 0) {
                continue;
            }
            int f = axis == 0   ? (coords[axis] > 0 ? CT_FACE_R : CT_FACE_L)
                    : axis == 1 ? (coords[axis] > 0 ? CT_FACE_U : CT_FACE_D)
                                : (coords[axis] > 0 ? CT_FACE_F : CT_FACE_B);
            have[f]++;
        }
        if (memcmp(want, have, sizeof want) == 0) {
            return cell;
        }
    }
    return -1;
}

int main(void) {
    for (size_t i = 0; i < sizeof probe_cases / sizeof probe_cases[0]; i++) {
        ct_move moves[PROBE_ALG_MAX];
        int count = ct_notation_parse(probe_cases[i].alg, PROBE_N, moves, PROBE_ALG_MAX);
        if (count < 0) {
            printf("%s %s parse failed %d\n", probe_cases[i].orbit, probe_cases[i].cycle, count);
            continue;
        }
        ct_cube c;
        probe_identity(&c);
        for (int k = 0; k < count; k++) {
            ct_apply(&c, moves[k]);
        }
        int perm[PROBE_CELLS];
        for (int q = 0; q < PROBE_CELLS; q++) {
            perm[c.facelet[probe_raw(q / 9, (q % 9) / 3, q % 3)]] = q;
        }
        printf("%-6s %-16s len=%2d cycle:", probe_cases[i].orbit, probe_cases[i].cycle, count);
        int moved = 0;
        for (int s = 0; s < PROBE_CELLS; s++) {
            if (perm[s] != s) {
                moved++;
            }
        }
        int start = -1;
        for (int s = 0; s < PROBE_CELLS; s++) {
            if (perm[s] != s) {
                start = s;
                break;
            }
        }
        int cur = start;
        for (int k = 0; k < 3 && cur >= 0; k++) {
            char names[4];
            probe_sticker_faces(cur, names);
            printf(" %d(%s)", cur, names);
            cur = perm[cur];
        }
        char names[4];
        probe_sticker_faces(cur, names);
        printf(" back-to %d(%s)", cur, names);
        printf(" moved=%d\n", moved);
    }
    printf("name cells:");
    const char *names[] = {"URB", "UBL", "ULF", "UFR", "FRU", "FLD", "BRD",
                           "UR",  "UF",  "FD",  "BR",  "BL",  "LU",  "DR"};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        printf(" %s=%d", names[i], probe_name_cell(names[i]));
    }
    printf("\n");
    return 0;
}
