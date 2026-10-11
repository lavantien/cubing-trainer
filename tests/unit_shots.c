#include <direct.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/app/app.h"
#include "../src/app/shots.h"
#include "../src/config/config.h"
#include "../src/packs/seed_registry.h"
#include "../src/storage/db.h"
#include "../src/ui/ui.h"
#include "framework.h"

#define CT_SHOTS_OUT_DIR "build/test_shots_api"

static sqlite3 *shots_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), 0);
    CT_ASSERT_EQ(ct_pack_seed_registered(db, "db/seeds/packs"), 0);
    return db;
}

static void shots_app(ct_app *app, sqlite3 *db) {
    memset(app, 0, sizeof *app);
    app->cfg = ct_config_defaults();
    app->db = db;
    app->fixed_dt = 1.0 / (double)app->cfg.window.target_fps;
    app->running = true;
}

CT_SUITE(shots_settings_tab_clamp) {
    CT_ASSERT_EQ(ct_screen_settings_select_tab(CT_SETTINGS_TAB_COUNT), CT_SETTINGS_TAB_BLD);
    CT_ASSERT_EQ(ct_screen_settings_select_tab(-1), CT_SETTINGS_TAB_GENERAL);
    CT_ASSERT_EQ(ct_screen_settings_select_tab(CT_SETTINGS_TAB_LETTERS), CT_SETTINGS_TAB_LETTERS);
}

CT_SUITE(shots_arg_guards) {
    sqlite3 *db = shots_db();
    ct_app app;
    shots_app(&app, db);
    ct_app dead;
    shots_app(&dead, db);
    dead.running = false;
    CT_ASSERT_EQ(ct_shots_menu(NULL, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_menu(&app, NULL), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_menu(&app, ""), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_menu(&dead, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, NULL), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_chart(&app, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 2, ""), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_review(NULL, false, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_cards(&app, "unfiled", true, NULL), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_cards(&app, NULL, true, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_cards(&app, "", true, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_settings(&app, CT_SETTINGS_TAB_GENERAL, ""), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_stats(NULL, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_settings(&app, CT_SETTINGS_TAB_COUNT, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_settings(&app, -1, CT_TEST_GUARD_PNG), CT_SHOTS_E_ARG);
    ct_shots_close();
    ct_db_close(db);
}

CT_SUITE(shots_state_guards) {
    sqlite3 *db = shots_db();
    ct_app app;
    shots_app(&app, db);
    CT_ASSERT_EQ(ct_shots_cards(&app, "nosuchdeck", true, CT_TEST_GUARD_PNG), CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 9, 0, CT_TEST_GUARD_PNG),
                 CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 0, -2, CT_TEST_GUARD_PNG),
                 CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(ct_shots_chart(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, CT_TEST_GUARD_PNG),
                 CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(
        ct_shots_chart(&app, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, CT_LETTER_STICKERS, CT_TEST_GUARD_PNG),
        CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(
        ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_SARAH_INTERMEDIATE, 0, 0, CT_TEST_GUARD_PNG),
        CT_SHOTS_E_STATE);
    ct_shots_close();
    ct_db_close(db);
}

CT_SUITE(shots_render_smoke) {
    sqlite3 *db = shots_db();
    ct_app app;
    shots_app(&app, db);
    ct_cards_form seed;
    ct_cards_form_open(&seed, db, "unfiled");
    CT_ASSERT_EQ(ct_cards_form_create(&seed, db, "unfiled"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_shots_menu(&app, CT_SHOTS_OUT_DIR "/menu.png"), CT_SHOTS_OK);
    CT_ASSERT_EQ(
        ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, CT_SHOTS_OUT_DIR "/trainer.png"),
        CT_SHOTS_OK);
    CT_ASSERT_EQ(
        ct_shots_chart(&app, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 2, CT_SHOTS_OUT_DIR "/chart.png"),
        CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_shots_review(&app, false, CT_SHOTS_OUT_DIR "/review_hidden.png"), CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_shots_review(&app, true, CT_SHOTS_OUT_DIR "/review_revealed.png"), CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_shots_cards(&app, "unfiled", true, CT_SHOTS_OUT_DIR "/cards.png"), CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_shots_settings(&app, CT_SETTINGS_TAB_LETTERS, CT_SHOTS_OUT_DIR "/letters.png"),
                 CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_shots_settings(&app, CT_SETTINGS_TAB_BLD, CT_SHOTS_OUT_DIR "/bld.png"),
                 CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_shots_settings(&app, CT_SETTINGS_TAB_GENERAL, CT_SHOTS_OUT_DIR "/general.png"),
                 CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_shots_stats(&app, CT_SHOTS_OUT_DIR "/stats.png"), CT_SHOTS_OK);
    const char *pngs[] = {"menu",  "trainer", "chart", "review_hidden", "review_revealed",
                          "cards", "letters", "bld",   "general",       "stats"};
    for (unsigned i = 0; i < sizeof pngs / sizeof pngs[0]; i++) {
        char path[CT_DB_PATH_MAX];
        snprintf(path, sizeof path, "%s/%s.png", CT_SHOTS_OUT_DIR, pngs[i]);
        FILE *f = fopen(path, "rb");
        CT_ASSERT(f != NULL);
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            CT_ASSERT(ftell(f) > 1000);
            fclose(f);
            CT_ASSERT_EQ(remove(path), 0);
        }
    }
    ct_shots_close();
    ct_db_close(db);
}

static void shots_scrub_dir(const char *dir) {
    struct stat st;
    if (stat(dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        _rmdir(dir);
    }
}

CT_SUITE(shots_path_bounds) {
    sqlite3 *db = shots_db();
    ct_app app;
    shots_app(&app, db);
    shots_scrub_dir("build/test_shots_api/trail");
    shots_scrub_dir("build/test_shots_api/bs");
    char huge[CT_APP_EXE_PATH_MAX + 2];
    memset(huge, 'a', sizeof huge - 1);
    huge[sizeof huge - 1] = '\0';
    CT_ASSERT_EQ(ct_shots_menu(&app, huge), CT_SHOTS_E_ARG);
    char edge[CT_APP_EXE_PATH_MAX + 1];
    memset(edge, 'a', sizeof edge - 1);
    edge[sizeof edge - 1] = '\0';
    CT_ASSERT_EQ(ct_shots_menu(&app, edge), CT_SHOTS_E_ARG);
    struct stat st;
    CT_ASSERT(stat("build/test_shots_api", &st) == 0);
    CT_ASSERT_EQ(ct_shots_menu(&app, "build/test_shots_api/trail/"), CT_SHOTS_E_WRITE);
    CT_ASSERT(stat("build/test_shots_api/trail", &st) != 0);
    CT_ASSERT_EQ(ct_shots_menu(&app, "a"), CT_SHOTS_E_WRITE);
    CT_ASSERT_EQ(ct_shots_menu(&app, "shotsprobe.png"), CT_SHOTS_OK);
    CT_ASSERT_EQ(remove("shotsprobe.png"), 0);
    CT_ASSERT_EQ(ct_shots_menu(&app, "build\\test_shots_api\\bs\\menu.png"), CT_SHOTS_OK);
    CT_ASSERT(stat("build/test_shots_api/bs", &st) == 0);
    CT_ASSERT_EQ(remove("build/test_shots_api/bs/menu.png"), 0);
    CT_ASSERT_EQ(_rmdir("build/test_shots_api/bs"), 0);
    ct_cards_form seed;
    ct_cards_form_open(&seed, db, "unfiled");
    CT_ASSERT_EQ(ct_cards_form_create(&seed, db, "unfiled"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_cards_form_create(&seed, db, "unfiled"), CT_REPO_OK);
    CT_ASSERT_EQ(ct_shots_cards(&app, "unfiled", true, "build/test_shots_api/sel.png"),
                 CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_screen_cards_form()->selected, 0);
    CT_ASSERT_EQ(remove("build/test_shots_api/sel.png"), 0);
    ct_shots_close();
    ct_db_close(db);
}

CT_SUITE(shots_review_reopen) {
    sqlite3 *db = shots_db();
    sqlite3 *db2 = shots_db();
    ct_app app;
    shots_app(&app, db);
    CT_ASSERT_EQ(ct_shots_review(&app, false, "build/test_shots_api/r1.png"), CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_screen_review_form()->db, db);
    app.db = db2;
    CT_ASSERT_EQ(ct_shots_review(&app, false, "build/test_shots_api/r2.png"), CT_SHOTS_OK);
    CT_ASSERT_EQ(ct_screen_review_form()->db, db2);
    CT_ASSERT_EQ(remove("build/test_shots_api/r1.png"), 0);
    CT_ASSERT_EQ(remove("build/test_shots_api/r2.png"), 0);
    ct_shots_close();
    ct_db_close(db);
    ct_db_close(db2);
}
