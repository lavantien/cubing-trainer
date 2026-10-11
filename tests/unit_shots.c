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
    CT_ASSERT_EQ(ct_shots_menu(NULL, "out.png"), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_menu(&app, NULL), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_menu(&app, ""), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_menu(&dead, "out.png"), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, NULL), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_chart(&app, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, 2, ""), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_review(NULL, false, "out.png"), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_cards(&app, "unfiled", true, NULL), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_cards(&app, NULL, true, "out.png"), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_cards(&app, "", true, "out.png"), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_settings(&app, CT_SETTINGS_TAB_GENERAL, ""), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_stats(NULL, "out.png"), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_settings(&app, CT_SETTINGS_TAB_COUNT, "out.png"), CT_SHOTS_E_ARG);
    CT_ASSERT_EQ(ct_shots_settings(&app, -1, "out.png"), CT_SHOTS_E_ARG);
    ct_shots_close();
    ct_db_close(db);
}

CT_SUITE(shots_state_guards) {
    sqlite3 *db = shots_db();
    ct_app app;
    shots_app(&app, db);
    CT_ASSERT_EQ(ct_shots_cards(&app, "nosuchdeck", true, "out.png"), CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 9, 0, "out.png"),
                 CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 0, -2, "out.png"),
                 CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(ct_shots_chart(&app, CT_EVENT_3X3, CT_METHOD_CFOP, 0, 0, "out.png"),
                 CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(
        ct_shots_chart(&app, CT_EVENT_3BLD, CT_METHOD_3STYLE, 1, CT_LETTER_STICKERS, "out.png"),
        CT_SHOTS_E_STATE);
    CT_ASSERT_EQ(
        ct_shots_trainer(&app, CT_EVENT_3X3, CT_METHOD_SARAH_INTERMEDIATE, 0, 0, "out.png"),
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
