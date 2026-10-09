#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/app/paths.h"
#include "../src/config/config.h"
#include "framework.h"

#define CT_PATH_TEST_DIR "build/test_paths"
#define CT_PATH_BUF_MAX 512

#if defined(_WIN32)
#include <direct.h>
#define CT_PATH_RMDIR _rmdir
#define CT_PATH_PUTENV _putenv
#else
#include <unistd.h>
#define CT_PATH_RMDIR rmdir
#define CT_PATH_PUTENV putenv
#endif

static bool dir_exists(const char *path) {
    DIR *d = opendir(path);
    if (d == NULL)
        return false;
    closedir(d);
    return true;
}

static void rmtree(const char *path) {
    DIR *d = opendir(path);
    if (d != NULL) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;
            char child[CT_PATH_BUF_MAX];
            snprintf(child, sizeof child, "%s/%s", path, e->d_name);
            rmtree(child);
        }
        closedir(d);
    }
    remove(path);
    CT_PATH_RMDIR(path);
}

static void wipe_test_dir(void) {
    rmtree(CT_PATH_TEST_DIR);
}

static void set_env(const char *name, const char *value) {
    char entry[CT_PATH_BUF_MAX];
    snprintf(entry, sizeof entry, "%s=%s", name, value);
    CT_ASSERT_EQ(CT_PATH_PUTENV(entry), 0);
}

CT_SUITE(path_root_len) {
    CT_ASSERT_EQ(ct_path_root_len(NULL), 0u);
    CT_ASSERT_EQ(ct_path_root_len(""), 0u);
    CT_ASSERT_EQ(ct_path_root_len("a/b/c"), 0u);
    CT_ASSERT_EQ(ct_path_root_len("./a/b"), 0u);
    CT_ASSERT_EQ(ct_path_root_len("../a"), 0u);
    CT_ASSERT_EQ(ct_path_root_len("a"), 0u);
    CT_ASSERT_EQ(ct_path_root_len("C:"), 2u);
    CT_ASSERT_EQ(ct_path_root_len("C:a"), 2u);
    CT_ASSERT_EQ(ct_path_root_len("C:/a/b"), 3u);
    CT_ASSERT_EQ(ct_path_root_len("C:\\a\\b"), 3u);
    CT_ASSERT_EQ(ct_path_root_len("C:/"), 3u);
    CT_ASSERT_EQ(ct_path_root_len("z:/x"), 3u);
    CT_ASSERT_EQ(ct_path_root_len("/a/b"), 1u);
    CT_ASSERT_EQ(ct_path_root_len("/"), 1u);
    CT_ASSERT_EQ(ct_path_root_len("//server/share/dir"), strlen("//server/share"));
    CT_ASSERT_EQ(ct_path_root_len("\\\\server\\share\\dir"), strlen("//server/share"));
    CT_ASSERT_EQ(ct_path_root_len("//server/share"), strlen("//server/share"));
    CT_ASSERT_EQ(ct_path_root_len("//server"), strlen("//server"));
    CT_ASSERT_EQ(ct_path_root_len("//"), 2u);
    CT_ASSERT_EQ(ct_path_root_len("1:/x"), 0u);
    CT_ASSERT_EQ(ct_path_root_len(":/x"), 0u);
}

CT_SUITE(path_mkdirs_relative) {
    wipe_test_dir();
    char deep[CT_PATH_BUF_MAX];
    snprintf(deep, sizeof deep, "%s/rel/a/b/c", CT_PATH_TEST_DIR);
    CT_ASSERT_EQ(ct_mkdirs(deep), 0);
    CT_ASSERT(dir_exists(deep));
    CT_ASSERT(dir_exists(CT_PATH_TEST_DIR));
    CT_ASSERT_EQ(ct_mkdirs(deep), 0);
    CT_ASSERT_EQ(ct_mkdirs(""), -1);
    wipe_test_dir();
}

CT_SUITE(path_mkdirs_drive) {
    wipe_test_dir();
    char cwd[CT_PATH_BUF_MAX];
    CT_ASSERT(getcwd(cwd, sizeof cwd) != NULL);
    CT_ASSERT(cwd[1] == ':');
    char deep[CT_PATH_BUF_MAX];
    snprintf(deep, sizeof deep, "%s\\%s\\drv\\x", cwd, CT_PATH_TEST_DIR);
    CT_ASSERT_EQ(ct_mkdirs(deep), 0);
    CT_ASSERT(dir_exists(deep));
    char fwd[CT_PATH_BUF_MAX];
    snprintf(fwd, sizeof fwd, "%s/%s/drv/y", cwd, CT_PATH_TEST_DIR);
    CT_ASSERT_EQ(ct_mkdirs(fwd), 0);
    CT_ASSERT(dir_exists(fwd));
    wipe_test_dir();
}

CT_SUITE(path_exe_relative) {
    char out[CT_PATH_BUF_MAX];
    CT_ASSERT_EQ(
        ct_exe_relative("C:/r/build/bin/app.exe", "../../db/migrations", out, (unsigned)sizeof out),
        0);
    CT_ASSERT_STR_EQ(out, "C:/r/build/bin/../../db/migrations");
    CT_ASSERT_EQ(ct_exe_relative("C:\\r\\build\\bin\\app.exe", "db", out, (unsigned)sizeof out), 0);
    CT_ASSERT_STR_EQ(out, "C:\\r\\build\\bin/db");
    CT_ASSERT_EQ(ct_exe_relative("bin/app.exe", "../x", out, (unsigned)sizeof out), 0);
    CT_ASSERT_STR_EQ(out, "bin/../x");
    CT_ASSERT_EQ(ct_exe_relative(NULL, "x", out, (unsigned)sizeof out), -1);
    CT_ASSERT_EQ(ct_exe_relative("a.exe", NULL, out, (unsigned)sizeof out), -1);
    CT_ASSERT_EQ(ct_exe_relative("a.exe", "x", NULL, (unsigned)sizeof out), -1);
    CT_ASSERT_EQ(ct_exe_relative("a.exe", "x", out, 0u), -1);
    CT_ASSERT_EQ(ct_exe_relative("C:/very/long/path/app.exe", "../../db/migrations", out, 4u), -1);
}

CT_SUITE(path_migrations_dir_override) {
    char out[CT_PATH_BUF_MAX];
    set_env(CT_MIGRATIONS_ENV, "C:/somewhere/migs");
    CT_ASSERT_EQ(ct_migrations_dir(out, (unsigned)sizeof out), 0);
    CT_ASSERT_STR_EQ(out, "C:/somewhere/migs");
    set_env(CT_MIGRATIONS_ENV, "");
    CT_ASSERT_EQ(ct_migrations_dir(out, (unsigned)sizeof out), 0);
    CT_ASSERT(strstr(out, "db/migrations") != NULL);
    CT_ASSERT(dir_exists(out));
    CT_ASSERT_EQ(ct_migrations_dir(out, 8u), -1);
    set_env(CT_MIGRATIONS_ENV, "");
}

CT_SUITE(path_data_dir_env) {
    char out[CT_PATH_BUF_MAX];
    char base[CT_PATH_BUF_MAX];
    char restore[CT_PATH_BUF_MAX];
    const char *original = getenv(CT_DATA_DIR_ENV);
    snprintf(restore, sizeof restore, "%s", original != NULL ? original : "");
    char empty_entry[CT_PATH_BUF_MAX];
    snprintf(empty_entry, sizeof empty_entry, "%s=", CT_DATA_DIR_ENV);
    CT_ASSERT_EQ(CT_PATH_PUTENV(empty_entry), 0);
    CT_ASSERT(getenv(CT_DATA_DIR_ENV) == NULL || getenv(CT_DATA_DIR_ENV)[0] == '\0');
    CT_ASSERT_EQ(ct_data_dir("cubing-trainer", out, (unsigned)sizeof out), -1);
    CT_ASSERT(out[0] == '\0');

    wipe_test_dir();
    snprintf(base, sizeof base, "%s/data", CT_PATH_TEST_DIR);
    set_env(CT_DATA_DIR_ENV, base);
    CT_ASSERT_EQ(ct_data_dir("cubing-trainer", out, (unsigned)sizeof out), 0);
    char want[CT_PATH_BUF_MAX];
    snprintf(want, sizeof want, "%s/cubing-trainer", base);
    CT_ASSERT_STR_EQ(out, want);
    CT_ASSERT(dir_exists(want));
    CT_ASSERT_EQ(ct_data_dir("custom", out, (unsigned)sizeof out), 0);
    snprintf(want, sizeof want, "%s/custom", base);
    CT_ASSERT_STR_EQ(out, want);
    CT_ASSERT(dir_exists(want));
    CT_ASSERT_EQ(ct_data_dir("cubing-trainer", out, 8u), -1);
    CT_ASSERT_EQ(ct_data_dir(NULL, out, (unsigned)sizeof out), -1);
    wipe_test_dir();
    if (original != NULL)
        set_env(CT_DATA_DIR_ENV, restore);
    else
        CT_ASSERT_EQ(CT_PATH_PUTENV(empty_entry), 0);
}
