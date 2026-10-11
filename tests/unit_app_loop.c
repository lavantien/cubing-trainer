#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define CT_APPLOOP_PUTENV _putenv
#else
#define CT_APPLOOP_PUTENV putenv
#endif

#include "../src/app/app.h"
#include "../src/config/config.h"
#include "framework.h"

#define CT_APPLOOP_BUF_MAX 1024

CT_SUITE(app_init_failures) {
    const char *saved_data = getenv(CT_DATA_DIR_ENV);
    const char *saved_mig = getenv(CT_MIGRATIONS_ENV);
    char restore_data[CT_APPLOOP_BUF_MAX];
    char restore_mig[CT_APPLOOP_BUF_MAX];
    snprintf(restore_data, sizeof restore_data, "%s", saved_data != NULL ? saved_data : "");
    snprintf(restore_mig, sizeof restore_mig, "%s", saved_mig != NULL ? saved_mig : "");

    char empty_data[64];
    snprintf(empty_data, sizeof empty_data, "%s=", CT_DATA_DIR_ENV);
    char empty_mig[64];
    snprintf(empty_mig, sizeof empty_mig, "%s=", CT_MIGRATIONS_ENV);
    char long_mig[2048];
    snprintf(long_mig, sizeof long_mig, "%s=", CT_MIGRATIONS_ENV);
    memset(long_mig + strlen(long_mig), 'm', sizeof long_mig - strlen(long_mig) - 1);
    long_mig[sizeof long_mig - 1] = '\0';

    CT_ASSERT_EQ(CT_APPLOOP_PUTENV(empty_data), 0);
    CT_ASSERT_EQ(CT_APPLOOP_PUTENV(empty_mig), 0);

    ct_app app;
    memset(&app, 0xAA, sizeof app);
    ct_app_init(&app);
    CT_ASSERT(!app.running);
    CT_ASSERT(app.db == NULL);
    CT_ASSERT_EQ(app.db_path[0], '\0');
    ct_app_shutdown(&app);

    char base[CT_APPLOOP_BUF_MAX];
    snprintf(base, sizeof base, "%s=", CT_DATA_DIR_ENV);
    snprintf(base + strlen(base), sizeof base - strlen(base), "%s", "build/test_app_loop");
    CT_ASSERT_EQ(CT_APPLOOP_PUTENV(base), 0);
    CT_ASSERT_EQ(CT_APPLOOP_PUTENV(long_mig), 0);

    memset(&app, 0xAA, sizeof app);
    ct_app_init(&app);
    CT_ASSERT(!app.running);
    CT_ASSERT(app.db == NULL);
    CT_ASSERT_EQ(app.migrations_path[0], '\0');
    ct_app_shutdown(&app);

    CT_ASSERT_EQ(CT_APPLOOP_PUTENV(empty_mig), 0);
    if (saved_data != NULL) {
        char back[CT_APPLOOP_BUF_MAX];
        snprintf(back, sizeof back, "%s=%s", CT_DATA_DIR_ENV, restore_data);
        CT_ASSERT_EQ(CT_APPLOOP_PUTENV(back), 0);
    } else {
        CT_ASSERT_EQ(CT_APPLOOP_PUTENV(empty_data), 0);
    }
    if (saved_mig != NULL) {
        char back[CT_APPLOOP_BUF_MAX];
        snprintf(back, sizeof back, "%s=%s", CT_MIGRATIONS_ENV, restore_mig);
        CT_ASSERT_EQ(CT_APPLOOP_PUTENV(back), 0);
    } else {
        CT_ASSERT_EQ(CT_APPLOOP_PUTENV(empty_mig), 0);
    }
}
