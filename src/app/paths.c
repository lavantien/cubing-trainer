#include "app/paths.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static int ct_path_is_sep(char c) {
    return c == '/' || c == '\\';
}

static int ct_path_is_drive_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

unsigned ct_path_root_len(const char *path) {
    if (path == NULL || path[0] == '\0')
        return 0;
    if (!ct_path_is_sep(path[0])) {
        if (ct_path_is_drive_letter(path[0]) && path[1] == ':')
            return ct_path_is_sep(path[CT_PATH_ROOT_DRIVE]) ? CT_PATH_ROOT_DRIVE_SEP
                                                            : CT_PATH_ROOT_DRIVE;
        return 0;
    }
    if (!ct_path_is_sep(path[1]))
        return CT_PATH_ROOT_ABS;
    unsigned i = 0;
    while (ct_path_is_sep(path[i]))
        i++;
    for (int comp = 0; comp < CT_PATH_UNC_COMPONENTS; comp++) {
        while (path[i] != '\0' && !ct_path_is_sep(path[i]))
            i++;
        if (path[i] == '\0')
            return i;
        if (comp + 1 < CT_PATH_UNC_COMPONENTS)
            i++;
    }
    return i;
}

static int ct_mkdir_one(const char *path) {
#if defined(_WIN32)
    if (_mkdir(path) == 0 || errno == EEXIST)
        return 0;
#else
    if (mkdir(path, CT_DIR_MODE) == 0 || errno == EEXIST)
        return 0;
#endif
    fprintf(stderr, "cubing-trainer: cannot create directory %s\n", path);
    return -1;
}

int ct_mkdirs(char *path) {
    if (path == NULL || path[0] == '\0') {
        fprintf(stderr, "cubing-trainer: refusing to create an empty directory path\n");
        return -1;
    }
    size_t len = strlen(path);
    size_t root = ct_path_root_len(path);
    for (size_t i = root; i < len; i++) {
        if (!ct_path_is_sep(path[i]))
            continue;
        char saved = path[i];
        path[i] = '\0';
        int rc = ct_mkdir_one(path);
        path[i] = saved;
        if (rc != 0)
            return -1;
    }
    if (ct_path_is_sep(path[len - 1]))
        return 0;
    return ct_mkdir_one(path);
}

int ct_exe_relative(const char *exe_path, const char *rel, char *out, unsigned cap) {
    if (exe_path == NULL || rel == NULL || out == NULL || cap == 0)
        return -1;
    const char *last = NULL;
    for (const char *p = exe_path; *p != '\0'; p++) {
        if (ct_path_is_sep(*p))
            last = p;
    }
    size_t dir_len = last == NULL ? strlen(exe_path) : (size_t)(last - exe_path);
    int written = snprintf(out, cap, "%.*s/%s", (int)dir_len, exe_path, rel);
    if (written < 0 || (unsigned)written >= cap)
        return -1;
    return 0;
}

static int ct_exe_path(char *out, unsigned cap) {
#if defined(_WIN32)
    unsigned long n = GetModuleFileNameA(NULL, out, (unsigned long)cap);
    if (n == 0 || n >= (unsigned long)cap)
        return -1;
    return 0;
#elif defined(__linux__)
    long n = readlink(CT_PROC_SELF_EXE, out, cap - 1);
    if (n <= 0 || (unsigned long)n + 1 >= (unsigned long)cap)
        return -1;
    out[n] = '\0';
    return 0;
#else
    (void)out;
    (void)cap;
    return -1;
#endif
}

int ct_exe_resource_dir(const char *rel, char *out, unsigned cap) {
    if (rel == NULL || out == NULL || cap == 0)
        return -1;
    char exe[CT_APP_EXE_PATH_MAX];
    if (ct_exe_path(exe, (unsigned)sizeof exe) != 0) {
        fprintf(stderr, "cubing-trainer: cannot locate the executable path\n");
        return -1;
    }
    if (ct_exe_relative(exe, rel, out, cap) != 0) {
        fprintf(stderr, "cubing-trainer: resource path %s relative to the executable is too long\n",
                rel);
        return -1;
    }
    return 0;
}

int ct_migrations_dir(char *out, unsigned cap) {
    if (out == NULL || cap == 0)
        return -1;
    const char *override = getenv(CT_MIGRATIONS_ENV);
    if (override != NULL && override[0] != '\0') {
        int written = snprintf(out, cap, "%s", override);
        if (written < 0 || (unsigned)written >= cap) {
            fprintf(stderr, "cubing-trainer: %s is too long\n", CT_MIGRATIONS_ENV);
            return -1;
        }
        return 0;
    }
    return ct_exe_resource_dir(CT_MIGRATIONS_EXE_REL, out, cap);
}

int ct_data_dir(const char *dir_name, char *out, unsigned cap) {
    if (out == NULL || cap == 0)
        return -1;
    out[0] = '\0';
    const char *base = getenv(CT_DATA_DIR_OVERRIDE_ENV);
    if (base == NULL || base[0] == '\0') {
        base = getenv(CT_DATA_DIR_ENV);
    }
    if (base == NULL || base[0] == '\0') {
        fprintf(stderr, "cubing-trainer: %s is not set, cannot locate the data directory\n",
                CT_DATA_DIR_ENV);
        return -1;
    }
    if (dir_name == NULL)
        return -1;
    int written = snprintf(out, cap, "%s/%s", base, dir_name);
    if (written < 0 || (unsigned)written >= cap) {
        fprintf(stderr, "cubing-trainer: data directory path too long\n");
        return -1;
    }
    if (ct_mkdirs(out) != 0) {
        out[0] = '\0';
        return -1;
    }
    return 0;
}
