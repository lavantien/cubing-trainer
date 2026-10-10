#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/app/app.h"
#include "../src/config/config.h"
#include "../src/packs/packs.h"
#include "../src/packs/seed_registry.h"
#include "../src/storage/db.h"
#include "framework.h"

#define CT_PACKSTART_BUF_MAX 512
#define CT_PACKSTART_TMP_DIR "build/test_pack_startup"

#if defined(_WIN32)
#include <direct.h>
#define CT_PACKSTART_RMDIR _rmdir
#define CT_PACKSTART_PUTENV _putenv
#define CT_PACKSTART_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define CT_PACKSTART_RMDIR rmdir
#define CT_PACKSTART_PUTENV putenv
#define CT_PACKSTART_MKDIR(p) mkdir(p, 0755)
#endif

struct ct_packstart_want {
    const char *file;
    ct_event event;
    ct_method method;
    ct_stage stage;
    int rows;
};

enum {
    CT_PACKSTART_L2E_ROWS = 12,
    CT_PACKSTART_REDUCTION_ROWS = 17,
    CT_PACKSTART_PRESENT = 4,
    CT_PACKSTART_TOTAL_ROWS = 714
};

static const struct ct_packstart_want ct_packstart_want_rows[] = {
    {"bigcube_l2e.tsv", CT_EVENT_5X5, CT_METHOD_REDUCTION, CT_STAGE_PARITY,
     CT_PACKSTART_REDUCTION_ROWS},
    {"bigcube_l2e.tsv", CT_EVENT_6X6, CT_METHOD_REDUCTION, CT_STAGE_PARITY,
     CT_PACKSTART_REDUCTION_ROWS},
    {"bigcube_l2e.tsv", CT_EVENT_7X7, CT_METHOD_REDUCTION, CT_STAGE_PARITY,
     CT_PACKSTART_REDUCTION_ROWS},
    {"bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY, 5},
    {"bigcube_parity.tsv", CT_EVENT_5X5, CT_METHOD_REDUCTION, CT_STAGE_PARITY,
     CT_PACKSTART_REDUCTION_ROWS},
    {"bigcube_parity.tsv", CT_EVENT_6X6, CT_METHOD_REDUCTION, CT_STAGE_PARITY,
     CT_PACKSTART_REDUCTION_ROWS},
    {"bigcube_parity.tsv", CT_EVENT_7X7, CT_METHOD_REDUCTION, CT_STAGE_PARITY,
     CT_PACKSTART_REDUCTION_ROWS},
    {"bld_3style_specials.tsv", CT_EVENT_3BLD, CT_METHOD_3STYLE, CT_STAGE_PARITY, 85},
    {"bld_m2.tsv", CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT, 24},
    {"bld_oldpoch.tsv", CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT, 40},
    {"clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT, 14},
    {"fmc_concepts.tsv", CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, 16},
    {"fto_l4t_redux.tsv", CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T, 36},
    {"mbld_concepts.tsv", CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, 14},
    {"megaminx_4lll.tsv", CT_EVENT_MEGAMINX, CT_METHOD_4LLL, CT_STAGE_4LLL, 39},
    {"pyraminx_l4e.tsv", CT_EVENT_PYRAMINX, CT_METHOD_L4E, CT_STAGE_L4E, 35},
    {"skewb_sarah.tsv", CT_EVENT_SKEWB, CT_METHOD_SARAH_INTERMEDIATE, CT_STAGE_SARAH, 15},
    {"sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO, 7},
    {"sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP, 16},
    {"sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE, 169},
    {"sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO, 7},
    {"sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP, 99},
    {"twoby2_cll.tsv", CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL, 42},
};

enum {
    CT_PACKSTART_PACKS = (int)(sizeof ct_packstart_want_rows / sizeof ct_packstart_want_rows[0])
};

static sqlite3 *packstart_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    return db;
}

static int pack_rows_total(sqlite3 *db) {
    sqlite3_stmt *st = NULL;
    int n = -1;
    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM event_pack", -1, &st, NULL) == SQLITE_OK &&
        sqlite3_step(st) == SQLITE_ROW)
        n = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return n;
}

static void rmtree(const char *path) {
    DIR *d = opendir(path);
    if (d != NULL) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;
            char child[CT_PACKSTART_BUF_MAX];
            snprintf(child, sizeof child, "%s/%s", path, e->d_name);
            rmtree(child);
        }
        closedir(d);
    }
    remove(path);
    CT_PACKSTART_RMDIR(path);
}

static void wipe_tmp(void) {
    rmtree(CT_PACKSTART_TMP_DIR);
}

static void set_env(const char *name, const char *value) {
    char entry[CT_PACKSTART_BUF_MAX];
    snprintf(entry, sizeof entry, "%s=%s", name, value);
    CT_ASSERT_EQ(CT_PACKSTART_PUTENV(entry), 0);
}

CT_SUITE(pack_startup_registry) {
    CT_ASSERT_EQ(ct_pack_seed_registry_count(), CT_PACKSTART_PACKS);
    for (int i = 0; i < CT_PACKSTART_PACKS; i++) {
        CT_ASSERT_STR_EQ(ct_pack_seed_registry[i].file, ct_packstart_want_rows[i].file);
        CT_ASSERT_EQ(ct_pack_seed_registry[i].event, ct_packstart_want_rows[i].event);
        CT_ASSERT_EQ(ct_pack_seed_registry[i].method, ct_packstart_want_rows[i].method);
        CT_ASSERT_EQ(ct_pack_seed_registry[i].stage, ct_packstart_want_rows[i].stage);
        char path[CT_PACKSTART_BUF_MAX];
        snprintf(path, sizeof path, "db/seeds/packs/%s", ct_packstart_want_rows[i].file);
        FILE *f = fopen(path, "rb");
        CT_ASSERT(f != NULL);
        if (f != NULL)
            fclose(f);
    }
    for (int i = 0; i < CT_PACKSTART_PACKS; i++) {
        for (int j = i + 1; j < CT_PACKSTART_PACKS; j++) {
            CT_ASSERT(strcmp(ct_pack_seed_registry[i].file, ct_pack_seed_registry[j].file) != 0 ||
                      ct_pack_seed_registry[i].event != ct_pack_seed_registry[j].event ||
                      ct_pack_seed_registry[i].method != ct_pack_seed_registry[j].method ||
                      ct_pack_seed_registry[i].stage != ct_pack_seed_registry[j].stage);
        }
    }
}

CT_SUITE(pack_startup_seeds_all) {
    sqlite3 *db = packstart_db();
    CT_ASSERT_EQ(ct_pack_seed_registered(db, "db/seeds/packs"), 0);
    for (int i = 0; i < CT_PACKSTART_PACKS; i++) {
        CT_ASSERT_EQ(ct_pack_count(db, ct_packstart_want_rows[i].event,
                                   ct_packstart_want_rows[i].method,
                                   ct_packstart_want_rows[i].stage),
                     ct_packstart_want_rows[i].rows);
    }
    CT_ASSERT_EQ(pack_rows_total(db), CT_PACKSTART_TOTAL_ROWS);

    CT_ASSERT_EQ(ct_pack_seed_registered(db, "db/seeds/packs"), 0);
    CT_ASSERT_EQ(pack_rows_total(db), CT_PACKSTART_TOTAL_ROWS);
    ct_db_close(db);
}

CT_SUITE(pack_startup_guards) {
    sqlite3 *db = packstart_db();
    CT_ASSERT_EQ(ct_pack_seed_registered(NULL, "db/seeds/packs"), -1);
    CT_ASSERT_EQ(ct_pack_seed_registered(db, NULL), -1);
    CT_ASSERT_EQ(ct_pack_seed_registered(db, ""), -1);
    CT_ASSERT_EQ(pack_rows_total(db), 0);
    CT_ASSERT_EQ(ct_pack_seed_registered(db, "build/definitely_missing_seeds_dir"), -1);
    CT_ASSERT_EQ(pack_rows_total(db), 0);
    CT_ASSERT_EQ(ct_pack_count(db, CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL), 0);
    ct_db_close(db);
}

static int copy_file_bytes(const char *from, const char *to) {
    FILE *src = fopen(from, "rb");
    if (src == NULL)
        return -1;
    FILE *dst = fopen(to, "wb");
    if (dst == NULL) {
        fclose(src);
        return -1;
    }
    char buf[CT_PACKSTART_BUF_MAX];
    size_t n;
    int ok = 1;
    while ((n = fread(buf, 1, sizeof buf, src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n)
            ok = 0;
    }
    fclose(src);
    fclose(dst);
    return ok ? 0 : -1;
}

CT_SUITE(pack_startup_partial) {
    wipe_tmp();
    char dir[CT_PACKSTART_BUF_MAX];
    snprintf(dir, sizeof dir, "%s/partial", CT_PACKSTART_TMP_DIR);
    CT_ASSERT_EQ(CT_PACKSTART_MKDIR(CT_PACKSTART_TMP_DIR), 0);
    CT_ASSERT_EQ(CT_PACKSTART_MKDIR(dir), 0);
    char from[CT_PACKSTART_BUF_MAX];
    char to[CT_PACKSTART_BUF_MAX];
    snprintf(from, sizeof from, "db/seeds/packs/%s", ct_packstart_want_rows[0].file);
    snprintf(to, sizeof to, "%s/%s", dir, ct_packstart_want_rows[0].file);
    CT_ASSERT_EQ(copy_file_bytes(from, to), 0);
    snprintf(from, sizeof from, "db/seeds/packs/%s", ct_packstart_want_rows[15].file);
    snprintf(to, sizeof to, "%s/%s", dir, ct_packstart_want_rows[15].file);
    CT_ASSERT_EQ(copy_file_bytes(from, to), 0);

    sqlite3 *db = packstart_db();
    CT_ASSERT_EQ(ct_pack_seed_registered(db, dir), CT_PACKSTART_PACKS - CT_PACKSTART_PRESENT);
    CT_ASSERT_EQ(ct_pack_count(db, ct_packstart_want_rows[0].event,
                               ct_packstart_want_rows[0].method, ct_packstart_want_rows[0].stage),
                 CT_PACKSTART_L2E_ROWS);
    CT_ASSERT_EQ(ct_pack_count(db, ct_packstart_want_rows[15].event,
                               ct_packstart_want_rows[15].method, ct_packstart_want_rows[15].stage),
                 ct_packstart_want_rows[15].rows);
    CT_ASSERT_EQ(ct_pack_count(db, ct_packstart_want_rows[7].event,
                               ct_packstart_want_rows[7].method, ct_packstart_want_rows[7].stage),
                 0);
    ct_db_close(db);
    wipe_tmp();
}

CT_SUITE(pack_startup_real_app_init) {
    const char *saved_data = getenv(CT_DATA_DIR_ENV);
    const char *saved_mig = getenv(CT_MIGRATIONS_ENV);
    char restore_data[CT_PACKSTART_BUF_MAX];
    char restore_mig[CT_PACKSTART_BUF_MAX];
    snprintf(restore_data, sizeof restore_data, "%s", saved_data != NULL ? saved_data : "");
    snprintf(restore_mig, sizeof restore_mig, "%s", saved_mig != NULL ? saved_mig : "");

    wipe_tmp();
    char base[CT_PACKSTART_BUF_MAX];
    snprintf(base, sizeof base, "%s/data", CT_PACKSTART_TMP_DIR);
    set_env(CT_DATA_DIR_ENV, base);
    set_env(CT_MIGRATIONS_ENV, "");

    ct_app app;
    ct_app_init(&app);
    CT_ASSERT(app.running);
    CT_ASSERT(app.db != NULL);
    CT_ASSERT_EQ(ct_db_version(app.db), 1);
    CT_ASSERT_EQ(pack_rows_total(app.db), CT_PACKSTART_TOTAL_ROWS);
    CT_ASSERT_EQ(ct_pack_count(app.db, CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CUBESHAPE), 169);
    CT_ASSERT_EQ(ct_pack_count(app.db, CT_EVENT_2X2, CT_METHOD_CLL, CT_STAGE_CLL), 42);
    CT_ASSERT_EQ(ct_pack_count(app.db, CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T), 36);
    CT_ASSERT_EQ(ct_pack_count(app.db, CT_EVENT_7X7, CT_METHOD_REDUCTION, CT_STAGE_PARITY),
                 CT_PACKSTART_REDUCTION_ROWS);

    ct_app_shutdown(&app);
    ct_app_init(&app);
    CT_ASSERT(app.running);
    CT_ASSERT_EQ(pack_rows_total(app.db), CT_PACKSTART_TOTAL_ROWS);
    ct_app_shutdown(&app);

    char db_file[CT_PACKSTART_BUF_MAX];
    snprintf(db_file, sizeof db_file, "%s/cubing-trainer/trainer.db", base);
    FILE *f = fopen(db_file, "rb");
    CT_ASSERT(f != NULL);
    if (f != NULL)
        fclose(f);

    wipe_tmp();
    if (saved_data != NULL)
        set_env(CT_DATA_DIR_ENV, restore_data);
    if (saved_mig != NULL)
        set_env(CT_MIGRATIONS_ENV, restore_mig);
}
