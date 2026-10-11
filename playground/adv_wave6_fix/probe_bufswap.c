#include <stdio.h>
#include <string.h>

#include "../src/app/app.h"
#include "../src/config/config.h"
#include "../src/engine/notation.h"
#include "../src/gen/synth.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/storage/repo_settings.h"
#include "../src/trainer/selector.h"

int main(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK) {
        printf("db open failed\n");
        return 1;
    }
    if (ct_db_migrate(db, "db/migrations") != CT_REPO_OK) {
        printf("migrate failed\n");
        return 1;
    }
    if (ct_letter_seed_speffz(db) != 0) {
        printf("seed failed\n");
        return 1;
    }
    ct_selector_scheme scheme;
    if (ct_selector_scheme_load(db, &scheme) != 0) {
        printf("scheme load failed\n");
        return 1;
    }
    printf("buffer_corner %d buffer_edge %d\n", scheme.buffer_corner, scheme.buffer_edge);
    ct_selector_list lists[8];
    int found = ct_selector_lists(CT_EVENT_3BLD, CT_METHOD_3STYLE, lists, 8);
    printf("lists %d\n", found);
    int t1 = -1;
    int t2 = -1;
    int list_id = -1;
    for (int i = 0; i < found; i++) {
        if (lists[i].stage == CT_STAGE_CONCEPT && lists[i].orbit == CT_ORBIT_EDGE) {
            list_id = i;
        }
    }
    printf("edge chart list_id %d\n", list_id);
    if (ct_selector_case_pair(&scheme, CT_EVENT_3BLD, CT_METHOD_3STYLE, list_id, 0, &t1, &t2) !=
        0) {
        printf("pair failed\n");
        return 1;
    }
    printf("case 0 pair t1 %d t2 %d\n", t1, t2);
    ct_app app;
    memset(&app, 0, sizeof app);
    app.cfg = ct_config_defaults();
    for (int pass = 0; pass < 2; pass++) {
        int buffer = pass == 0 ? scheme.buffer_edge : scheme.buffer_corner;
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(CT_ORBIT_EDGE, buffer, t1, t2, &app.cfg, &res);
        char text[512];
        ct_notation_format(res.seq, res.len, 3, text, (unsigned)sizeof text);
        printf("pass %d buffer %d rc %d found %d len %d seq %s\n", pass, buffer, rc, res.found,
               res.len, text);
    }
    ct_db_close(db);
    return 0;
}
