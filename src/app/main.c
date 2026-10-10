#include <stdio.h>
#include <string.h>

#include "app/app.h"
#include "app/script.h"

static int ct_main_scripted(int argc, char **argv, ct_app *app) {
    if (argc != CT_SCRIPT_ARGC || strcmp(argv[1], "--headless") != 0 ||
        strcmp(argv[2], "--script") != 0) {
        fprintf(stderr, "usage: cubing-trainer [--headless --script tests/e2e/<case>.txt]\n");
        return CT_SCRIPT_EXIT_USAGE;
    }
    ct_app_init(app);
    if (!app->running) {
        ct_app_shutdown(app);
        return 1;
    }
    int rc = ct_app_run_scripted(app, argv[3]);
    ct_app_shutdown(app);
    return rc;
}

int main(int argc, char **argv) {
    ct_app app;
    if (argc > 1) {
        return ct_main_scripted(argc, argv, &app);
    }
    ct_app_init(&app);
    if (!app.running) {
        ct_app_shutdown(&app);
        return 1;
    }
    ct_app_run(&app);
    ct_app_shutdown(&app);
    return 0;
}
