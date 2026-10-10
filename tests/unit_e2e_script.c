#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/app/app.h"
#include "../src/app/paths.h"
#include "../src/app/script.h"
#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "framework.h"

#if defined(_WIN32)
#include <process.h>
#define CT_E2E_PUTENV _putenv
#define CT_E2E_PID _getpid()
#else
#include <unistd.h>
#define CT_E2E_PUTENV putenv
#define CT_E2E_PID getpid()
#endif

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

static const char e2e_review_text[] = "cards_open unfiled\n"
                                      "cards_new unfiled\n"
                                      "cards_field alg3 \"R U R'\"\n"
                                      "cards_save\n"
                                      "due unfiled new-1 2020-01-01\n"
                                      "review_begin\n"
                                      "review_reveal\n"
                                      "wait 120\n"
                                      "review_grade good\n"
                                      "review_end\n"
                                      "review_begin\n"
                                      "review_grade easy\n"
                                      "review_end\n"
                                      "expect count flashcard == 1\n"
                                      "expect srs unfiled new-1 reps == 2\n"
                                      "expect srs unfiled new-1 state == 1\n"
                                      "expect srs unfiled new-1 lapses == 0\n"
                                      "expect srs unfiled new-1 due == today\n"
                                      "expect reviews unfiled new-1 == 3,4\n"
                                      "expect count session_log == 2\n";

CT_SUITE(e2e_parse_flow) {
    ct_script s;
    char err[CT_SCRIPT_ERR_MAX];
    CT_ASSERT_EQ(ct_script_parse(e2e_review_text, &s, err, sizeof err), 0);
    CT_ASSERT_EQ(s.count, 20);
    CT_ASSERT_EQ(s.cmds[0].kind, CT_SCRIPT_CARDS_OPEN);
    CT_ASSERT_EQ(s.cmds[0].argc, 1);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[0], 0), "unfiled");
    CT_ASSERT_EQ(s.cmds[2].kind, CT_SCRIPT_CARDS_FIELD);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[2], 0), "alg3");
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[2], 1), "R U R'");
    CT_ASSERT_EQ(s.cmds[8].kind, CT_SCRIPT_REVIEW_GRADE);
    CT_ASSERT_EQ(s.cmds[13].kind, CT_SCRIPT_EXPECT_COUNT);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[13], 0), "flashcard");
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[13], 1), "==");
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[13], 2), "1");
    CT_ASSERT_EQ(s.cmds[14].kind, CT_SCRIPT_EXPECT_SRS);
    CT_ASSERT_EQ(s.cmds[14].argc, 5);
    CT_ASSERT_EQ(s.cmds[17].kind, CT_SCRIPT_EXPECT_SRS);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[17], 4), "today");
    CT_ASSERT_EQ(s.cmds[18].kind, CT_SCRIPT_EXPECT_REVIEWS);
    CT_ASSERT_EQ(s.cmds[18].argc, 4);
    CT_ASSERT_EQ(s.cmds[19].kind, CT_SCRIPT_EXPECT_COUNT);
}

CT_SUITE(e2e_parse_comments_and_quotes) {
    static const char text[] = "# leading comment\n"
                               "\n"
                               "restart\n"
                               "   # indented comment\n"
                               "cards_field notes \"two words\" \n";
    ct_script s;
    char err[CT_SCRIPT_ERR_MAX];
    CT_ASSERT_EQ(ct_script_parse(text, &s, err, sizeof err), 0);
    CT_ASSERT_EQ(s.count, 2);
    CT_ASSERT_EQ(s.cmds[0].kind, CT_SCRIPT_RESTART);
    CT_ASSERT_EQ(s.cmds[1].kind, CT_SCRIPT_CARDS_FIELD);
    CT_ASSERT_STR_EQ(ct_script_arg(&s, &s.cmds[1], 1), "two words");
}

CT_SUITE(e2e_parse_errors) {
    ct_script s;
    char err[CT_SCRIPT_ERR_MAX];
    CT_ASSERT_EQ(ct_script_parse("", &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT(strstr(err, "empty") != NULL);
    CT_ASSERT_EQ(ct_script_parse("# only comments\n", &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT_EQ(ct_script_parse("nope unfiled\n", &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT(strstr(err, "line 1") != NULL);
    CT_ASSERT_EQ(ct_script_parse("restart extra\n", &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT(strstr(err, "arity") != NULL);
    CT_ASSERT_EQ(ct_script_parse("cards_open\n", &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT_EQ(ct_script_parse("cards_field alg3 \"open quote\n", &s, err, sizeof err),
                 CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT(strstr(err, "quote") != NULL);
    CT_ASSERT_EQ(ct_script_parse("expect wat x == 1\n", &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT_EQ(ct_script_parse("expect count flashcard ~= 1\n", &s, err, sizeof err),
                 CT_SCRIPT_EXIT_PARSE);
}

CT_SUITE(e2e_parse_caps) {
    char big[CT_SCRIPT_TEXT_MAX + 2];
    for (unsigned i = 0; i < sizeof big - 1; i++) {
        big[i] = 'a';
    }
    big[sizeof big - 1] = '\0';
    ct_script s;
    char err[CT_SCRIPT_ERR_MAX];
    CT_ASSERT_EQ(ct_script_parse(big, &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);

    char many[CT_SCRIPT_TEXT_MAX];
    int at = 0;
    for (int i = 0; i < CT_SCRIPT_CMDS_MAX + 1; i++) {
        at += snprintf(many + at, sizeof many - (unsigned)at, "wait 1\n");
    }
    CT_ASSERT_EQ(ct_script_parse(many, &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT(strstr(err, "too many") != NULL);
}

CT_SUITE(e2e_run_review_loop) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    CT_ASSERT_EQ(e2e_run(&app, e2e_review_text, err), 0);
    ct_db_close(db);
}

CT_SUITE(e2e_run_expect_failure) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] = "cards_open unfiled\n"
                               "expect count flashcard == 5\n";
    CT_ASSERT_EQ(e2e_run(&app, text, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "line 2") != NULL);
    ct_db_close(db);
}

CT_SUITE(e2e_run_cfop_cards) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] =
        "cfop_populate pll\n"
        "expect count flashcard == 21\n"
        "cards_open cfop_pll\n"
        "cards_select 0\n"
        "cards_edit\n"
        "cards_field notes tuned\n"
        "cards_save\n"
        "expect card cfop_pll pll.Aa notes == tuned\n"
        "expect card cfop_pll pll.Aa front == {\"stage\":\"pll\",\"case\":\"Aa\"}\n"
        "cards_delete\n"
        "expect count flashcard == 20\n";
    CT_ASSERT_EQ(e2e_run(&app, text, err), 0);
    ct_db_close(db);
}

CT_SUITE(e2e_run_settings_form) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] = "settings_open\n"
                               "settings_retention 0.85\n"
                               "settings_move R off\n"
                               "settings_move F on\n"
                               "settings_save\n"
                               "expect setting srs.request_retention == 0.850000\n"
                               "expect setting gen.preferred_moves == UDF\n";
    CT_ASSERT_EQ(e2e_run(&app, text, err), 0);
    ct_db_close(db);
}

CT_SUITE(e2e_run_restart_merge) {
    char dir[CT_DB_PATH_MAX];
    snprintf(dir, sizeof dir, "build/test_e2e_script/p%ld", (long)CT_E2E_PID);
    char binding[CT_DB_PATH_MAX + 32];
    snprintf(binding, sizeof binding, "CT_DATA_DIR=%s", dir);
    CT_ASSERT_EQ(CT_E2E_PUTENV(binding), 0);
    CT_ASSERT_EQ(CT_E2E_PUTENV("CT_MIGRATIONS_DIR=db/migrations"), 0);
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] = "restart\n"
                               "settings_open\n"
                               "settings_retention 0.85\n"
                               "settings_save\n"
                               "restart\n"
                               "expect count flashcard == 0\n"
                               "expect cfg srs.request_retention == 0.85\n"
                               "expect cfg gen.preferred_moves == UDR\n";
    CT_ASSERT_EQ(e2e_run(&app, text, err), 0);
    ct_app_shutdown(&app);
    CT_ASSERT_EQ(CT_E2E_PUTENV("CT_DATA_DIR="), 0);
    CT_ASSERT_EQ(CT_E2E_PUTENV("CT_MIGRATIONS_DIR="), 0);
}

CT_SUITE(e2e_run_trainer_generate) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] =
        "trainer_list 3bld 3style 0\n"
        "trainer_case 0\n"
        "trainer_generate\n"
        "trainer_save 3style_corner\n"
        "expect algcount == 1\n"
        "expect count generated_alg == 1\n"
        "expect count flashcard == 1\n"
        "expect genalg AB sequence == \"U' R2 Uw R2 Uw' F2 Uw R2 Uw' F2 R2 U\"\n"
        "expect genalg AB classification == \"per special\"\n"
        "expect genalg AB qtm == 18\n"
        "expect card 3style_corner AB alg3 == \"U' R2 Uw R2 Uw' F2 Uw R2 Uw' F2 R2 U\"\n";
    CT_ASSERT_EQ(e2e_run(&app, text, err), 0);
    ct_db_close(db);
}

CT_SUITE(e2e_data_dir_override) {
    char out[CT_DB_PATH_MAX];
    CT_ASSERT_EQ(CT_E2E_PUTENV("CT_DATA_DIR=build/test_e2e_script/override"), 0);
    CT_ASSERT_EQ(ct_data_dir("cubing-trainer", out, (unsigned)sizeof out), 0);
    CT_ASSERT(strstr(out, "build/test_e2e_script/override/cubing-trainer") == out);
    CT_ASSERT_EQ(CT_E2E_PUTENV("CT_DATA_DIR="), 0);
    CT_ASSERT_EQ(ct_data_dir("cubing-trainer", out, (unsigned)sizeof out), 0);
    const char *base = getenv(CT_DATA_DIR_ENV);
    CT_ASSERT(base != NULL && base[0] != '\0');
    CT_ASSERT(strstr(out, base) == out);
}
