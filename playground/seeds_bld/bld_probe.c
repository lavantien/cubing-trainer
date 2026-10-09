#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/targets.h"
#include "packs/packs.h"

enum {
    PROBE_CAP = 64,
    PROBE_LINE = 512,
    PROBE_MOVES = 32,
    PROBE_URLS = 4,
    PROBE_GAPS = 3,
    PROBE_M2_ROWS = 22,
    PROBE_OK = 0,
    PROBE_FAIL = 1
};

static const char *const probe_urls[PROBE_URLS] = {
    "https://jperm.net/bld",
    "http://www.stefan-pochmann.de/spocc/blindsolving/3x3/old.php",
    "https://www.speedsolving.com/wiki/index.php/Old_Pochmann",
    "https://www.speedsolving.com/wiki/index.php/M2",
};

static const char *const probe_gap_tokens[PROBE_GAPS] = {"U2'", "L2'", "M2'"};

static const char PROBE_MIGRATION[] = "db/migrations/001_init.sql";
static const char PROBE_OP_TSV[] = "db/seeds/packs/bld_oldpoch.tsv";
static const char PROBE_M2_TSV[] = "db/seeds/packs/bld_m2.tsv";

static const int probe_m2_letters[PROBE_M2_ROWS] = {
    'A', 'J', 'V', 'T', 'B', 'L', 'X', 'R', 'D', 'M', 'P', 'O',
    'N', 'E', 'F', 'G', 'H', 'W', 'C', 'I', 'Q', 'S',
};

static int probe_fail = PROBE_OK;
static int probe_gap_hits[PROBE_GAPS];

static void probe_check(int cond, const char *what) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", what);
        probe_fail = PROBE_FAIL;
    }
}

static char *probe_slurp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

static int probe_url_known(const char *url) {
    for (int k = 0; k < PROBE_URLS; k++) {
        if (strcmp(probe_urls[k], url) == 0)
            return 1;
    }
    return 0;
}

static int probe_json_balanced(const char *s) {
    int braces = 0;
    int quotes = 0;
    for (const char *p = s; *p != '\0'; p++) {
        if (*p == '{')
            braces++;
        else if (*p == '}')
            braces--;
        else if (*p == '"')
            quotes++;
    }
    return braces == 0 && quotes % 2 == 0;
}

static int probe_schema(sqlite3 *db) {
    char *sql = probe_slurp(PROBE_MIGRATION);
    if (sql == NULL)
        return 0;
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "FAIL migration exec %s\n", err != NULL ? err : "?");
        sqlite3_free(err);
    }
    free(sql);
    return rc == SQLITE_OK;
}

static int probe_stored_hash(sqlite3 *db, const ct_pack_entry *e, char *out, size_t cap) {
    sqlite3_stmt *st = NULL;
    out[0] = '\0';
    if (sqlite3_prepare_v2(db,
                           "SELECT content_hash FROM event_pack WHERE event=? AND method=?"
                           " AND stage=? AND case_key=?",
                           -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return 0;
    }
    sqlite3_bind_int(st, 1, (int)e->event);
    sqlite3_bind_int(st, 2, (int)e->method);
    sqlite3_bind_int(st, 3, (int)e->stage);
    sqlite3_bind_text(st, 4, e->case_key, -1, SQLITE_STATIC);
    int ok = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char *h = sqlite3_column_text(st, 0);
        if (h != NULL) {
            snprintf(out, cap, "%s", (const char *)h);
            ok = 1;
        }
    }
    sqlite3_finalize(st);
    return ok;
}

static int probe_token_gap(const char *tok) {
    for (int k = 0; k < PROBE_GAPS; k++) {
        if (strcmp(probe_gap_tokens[k], tok) == 0) {
            probe_gap_hits[k]++;
            return 1;
        }
    }
    return 0;
}

static void probe_strip_brackets(char *alg) {
    size_t w = 0;
    for (size_t r = 0; alg[r] != '\0'; r++) {
        if (alg[r] != '(' && alg[r] != ')')
            alg[w++] = alg[r];
    }
    alg[w] = '\0';
}

enum { PROBE_PARSE_GAP = -2 };

static int probe_parse(const char *alg, ct_move *out, int cap) {
    char buf[CT_PACK_ALG_MAX];
    snprintf(buf, sizeof buf, "%s", alg);
    probe_strip_brackets(buf);
    int n = ct_notation_parse(buf, 3, out, cap);
    if (n <= 0) {
        int gap_only = 1;
        char save[CT_PACK_ALG_MAX];
        snprintf(save, sizeof save, "%s", buf);
        char *ctx = NULL;
        for (char *tok = strtok_r(save, " ", &ctx); tok != NULL; tok = strtok_r(NULL, " ", &ctx)) {
            ct_move m;
            if (ct_notation_parse_one(tok, 3, &m) != 0) {
                if (!probe_token_gap(tok)) {
                    char what[PROBE_LINE];
                    snprintf(what, sizeof what, "token %s", tok);
                    probe_check(0, what);
                    gap_only = 0;
                }
            }
        }
        if (gap_only)
            return PROBE_PARSE_GAP;
    }
    return n;
}

static int probe_flat(int face, int row, int col) {
    return face * 9 + row * 3 + col;
}

static int probe_flat_of_cell(int cell) {
    return probe_flat(cell / CT_CUBE_STRIDE / CT_CUBE_STRIDE, (cell % (CT_CUBE_STRIDE * CT_CUBE_STRIDE)) / CT_CUBE_STRIDE,
                       cell % CT_CUBE_STRIDE);
}

static int probe_cell(int face, int row, int col) {
    return face * CT_CUBE_STRIDE * CT_CUBE_STRIDE + row * CT_CUBE_STRIDE + col;
}

static int probe_letter_cell(int orbit, int letter) {
    int face = 0;
    int row = 0;
    int col = 0;
    ct_sticker_cell(orbit, letter, &face, &row, &col);
    return probe_cell(face, row, col);
}

static void probe_identity_facelets(ct_cube *c) {
    ct_cube_init(c, 3);
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++)
                c->facelet[probe_cell(face, row, col)] = (uint8_t)probe_flat(face, row, col);
        }
    }
}

static int probe_apply_alg(ct_cube *c, const char *alg) {
    ct_move moves[PROBE_MOVES];
    int n = probe_parse(alg, moves, PROBE_MOVES);
    if (n != PROBE_PARSE_GAP)
        probe_check(n > 0, alg);
    if (n <= 0)
        return 0;
    for (int k = 0; k < n; k++)
        ct_apply(c, moves[k]);
    return 1;
}

static int probe_apply_inverse(ct_cube *c, const char *alg) {
    ct_move moves[PROBE_MOVES];
    int n = probe_parse(alg, moves, PROBE_MOVES);
    if (n <= 0)
        return 0;
    for (int k = n - 1; k >= 0; k--)
        ct_apply(c, ct_move_inverse(moves[k]));
    return 1;
}

static int probe_find_sticker(const ct_cube *c, int flat_id) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                if (c->facelet[probe_cell(face, row, col)] == (uint8_t)flat_id)
                    return probe_cell(face, row, col);
            }
        }
    }
    return -1;
}

static int probe_restored(const ct_cube *c) {
    for (int face = 0; face < CT_CUBE_FACE_COUNT; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                if (c->facelet[probe_cell(face, row, col)] != (uint8_t)probe_flat(face, row, col))
                    return 0;
            }
        }
    }
    return 1;
}

static void probe_loader_rows(sqlite3 *db, const char *tsv, ct_event ev, ct_method m, ct_stage st,
                              int want, const char *label) {
    char what[PROBE_LINE];
    snprintf(what, sizeof what, "%s seed rows", label);
    probe_check(ct_pack_seed_file(db, tsv, ev, m, st) == want, what);
    snprintf(what, sizeof what, "%s count", label);
    probe_check(ct_pack_count(db, ev, m, st) == want, what);
    snprintf(what, sizeof what, "%s idempotent", label);
    probe_check(ct_pack_seed_file(db, tsv, ev, m, st) == want, what);
    ct_pack_entry entries[PROBE_CAP];
    int n = ct_pack_list(db, ev, m, st, entries, PROBE_CAP);
    snprintf(what, sizeof what, "%s list", label);
    probe_check(n == want, what);
    for (int k = 0; k < n; k++) {
        const ct_pack_entry *e = &entries[k];
        snprintf(what, sizeof what, "%s row %s", label, e->case_key);
        probe_check(e->alg[0] != '\0', what);
        probe_check(e->front_json[0] == '{' && probe_json_balanced(e->front_json), what);
        probe_check(e->source_at[0] != '\0' && strcmp(e->source_at, "2026-10-10") == 0, what);
        probe_check(probe_url_known(e->source_url), what);
        char h1[CT_PACK_HASH_MAX];
        char h2[CT_PACK_HASH_MAX];
        char stored[CT_PACK_HASH_MAX];
        probe_check(ct_pack_content_hash(e, h1) == CT_PACK_OK, what);
        ct_pack_entry again = *e;
        probe_check(ct_pack_content_hash(&again, h2) == CT_PACK_OK, what);
        probe_check(strcmp(h1, h2) == 0, what);
        probe_check(probe_stored_hash(db, e, stored, sizeof stored) && strcmp(stored, h1) == 0,
                    what);
    }
}

static int probe_letter_of_row(const ct_pack_entry *e) {
    const char *key = "\"letter\":\"";
    const char *p = strstr(e->front_json, key);
    if (p == NULL)
        return -1;
    return p[strlen(key)] - 'A';
}

static void probe_op_rows(const ct_pack_entry *entries, int n) {
    int edge_ok = 0;
    int corner_d = 0;
    int corner_f = 0;
    int corner_r = 0;
    const int dfr_d = probe_letter_cell(CT_TARGET_ORBIT_CORNER, 'V' - 'A');
    const int dfr_f = probe_letter_cell(CT_TARGET_ORBIT_CORNER, 'K' - 'A');
    const int dfr_r = probe_letter_cell(CT_TARGET_ORBIT_CORNER, 'P' - 'A');
    for (int k = 0; k < n; k++) {
        const ct_pack_entry *e = &entries[k];
        if (strstr(e->front_json, "\"letter\"") == NULL || strncmp(e->alg, "apply", 5) == 0)
            continue;
        int letter = probe_letter_of_row(e);
        char what[PROBE_LINE];
        snprintf(what, sizeof what, "op letter row %s", e->case_key);
        probe_check(letter >= 0 && letter < CT_TARGET_STICKERS, what);
        int orbit = strstr(e->front_json, "edge") != NULL ? CT_TARGET_ORBIT_EDGE
                                                          : CT_TARGET_ORBIT_CORNER;
        int flat_id = probe_flat_of_cell(probe_letter_cell(orbit, letter));
        ct_cube c;
        probe_identity_facelets(&c);
        if (probe_apply_alg(&c, e->alg) == 0)
            continue;
        int landed = probe_find_sticker(&c, flat_id);
        probe_check(landed >= 0, what);
        if (orbit == CT_TARGET_ORBIT_EDGE) {
            snprintf(what, sizeof what, "edge setup %s lands on face %d row %d col %d",
                     e->case_key, landed / 49, (landed % 49) / 7, landed % 7);
            probe_check(landed == probe_cell(CT_FACE_U, 1, 0), what);
            if (landed == probe_cell(CT_FACE_U, 1, 0))
                edge_ok++;
        } else {
            snprintf(what, sizeof what, "corner setup %s lands on face %d row %d col %d",
                     e->case_key, landed / 49, (landed % 49) / 7, landed % 7);
            probe_check(landed == dfr_d || landed == dfr_f || landed == dfr_r, what);
            if (landed == dfr_d)
                corner_d++;
            else if (landed == dfr_f)
                corner_f++;
            else if (landed == dfr_r)
                corner_r++;
        }
        probe_identity_facelets(&c);
        if (probe_apply_alg(&c, e->alg) == 0)
            continue;
        probe_apply_inverse(&c, e->alg);
        snprintf(what, sizeof what, "setup self inverse %s", e->case_key);
        probe_check(probe_restored(&c), what);
    }
    printf("op edge setups at UL-U=%d corner landings DFR D/F/R=%d/%d/%d\n", edge_ok, corner_d,
           corner_f, corner_r);
}

static void probe_m2_rows(const ct_pack_entry *entries, int n) {
    int strict = 0;
    int relaxed = 0;
    const int buffer_d = probe_letter_cell(CT_TARGET_ORBIT_EDGE, 'U' - 'A');
    const int buffer_f = probe_letter_cell(CT_TARGET_ORBIT_EDGE, 'K' - 'A');
    for (int k = 0; k < n && k < PROBE_M2_ROWS; k++) {
        int letter = probe_m2_letters[k];
        char what[PROBE_LINE];
        snprintf(what, sizeof what, "m2 row %s target %c", entries[k].case_key, (char)letter);
        int target_flat = probe_flat_of_cell(probe_letter_cell(CT_TARGET_ORBIT_EDGE, letter - 'A'));
        ct_cube c;
        probe_identity_facelets(&c);
        if (probe_apply_alg(&c, entries[k].alg) == 0) {
            printf("m2 %c unparsed, skipped\n", (char)letter);
            continue;
        }
        int got_d = c.facelet[buffer_d];
        int got_f = c.facelet[buffer_f];
        if (got_d == (uint8_t)target_flat || got_f == (uint8_t)target_flat) {
            strict++;
            printf("m2 %c sticker-exact buffer=%s\n", (char)letter,
                   got_d == (uint8_t)target_flat ? "DF-D" : "DF-F");
        } else {
            int at = probe_find_sticker(&c, target_flat);
            snprintf(what, sizeof what, "m2 %c target sticker misplaced, now at %d", (char)letter,
                     at);
            probe_check(at >= 0, what);
            if (at >= 0)
                relaxed++;
        }
    }
    printf("m2 strict=%d relaxed-misplaced=%d total=%d\n", strict, relaxed, n);
    probe_check(strict == PROBE_M2_ROWS, "m2 strict count");
}

int main(void) {
    sqlite3 *db = NULL;
    probe_check(sqlite3_open(":memory:", &db) == SQLITE_OK, "open db");
    probe_check(probe_schema(db), "schema");
    probe_loader_rows(db, PROBE_OP_TSV, CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_PLL, 38,
                      "oldpoch");
    probe_loader_rows(db, PROBE_M2_TSV, CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_EP, 22, "m2");
    ct_pack_entry op_rows[PROBE_CAP];
    int n_op =
        ct_pack_list(db, CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_PLL, op_rows, PROBE_CAP);
    probe_check(n_op == 38, "op list");
    probe_op_rows(op_rows, n_op);
    ct_pack_entry m2_rows[PROBE_CAP];
    int n_m2 = ct_pack_list(db, CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_EP, m2_rows, PROBE_CAP);
    probe_check(n_m2 == 22, "m2 list");
    probe_m2_rows(m2_rows, n_m2);
    for (int k = 0; k < n_op; k++) {
        ct_move moves[PROBE_MOVES];
        if (strncmp(op_rows[k].alg, "apply", 5) != 0)
            probe_parse(op_rows[k].alg, moves, PROBE_MOVES);
    }
    for (int k = 0; k < n_m2; k++) {
        ct_move moves[PROBE_MOVES];
        probe_parse(m2_rows[k].alg, moves, PROBE_MOVES);
    }
    sqlite3_close(db);
    if (probe_fail == PROBE_OK) {
        printf("bld probe ok, notation gaps:");
        for (int k = 0; k < PROBE_GAPS; k++)
            if (probe_gap_hits[k] > 0)
                printf(" %s=%d", probe_gap_tokens[k], probe_gap_hits[k]);
        printf("\n");
    }
    return probe_fail;
}
