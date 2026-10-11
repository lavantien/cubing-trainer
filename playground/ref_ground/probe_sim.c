#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/targets.h"

#define CELL(f, r, c) ((f) * 49 + (r) * 7 + (c))
#define MAXMOVES 64
#define NLABEL 54

static const char face_letter[6] = {'U', 'L', 'F', 'R', 'B', 'D'};
static const int face_axis[6] = {1, 0, 2, 0, 2, 1};
static int letter_face[256];

static int name_to_sticker[2][512];
static char sticker_name[2][24][4];
static int center_cell[6];
static int rot_cell[24][294];
static int rot_count;

static void axis_letter(int axis, int sign, char *out) {
    if (axis == 1)
        *out = sign > 0 ? 'U' : 'D';
    else if (axis == 2)
        *out = sign > 0 ? 'F' : 'B';
    else
        *out = sign > 0 ? 'R' : 'L';
}

static void build_names(void) {
    for (int f = 0; f < 6; f++)
        letter_face[(unsigned char)face_letter[f]] = f;
    static const int priority[3] = {1, 2, 0};
    for (int orbit = 0; orbit < 2; orbit++) {
        for (int s = 0; s < 24; s++) {
            int face, row, col;
            ct_sticker_cell(orbit, s, &face, &row, &col);
            ct_v3 c, n;
            ct_sticker_world(3, face, row, col, &c, &n);
            char buf[4];
            int len = 0;
            int self = face_axis[face];
            int selfval = self == 0 ? c.x : (self == 1 ? c.y : c.z);
            axis_letter(self, selfval > 0 ? 1 : -1, buf + len);
            len++;
            int want = orbit == 0 ? 2 : 1;
            for (int k = 0; k < 3 && want > 0; k++) {
                int axis = priority[k];
                if (axis == self)
                    continue;
                int val = axis == 0 ? c.x : (axis == 1 ? c.y : c.z);
                if (val == 0)
                    continue;
                axis_letter(axis, val > 0 ? 1 : -1, buf + len);
                len++;
                want--;
            }
            buf[len] = '\0';
            strcpy(sticker_name[orbit][s], buf);
            int key;
            if (orbit == 0)
                key = letter_face[(unsigned char)buf[0]] * 36 +
                      letter_face[(unsigned char)buf[1]] * 6 + letter_face[(unsigned char)buf[2]];
            else
                key = 217 + letter_face[(unsigned char)buf[0]] * 6 +
                      letter_face[(unsigned char)buf[1]];
            name_to_sticker[orbit][key] = s + 1;
        }
    }
}

static int lookup(int orbit, const char *name) {
    while (*name == ' ')
        name++;
    int len = 0;
    while (name[len] && name[len] != ' ')
        len++;
    if ((orbit == 0 && len != 3) || (orbit == 1 && len != 2))
        return -1;
    for (int i = 0; i < len; i++)
        if (letter_face[(unsigned char)name[i]] == 0 && name[i] != 'U')
            return -1;
    int key;
    if (orbit == 0)
        key = letter_face[(unsigned char)name[0]] * 36 + letter_face[(unsigned char)name[1]] * 6 +
              letter_face[(unsigned char)name[2]];
    else
        key = 217 + letter_face[(unsigned char)name[0]] * 6 + letter_face[(unsigned char)name[1]];
    return name_to_sticker[orbit][key] - 1;
}

static int sticker_cell(int st) {
    int face, row, col;
    ct_sticker_cell(st < 24 ? 0 : 1, st % 24, &face, &row, &col);
    return CELL(face, row, col);
}

static int sticker_face(int st) { return st % 24 / 4; }

static int piece_of(int st) {
    return st < 24 ? ct_corner_piece_of(st) : ct_edge_piece_of(st - 24);
}

static int axis_of(int st) { return face_axis[sticker_face(st)]; }

static int home_cell(int label) {
    return label < 48 ? sticker_cell(label) : center_cell[label - 48];
}

static void label_solved(ct_cube *c) {
    ct_cube_init(c, 3);
    for (int st = 0; st < 48; st++)
        c->facelet[sticker_cell(st)] = (uint8_t)st;
    for (int f = 0; f < 6; f++) {
        center_cell[f] = CELL(f, 1, 1);
        c->facelet[center_cell[f]] = (uint8_t)(48 + f);
    }
}

static void compute_fwd(const ct_cube *c, int fwd[NLABEL]) {
    int posof[294];
    for (int i = 0; i < 294; i++)
        posof[i] = -1;
    for (int f = 0; f < 6; f++)
        for (int r = 0; r < 3; r++)
            for (int cl = 0; cl < 3; cl++)
                posof[c->facelet[CELL(f, r, cl)]] = CELL(f, r, cl);
    for (int l = 0; l < NLABEL; l++)
        fwd[l] = posof[l];
}

static void build_rotations(void) {
    static const char *seqs[7] = {"", "x", "x2", "x'", "y", "y2", "y'"};
    int seen[24][NLABEL];
    for (int a = 0; a < 7 && rot_count < 24; a++) {
        for (int b = 0; b < 7 && rot_count < 24; b++) {
            char combo[32];
            combo[0] = '\0';
            strcat(combo, seqs[a]);
            if (seqs[b][0]) {
                if (combo[0])
                    strcat(combo, " ");
                strcat(combo, seqs[b]);
            }
            ct_cube c;
            label_solved(&c);
            ct_move mv[MAXMOVES];
            int n = ct_notation_parse(combo, 3, mv, MAXMOVES);
            if (n < 0)
                continue;
            for (int i = 0; i < n; i++)
                ct_apply(&c, mv[i]);
            int fwd[NLABEL];
            compute_fwd(&c, fwd);
            int dup = 0;
            for (int k = 0; k < rot_count; k++)
                if (memcmp(seen[k], fwd, sizeof fwd) == 0)
                    dup = 1;
            if (dup)
                continue;
            memcpy(seen[rot_count], fwd, sizeof fwd);
            for (int l = 0; l < NLABEL; l++)
                rot_cell[rot_count][fwd[l]] = home_cell(l);
            rot_count++;
        }
    }
}

static int centers_fixed(const int fwd[NLABEL]) {
    for (int f = 0; f < 6; f++)
        if (fwd[48 + f] != center_cell[f])
            return 0;
    return 1;
}

static int apply_alg(const char *alg, int fwd[NLABEL], int *qtm) {
    ct_move mv[MAXMOVES];
    int n = ct_notation_parse(alg, 3, mv, MAXMOVES);
    if (n <= 0)
        return n;
    *qtm = 0;
    for (int i = 0; i < n; i++) {
        int rotation = mv[i].depth_from == 1 && mv[i].depth_to == 3;
        if (!rotation)
            *qtm += mv[i].quarter == 2 ? 2 : 1;
    }
    ct_cube c;
    label_solved(&c);
    for (int i = 0; i < n; i++)
        ct_apply(&c, mv[i]);
    compute_fwd(&c, fwd);
    if (centers_fixed(fwd))
        return n;
    for (int r = 0; r < rot_count; r++) {
        int norm[NLABEL];
        for (int l = 0; l < NLABEL; l++)
            norm[l] = rot_cell[r][fwd[l]];
        if (centers_fixed(norm)) {
            memcpy(fwd, norm, sizeof norm);
            return n;
        }
    }
    return n;
}

static int moved_stickers(const int fwd[NLABEL], int orbit, int out[24]) {
    int n = 0;
    for (int s = 0; s < 24; s++) {
        int st = orbit * 24 + s;
        if (fwd[st] != sticker_cell(st))
            out[n++] = st;
    }
    return n;
}

static int distinct_pieces(const int *stickers, int n) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        int seen = 0;
        for (int j = 0; j < i; j++)
            if (piece_of(stickers[j]) == piece_of(stickers[i]))
                seen = 1;
        if (!seen)
            count++;
    }
    return count;
}

static int dest_sticker(const int fwd[NLABEL], int st) {
    int dst_cell = fwd[st];
    int orbit = st < 24 ? 0 : 1;
    for (int t = 0; t < 24; t++)
        if (sticker_cell(orbit * 24 + t) == dst_cell)
            return orbit * 24 + t;
    return -1;
}

static void piece_stickers_of(int piece, int orbit, int out[3], int *n) {
    *n = 0;
    for (int t = 0; t < 24; t++)
        if (piece_of(orbit * 24 + t) == piece)
            out[(*n)++] = orbit * 24 + t;
}

static int prio(int axis) { return axis == 1 ? 0 : (axis == 2 ? 1 : 2); }

static const char *piece_head_name(int piece, int orbit) {
    int st[3];
    int n;
    piece_stickers_of(piece, orbit, st, &n);
    int best = st[0];
    for (int i = 1; i < n; i++)
        if (prio(axis_of(st[i])) < prio(axis_of(best)))
            best = st[i];
    return sticker_name[orbit][best % 24];
}

static int edge_orient_ok(const int fwd[NLABEL], int piece) {
    int st[3];
    int n;
    piece_stickers_of(piece, 1, st, &n);
    int s0 = st[0];
    int s1 = st[1];
    if (prio(axis_of(s1)) < prio(axis_of(s0))) {
        int t = s0;
        s0 = s1;
        s1 = t;
    }
    int d0 = dest_sticker(fwd, s0);
    int d1 = dest_sticker(fwd, s1);
    if (d0 < 0 || d1 < 0)
        return 0;
    return prio(axis_of(d0)) < prio(axis_of(d1));
}

static int twist_dir(const int fwd[NLABEL], int piece) {
    int st[3];
    int n;
    piece_stickers_of(piece, 0, st, &n);
    if (n != 3)
        return 0;
    int anchor = -1;
    for (int i = 0; i < 3; i++)
        if (axis_of(st[i]) == 1)
            anchor = st[i];
    int dst = dest_sticker(fwd, anchor);
    if (dst < 0 || axis_of(dst) == 1)
        return 0;
    ct_v3 v = {0, 0, 0};
    for (int i = 0; i < 3; i++) {
        int d = dest_sticker(fwd, st[i]);
        if (d < 0)
            return 0;
        v.x += ct_frames[sticker_face(d)].normal.x;
        v.y += ct_frames[sticker_face(d)].normal.y;
        v.z += ct_frames[sticker_face(d)].normal.z;
    }
    if (v.y == 0)
        return 0;
    ct_v3 na = {0, v.y > 0 ? 1 : -1, 0};
    ct_v3 nb = ct_frames[sticker_face(dst)].normal;
    ct_v3 cross = {na.y * nb.z - na.z * nb.y, na.z * nb.x - na.x * nb.z, na.x * nb.y - na.y * nb.x};
    return (cross.x * v.x + cross.y * v.y + cross.z * v.z) > 0 ? 1 : -1;
}

static void twist_cycle_str(const int fwd[NLABEL], int piece, char *out, size_t cap) {
    int st[3];
    int n;
    piece_stickers_of(piece, 0, st, &n);
    size_t len = 0;
    for (int i = 0; i < n; i++) {
        int d = dest_sticker(fwd, st[i]);
        len += (size_t)snprintf(out + len, cap - len, "%s>%s ", sticker_name[0][st[i] % 24],
                                d >= 0 ? sticker_name[0][d % 24] : "?");
    }
    if (len > 0 && out[len - 1] == ' ')
        out[len - 1] = '\0';
}

static void strip(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\r' || s[len - 1] == '\n'))
        s[--len] = '\0';
    char *p = s;
    while (*p == ' ')
        p++;
    memmove(s, p, strlen(p) + 1);
}

static int split_char(char *s, char sep, char *out[], int max) {
    int n = 0;
    char *start = s;
    for (char *p = s;; p++) {
        if (*p == sep || *p == '\0') {
            int end = *p == sep;
            *p = '\0';
            if (n < max)
                out[n] = start;
            n++;
            if (!end)
                return n > max ? -1 : n;
            start = p + 1;
        }
    }
}

static void note_add(char *note, size_t cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t len = strlen(note);
    vsnprintf(note + len, cap - len, fmt, ap);
    va_end(ap);
}

static void emit_row(FILE *out, char **f, int moves, int qtm, const char *match, const char *note) {
    fprintf(out, "%s\t%s\t%s\t%s\t%d\t%d\t%s\t%s\n", f[0], f[1], f[2], f[3], moves, qtm, match,
            note);
}

static FILE *assigndump;

static void dump_assignments(const int fwd[NLABEL], int orbit, char **f) {
    if (!assigndump)
        return;
    int moved[24];
    int mn = moved_stickers(fwd, orbit, moved);
    int done[24] = {0};
    for (int i = 0; i < mn; i++) {
        if (done[i])
            continue;
        int p = piece_of(moved[i]);
        int st[3];
        int n;
        piece_stickers_of(p, orbit, st, &n);
        for (int j = i; j < mn; j++)
            if (piece_of(moved[j]) == p)
                done[j] = 1;
        char map[64];
        int pos = 0;
        for (int k = 0; k < n; k++) {
            int d = dest_sticker(fwd, st[k]);
            if (d < 0)
                return;
            pos += snprintf(map + pos, sizeof map - pos, "%s%d>%d", k ? "," : "",
                            axis_of(st[k]), axis_of(d));
        }
        fprintf(assigndump, "%s\t%s\t%s\t%d\t%s\n", f[0], f[1], f[3], orbit, map);
    }
}

static void check_3style(FILE *out, char **f, int fwd[NLABEL], int orbit, int moves, int qtm) {
    char posbuf[128];
    strncpy(posbuf, f[3], sizeof posbuf - 1);
    posbuf[sizeof posbuf - 1] = '\0';
    char *toks[4];
    int ntok = split_char(posbuf, '-', toks, 4);
    char note[512] = "";
    const char *match = "NO";
    int b = -1, t1 = -1, t2 = -1;
    if (ntok != 3) {
        note_add(note, sizeof note, "pos_tokens=%d", ntok);
    } else {
        b = lookup(orbit, toks[0]);
        t1 = lookup(orbit, toks[1]);
        t2 = lookup(orbit, toks[2]);
        if (b < 0 || t1 < 0 || t2 < 0)
            note_add(note, sizeof note, "unknown_token");
    }
    if (b >= 0 && t1 >= 0 && t2 >= 0) {
        int base = orbit * 24;
        int other[24];
        int on = moved_stickers(fwd, 1 - orbit, other);
        int moved[24];
        int mn = moved_stickers(fwd, orbit, moved);
        int dp = distinct_pieces(moved, mn);
        if (on != 0) {
            note_add(note, sizeof note, "other_orbit_moved=%d", on);
        } else if (dp != 3) {
            note_add(note, sizeof note, "moved_pieces=%d", dp);
        } else {
            int pb = piece_of(base + b);
            int p1 = piece_of(base + t1);
            int p2 = piece_of(base + t2);
            int dir_a = fwd[base + b] == sticker_cell(base + t1) &&
                        fwd[base + t1] == sticker_cell(base + t2) &&
                        fwd[base + t2] == sticker_cell(base + b);
            int dir_b = fwd[base + b] == sticker_cell(base + t2) &&
                        fwd[base + t2] == sticker_cell(base + t1) &&
                        fwd[base + t1] == sticker_cell(base + b);
            int extra = 0;
            for (int i = 0; i < mn; i++) {
                int p = piece_of(moved[i]);
                if (p != pb && p != p1 && p != p2)
                    extra = 1;
            }
            if ((dir_a || dir_b) && !extra)
                dump_assignments(fwd, orbit, f);
            if (pb == p1 || p1 == p2 || pb == p2)
                note_add(note, sizeof note, "tokens_share_piece");
            else if (extra)
                note_add(note, sizeof note, "extra_pieces");
            else if (!dir_a && !dir_b)
                note_add(note, sizeof note, "named_stickers_not_cycling");
            else {
                match = dir_a ? "CYCLE_A" : "CYCLE_B";
                int pieces[3] = {pb, p1, p2};
                note_add(note, sizeof note, "defects:");
                int clean = 1;
                for (int k = 0; k < 3; k++) {
                    int bad;
                    if (orbit == 0) {
                        int tw = twist_dir(fwd, pieces[k]);
                        bad = tw != 0;
                        note_add(note, sizeof note, "%s%s=%+d", k ? "," : "",
                                 piece_head_name(pieces[k], 0), tw);
                    } else {
                        bad = !edge_orient_ok(fwd, pieces[k]);
                        note_add(note, sizeof note, "%s%s=%s", k ? "," : "",
                                 piece_head_name(pieces[k], 1), bad ? "flip" : "ok");
                    }
                    if (bad)
                        clean = 0;
                }
                if (clean)
                    note_add(note, sizeof note, " clean");
            }
        }
    }
    emit_row(out, f, moves, qtm, match, note);
}

static void check_flip(FILE *out, char **f, int fwd[NLABEL], int moves, int qtm) {
    char posbuf[128];
    strncpy(posbuf, f[3], sizeof posbuf - 1);
    posbuf[sizeof posbuf - 1] = '\0';
    char *toks[3];
    int ntok = split_char(posbuf, '&', toks, 3);
    char note[512] = "";
    const char *match = "NO";
    int e1 = -1, e2 = -1;
    if (ntok != 2) {
        note_add(note, sizeof note, "pos_tokens=%d", ntok);
    } else {
        strip(toks[0]);
        strip(toks[1]);
        e1 = lookup(1, toks[0]);
        e2 = lookup(1, toks[1]);
        if (e1 < 0 || e2 < 0)
            note_add(note, sizeof note, "unknown_token");
    }
    if (e1 >= 0 && e2 >= 0) {
        int cmoved[24];
        int cn = moved_stickers(fwd, 0, cmoved);
        int moved[24];
        int mn = moved_stickers(fwd, 1, moved);
        int dp = distinct_pieces(moved, mn);
        if (cn != 0) {
            note_add(note, sizeof note, "corners_moved=%d", cn);
        } else if (dp != 2) {
            note_add(note, sizeof note, "edge_pieces_moved=%d", dp);
        } else {
            int p1 = piece_of(24 + e1);
            int p2 = piece_of(24 + e2);
            int s1[3], s2[3];
            int n1, n2;
            piece_stickers_of(p1, 1, s1, &n1);
            piece_stickers_of(p2, 1, s2, &n2);
            int flip1 = fwd[s1[0]] == sticker_cell(s1[1]) && fwd[s1[1]] == sticker_cell(s1[0]);
            int flip2 = fwd[s2[0]] == sticker_cell(s2[1]) && fwd[s2[1]] == sticker_cell(s2[0]);
            int rightpieces = 1;
            for (int i = 0; i < mn; i++)
                if (piece_of(moved[i]) != p1 && piece_of(moved[i]) != p2)
                    rightpieces = 0;
            if (!rightpieces)
                note_add(note, sizeof note, "wrong_pieces");
            else if (!flip1 || !flip2)
                note_add(note, sizeof note, "not_inplace_flip");
            else
                match = "YES";
        }
    }
    emit_row(out, f, moves, qtm, match, note);
}

static void check_twist(FILE *out, char **f, int fwd[NLABEL], int moves, int qtm) {
    char posbuf[128];
    strncpy(posbuf, f[3], sizeof posbuf - 1);
    posbuf[sizeof posbuf - 1] = '\0';
    char *toks[3];
    int ntok = split_char(posbuf, '&', toks, 3);
    char note[512] = "";
    const char *match = "NO";
    int c1 = -1, c2 = -1;
    if (ntok != 2) {
        note_add(note, sizeof note, "pos_tokens=%d", ntok);
    } else {
        strip(toks[0]);
        strip(toks[1]);
        c1 = lookup(0, toks[0]);
        c2 = lookup(0, toks[1]);
        if (c1 < 0 || c2 < 0)
            note_add(note, sizeof note, "unknown_token");
    }
    if (c1 >= 0 && c2 >= 0) {
        int emoved[24];
        int en = moved_stickers(fwd, 1, emoved);
        int moved[24];
        int mn = moved_stickers(fwd, 0, moved);
        int dp = distinct_pieces(moved, mn);
        if (en != 0) {
            note_add(note, sizeof note, "edges_moved=%d", en);
        } else if (dp != 2) {
            note_add(note, sizeof note, "corner_pieces_moved=%d", dp);
        } else {
            int p1 = piece_of(c1);
            int p2 = piece_of(c2);
            char cyc1[48] = "";
            char cyc2[48] = "";
            twist_cycle_str(fwd, p1, cyc1, sizeof cyc1);
            twist_cycle_str(fwd, p2, cyc2, sizeof cyc2);
            int d1 = twist_dir(fwd, p1);
            int d2 = twist_dir(fwd, p2);
            note_add(note, sizeof note, "cyc1=[%s] d1=%+d cyc2=[%s] d2=%+d", cyc1, d1, cyc2, d2);
            int rightpieces = 1;
            for (int i = 0; i < mn; i++)
                if (piece_of(moved[i]) != p1 && piece_of(moved[i]) != p2)
                    rightpieces = 0;
            if (!rightpieces)
                note_add(note, sizeof note, " wrong_pieces");
            else if (d1 == 0 || d2 == 0)
                note_add(note, sizeof note, " not_inplace_3cycle");
            else if (d1 == d2)
                note_add(note, sizeof note, " same_direction");
            else
                match = "YES";
        }
    }
    emit_row(out, f, moves, qtm, match, note);
}

static void check_parity(FILE *out, char **f, int fwd[NLABEL], int moves, int qtm) {
    char posbuf[128];
    strncpy(posbuf, f[3], sizeof posbuf - 1);
    posbuf[sizeof posbuf - 1] = '\0';
    char note[512] = "";
    const char *match = "NO";
    int e1 = -1, e2 = -1, c1 = -1, c2 = -1;
    char *comma = strchr(posbuf, ',');
    if (!comma) {
        note_add(note, sizeof note, "no_comma");
    } else {
        *comma = '\0';
        char *etoks[3];
        int ne = split_char(posbuf, '-', etoks, 3);
        char *ctoks[3];
        int nc = split_char(comma + 1, '-', ctoks, 3);
        if (ne != 2 || nc != 2) {
            note_add(note, sizeof note, "ne=%d nc=%d", ne, nc);
        } else {
            strip(ctoks[1]);
            e1 = lookup(1, etoks[0]);
            e2 = lookup(1, etoks[1]);
            c1 = lookup(0, ctoks[0]);
            c2 = lookup(0, ctoks[1]);
            if (e1 < 0 || e2 < 0 || c1 < 0 || c2 < 0)
                note_add(note, sizeof note, "unknown_token");
        }
    }
    if (e1 >= 0 && e2 >= 0 && c1 >= 0 && c2 >= 0) {
        int emoved[24];
        int en = moved_stickers(fwd, 1, emoved);
        int cmoved[24];
        int cn = moved_stickers(fwd, 0, cmoved);
        int ed = distinct_pieces(emoved, en);
        int cd = distinct_pieces(cmoved, cn);
        int pe1 = piece_of(24 + e1), pe2 = piece_of(24 + e2);
        int pc1 = piece_of(c1), pc2 = piece_of(c2);
        int extra = 0;
        for (int i = 0; i < en; i++)
            if (piece_of(emoved[i]) != pe1 && piece_of(emoved[i]) != pe2)
                extra = 1;
        for (int i = 0; i < cn; i++)
            if (piece_of(cmoved[i]) != pc1 && piece_of(cmoved[i]) != pc2)
                extra = 1;
        if (ed != 2 || cd != 2) {
            note_add(note, sizeof note, "ed=%d cd=%d", ed, cd);
        } else if (pe1 == pe2 || pc1 == pc2) {
            note_add(note, sizeof note, "tokens_share_piece");
        } else if (extra) {
            note_add(note, sizeof note, "extra_pieces");
        } else {
            int swap_e = piece_of(dest_sticker(fwd, 24 + e1)) == pe2;
            int swap_c = piece_of(dest_sticker(fwd, c1)) == pc2;
            if (!swap_e || !swap_c)
                note_add(note, sizeof note, "not_swap se=%d sc=%d", swap_e, swap_c);
            else {
                match = "YES";
                int bad = !edge_orient_ok(fwd, pe1) || !edge_orient_ok(fwd, pe2);
                note_add(note, sizeof note, "defects: %s=%s,%s=%s %s=%+d,%s=%+d",
                         piece_head_name(pe1, 1), edge_orient_ok(fwd, pe1) ? "ok" : "flip",
                         piece_head_name(pe2, 1), edge_orient_ok(fwd, pe2) ? "ok" : "flip",
                         piece_head_name(pc1, 0), twist_dir(fwd, pc1),
                         piece_head_name(pc2, 0), twist_dir(fwd, pc2));
                if (!bad && twist_dir(fwd, pc1) == 0 && twist_dir(fwd, pc2) == 0)
                    note_add(note, sizeof note, " clean");
            }
        }
    }
    emit_row(out, f, moves, qtm, match, note);
}

static void report_ltct(FILE *out, char **f, int fwd[NLABEL], int moves, int qtm) {
    char posbuf[128];
    strncpy(posbuf, f[3], sizeof posbuf - 1);
    posbuf[sizeof posbuf - 1] = '\0';
    char note[2048] = "";
    int e1 = -1, e2 = -1, c1 = -1, c2 = -1, c3 = -1;
    char *comma = strchr(posbuf, ',');
    if (comma) {
        *comma = '\0';
        char *etoks[3];
        int ne = split_char(posbuf, '-', etoks, 3);
        char *cpart = comma + 1;
        char *br = strchr(cpart, '[');
        char *bname = NULL;
        if (br) {
            *br = '\0';
            bname = br + 1;
            char *close = strchr(bname, ']');
            if (close)
                *close = '\0';
        }
        char *ctoks[3];
        int nc = split_char(cpart, '-', ctoks, 3);
        if (ne == 2) {
            e1 = lookup(1, etoks[0]);
            e2 = lookup(1, etoks[1]);
        }
        if (nc == 2) {
            strip(ctoks[1]);
            c1 = lookup(0, ctoks[0]);
            c2 = lookup(0, ctoks[1]);
        }
        if (bname)
            c3 = lookup(0, bname);
    }
    note_add(note, sizeof note, "named e=%s/%s c=%s/%s[%s]", e1 >= 0 ? sticker_name[1][e1] : "?",
             e2 >= 0 ? sticker_name[1][e2] : "?", c1 >= 0 ? sticker_name[0][c1] : "?",
             c2 >= 0 ? sticker_name[0][c2] : "?", c3 >= 0 ? sticker_name[0][c3] : "?");
    int emoved[24];
    int en = moved_stickers(fwd, 1, emoved);
    int cmoved[24];
    int cn = moved_stickers(fwd, 0, cmoved);
    int done[24] = {0};
    note_add(note, sizeof note, " | edges:");
    for (int i = 0; i < en; i++) {
        if (done[i])
            continue;
        int p = piece_of(emoved[i]);
        int st[3];
        int n;
        piece_stickers_of(p, 1, st, &n);
        for (int j = i; j < en; j++)
            if (piece_of(emoved[j]) == p)
                done[j] = 1;
        int d = dest_sticker(fwd, st[0]);
        int flip = !edge_orient_ok(fwd, p);
        note_add(note, sizeof note, " %s->%s%s", sticker_name[1][st[0] % 24],
                 d >= 0 ? sticker_name[1][d % 24] : "?", flip ? "(flip)" : "");
    }
    memset(done, 0, sizeof done);
    note_add(note, sizeof note, " | corners:");
    for (int i = 0; i < cn; i++) {
        if (done[i])
            continue;
        int p = piece_of(cmoved[i]);
        int st[3];
        int n;
        piece_stickers_of(p, 0, st, &n);
        for (int j = i; j < cn; j++)
            if (piece_of(cmoved[j]) == p)
                done[j] = 1;
        int d = dest_sticker(fwd, st[0]);
        char cyc[48] = "";
        twist_cycle_str(fwd, p, cyc, sizeof cyc);
        note_add(note, sizeof note, " %s->%s [%s] tw=%+d", sticker_name[0][st[0] % 24],
                 d >= 0 ? sticker_name[0][d % 24] : "?", cyc, twist_dir(fwd, p));
    }
    emit_row(out, f, moves, qtm, "REPORTED", note);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: probe_sim rows.tsv outdir\n");
        return 1;
    }
    build_names();
    ct_cube c;
    label_solved(&c);
    build_rotations();

    char path[512];
    snprintf(path, sizeof path, "%s/position_map.tsv", argv[2]);
    FILE *pm = fopen(path, "w");
    fprintf(pm, "orbit\tsticker\tletter\tname\tface\trow\tcol\tpiece\n");
    for (int orbit = 0; orbit < 2; orbit++) {
        for (int s = 0; s < 24; s++) {
            int face, row, col;
            ct_sticker_cell(orbit, s, &face, &row, &col);
            fprintf(pm, "%s\t%d\t%c\t%s\t%d\t%d\t%d\t%d\n", orbit == 0 ? "corner" : "edge",
                    orbit * 24 + s, 'A' + s, sticker_name[orbit][s], face, row, col,
                    piece_of(orbit * 24 + s));
        }
    }
    fclose(pm);
    snprintf(path, sizeof path, "%s/position_map_inverse.tsv", argv[2]);
    FILE *pi = fopen(path, "w");
    fprintf(pi, "name\torbit\tsticker\tletter\n");
    for (int orbit = 0; orbit < 2; orbit++)
        for (int s = 0; s < 24; s++)
            fprintf(pi, "%s\t%s\t%d\t%c\n", sticker_name[orbit][s],
                    orbit == 0 ? "corner" : "edge", orbit * 24 + s, 'A' + s);
    fclose(pi);

    ct_target_pair pairs[24 * 24];
    const char *pairfiles[3] = {"pairs_corner0.tsv", "pairs_corner2.tsv", "pairs_edge2.tsv"};
    for (int k = 0; k < 3; k++) {
        snprintf(path, sizeof path, "%s/%s", argv[2], pairfiles[k]);
        FILE *pf = fopen(path, "w");
        fprintf(pf, "t1\tt2\tletters\n");
        int total;
        if (k < 2)
            total = ct_targets_corner(k == 0 ? 0 : 2, pairs, 24 * 24);
        else
            total = ct_targets_edge(2, pairs, 24 * 24);
        int off = k == 2 ? 24 : 0;
        for (int i = 0; i < total; i++)
            fprintf(pf, "%d\t%d\t%c%c\n", pairs[i].t1 + off, pairs[i].t2 + off,
                    'A' + pairs[i].t1, 'A' + pairs[i].t2);
        fclose(pf);
    }

    snprintf(path, sizeof path, "%s/simcheck.tsv", argv[2]);
    FILE *out = fopen(path, "w");
    snprintf(path, sizeof path, "%s/assigndump.tsv", argv[2]);
    assigndump = fopen(path, "w");
    fprintf(assigndump, "solver\tsheet\tposition\torbit\tmap\n");
    fprintf(out, "solver\tsheet\trowid\tposition\tmoves\tqtm\tmatch\tnote\n");
    FILE *in = fopen(argv[1], "r");
    if (!in) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }
    char line[2048];
    while (fgets(line, sizeof line, in)) {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = '\0';
        char *fields[5];
        int nf = 0;
        char *start = line;
        for (char *p = line;; p++) {
            if (*p == '\t' || *p == '\0') {
                int end = *p == '\t';
                *p = '\0';
                if (nf < 5)
                    fields[nf] = start;
                nf++;
                if (!end)
                    break;
                start = p + 1;
            }
        }
        if (nf != 5)
            continue;
        char alg[512];
        strncpy(alg, fields[4], sizeof alg - 1);
        alg[sizeof alg - 1] = '\0';
        strip(alg);
        int fwd[NLABEL];
        int qtm = 0;
        int n = apply_alg(alg, fwd, &qtm);
        if (n <= 0) {
            char note[64];
            snprintf(note, sizeof note, "parse_rc=%d", n);
            emit_row(out, fields, 0, 0, "PARSE_FAIL", note);
            continue;
        }
        const char *sheet = fields[1];
        if (strcmp(sheet, "corner") == 0)
            check_3style(out, fields, fwd, 0, n, qtm);
        else if (strcmp(sheet, "edge") == 0)
            check_3style(out, fields, fwd, 1, n, qtm);
        else if (strcmp(sheet, "flips") == 0)
            check_flip(out, fields, fwd, n, qtm);
        else if (strcmp(sheet, "twists") == 0)
            check_twist(out, fields, fwd, n, qtm);
        else if (strcmp(sheet, "parity") == 0)
            check_parity(out, fields, fwd, n, qtm);
        else if (strcmp(sheet, "ltct") == 0)
            report_ltct(out, fields, fwd, n, qtm);
    }
    fclose(in);
    fclose(out);
    return 0;
}
