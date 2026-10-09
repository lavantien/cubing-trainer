#include "app/app.h"

int main(void) {
    ct_app app;
    ct_app_init(&app);
    if (!app.running) {
        ct_app_shutdown(&app);
        return 1;
    }
    ct_app_run(&app);
    ct_app_shutdown(&app);
    return 0;
}
