#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define CT_MKDIR(p) _mkdir(p)
#define CT_RMDIR(p) _rmdir(p)
#define CT_PID() _getpid()
#else
#include <unistd.h>
#define CT_MKDIR(p) mkdir(p, 0755)
#define CT_RMDIR(p) rmdir(p)
#define CT_PID() getpid()
#endif

#define CT_MUT_REL_MAX 512
#define CT_MUT_SRC_MAX (64u * 1024u * 1024u)
#define CT_MUT_FILE_MAX 1024
#define CT_MUT_SITE_CAP_INIT 1024
#define CT_MUT_TEXT_MAX 40
#define CT_MUT_OBJ_MAX 4096
#define CT_MUT_SUITE_MAX 128
#define CT_MUT_CMD_MAX 32768
#define CT_MUT_ALLOW_MAX 1024
#define CT_MUT_SURV_MAX 1024
#define CT_MUT_ONLY_MAX 256
#define CT_MUT_EXCLUDE_MAX 256
#define CT_MUT_EXCLUDE_NAME_MAX 64

typedef enum { tk_ident, tk_num, tk_str, tk_chr, tk_punct } tk_kind;

typedef struct {
    tk_kind kind;
    size_t off;
    size_t len;
    long line;
} ct_token;

typedef enum {
    op_arith,
    op_cmp,
    op_logic,
    op_int_inc,
    op_int_dec,
    op_bool_flip,
    op_not_del,
    op_ret_lit
} ct_mut_op;

typedef struct {
    size_t file;
    size_t tok;
    ct_mut_op op;
    char text[CT_MUT_TEXT_MAX];
} ct_site;

typedef struct {
    char rel[CT_MUT_REL_MAX];
    char *src;
    size_t len;
    ct_token *toks;
    size_t ntoks;
} ct_mut_file;

typedef struct {
    bool list;
    bool keep;
    const char *cc;
    const char *cflags;
    const char *ldflags;
    const char *ldlibs;
    const char *allowlist;
    size_t only[CT_MUT_ONLY_MAX];
    size_t only_n;
    char exclude[CT_MUT_EXCLUDE_MAX][CT_MUT_EXCLUDE_NAME_MAX];
    size_t exclude_n;
} ct_cfg;

typedef struct {
    size_t id;
    size_t file;
    long line;
    ct_mut_op op;
    char old[32];
    char text[CT_MUT_TEXT_MAX];
} ct_surv;

typedef struct {
    char key[CT_MUT_REL_MAX];
    char just[384];
} ct_allow;

static ct_mut_file g_files[CT_MUT_FILE_MAX];
static size_t g_nfiles;
static ct_site *g_sites;
static size_t g_nsites;
static size_t g_site_cap;
static char (*g_objs)[CT_MUT_REL_MAX];
static size_t g_nobjs;
static const char *g_suites[CT_MUT_SUITE_MAX];
static size_t g_nsuites;
static ct_cfg g_cfg;
static ct_surv g_surv[CT_MUT_SURV_MAX];
static size_t g_nsurv;
static ct_allow g_allow[CT_MUT_ALLOW_MAX];
static size_t g_nallow;

static const char *const ct_punct3[] = {"<<=", ">>=", "..."};
static const char *const ct_punct2[] = {"->",  "++",  "--",  "<<",  ">>",  "<=",  ">=",  "==",  "!=",
                                        "&&",  "||",  "+=",  "-=",  "*=",  "/=",  "%=",  "&=",  "|=",
                                        "^=",  "##"};
static const char *const ct_punct1 = "+-*/%<>=!&|^~?:;,.()[]{}#";
static const char *const ct_op_names[] = {
    "arith", "cmp", "logic", "int+1", "int-1", "bool", "not-del", "ret-lit",
};
static const struct {
    char tok[3];
    const char *rep;
    ct_mut_op op;
} ct_punct_maps[] = {
    {"+", "-", op_arith},   {"-", "+", op_arith},   {"*", "/", op_arith},   {"/", "*", op_arith},
    {"<", "<=", op_cmp},    {">", ">=", op_cmp},    {"<=", "<", op_cmp},    {">=", ">", op_cmp},
    {"==", "!=", op_cmp},   {"!=", "==", op_cmp},   {"&&", "||", op_logic}, {"||", "&&", op_logic},
    {"!", "", op_not_del},
};

static const char *ct_op_name(ct_mut_op op) {
    return ct_op_names[op];
}

static bool ct_is_ident_start(int c) {
    return isalpha(c) || c == '_';
}

static bool ct_is_ident_char(int c) {
    return isalnum(c) || c == '_';
}

static bool ct_is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool ct_ends_with(const char *s, const char *suffix) {
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    return n >= m && strcmp(s + n - m, suffix) == 0;
}

static double ct_now_sec(void) {
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) {
        return (double)time(NULL);
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static char *ct_read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    long n;
    char *buf;
    if (f == NULL || fseek(f, 0, SEEK_END) != 0) {
        if (f != NULL) {
            fclose(f);
        }
        return NULL;
    }
    n = ftell(f);
    if (n < 0 || (unsigned long)n > CT_MUT_SRC_MAX) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    buf = malloc((size_t)n + 1);
    if (buf == NULL || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[n] = '\0';
    fclose(f);
    *len = (size_t)n;
    return buf;
}

static bool ct_push_token(ct_token **toks, size_t *cap, size_t *n, tk_kind kind, size_t off, size_t len,
                          long line) {
    if (*n == *cap) {
        *cap *= 2;
        ct_token *grown = realloc(*toks, *cap * sizeof **toks);
        if (grown == NULL) {
            free(*toks);
            return false;
        }
        *toks = grown;
    }
    (*toks)[*n].kind = kind;
    (*toks)[*n].off = off;
    (*toks)[*n].len = len;
    (*toks)[*n].line = line;
    (*n)++;
    return true;
}

static bool ct_tokenize(const char *src, size_t len, ct_token **out, size_t *nout) {
    size_t cap = 1024;
    ct_token *toks = malloc(cap * sizeof *toks);
    size_t n = 0;
    size_t i = 0;
    long line = 1;
    bool bol = true;
    if (toks == NULL) {
        return false;
    }
#define CT_PUSH(kind_value, start, length)                                                        \
    if (!ct_push_token(&toks, &cap, &n, (kind_value), (start), (length), line)) {                 \
        return false;                                                                             \
    }

    while (i < len) {
        char c = src[i];
        if (c == '\n') {
            line++;
            i++;
            bol = true;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') {
            i++;
            continue;
        }
        if (c == '/' && i + 1 < len && src[i + 1] == '/') {
            while (i < len && src[i] != '\n') {
                i++;
            }
            continue;
        }
        if (c == '/' && i + 1 < len && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < len && !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n') {
                    line++;
                }
                i++;
            }
            i = (i + 1 < len) ? i + 2 : len;
            bol = false;
            continue;
        }
        if (c == '#' && bol) {
            while (i < len) {
                if (src[i] == '\n') {
                    if (i > 0 && src[i - 1] == '\\') {
                        line++;
                        i++;
                        continue;
                    }
                    break;
                }
                i++;
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            char quote = c;
            size_t start = i;
            i++;
            while (i < len && src[i] != quote) {
                if (src[i] == '\\' && i + 1 < len) {
                    i++;
                } else if (src[i] == '\n') {
                    line++;
                }
                i++;
            }
            i = (i < len) ? i + 1 : len;
            CT_PUSH(quote == '"' ? tk_str : tk_chr, start, i - start);
            bol = false;
            continue;
        }
        if (ct_is_ident_start((unsigned char)c)) {
            size_t start = i;
            while (i < len && ct_is_ident_char((unsigned char)src[i])) {
                i++;
            }
            CT_PUSH(tk_ident, start, i - start);
            bol = false;
            continue;
        }
        if (isdigit((unsigned char)c) ||
            (c == '.' && i + 1 < len && isdigit((unsigned char)src[i + 1]))) {
            size_t start = i;
            bool hex = false;
            i++;
            if (src[start] == '0' && i < len && (src[i] == 'x' || src[i] == 'X')) {
                hex = true;
                i++;
            }
            while (i < len) {
                char d = src[i];
                char prev = src[i - 1];
                bool exp_sign = hex ? (prev == 'p' || prev == 'P') : (prev == 'e' || prev == 'E');
                if (isalnum((unsigned char)d) || d == '.' || ((d == '+' || d == '-') && exp_sign)) {
                    i++;
                    continue;
                }
                break;
            }
            CT_PUSH(tk_num, start, i - start);
            bol = false;
            continue;
        }
        {
            size_t matched = 0;
            for (size_t p = 0; p < sizeof ct_punct3 / sizeof *ct_punct3 && matched == 0; p++) {
                if (i + 3 <= len && strncmp(src + i, ct_punct3[p], 3) == 0) {
                    matched = 3;
                }
            }
            for (size_t p = 0; p < sizeof ct_punct2 / sizeof *ct_punct2 && matched == 0; p++) {
                if (i + 2 <= len && strncmp(src + i, ct_punct2[p], 2) == 0) {
                    matched = 2;
                }
            }
            if (matched == 0 && strchr(ct_punct1, c) != NULL) {
                matched = 1;
            }
            if (matched == 0) {
                fprintf(stderr, "mutate: stray character '%c' at line %ld\n", c, line);
                free(toks);
                return false;
            }
            CT_PUSH(tk_punct, i, matched);
            i += matched;
            bol = false;
            continue;
        }
    }
#undef CT_PUSH
    *out = toks;
    *nout = n;
    return true;
}

static bool ct_parse_int(const char *s, size_t n, unsigned long long *val, char *suffix, size_t suffix_cap) {
    char core[80];
    size_t end = n;
    size_t slen = 0;
    if (n == 0 || !isdigit((unsigned char)s[0])) {
        return false;
    }
    while (end > 0 && strchr("uUlL", s[end - 1]) != NULL) {
        end--;
        slen++;
    }
    if (slen + 1 > suffix_cap || end == 0 || end >= sizeof core) {
        return false;
    }
    memcpy(suffix, s + end, slen);
    suffix[slen] = '\0';
    memcpy(core, s, end);
    core[end] = '\0';
    bool hex = end > 2 && core[0] == '0' && (core[1] == 'x' || core[1] == 'X');
    for (size_t j = hex ? 2 : 0; j < end; j++) {
        bool ok = hex ? isxdigit((unsigned char)core[j]) : isdigit((unsigned char)core[j]);
        if (!ok) {
            return false;
        }
    }
    if (hex && end == 2) {
        return false;
    }
    errno = 0;
    char *stop = NULL;
    unsigned long long v = strtoull(core, &stop, 0);
    if (stop == NULL || *stop != '\0' || errno == ERANGE) {
        return false;
    }
    *val = v;
    return true;
}

static void ct_add_site(size_t file, size_t tok, ct_mut_op op, const char *text) {
    if (g_nsites == g_site_cap) {
        size_t cap = g_site_cap == 0 ? CT_MUT_SITE_CAP_INIT : g_site_cap * 2;
        ct_site *grown = realloc(g_sites, cap * sizeof *g_sites);
        if (grown == NULL) {
            fprintf(stderr, "mutate: out of memory for sites\n");
            exit(2);
        }
        g_sites = grown;
        g_site_cap = cap;
    }
    g_sites[g_nsites].file = file;
    g_sites[g_nsites].tok = tok;
    g_sites[g_nsites].op = op;
    snprintf(g_sites[g_nsites].text, CT_MUT_TEXT_MAX, "%s", text);
    g_nsites++;
}

static void ct_enum_sites(size_t f) {
    ct_mut_file *mf = &g_files[f];
    for (size_t t = 0; t < mf->ntoks; t++) {
        ct_token *k = &mf->toks[t];
        const char *p = mf->src + k->off;
        if (k->kind == tk_punct) {
            for (size_t m = 0; m < sizeof ct_punct_maps / sizeof *ct_punct_maps; m++) {
                if (k->len == strlen(ct_punct_maps[m].tok) && strncmp(p, ct_punct_maps[m].tok, k->len) == 0) {
                    ct_add_site(f, t, ct_punct_maps[m].op, ct_punct_maps[m].rep);
                    break;
                }
            }
        } else if (k->kind == tk_ident) {
            if (k->len == 4 && strncmp(p, "true", 4) == 0) {
                ct_add_site(f, t, op_bool_flip, "false");
            } else if (k->len == 5 && strncmp(p, "false", 5) == 0) {
                ct_add_site(f, t, op_bool_flip, "true");
            } else if (k->len == 6 && strncmp(p, "return", 6) == 0 && t + 2 < mf->ntoks) {
                ct_token *v = &mf->toks[t + 1];
                ct_token *semi = &mf->toks[t + 2];
                unsigned long long val = 0;
                char suffix[8];
                if (v->kind == tk_num && semi->kind == tk_punct && semi->len == 1 &&
                    mf->src[semi->off] == ';' &&
                    ct_parse_int(mf->src + v->off, v->len, &val, suffix, sizeof suffix)) {
                    if (val == 0) {
                        ct_add_site(f, t + 1, op_ret_lit, "1");
                    } else if (val == 1) {
                        ct_add_site(f, t + 1, op_ret_lit, "0");
                    }
                }
            }
        } else if (k->kind == tk_num) {
            unsigned long long val = 0;
            char suffix[8];
            if (ct_parse_int(p, k->len, &val, suffix, sizeof suffix)) {
                char buf[CT_MUT_TEXT_MAX];
                if (val < ULLONG_MAX) {
                    snprintf(buf, sizeof buf, "%llu%s", val + 1, suffix);
                    ct_add_site(f, t, op_int_inc, buf);
                }
                if (val == 0) {
                    snprintf(buf, sizeof buf, "-1%s", suffix);
                } else {
                    snprintf(buf, sizeof buf, "%llu%s", val - 1, suffix);
                }
                ct_add_site(f, t, op_int_dec, buf);
            }
        }
    }
}

static void ct_normalize_slashes(char *s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\\') {
            s[i] = '/';
        }
    }
}

static int ct_rel_cmp(const void *a, const void *b) {
    const ct_mut_file *fa = a;
    const ct_mut_file *fb = b;
    return strcmp(fa->rel, fb->rel);
}

static void ct_add_source(const char *rel) {
    if (g_nfiles == CT_MUT_FILE_MAX) {
        fprintf(stderr, "mutate: too many source files, max %d\n", CT_MUT_FILE_MAX);
        exit(2);
    }
    snprintf(g_files[g_nfiles].rel, CT_MUT_REL_MAX, "%s", rel);
    g_files[g_nfiles].src = NULL;
    g_files[g_nfiles].len = 0;
    g_files[g_nfiles].toks = NULL;
    g_files[g_nfiles].ntoks = 0;
    g_nfiles++;
}

static void ct_add_obj(const char *path) {
    if (g_nobjs == CT_MUT_OBJ_MAX) {
        fprintf(stderr, "mutate: too many link objects, max %d\n", CT_MUT_OBJ_MAX);
        exit(2);
    }
    snprintf(g_objs[g_nobjs], CT_MUT_REL_MAX, "%s", path);
    g_nobjs++;
}

static bool ct_obj_kept(const char *obj) {
    char rel[CT_MUT_REL_MAX];
    const char *tobj;
    const char *base;
    struct stat st;
    snprintf(rel, CT_MUT_REL_MAX, "%s", obj);
    base = strrchr(rel, '/');
    base = base == NULL ? rel : base + 1;
    for (size_t i = 0; i < g_cfg.exclude_n; i++) {
        if (strcmp(g_cfg.exclude[i], base) == 0) {
            return false;
        }
    }
    tobj = strstr(rel, "/tobj/");
    if (tobj == NULL) {
        return true;
    }
    char srcp[CT_MUT_REL_MAX];
    snprintf(srcp, sizeof srcp, "%s", tobj + 6);
    size_t n = strlen(srcp);
    if (n > 2 && strcmp(srcp + n - 2, ".o") == 0) {
        srcp[n - 2] = '\0';
        strcat(srcp, ".c");
    }
    if (strcmp(srcp, "src/app/main.c") == 0) {
        return false;
    }
    return stat(srcp, &st) == 0;
}

static void ct_walk(const char *dir, bool sources) {
    DIR *d = opendir(dir);
    if (d == NULL) {
        if (sources) {
            fprintf(stderr, "mutate: cannot open scope directory %s\n", dir);
            exit(2);
        }
        return;
    }
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
            continue;
        }
        char rel[CT_MUT_REL_MAX];
        if (snprintf(rel, sizeof rel, "%s/%s", dir, e->d_name) >= (int)sizeof rel) {
            continue;
        }
        if (ct_is_dir(rel)) {
            ct_walk(rel, sources);
            continue;
        }
        if (sources && ct_ends_with(rel, ".c")) {
            ct_add_source(rel);
        } else if (!sources && ct_ends_with(rel, ".o") && ct_obj_kept(rel)) {
            ct_add_obj(rel);
        }
    }
    closedir(d);
}

static void ct_collect_scope(const char *scope) {
    struct stat st;
    if (stat(scope, &st) != 0) {
        fprintf(stderr, "mutate: scope %s not found\n", scope);
        exit(2);
    }
    if (S_ISDIR(st.st_mode)) {
        ct_walk(scope, true);
        return;
    }
    if (!ct_ends_with(scope, ".c")) {
        fprintf(stderr, "mutate: scope %s is neither directory nor .c file\n", scope);
        exit(2);
    }
    ct_add_source(scope);
}

static void ct_collect_objects(const char *build) {
    char path[CT_MUT_REL_MAX];
    g_objs = malloc(CT_MUT_OBJ_MAX * CT_MUT_REL_MAX);
    if (g_objs == NULL) {
        fprintf(stderr, "mutate: out of memory for objects\n");
        exit(2);
    }
    if (snprintf(path, sizeof path, "%s/tobj", build) < (int)sizeof path) {
        ct_walk(path, false);
    }
    if (snprintf(path, sizeof path, "%s/obj/raygui_impl.o", build) < (int)sizeof path) {
        struct stat st;
        if (stat(path, &st) == 0) {
            ct_add_obj(path);
        }
    }
    if (snprintf(path, sizeof path, "%s/obj/third_party", build) < (int)sizeof path) {
        ct_walk(path, false);
    }
    if (g_nobjs == 0) {
        fprintf(stderr, "mutate: no link objects under %s, run make test first\n", build);
        exit(2);
    }
    qsort(g_objs, g_nobjs, CT_MUT_REL_MAX, ct_rel_cmp);
}

static void ct_orig_object(const char *build, const char *rel, char *out, size_t cap) {
    char stripped[CT_MUT_REL_MAX];
    size_t n = strlen(rel);
    snprintf(stripped, sizeof stripped, "%s", rel);
    if (n > 2 && strcmp(stripped + n - 2, ".c") == 0) {
        stripped[n - 2] = '\0';
    }
    snprintf(out, cap, "%s/tobj/%s.o", build, stripped);
}

static bool ct_link_exe(const char *out_exe, const char *repl_orig, const char *repl_new) {
    char cmd[CT_MUT_CMD_MAX];
    int w = snprintf(cmd, sizeof cmd, "%s %s -o %s", g_cfg.cc, g_cfg.ldflags, out_exe);
    if (w < 0 || (size_t)w >= sizeof cmd) {
        return false;
    }
    for (size_t i = 0; i < g_nobjs; i++) {
        const char *use = g_objs[i];
        if (repl_orig != NULL && strcmp(use, repl_orig) == 0) {
            use = repl_new;
        }
        size_t n = strlen(cmd);
        if (n + strlen(use) + 2 >= sizeof cmd) {
            fprintf(stderr, "mutate: link command overflow\n");
            return false;
        }
        snprintf(cmd + n, sizeof cmd - n, " %s", use);
    }
    size_t n = strlen(cmd);
    snprintf(cmd + n, sizeof cmd - n, " %s", g_cfg.ldlibs);
    return system(cmd) == 0;
}

static void ct_cmd_path(char *dst, size_t cap, const char *src) {
    snprintf(dst, cap, "%s", src);
#ifdef _WIN32
    for (char *p = dst; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\\';
        }
    }
#endif
}

static int ct_run_suite(const char *exe, const char *suite) {
    char cmd[CT_MUT_CMD_MAX];
    char exe_cmd[CT_MUT_REL_MAX];
    ct_cmd_path(exe_cmd, sizeof exe_cmd, exe);
    if (suite == NULL) {
        snprintf(cmd, sizeof cmd, "%s > NUL 2>&1", exe_cmd);
    } else {
        snprintf(cmd, sizeof cmd, "%s %s > NUL 2>&1", exe_cmd, suite);
    }
    return system(cmd);
}

static int ct_run_suites(const char *exe) {
    if (g_nsuites == 0) {
        return ct_run_suite(exe, NULL);
    }
    for (size_t i = 0; i < g_nsuites; i++) {
        int rc = ct_run_suite(exe, g_suites[i]);
        if (rc != 0) {
            fprintf(stderr, "mutate: suite %s exited %d\n", g_suites[i], rc);
            return rc;
        }
    }
    return 0;
}

static bool ct_write_mutant(size_t f, size_t t, const char *text, const char *dst) {
    ct_mut_file *mf = &g_files[f];
    ct_token *k = &mf->toks[t];
    FILE *o = fopen(dst, "wb");
    if (o == NULL) {
        return false;
    }
    bool ok = fwrite(mf->src, 1, k->off, o) == k->off;
    if (ok && text[0] != '\0') {
        ok = fwrite(text, 1, strlen(text), o) == strlen(text);
    }
    if (ok) {
        size_t tail = mf->len - k->off - k->len;
        ok = fwrite(mf->src + k->off + k->len, 1, tail, o) == tail;
    }
    if (fclose(o) != 0) {
        ok = false;
    }
    return ok;
}

static void ct_print_site(const char *status, size_t id, const ct_site *s) {
    ct_mut_file *mf = &g_files[s->file];
    ct_token *k = &mf->toks[s->tok];
    char old[32];
    size_t n = k->len < sizeof old - 1 ? k->len : sizeof old - 1;
    memcpy(old, mf->src + k->off, n);
    old[n] = '\0';
    printf("%-9s m%04zu %s:%ld [%s %s -> %s]\n", status, id, mf->rel, k->line, ct_op_name(s->op), old,
           s->text[0] != '\0' ? s->text : "(deleted)");
}

static bool ct_only_selected(size_t id) {
    if (g_cfg.only_n == 0) {
        return true;
    }
    for (size_t i = 0; i < g_cfg.only_n; i++) {
        if (g_cfg.only[i] == id) {
            return true;
        }
    }
    return false;
}

static bool ct_glob_match(const char *p, const char *s) {
    while (*p != '\0') {
        if (*p == '*') {
            p++;
            if (*p == '\0') {
                return true;
            }
            for (const char *t = s;; t++) {
                if (ct_glob_match(p, t)) {
                    return true;
                }
                if (*t == '\0') {
                    return false;
                }
            }
        }
        if (*s == '\0') {
            return false;
        }
        if (*p == '?' || *p == *s) {
            p++;
            s++;
            continue;
        }
        return false;
    }
    return *s == '\0';
}

static bool ct_allow_matches(const ct_allow *a, const char *rel, long line) {
    size_t n = strlen(a->key);
    size_t idx = n;
    while (idx > 0 && a->key[idx - 1] >= '0' && a->key[idx - 1] <= '9') {
        idx--;
    }
    if (idx > 0 && idx < n && a->key[idx - 1] == ':') {
        char file_part[CT_MUT_REL_MAX];
        long want = strtol(a->key + idx, NULL, 10);
        size_t file_len = idx - 1;
        if (file_len >= sizeof file_part) {
            return false;
        }
        memcpy(file_part, a->key, file_len);
        file_part[file_len] = '\0';
        return ct_glob_match(file_part, rel) && want == line;
    }
    return ct_glob_match(a->key, rel);
}

static void ct_load_allowlist(void) {
    FILE *f = fopen(g_cfg.allowlist, "r");
    if (f == NULL) {
        return;
    }
    char line[1024];
    while (fgets(line, sizeof line, f) != NULL) {
        if (g_nallow == CT_MUT_ALLOW_MAX) {
            fprintf(stderr, "mutate: allowlist capacity %d exceeded\n", CT_MUT_ALLOW_MAX);
            exit(2);
        }
        char *nl = strchr(line, '\n');
        char *tab = strchr(line, '\t');
        if (nl != NULL) {
            *nl = '\0';
        }
        if (tab == NULL || line[0] == '\0' || tab[1] == '\0') {
            continue;
        }
        *tab = '\0';
        snprintf(g_allow[g_nallow].key, CT_MUT_REL_MAX, "%s", line);
        snprintf(g_allow[g_nallow].just, sizeof g_allow[g_nallow].just, "%s", tab + 1);
        g_nallow++;
    }
    fclose(f);
}

static const char *ct_env(const char *name, const char *dflt) {
    const char *v = getenv(name);
    return v != NULL && v[0] != '\0' ? v : dflt;
}

static void ct_split_list(char *buf, char out[][CT_MUT_EXCLUDE_NAME_MAX], size_t *out_n, size_t out_max) {
    char *save = NULL;
    for (char *tok = strtok_r(buf, ", ", &save); tok != NULL; tok = strtok_r(NULL, ", ", &save)) {
        if (*out_n == out_max) {
            fprintf(stderr, "mutate: list capacity exceeded\n");
            exit(2);
        }
        snprintf(out[*out_n], CT_MUT_EXCLUDE_NAME_MAX, "%s", tok);
        (*out_n)++;
    }
}

static void ct_load_cfg(void) {
    char buf[2048];
    g_cfg.cc = ct_env("CT_MUTATE_CC", "clang");
    g_cfg.cflags = ct_env(
        "CT_MUTATE_CFLAGS",
        "--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g "
        "-fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src "
        "-Ithird_party/sqlite");
    g_cfg.ldflags = ct_env("CT_MUTATE_LDFLAGS", "--target=x86_64-w64-mingw32 -fuse-ld=lld -g");
    g_cfg.ldlibs = ct_env("CT_MUTATE_LDLIBS", "-static-libgcc -lopengl32 -lgdi32 -lwinmm");
    g_cfg.allowlist = ct_env("CT_MUTATE_ALLOWLIST", "tests/mutant_allowlist.txt");
    g_cfg.list = ct_env("CT_MUTATE_LIST", "0")[0] == '1';
    g_cfg.keep = ct_env("CT_MUTATE_KEEP", "0")[0] == '1';
    snprintf(buf, sizeof buf, "%s", ct_env("CT_MUTATE_ONLY", ""));
    char *save = NULL;
    for (char *tok = strtok_r(buf, ", ", &save); tok != NULL; tok = strtok_r(NULL, ", ", &save)) {
        if (g_cfg.only_n == CT_MUT_ONLY_MAX) {
            fprintf(stderr, "mutate: too many CT_MUTATE_ONLY ids\n");
            exit(2);
        }
        g_cfg.only[g_cfg.only_n++] = (size_t)strtoul(tok, NULL, 10);
    }
    snprintf(buf, sizeof buf, "%s", ct_env("CT_MUTATE_EXCLUDE", ""));
    ct_split_list(buf, g_cfg.exclude, &g_cfg.exclude_n, CT_MUT_EXCLUDE_MAX);
}

static void ct_rmtree(const char *path) {
    DIR *d = opendir(path);
    if (d != NULL) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
                continue;
            }
            char child[CT_MUT_REL_MAX];
            if (snprintf(child, sizeof child, "%s/%s", path, e->d_name) >= (int)sizeof child) {
                continue;
            }
            if (ct_is_dir(child)) {
                ct_rmtree(child);
            } else {
                remove(child);
            }
        }
        closedir(d);
    }
    CT_RMDIR(path);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: mutate.exe <scope_dir_or_file> <build_dir> [suite filter ...]\n");
        return 2;
    }
    char scope[CT_MUT_REL_MAX];
    snprintf(scope, sizeof scope, "%s", argv[1]);
    ct_normalize_slashes(scope);
    const char *build = argv[2];
    for (int i = 3; i < argc; i++) {
        if (g_nsuites == CT_MUT_SUITE_MAX) {
            fprintf(stderr, "mutate: too many suite filters, max %d\n", CT_MUT_SUITE_MAX);
            return 2;
        }
        g_suites[g_nsuites++] = argv[i];
    }
    ct_load_cfg();
    ct_collect_scope(scope);
    qsort(g_files, g_nfiles, sizeof g_files[0], ct_rel_cmp);
    for (size_t f = 0; f < g_nfiles; f++) {
        ct_mut_file *mf = &g_files[f];
        mf->src = ct_read_file(mf->rel, &mf->len);
        if (mf->src == NULL) {
            fprintf(stderr, "mutate: cannot read %s\n", mf->rel);
            return 2;
        }
        if (!ct_tokenize(mf->src, mf->len, &mf->toks, &mf->ntoks)) {
            fprintf(stderr, "mutate: tokenize failed for %s\n", mf->rel);
            return 2;
        }
        ct_enum_sites(f);
    }
    printf("mutate: scope=%s files=%zu sites=%zu suites=%zu\n", scope, g_nfiles, g_nsites, g_nsuites);
    if (g_cfg.list) {
        for (size_t i = 0; i < g_nsites; i++) {
            ct_print_site("SITE", i + 1, &g_sites[i]);
        }
        printf("mutate: listed %zu sites\n", g_nsites);
        return 0;
    }
    ct_load_allowlist();
    ct_collect_objects(build);
    char mutants_dir[CT_MUT_REL_MAX];
    char base_exe[CT_MUT_REL_MAX];
    char mutant_exe[CT_MUT_REL_MAX];
    long pid = CT_PID();
    snprintf(mutants_dir, sizeof mutants_dir, "%s/mutants/p%ld", build, pid);
    snprintf(base_exe, sizeof base_exe, "%s/bin/ct_mutant_base_p%ld.exe", build, pid);
    snprintf(mutant_exe, sizeof mutant_exe, "%s/bin/ct_mutant_p%ld.exe", build, pid);
    ct_rmtree(mutants_dir);
    char mutants_root[CT_MUT_REL_MAX];
    snprintf(mutants_root, sizeof mutants_root, "%s/mutants", build);
    CT_MKDIR(mutants_root);
    if (CT_MKDIR(mutants_dir) != 0) {
        fprintf(stderr, "mutate: cannot create %s\n", mutants_dir);
        return 2;
    }
    if (!ct_link_exe(base_exe, NULL, NULL)) {
        fprintf(stderr, "mutate: baseline link failed\n");
        return 2;
    }
    ct_run_suite(base_exe, "ct_mutate_warmup_no_such_suite");
    int base_rc = ct_run_suites(base_exe);
    if (base_rc != 0) {
        fprintf(stderr, "mutate: baseline suites failed with exit %d\n", base_rc);
        return 2;
    }
    unsigned long generated = 0;
    unsigned long build_failed = 0;
    unsigned long killed_tests = 0;
    unsigned long survived = 0;
    unsigned long skipped = 0;
    double t0 = ct_now_sec();
    for (size_t i = 0; i < g_nsites; i++) {
        ct_site *s = &g_sites[i];
        size_t id = i + 1;
        if (!ct_only_selected(id)) {
            skipped++;
            continue;
        }
        generated++;
        ct_mut_file *mf = &g_files[s->file];
        char orig_obj[CT_MUT_REL_MAX];
        ct_orig_object(build, mf->rel, orig_obj, sizeof orig_obj);
        struct stat st;
        if (stat(orig_obj, &st) != 0) {
            fprintf(stderr, "mutate: missing object %s for %s\n", orig_obj, mf->rel);
            return 2;
        }
        const char *slash = strrchr(mf->rel, '/');
        const char *base = slash == NULL ? mf->rel : slash + 1;
        char name[CT_MUT_REL_MAX];
        snprintf(name, sizeof name, "%.*s", (int)(strlen(base) - 2), base);
        char mutant_c[CT_MUT_REL_MAX];
        char mutant_o[CT_MUT_REL_MAX];
        snprintf(mutant_c, sizeof mutant_c, "%s/m%04zu_%s.c", mutants_dir, id, name);
        snprintf(mutant_o, sizeof mutant_o, "%s/m%04zu_%s.o", mutants_dir, id, name);
        if (!ct_write_mutant(s->file, s->tok, s->text, mutant_c)) {
            fprintf(stderr, "mutate: cannot write %s\n", mutant_c);
            return 2;
        }
        char cmd[CT_MUT_CMD_MAX];
        char dir_include[CT_MUT_REL_MAX + 8];
        const char *last_slash = strrchr(mf->rel, '/');
        if (last_slash != NULL) {
            snprintf(dir_include, sizeof dir_include, "-I%.*s ", (int)(last_slash - mf->rel),
                     mf->rel);
        } else {
            dir_include[0] = '\0';
        }
        snprintf(cmd, sizeof cmd, "%s %s%s -c %s -o %s", g_cfg.cc, dir_include, g_cfg.cflags,
                 mutant_c, mutant_o);
        if (system(cmd) != 0) {
            build_failed++;
            ct_print_site("KILLED", id, s);
            printf("          build break\n");
            if (!g_cfg.keep) {
                remove(mutant_c);
            }
            continue;
        }
        if (!ct_link_exe(mutant_exe, orig_obj, mutant_o)) {
            build_failed++;
            ct_print_site("KILLED", id, s);
            printf("          link break\n");
            if (!g_cfg.keep) {
                remove(mutant_c);
                remove(mutant_o);
            }
            continue;
        }
        ct_run_suite(mutant_exe, "ct_mutate_warmup_no_such_suite");
        if (ct_run_suites(mutant_exe) != 0) {
            killed_tests++;
            ct_print_site("KILLED", id, s);
        } else {
            survived++;
            ct_print_site("SURVIVED", id, s);
            if (g_nsurv == CT_MUT_SURV_MAX) {
                fprintf(stderr, "mutate: survivor capacity exceeded\n");
                return 2;
            }
            ct_token *k = &mf->toks[s->tok];
            g_surv[g_nsurv].id = id;
            g_surv[g_nsurv].file = s->file;
            g_surv[g_nsurv].line = k->line;
            g_surv[g_nsurv].op = s->op;
            size_t n = k->len < sizeof g_surv[g_nsurv].old - 1 ? k->len : sizeof g_surv[g_nsurv].old - 1;
            memcpy(g_surv[g_nsurv].old, mf->src + k->off, n);
            g_surv[g_nsurv].old[n] = '\0';
            snprintf(g_surv[g_nsurv].text, CT_MUT_TEXT_MAX, "%s", s->text);
            g_nsurv++;
        }
        if (!g_cfg.keep) {
            remove(mutant_c);
            remove(mutant_o);
        }
    }
    unsigned long killed = build_failed + killed_tests;
    unsigned long accepted = 0;
    unsigned long unexplained = 0;
    for (size_t i = 0; i < g_nsurv; i++) {
        ct_surv *v = &g_surv[i];
        const char *just = NULL;
        for (size_t a = 0; a < g_nallow && just == NULL; a++) {
            if (ct_allow_matches(&g_allow[a], g_files[v->file].rel, v->line)) {
                just = g_allow[a].just;
            }
        }
        if (just != NULL) {
            accepted++;
            printf("ACCEPTED  m%04zu %s:%ld [%s %s -> %s] :: %s\n", v->id, g_files[v->file].rel, v->line,
                   ct_op_name(v->op), v->old, v->text[0] != '\0' ? v->text : "(deleted)", just);
        } else {
            unexplained++;
        }
    }
    double wall = ct_now_sec() - t0;
    printf("mutate: generated=%lu build_failed=%lu killed_by_tests=%lu killed=%lu survived=%lu "
           "accepted=%lu unexplained=%lu skipped=%lu\n",
           generated, build_failed, killed_tests, killed, survived, accepted, unexplained, skipped);
    if (generated > 0) {
        printf("mutate: kill rate %lu/%lu = %.1f%%, with accepted equivalents %lu/%lu = %.1f%%\n", killed,
               generated, 100.0 * (double)killed / (double)generated, killed + accepted, generated,
               100.0 * (double)(killed + accepted) / (double)generated);
    }
    printf("mutate: wall %.1fs\n", wall);
    remove(base_exe);
    remove(mutant_exe);
    if (!g_cfg.keep) {
        ct_rmtree(mutants_dir);
    }
    if (unexplained > 0) {
        fprintf(stderr, "mutate: %lu unexplained survivors\n", unexplained);
        return 1;
    }
    return 0;
}
