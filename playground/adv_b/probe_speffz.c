#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/cube.h"
#include "gen/targets.h"

static int failures = 0;

static void check(int cond, const char *what) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    }
}

static const char *corner_derived[24] = {"UBL", "UBR", "UFR", "UFL", "LUB", "LUF", "LDF",
                                         "LDB",  "FUL",  "FUR",  "FDR",  "FDL",  "RUF",  "RUB",
                                         "RDB",  "RDF",  "BUR",  "BUL",  "BDL",  "BDR",  "DFL",
                                         "DFR",  "DBR",  "DBL"};
static const char *edge_derived[24] = {"UB", "UR", "UF", "UL", "LU", "LF", "LD", "LB",
                                       "FU", "FR", "FD", "FL", "RU", "RB", "RD", "RF",
                                       "BU", "BL", "BD", "BR", "DF", "DR", "DB", "DL"};

static int axis_char_to_delta(char c) {
    switch (c) {
    case 'U':
        return 1;
    case 'D':
        return -1;
    case 'R':
        return 2;
    case 'L':
        return -2;
    case 'F':
        return 3;
    case 'B':
        return -3;
    default:
        return 0;
    }
}

static void name_to_lattice(const char *name, int n, ct_v3 *out) {
    int h = n - 1;
    int x = 0;
    int y = 0;
    int z = 0;
    for (const char *p = name; *p; p++) {
        switch (axis_char_to_delta(*p)) {
        case 1:
            y = h;
            break;
        case -1:
            y = -h;
            break;
        case 2:
            x = h;
            break;
        case -2:
            x = -h;
            break;
        case 3:
            z = h;
            break;
        case -3:
            z = -h;
            break;
        default:
            break;
        }
    }
    *out = (ct_v3){x, y, z};
}

static const char *face_names[6] = {"U", "L", "F", "R", "B", "D"};

int main(void) {
    int n = 3;
    int corner_mismatch = 0;
    printf("corner diff (derived vs ct_sticker_cell world position):\n");
    for (int i = 0; i < 24; i++) {
        int face;
        int row;
        int col;
        ct_sticker_cell(CT_TARGET_ORBIT_CORNER, i, &face, &row, &col);
        ct_v3 center;
        ct_v3 normal;
        ct_sticker_world(n, face, row, col, &center, &normal);
        ct_v3 want;
        name_to_lattice(corner_derived[i], n, &want);
        int ok = center.x == want.x && center.y == want.y && center.z == want.z;
        if (!ok) {
            corner_mismatch++;
            printf("  MISMATCH idx=%d letter=%c derived=%s cell=(%s,%d,%d) world=(%d,%d,%d)\n",
                   i, 'A' + i, corner_derived[i], face_names[face], row, col, center.x, center.y,
                   center.z);
        }
        check(face == i / 4, "corner face grouping U L F R B D");
    }
    printf("  corner mismatches=%d\n", corner_mismatch);

    int edge_mismatch = 0;
    printf("edge diff (derived vs ct_sticker_cell world position):\n");
    for (int i = 0; i < 24; i++) {
        int face;
        int row;
        int col;
        ct_sticker_cell(CT_TARGET_ORBIT_EDGE, i, &face, &row, &col);
        ct_v3 center;
        ct_v3 normal;
        ct_sticker_world(n, face, row, col, &center, &normal);
        ct_v3 want;
        name_to_lattice(edge_derived[i], n, &want);
        int ok = center.x == want.x && center.y == want.y && center.z == want.z;
        if (!ok) {
            edge_mismatch++;
            printf("  MISMATCH idx=%d letter=%c derived=%s cell=(%s,%d,%d) world=(%d,%d,%d)\n", i,
                   'A' + i, edge_derived[i], face_names[face], row, col, center.x, center.y,
                   center.z);
        }
        check(face == i / 4, "edge face grouping U L F R B D");
    }
    printf("  edge mismatches=%d\n", edge_mismatch);

    check(corner_derived[0][0] == 'U' && strcmp(corner_derived[0], "UBL") == 0, "corner buffer UBL is letter A index 0");
    check(strcmp(edge_derived[2], "UF") == 0, "edge buffer UF is letter C index 2");

    static const int view_right[6][3] = {{1, 0, 0},  {0, 0, 1},  {1, 0, 0},
                                         {0, 0, -1}, {-1, 0, 0}, {1, 0, 0}};
    static const int view_up[6][3] = {{0, 0, -1}, {0, 1, 0}, {0, 1, 0},
                                      {0, 1, 0},  {0, 1, 0}, {0, 0, 1}};
    for (int face = 0; face < 6; face++) {
        for (int orbit = 0; orbit < 2; orbit++) {
            int base = face * 4;
            double u[4];
            double r[4];
            for (int p = 0; p < 4; p++) {
                int f2;
                int row;
                int col;
                ct_sticker_cell(orbit == 0 ? CT_TARGET_ORBIT_CORNER : CT_TARGET_ORBIT_EDGE, base + p, &f2,
                                &row, &col);
                ct_v3 c;
                ct_v3 nm;
                ct_sticker_world(n, f2, row, col, &c, &nm);
                u[p] = c.x * view_up[face][0] + c.y * view_up[face][1] + c.z * view_up[face][2];
                r[p] = c.x * view_right[face][0] + c.y * view_right[face][1] +
                       c.z * view_right[face][2];
            }
            double cross01 = (r[1] - r[0]) * (u[2] - u[1]) - (u[1] - u[0]) * (r[2] - r[1]);
            char kind = orbit == 0 ? 'c' : 'e';
            int cw = cross01 < 0;
            if (!cw) {
                failures++;
            }
            printf("face %s orbit %c clockwise=%d (p0 u=%+.0f r=%+.0f)\n", face_names[face], kind,
                   cw, u[0], r[0]);
            if (orbit == 0) {
                check(u[0] > 0 && r[0] < 0, "corner p0 is top-left of own view");
                check(u[1] > 0 && r[1] > 0, "corner p1 is top-right");
                check(u[2] < 0 && r[2] > 0, "corner p2 is bottom-right");
                check(u[3] < 0 && r[3] < 0, "corner p3 is bottom-left");
            } else {
                check(u[0] > 0 && r[0] == 0, "edge p0 is top");
                check(u[1] == 0 && r[1] > 0, "edge p1 is right");
                check(u[2] < 0 && r[2] == 0, "edge p2 is bottom");
                check(u[3] == 0 && r[3] < 0, "edge p3 is left");
            }
        }
    }

    FILE *f = fopen("db/seeds/speffz.sql", "r");
    check(f != NULL, "speffz seed readable");
    if (f != NULL) {
        char line[256];
        int seen[2][24];
        memset(seen, 0, sizeof seen);
        while (fgets(line, sizeof line, f) != NULL) {
            int scheme;
            int orbit;
            int sticker;
            char letter;
            if (sscanf(line, "INSERT INTO letter_map(scheme_id, orbit, sticker, letter)"
                             " VALUES(%d, %d, %d, '%c')",
                       &scheme, &orbit, &sticker, &letter) == 4) {
                check(scheme == 1 && orbit >= 0 && orbit < 2 && sticker >= 0 && sticker < 24,
                      "seed row in range");
                seen[orbit][sticker] = letter;
            }
        }
        fclose(f);
        for (int orbit = 0; orbit < 2; orbit++) {
            for (int i = 0; i < 24; i++) {
                check(seen[orbit][i] == 'A' + i, "seed letter is identity A+i");
            }
        }
        printf("seed identity check done\n");
    }

    printf("%s failures=%d\n", failures == 0 ? "ALL-OK" : "HAD-FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
