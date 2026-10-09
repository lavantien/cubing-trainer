#include <stdio.h>
#include <string.h>

#include "app/app.h"
#include "app/paths.h"
#include "raylib.h"
#include "ui/ui.h"

void ct_app_init(ct_app *app) {
    app->db = NULL;
    app->cfg = ct_config_defaults();
    app->screen = CT_SCREEN_MENU;
    app->running = false;
    app->acc = 0.0;
    app->fixed_dt = 1.0 / app->cfg.window.target_fps;
    if (ct_data_dir(app->cfg.db.dir_name, app->db_path, (unsigned)sizeof app->db_path) != 0) {
        app->db_path[0] = '\0';
        return;
    }
    size_t dir_len = strlen(app->db_path);
    int name_written = snprintf(app->db_path + dir_len, sizeof app->db_path - dir_len, "/%s",
                                app->cfg.db.filename);
    if (name_written < 0 || (size_t)name_written >= sizeof app->db_path - dir_len) {
        fprintf(stderr, "cubing-trainer: database path too long\n");
        app->db_path[0] = '\0';
        return;
    }
    if (ct_migrations_dir(app->migrations_path, (unsigned)sizeof app->migrations_path) != 0) {
        app->migrations_path[0] = '\0';
        return;
    }
    if (ct_db_open(&app->db, app->db_path) != 0) {
        fprintf(stderr, "cubing-trainer: cannot open database %s\n", app->db_path);
        app->db = NULL;
        return;
    }
    if (ct_db_migrate(app->db, app->migrations_path) != 0) {
        fprintf(stderr, "cubing-trainer: migration failed from %s\n", app->migrations_path);
        ct_db_close(app->db);
        app->db = NULL;
        return;
    }
    if (ct_repo_settings_merge(app->db, &app->cfg) != 0) {
        fprintf(stderr, "cubing-trainer: cannot read settings from %s\n", app->db_path);
        ct_db_close(app->db);
        app->db = NULL;
        return;
    }
    app->running = true;
}

void ct_app_run(ct_app *app) {
    if (app == NULL || !app->running) {
        return;
    }
    InitWindow((int)app->cfg.window.width, (int)app->cfg.window.height, "cubing-trainer");
    SetTargetFPS((int)app->cfg.window.target_fps);
    while (app->running && !WindowShouldClose()) {
        app->acc += GetFrameTime();
        double catchup_cap =
            app->fixed_dt * (app->cfg.window.target_fps / (double)CT_LOOP_CATCHUP_STEPS);
        if (app->acc > catchup_cap) {
            app->acc = catchup_cap;
        }
        while (app->acc >= app->fixed_dt) {
            ct_ui_update(app);
            app->acc -= app->fixed_dt;
        }
        BeginDrawing();
        ct_ui_draw(app);
        EndDrawing();
    }
    CloseWindow();
}

void ct_app_shutdown(ct_app *app) {
    ct_db_close(app->db);
    app->db = NULL;
}
