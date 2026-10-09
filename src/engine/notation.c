#include "engine/notation.h"

#include <string.h>

static const ct_notation_entry ct_notation_table[] = {
    {'U', CT_FACE_U, CT_NOTATION_KIND_FACE},     {'D', CT_FACE_D, CT_NOTATION_KIND_FACE},
    {'L', CT_FACE_L, CT_NOTATION_KIND_FACE},     {'R', CT_FACE_R, CT_NOTATION_KIND_FACE},
    {'F', CT_FACE_F, CT_NOTATION_KIND_FACE},     {'B', CT_FACE_B, CT_NOTATION_KIND_FACE},
    {'x', CT_FACE_R, CT_NOTATION_KIND_ROTATION}, {'y', CT_FACE_U, CT_NOTATION_KIND_ROTATION},
    {'z', CT_FACE_F, CT_NOTATION_KIND_ROTATION}, {'M', CT_FACE_L, CT_NOTATION_KIND_SLICE},
    {'E', CT_FACE_D, CT_NOTATION_KIND_SLICE},    {'S', CT_FACE_F, CT_NOTATION_KIND_SLICE},
};

static const char ct_notation_face_letters[CT_CUBE_FACE_COUNT] = {
    [CT_FACE_U] = 'U', [CT_FACE_L] = 'L', [CT_FACE_F] = 'F',
    [CT_FACE_R] = 'R', [CT_FACE_B] = 'B', [CT_FACE_D] = 'D',
};

static bool ct_notation_valid_n(int n) {
    return n >= 1 && n <= CT_CUBE_MAX_N;
}

static int ct_notation_middle(int n) {
    return (n + 1) / 2;
}

static const ct_notation_entry *ct_notation_find(char c) {
    for (size_t k = 0; k < sizeof ct_notation_table / sizeof ct_notation_table[0]; k++) {
        if (ct_notation_table[k].letter == c)
            return &ct_notation_table[k];
    }
    return NULL;
}

static int ct_notation_rotation_letter(int face) {
    if (face == CT_FACE_R)
        return 'x';
    if (face == CT_FACE_U)
        return 'y';
    if (face == CT_FACE_F)
        return 'z';
    return 0;
}

static int ct_notation_slice_letter(int face) {
    if (face == CT_FACE_L)
        return 'M';
    if (face == CT_FACE_D)
        return 'E';
    if (face == CT_FACE_F)
        return 'S';
    return 0;
}

static int ct_notation_parse_span(const char *s, size_t len, int n, ct_move *out) {
    if (s == NULL || out == NULL || !ct_notation_valid_n(n))
        return CT_NOTATION_E_INPUT;
    size_t i = 0;
    int digit = 0;
    if (len > 0 && s[0] >= '0' && s[0] <= '9') {
        digit = s[0] - '0';
        if (digit < CT_NOTATION_DIGIT_MIN || digit > CT_NOTATION_DIGIT_MAX)
            return CT_NOTATION_E_DIGIT;
        i = 1;
    }
    if (i >= len)
        return CT_NOTATION_E_TOKEN;
    char c = s[i];
    i++;
    const ct_notation_entry *e = ct_notation_find(c);
    int wide = 0;
    if (e == NULL && c >= 'a' && c <= 'z') {
        const ct_notation_entry *upper = ct_notation_find((char)(c - 'a' + 'A'));
        if (upper != NULL && upper->kind == CT_NOTATION_KIND_FACE) {
            e = upper;
            wide = 1;
        }
    }
    if (e == NULL)
        return CT_NOTATION_E_TOKEN;
    if (digit != 0 && e->kind != CT_NOTATION_KIND_FACE)
        return CT_NOTATION_E_TOKEN;
    if (!wide && e->kind == CT_NOTATION_KIND_FACE && i < len && s[i] == 'w') {
        wide = 1;
        i++;
    }
    int quarter;
    if (i < len && s[i] == '\'') {
        i++;
        if (i < len && s[i] == '2') {
            i++;
            quarter = 2;
        } else {
            quarter = 3;
        }
    } else if (i < len && s[i] == '2') {
        i++;
        quarter = 2;
    } else {
        quarter = 1;
    }
    if (i != len)
        return CT_NOTATION_E_SUFFIX;
    int from;
    int to;
    if (e->kind == CT_NOTATION_KIND_ROTATION) {
        from = 1;
        to = n;
    } else if (e->kind == CT_NOTATION_KIND_SLICE) {
        from = ct_notation_middle(n);
        to = from;
    } else if (wide) {
        from = 1;
        to = digit != 0 ? digit : 2;
    } else {
        from = digit != 0 ? digit : 1;
        to = from;
    }
    if (from < 1 || from > to || to > n)
        return CT_NOTATION_E_DEPTH;
    out->face = e->face;
    out->depth_from = from;
    out->depth_to = to;
    out->quarter = quarter;
    return 0;
}

int ct_notation_parse_one(const char *token, int n, ct_move *out) {
    if (token == NULL || out == NULL)
        return CT_NOTATION_E_INPUT;
    return ct_notation_parse_span(token, strlen(token), n, out);
}

int ct_notation_parse(const char *alg, int n, ct_move *out, int cap) {
    if (alg == NULL || !ct_notation_valid_n(n) || cap < 0 || (cap > 0 && out == NULL))
        return CT_NOTATION_E_INPUT;
    int count = 0;
    size_t i = 0;
    while (alg[i] != '\0') {
        while (alg[i] == ' ')
            i++;
        if (alg[i] == '\0')
            break;
        size_t start = i;
        while (alg[i] != '\0' && alg[i] != ' ')
            i++;
        ct_move m;
        int r = ct_notation_parse_span(alg + start, i - start, n, &m);
        if (r < 0)
            return r;
        if (count == cap)
            return CT_NOTATION_E_CAP;
        out[count] = m;
        count++;
    }
    return count;
}

int ct_notation_format_one(const ct_move *m, int n, char *out, unsigned cap) {
    if (m == NULL || out == NULL || !ct_notation_valid_n(n))
        return CT_NOTATION_E_INPUT;
    if (m->face < 0 || m->face >= CT_CUBE_FACE_COUNT)
        return CT_NOTATION_E_MOVE;
    if (m->quarter < 1 || m->quarter > CT_MOVE_QUARTERS - 1)
        return CT_NOTATION_E_MOVE;
    if (m->depth_from < 1 || m->depth_from > m->depth_to || m->depth_to > n)
        return CT_NOTATION_E_MOVE;
    char buf[CT_NOTATION_TOKEN_MAX];
    unsigned len = 0;
    int middle = ct_notation_middle(n);
    int letter = 0;
    if (m->depth_from == 1 && m->depth_to == n)
        letter = ct_notation_rotation_letter(m->face);
    if (letter == 0 && m->depth_from == middle && m->depth_to == middle)
        letter = ct_notation_slice_letter(m->face);
    if (letter != 0) {
        buf[len++] = (char)letter;
    } else if (m->depth_from == 1 && m->depth_to > 1) {
        if (m->depth_to > 2)
            buf[len++] = (char)('0' + m->depth_to);
        buf[len++] = ct_notation_face_letters[m->face];
        buf[len++] = 'w';
    } else if (m->depth_from == m->depth_to) {
        if (m->depth_from > 1)
            buf[len++] = (char)('0' + m->depth_from);
        buf[len++] = ct_notation_face_letters[m->face];
    } else {
        return CT_NOTATION_E_MOVE;
    }
    if (m->quarter == 2)
        buf[len++] = '2';
    else if (m->quarter == 3)
        buf[len++] = '\'';
    if (len + 1 > cap)
        return CT_NOTATION_E_CAP;
    memcpy(out, buf, len);
    out[len] = '\0';
    return (int)len;
}

int ct_notation_format(const ct_move *moves, int count, int n, char *out, unsigned cap) {
    if (out == NULL || !ct_notation_valid_n(n) || count < 0 || (count > 0 && moves == NULL))
        return CT_NOTATION_E_INPUT;
    unsigned total = 0;
    for (int k = 0; k < count; k++) {
        char buf[CT_NOTATION_TOKEN_MAX];
        int r = ct_notation_format_one(&moves[k], n, buf, sizeof buf);
        if (r < 0)
            return r;
        total += (unsigned)r + (k > 0 ? 1u : 0u);
    }
    if (total + 1 > cap)
        return CT_NOTATION_E_CAP;
    unsigned pos = 0;
    for (int k = 0; k < count; k++) {
        if (k > 0)
            out[pos++] = ' ';
        char buf[CT_NOTATION_TOKEN_MAX];
        int r = ct_notation_format_one(&moves[k], n, buf, sizeof buf);
        memcpy(out + pos, buf, (size_t)r);
        pos += (unsigned)r;
    }
    out[pos] = '\0';
    return (int)pos;
}
