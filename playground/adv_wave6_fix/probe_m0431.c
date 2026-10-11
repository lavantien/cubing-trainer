#include <stdio.h>
#include <string.h>

#include "../../src/app/app.h"
#include "../../src/app/script.h"
#include "../../src/config/config.h"
#include "../../src/storage/db.h"
#include "../../src/storage/repo_cards.h"
#include "../../src/trainer/cfop.h"

int ct_script_parse(const char *src, ct_script *out, char *err, unsigned cap);
int ct_script_run(ct_app *app, ct_script_rt *rt, const ct_script *s, char *err, unsigned cap);

int main(void) {
    sqlite3 *db = NULL;
    ct_db_open(&db, ":memory:");
    ct_db_migrate(db, "db/migrations");
    ct_app app;
    memset(&app, 0, sizeof app);
    app.cfg = ct_config_defaults();
    app.db = db;
    app.fixed_dt = 1.0 / (double)app.cfg.window.target_fps;
    app.running = true;
    static const char text[] = "cfop_populate pll\n"
                               "cards_open cfop_pll\n"
                               "cards_select 0\n"
                               "cards_edit\n"
                               "cards_field notes tuned\n"
                               "cards_save\n";
    ct_script script;
    char err[512];
    int rc = ct_script_parse(text, &script, err, (unsigned)sizeof err);
    printf("parse rc %d count %d\n", rc, script.count);
    for (int i = 0; i < script.count; i++) {
        printf("cmd %d kind %d argc %d:", i, (int)script.cmds[i].kind, script.cmds[i].argc);
        for (int a = 0; a < script.cmds[i].argc; a++) {
            const char *v = ct_script_arg(&script, &script.cmds[i], a);
            printf(" [%s]", v == NULL ? "(null)" : v);
        }
        const char *beyond = ct_script_arg(&script, &script.cmds[i], script.cmds[i].argc);
        printf(" beyond-arg [%s]\n", beyond == NULL ? "(null)" : beyond);
    }
    ct_script_rt rt;
    memset(&rt, 0, sizeof rt);
    rc = ct_script_run(&app, &rt, &script, err, (unsigned)sizeof err);
    printf("run rc %d err [%s]\n", rc, err);
    ct_card card;
    if (ct_repo_card_get_by_key(db, "cfop_pll", "pll.Aa", CT_DIR_FR, &card) == CT_REPO_OK) {
        printf("notes [%s]\n", card.notes);
    }
    ct_db_close(db);
    return 0;
}
