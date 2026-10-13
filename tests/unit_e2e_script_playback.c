#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../src/app/app.h"
#include "../src/app/script.h"
#include "../src/app/script_internal.h"
#include "../src/app/shots.h"
#include "../src/config/config.h"
#include "../src/packs/seed_registry.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/ui/ui.h"
#include "framework.h"

static sqlite3 *e2e_db(void) {
    sqlite3 *db = NULL;
    CT_ASSERT_EQ(ct_db_open(&db, ":memory:"), SQLITE_OK);
    CT_ASSERT_EQ(ct_db_migrate(db, "db/migrations"), CT_REPO_OK);
    return db;
}

static void e2e_app(ct_app *app, sqlite3 *db) {
    memset(app, 0, sizeof *app);
    app->cfg = ct_config_defaults();
    app->db = db;
    app->fixed_dt = 1.0 / (double)app->cfg.window.target_fps;
    app->running = true;
}

static int e2e_run(ct_app *app, const char *text, char *err) {
    ct_script script;
    int rc = ct_script_parse(text, &script, err, CT_SCRIPT_ERR_MAX);
    if (rc != 0) {
        return rc;
    }
    ct_script_rt rt;
    memset(&rt, 0, sizeof rt);
    return ct_script_run(app, &rt, &script, err, CT_SCRIPT_ERR_MAX);
}

CT_SUITE(e2e_parse_shot_words) {
    ct_script s;
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] = "shot_menu menu.png\n"
                               "shot_trainer 3x3 cfop 0 0 trainer.png\n"
                               "shot_chart 3bld 3style 1 2 chart.png\n"
                               "shot_review hidden review_hidden.png\n"
                               "shot_review revealed review_revealed.png\n"
                               "shot_cards edit unfiled cards.png\n"
                               "shot_settings letters settings.png\n"
                               "shot_stats stats.png\n"
                               "reviewed unfiled new-1 good 2026-01-05 12.5 3.25\n";
    CT_ASSERT_EQ(ct_script_parse(text, &s, err, sizeof err), 0);
    CT_ASSERT_EQ(s.count, 9);
    CT_ASSERT_EQ(s.cmds[0].kind, CT_SCRIPT_SHOT_MENU);
    CT_ASSERT_EQ(s.cmds[0].argc, CT_SCRIPT_ARGC_1);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[0], 0), "menu.png");
    CT_ASSERT_EQ(s.cmds[1].kind, CT_SCRIPT_SHOT_TRAINER);
    CT_ASSERT_EQ(s.cmds[1].argc, CT_SCRIPT_ARGC_5);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[1], 0), "3x3");
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[1], 4), "trainer.png");
    CT_ASSERT_EQ(s.cmds[2].kind, CT_SCRIPT_SHOT_CHART);
    CT_ASSERT_EQ(s.cmds[2].argc, CT_SCRIPT_ARGC_5);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[2], 3), "2");
    CT_ASSERT_EQ(s.cmds[3].kind, CT_SCRIPT_SHOT_REVIEW);
    CT_ASSERT_EQ(s.cmds[3].argc, CT_SCRIPT_ARGC_2);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[3], 0), "hidden");
    CT_ASSERT_EQ(s.cmds[4].kind, CT_SCRIPT_SHOT_REVIEW);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[4], 0), "revealed");
    CT_ASSERT_EQ(s.cmds[5].kind, CT_SCRIPT_SHOT_CARDS);
    CT_ASSERT_EQ(s.cmds[5].argc, CT_SCRIPT_ARGC_3);
    CT_ASSERT_EQ(s.cmds[6].kind, CT_SCRIPT_SHOT_SETTINGS);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[6], 0), "letters");
    CT_ASSERT_EQ(s.cmds[7].kind, CT_SCRIPT_SHOT_STATS);
    CT_ASSERT_EQ(s.cmds[7].argc, CT_SCRIPT_ARGC_1);
    CT_ASSERT_EQ(s.cmds[8].kind, CT_SCRIPT_REVIEWED);
    CT_ASSERT_EQ(s.cmds[8].argc, 6);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[8], 5), "3.25");
    static const char bad_arity[] = "shot_menu a b\n";
    CT_ASSERT_EQ(ct_script_parse(bad_arity, &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    static const char bad_word[] = "shot_bogus a\n";
    CT_ASSERT_EQ(ct_script_parse(bad_word, &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
}

CT_SUITE(e2e_run_shot_guards) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char bad_mode[] = "shot_review paused " CT_TEST_GUARD_PNG "\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_mode, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "review state must be hidden or revealed") != NULL);
    static const char bad_tab[] = "shot_settings colors " CT_TEST_GUARD_PNG "\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_tab, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "unknown settings tab") != NULL);
    static const char bad_browse[] = "shot_cards modify unfiled " CT_TEST_GUARD_PNG "\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_browse, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "cards state must be browse or edit") != NULL);
    static const char bad_case[] = "shot_trainer 3x3 cfop 0 99999 " CT_TEST_GUARD_PNG "\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_case, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "screenshot capture failed") != NULL);
    static const char bad_cell[] = "shot_chart 3bld 3style 0 99 " CT_TEST_GUARD_PNG "\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_cell, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "screenshot capture failed") != NULL);
    static const char bad_event[] = "shot_trainer zzz cfop 0 0 " CT_TEST_GUARD_PNG "\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_event, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "unknown event or method") != NULL);
    ct_db_close(db);
}

CT_SUITE(e2e_run_shot_render) {
    sqlite3 *db = e2e_db();
    CT_ASSERT_EQ(ct_pack_seed_registered(db, "db/seeds/packs"), 0);
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] = "cards_open unfiled\n"
                               "cards_new unfiled\n"
                               "cards_field alg3 \"R U R' U R U2 R'\"\n"
                               "cards_field algoh \"R U R'\"\n"
                               "cards_field algbig \"R U2 R'\"\n"
                               "cards_save\n"
                               "trainer_list 3bld 3style 0\n"
                               "trainer_case 0\n"
                               "trainer_generate\n"
                               "shot_menu build/test_shots/menu.png\n"
                               "shot_trainer 3x3 cfop 0 0 build/test_shots/trainer.png\n"
                               "shot_chart 3bld 3style 1 2 build/test_shots/chart.png\n"
                               "shot_trainer 3bld 3style 0 0 build/test_shots/case.png\n"
                               "shot_review hidden build/test_shots/review_hidden.png\n"
                               "shot_review revealed build/test_shots/review_revealed.png\n"
                               "shot_cards edit unfiled build/test_shots/cards.png\n"
                               "shot_cards browse unfiled build/test_shots/cards_browse.png\n"
                               "shot_settings general build/test_shots/settings_general.png\n"
                               "shot_settings letters build/test_shots/settings.png\n"
                               "shot_settings bld build/test_shots/settings_bld.png\n"
                               "shot_stats build/test_shots/stats.png\n";
    CT_ASSERT_EQ(e2e_run(&app, text, err), 0);
    static const char hidden_state[] = "shot_review hidden build/test_shots/rh.png\n";
    CT_ASSERT_EQ(e2e_run(&app, hidden_state, err), 0);
    CT_ASSERT(ct_screen_review_form()->revealed == false);
    static const char revealed_state[] = "shot_review revealed build/test_shots/rr.png\n";
    CT_ASSERT_EQ(e2e_run(&app, revealed_state, err), 0);
    CT_ASSERT(ct_screen_review_form()->revealed == true);
    static const char browse_state[] = "shot_cards browse unfiled build/test_shots/cb.png\n";
    CT_ASSERT_EQ(e2e_run(&app, browse_state, err), 0);
    CT_ASSERT(ct_screen_cards_form()->editing == false);
    static const char edit_state[] = "shot_cards edit unfiled build/test_shots/ce.png\n";
    CT_ASSERT_EQ(e2e_run(&app, edit_state, err), 0);
    CT_ASSERT(ct_screen_cards_form()->editing == true);
    const char *pngs[] = {"menu",
                          "trainer",
                          "chart",
                          "case",
                          "review_hidden",
                          "review_revealed",
                          "cards",
                          "cards_browse",
                          "settings",
                          "settings_general",
                          "settings_bld",
                          "stats",
                          "rh",
                          "rr",
                          "cb",
                          "ce"};
    for (unsigned i = 0; i < sizeof pngs / sizeof pngs[0]; i++) {
        char path[CT_DB_PATH_MAX];
        snprintf(path, sizeof path, "build/test_shots/%s.png", pngs[i]);
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
