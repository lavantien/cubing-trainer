#ifndef CT_APP_H
#define CT_APP_H

#include <stdbool.h>

#include "config/config.h"
#include "storage/db.h"

typedef struct {
    ct_config cfg;
    sqlite3 *db;
    ct_ui_screen screen;
    bool running;
    double acc;
    double fixed_dt;
    char db_path[CT_DB_PATH_MAX];
    char migrations_path[CT_DB_PATH_MAX];
} ct_app;

void ct_app_init(ct_app *app);
void ct_app_run(ct_app *app);
void ct_app_shutdown(ct_app *app);

#endif
