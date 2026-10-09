#ifndef CT_APP_H
#define CT_APP_H

#include <stdbool.h>

#include "config/config.h"
#include "storage/db.h"

typedef enum
{
    CT_SCREEN_MENU,
    CT_SCREEN_TRAINERS,
    CT_SCREEN_CARDS,
    CT_SCREEN_REVIEW,
    CT_SCREEN_STATS,
    CT_SCREEN_SETTINGS,
} ct_ui_screen;

typedef struct
{
    ct_config cfg;
    sqlite3 *db;
    ct_ui_screen screen;
    bool running;
    double acc;
    double fixed_dt;
    char db_path[260];
    char migrations_path[260];
} ct_app;

void ct_app_init(ct_app *app);
void ct_app_run(ct_app *app);
void ct_app_shutdown(ct_app *app);

#endif
