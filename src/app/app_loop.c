#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "raylib.h"
#include "ui/ui.h"

static void ct_mkdirs(char *path) {
    size_t len = strlen(path);
    for (size_t i = 4; i < len; i++) {
        if (path[i] != '/' && path[i] != '\\') {
            continue;
        }
        char saved = path[i];
        path[i] = '\0';
        if (_mkdir(path) != 0 && errno != EEXIST) {
            fprintf(stderr, "cubing-trainer: cannot create directory %s\n", path);
            path[i] = saved;
            return;
        }
        path[i] = saved;
    }
}

void ct_app_init(ct_app *app) {
    app->db = NULL;
    app->cfg = ct_config_defaults();
    const char *base = getenv("LOCALAPPDATA");
    if (base == NULL) {
        base = ".";
    }
    int written = snprintf(app->db_path, sizeof app->db_path, "%s/%s", base, app->cfg.db.dir_name);
    if (written < 0 || (size_t)written >= sizeof app->db_path) {
        fprintf(stderr, "cubing-trainer: data directory path too long\n");
        app->db_path[0] = '\0';
    }
    ct_mkdirs(app->db_path);
    size_t dir_len = strlen(app->db_path);
    int name_written = snprintf(app->db_path + dir_len, sizeof app->db_path - dir_len, "/%s",
                                app->cfg.db.filename);
    if (name_written < 0 || (size_t)name_written >= sizeof app->db_path - dir_len) {
        fprintf(stderr, "cubing-trainer: database path too long\n");
        app->db_path[0] = '\0';
    }
    snprintf(app->migrations_path, sizeof app->migrations_path, "db/migrations");
    bool db_ready = false;
    if (ct_db_open(&app->db, app->db_path) != 0) {
        fprintf(stderr, "cubing-trainer: cannot open database %s\n", app->db_path);
        app->db = NULL;
    } else if (ct_db_migrate(app->db, app->migrations_path) != 0) {
        fprintf(stderr, "cubing-trainer: migration failed from %s\n", app->migrations_path);
        ct_db_close(app->db);
        app->db = NULL;
    } else {
        db_ready = true;
    }
    app->screen = CT_SCREEN_MENU;
    app->running = db_ready;
    app->acc = 0.0;
    app->fixed_dt = 1.0 / app->cfg.window.target_fps;
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
