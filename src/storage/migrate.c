#include "db.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"

typedef struct {
    unsigned long version;
    char name[CT_DB_PATH_MAX];
} ct_migration;

static int ct_mig_parse_name(const char *name, unsigned long *version) {
    size_t i = 0;
    while (isdigit((unsigned char)name[i]))
        i++;
    if (i == 0 || name[i] != '_')
        return 0;
    size_t len = strlen(name);
    if (len < 5 || strcmp(name + len - 4, ".sql") != 0)
        return 0;
    if (i >= CT_DB_PATH_MAX)
        return 0;
    char digits[CT_DB_PATH_MAX];
    memcpy(digits, name, i);
    digits[i] = '\0';
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(digits, &end, CT_DECIMAL_BASE);
    if (end == NULL || *end != '\0' || errno == ERANGE || v > (unsigned long)LONG_MAX)
        return -1;
    *version = v;
    return 1;
}

static int ct_mig_cmp(const void *a, const void *b) {
    const ct_migration *ma = a;
    const ct_migration *mb = b;
    if (ma->version != mb->version)
        return ma->version < mb->version ? -1 : 1;
    return strcmp(ma->name, mb->name);
}

static char *ct_mig_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    size_t cap = CT_MIG_COPY_BUF;
    size_t len = 0;
    char *buf = malloc(cap + 1);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }
    for (;;) {
        size_t n = fread(buf + len, 1, cap - len, f);
        if (n == 0)
            break;
        len += n;
        if (len < cap)
            break;
        if (cap > ((size_t)-1) / 2) {
            free(buf);
            fclose(f);
            return NULL;
        }
        cap *= 2;
        char *grown = realloc(buf, cap + 1);
        if (grown == NULL) {
            free(buf);
            fclose(f);
            return NULL;
        }
        buf = grown;
    }
    if (ferror(f)) {
        free(buf);
        fclose(f);
        return NULL;
    }
    if (memchr(buf, '\0', len) != NULL) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[len] = '\0';
    if (fclose(f) != 0) {
        free(buf);
        return NULL;
    }
    return buf;
}

static int ct_mig_copy_file(const char *from, const char *to) {
    FILE *src = fopen(from, "rb");
    if (src == NULL)
        return 0;
    FILE *dst = fopen(to, "wb");
    if (dst == NULL) {
        fclose(src);
        return 0;
    }
    char buf[CT_MIG_COPY_BUF];
    int ok = 1;
    for (;;) {
        size_t n = fread(buf, 1, sizeof buf, src);
        if (n == 0)
            break;
        if (fwrite(buf, 1, n, dst) != n) {
            ok = 0;
            break;
        }
        if (n < sizeof buf)
            break;
    }
    if (ferror(src))
        ok = 0;
    if (fclose(src) != 0)
        ok = 0;
    if (fclose(dst) != 0)
        ok = 0;
    return ok;
}

static int ct_mig_collect(const char *dir, ct_migration **out, size_t *count) {
    *out = NULL;
    *count = 0;
    DIR *d = opendir(dir);
    if (d == NULL)
        return 1;
    ct_migration *migs = NULL;
    size_t n = 0;
    size_t cap = 0;
    struct dirent *ent;
    int rc = 0;
    while ((ent = readdir(d)) != NULL) {
        unsigned long version = 0;
        int parsed = ct_mig_parse_name(ent->d_name, &version);
        if (parsed == 0)
            continue;
        if (parsed < 0) {
            rc = 1;
            break;
        }
        size_t name_len = strlen(ent->d_name);
        if (name_len >= CT_DB_PATH_MAX) {
            rc = 1;
            break;
        }
        if (n == cap) {
            size_t ncap = cap == 0 ? 8 : cap * 2;
            ct_migration *nm = realloc(migs, ncap * sizeof *nm);
            if (nm == NULL) {
                rc = 1;
                break;
            }
            migs = nm;
            cap = ncap;
        }
        memcpy(migs[n].name, ent->d_name, name_len + 1);
        migs[n].version = version;
        n++;
    }
    closedir(d);
    if (rc != 0) {
        free(migs);
        return 1;
    }
    if (n > 1)
        qsort(migs, n, sizeof *migs, ct_mig_cmp);
    *out = migs;
    *count = n;
    return 0;
}

static int ct_mig_backup(sqlite3 *db, const char *dbpath) {
    char *err = NULL;
    sqlite3_exec(db, "PRAGMA wal_checkpoint(TRUNCATE)", NULL, NULL, &err);
    sqlite3_free(err);
    char bak[CT_MIG_PATH_MAX];
    int bak_len = snprintf(bak, sizeof bak, "%s.bak", dbpath);
    if (bak_len < 0 || (size_t)bak_len >= sizeof bak)
        return 1;
    return ct_mig_copy_file(dbpath, bak) ? 0 : 1;
}

int ct_db_migrate(sqlite3 *db, const char *migrations_dir) {
    if (db == NULL || migrations_dir == NULL)
        return 1;
    ct_migration *migs = NULL;
    size_t count = 0;
    if (ct_mig_collect(migrations_dir, &migs, &count) != 0)
        return 1;
    int current = ct_db_version(db);
    if (current < 0) {
        free(migs);
        return 1;
    }
    if (count > 0) {
        if (current > (long)migs[count - 1].version) {
            free(migs);
            return 1;
        }
        int seen_pending = 0;
        for (size_t i = 0; i < count; i++) {
            if ((long)migs[i].version <= current)
                continue;
            unsigned long want =
                seen_pending == 0 ? (unsigned long)current + 1ul : migs[i - 1].version + 1ul;
            if (migs[i].version != want) {
                free(migs);
                return 1;
            }
            seen_pending = 1;
        }
    }
    int pending = 0;
    for (size_t i = 0; i < count; i++) {
        if ((long)migs[i].version > current) {
            pending = 1;
            break;
        }
    }
    int rc = 0;
    if (pending) {
        const char *dbpath = sqlite3_db_filename(db, "main");
        if (dbpath != NULL && dbpath[0] != '\0' && current > 0) {
            rc = ct_mig_backup(db, dbpath);
        }
    }
    for (size_t i = 0; i < count && rc == 0; i++) {
        if ((long)migs[i].version <= current)
            continue;
        char path[CT_MIG_PATH_MAX];
        int path_len = snprintf(path, sizeof path, "%s/%s", migrations_dir, migs[i].name);
        if (path_len < 0 || (size_t)path_len >= sizeof path) {
            rc = 1;
            break;
        }
        char *sql = ct_mig_read_file(path);
        if (sql == NULL) {
            rc = 1;
            break;
        }
        char pragma[CT_MIG_PRAGMA_MAX];
        int pragma_len =
            snprintf(pragma, sizeof pragma, "PRAGMA user_version = %lu", migs[i].version);
        if (pragma_len < 0 || (size_t)pragma_len >= sizeof pragma) {
            free(sql);
            rc = 1;
            break;
        }
        char *err = NULL;
        int step = sqlite3_exec(db, "BEGIN", NULL, NULL, &err);
        if (step == SQLITE_OK)
            step = sqlite3_exec(db, sql, NULL, NULL, &err);
        if (step == SQLITE_OK)
            step = sqlite3_exec(db, pragma, NULL, NULL, &err);
        if (step == SQLITE_OK)
            step = sqlite3_exec(db, "COMMIT", NULL, NULL, &err);
        if (step != SQLITE_OK) {
            sqlite3_free(err);
            err = NULL;
            sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
            rc = 1;
        }
        sqlite3_free(err);
        free(sql);
    }
    free(migs);
    if (rc != 0 || !ct_db_schema_ok(db))
        return 1;
    return 0;
}
