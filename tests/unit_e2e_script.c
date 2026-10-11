#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/app/app.h"
#include "../src/app/paths.h"
#include "../src/app/script.h"
#include "../src/app/script_internal.h"
#include "../src/config/config.h"
#include "../src/storage/db.h"
#include "../src/storage/repo_cards.h"
#include "../src/trainer/session.h"
#include "../src/ui/ui.h"
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

static const char e2e_reviews_cap_setup[] = "cards_open unfiled\n"
                                            "cards_new unfiled\n"
                                            "cards_field alg3 \"R U R'\"\n"
                                            "cards_save\n"
                                            "due unfiled new-1 2020-01-01\n";

static void e2e_reviews_cap_case(int cycles, const char *want, int want_rc) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    char chunk[2048];
    int done = 0;
    while (done < cycles) {
        int take = done == 0 ? (cycles - done < 30 ? cycles - done : 30)
                             : (cycles - done < 32 ? cycles - done : 32);
        int at = snprintf(chunk, sizeof chunk, "%s", done == 0 ? e2e_reviews_cap_setup : "");
        for (int i = 0; i < take; i++) {
            at += snprintf(chunk + at, sizeof chunk - (size_t)at,
                           "review_begin\nreview_reveal\nreview_grade again\nreview_end\n");
        }
        CT_ASSERT_EQ(e2e_run(&app, chunk, err), 0);
        done += take;
    }
    char final[CT_SCRIPT_ERR_MAX + 64];
    snprintf(final, sizeof final, "expect reviews unfiled new-1 == %s\n", want);
    CT_ASSERT_EQ(e2e_run(&app, final, err), want_rc);
    ct_db_close(db);
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
    CT_ASSERT_EQ(ct_script_parse("restart\n", &s, err, 0), CT_SCRIPT_EXIT_PARSE);
    CT_ASSERT_EQ(ct_script_op("=="), CT_SCRIPT_EQ);
    CT_ASSERT_EQ(ct_script_op(">="), CT_SCRIPT_GE);
    CT_ASSERT_EQ(ct_script_op("<="), CT_SCRIPT_LE);
    CT_ASSERT_EQ(ct_script_op("!="), CT_SCRIPT_NE);
    CT_ASSERT_EQ(ct_script_op("~="), -1);
    long day = 0;
    CT_ASSERT_EQ(ct_script_iso_day("2029-09-19", &day), 0);
    CT_ASSERT_EQ(day, ct_session_date_to_day(2029, 9, 19));
    CT_ASSERT_EQ(ct_script_iso_day("202x-09-19", &day), -1);
    CT_ASSERT_EQ(ct_script_iso_day("2029-0x-19", &day), -1);
    CT_ASSERT_EQ(ct_script_iso_day("2029-09-1x", &day), -1);
    CT_ASSERT_EQ(ct_script_iso_day("2029x09-19", &day), -1);
    CT_ASSERT_EQ(ct_script_iso_day("2029-09x19", &day), -1);
    long num = 0;
    CT_ASSERT_EQ(ct_script_long("x", &num), -1);
    CT_ASSERT_EQ(ct_script_long("12x", &num), -1);
    CT_ASSERT_EQ(ct_script_long("", &num), -1);
    double dnum = 0;
    CT_ASSERT_EQ(ct_script_double("x", &dnum), -1);
    CT_ASSERT_EQ(ct_script_double("nan", &dnum), -1);
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

    char exact[CT_SCRIPT_TEXT_MAX + 1];
    int fill = snprintf(exact, sizeof exact, "restart\n#");
    memset(exact + fill, 'x', (int)sizeof exact - 1 - fill);
    exact[CT_SCRIPT_TEXT_MAX - 1] = '\0';
    CT_ASSERT_EQ(strlen(exact), (size_t)CT_SCRIPT_TEXT_MAX - 1);
    CT_ASSERT_EQ(ct_script_parse(exact, &s, err, sizeof err), 0);
    exact[CT_SCRIPT_TEXT_MAX - 1] = 'x';
    exact[CT_SCRIPT_TEXT_MAX] = '\0';
    CT_ASSERT_EQ(strlen(exact), (size_t)CT_SCRIPT_TEXT_MAX);
    CT_ASSERT_EQ(ct_script_parse(exact, &s, err, sizeof err), CT_SCRIPT_EXIT_PARSE);
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
    CT_ASSERT(strstr(err, "count flashcard is 0, expected == 5") != NULL);
    static const char dbl_text[] = "expect cfg srs.request_retention == 0.95\n";
    CT_ASSERT_EQ(e2e_run(&app, dbl_text, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "cfg srs.request_retention is 0.9, expected == 0.95") != NULL);
    ct_db_close(db);
}

CT_SUITE(e2e_run_expect_surfaces) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char tables[] = "expect count settings != 99999999\n";
    CT_ASSERT_EQ(e2e_run(&app, tables, err), 0);
    static const char unknown[] = "expect count bogus == 1\n";
    CT_ASSERT_EQ(e2e_run(&app, unknown, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "unknown table") != NULL);
    static const char master[] = "expect count sqlite_master == 1\n";
    CT_ASSERT_EQ(e2e_run(&app, master, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "unknown table") != NULL);
    static const char fields[] = "cfop_populate pll\n"
                                 "expect card cfop_pll pll.Aa deck == cfop_pll\n"
                                 "expect card cfop_pll pll.Aa key == pll.Aa\n";
    CT_ASSERT_EQ(e2e_run(&app, fields, err), 0);
    static const char wrong_notes[] = "cfop_populate pll\n"
                                      "expect card cfop_pll pll.Aa notes == wrong\n";
    CT_ASSERT_EQ(e2e_run(&app, wrong_notes, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "card notes is [], expected == [wrong]") != NULL);
    static const char bad_field[] = "cfop_populate pll\n"
                                    "expect card cfop_pll pll.Aa wat == x\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_field, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "unknown card field") != NULL);
    ct_db_close(db);
}

CT_SUITE(e2e_run_expect_srs_reviews) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char setup[] = "cards_open unfiled\n"
                                "cards_new unfiled\n"
                                "cards_field alg3 \"R U R'\"\n"
                                "cards_save\n"
                                "due unfiled new-1 2020-01-01\n";
    char passing[256];
    snprintf(passing, sizeof passing,
             "%s"
             "expect srs unfiled new-1 due == 2020-01-01\n"
             "expect reviews unfiled new-1 != 9\n",
             setup);
    CT_ASSERT_EQ(e2e_run(&app, passing, err), 0);
    char notadate[256];
    snprintf(notadate, sizeof notadate, "%sexpect srs unfiled new-1 due == notadate\n",
             setup);
    CT_ASSERT_EQ(e2e_run(&app, notadate, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "due value is not a date") != NULL);
    char wrong_day[256];
    snprintf(wrong_day, sizeof wrong_day, "%sexpect srs unfiled new-1 due == 2021-01-01\n",
             setup);
    CT_ASSERT_EQ(e2e_run(&app, wrong_day, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "due is [2020-01-01], expected [2021-01-01]") != NULL);
    char wrong_reps[256];
    snprintf(wrong_reps, sizeof wrong_reps, "%sexpect srs unfiled new-1 reps == 5\n",
             setup);
    CT_ASSERT_EQ(e2e_run(&app, wrong_reps, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "srs reps is 0, expected == 5") != NULL);
    char wrong_lapses[256];
    snprintf(wrong_lapses, sizeof wrong_lapses,
             "%sexpect srs unfiled new-1 lapses == 7\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, wrong_lapses, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "srs lapses is 0, expected == 7") != NULL);
    char wrong_reviews[256];
    snprintf(wrong_reviews, sizeof wrong_reviews,
             "%sexpect reviews unfiled new-1 == 9\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, wrong_reviews, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "ratings are [], expected == [9]") != NULL);
    char bad_op[256];
    snprintf(bad_op, sizeof bad_op, "%sexpect reviews unfiled new-1 <= 9\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, bad_op, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "reviews supports == or !=") != NULL);
    ct_db_close(db);
}

CT_SUITE(e2e_run_expect_genalg) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char setup[] = "trainer_list 3bld 3style 0\n"
                                "trainer_case 0\n"
                                "trainer_generate\n";
    char passing[512];
    snprintf(passing, sizeof passing,
             "%s"
             "expect genalg AB setup == \"U' R2 Uw\"\n"
             "expect genalg AB interchange == R2\n"
             "expect genalg AB insertion == \"R R Uw' F2 Uw\"\n"
             "expect genalg AB score == 31.2\n"
             "expect genalg AB score >= 31.19\n"
             "expect genalg AB score <= 31.21\n",
             setup);
    CT_ASSERT_EQ(e2e_run(&app, passing, err), 0);
    char wrong_score[256];
    snprintf(wrong_score, sizeof wrong_score, "%sexpect genalg AB score >= 999\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, wrong_score, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "genalg score is 31.2, expected >= 999") != NULL);
    char bad_score[256];
    snprintf(bad_score, sizeof bad_score, "%sexpect genalg AB score == abc\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, bad_score, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "bad score operator or value") != NULL);
    char wrong_qtm[256];
    snprintf(wrong_qtm, sizeof wrong_qtm, "%sexpect genalg AB qtm == 19\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, wrong_qtm, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "genalg qtm is 18, expected == 19") != NULL);
    char wrong_algcount[256];
    snprintf(wrong_algcount, sizeof wrong_algcount, "%sexpect algcount == 2\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, wrong_algcount, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "algcount is 1, expected == 2") != NULL);
    char bad_field[256];
    snprintf(bad_field, sizeof bad_field, "%sexpect genalg AB wat == 1\n", setup);
    CT_ASSERT_EQ(e2e_run(&app, bad_field, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "unknown genalg field") != NULL);
    ct_db_close(db);
}

CT_SUITE(e2e_run_expect_reviews_cap) {
    char want255[CT_SCRIPT_ERR_MAX];
    int w = 0;
    for (int i = 0; i < 127; i++) {
        want255[w++] = '1';
        want255[w++] = ',';
    }
    want255[w++] = '1';
    want255[w] = '\0';
    e2e_reviews_cap_case(129, want255, 0);
    char want256[CT_SCRIPT_ERR_MAX + 2];
    w = 0;
    for (int i = 0; i < 127; i++) {
        want256[w++] = '1';
        want256[w++] = ',';
    }
    want256[w++] = '1';
    want256[w++] = '1';
    want256[w] = '\0';
    e2e_reviews_cap_case(130, want256, CT_SCRIPT_EXIT_EXPECT);
}

CT_SUITE(e2e_run_expect_cfg) {
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
    static const char saved[] = "restart\n"
                                "settings_open\n"
                                "settings_retention 0.85\n"
                                "settings_move U off\n"
                                "settings_move R off\n"
                                "settings_move F on\n"
                                "settings_save\n";
    char passing[512];
    snprintf(passing, sizeof passing,
             "%s"
             "restart\n"
             "expect cfg gen.preferred_moves == DF\n"
             "expect cfg anim.move_seconds == 0.25\n"
             "expect setting srs.request_retention == 0.850000\n",
             saved);
    CT_ASSERT_EQ(e2e_run(&app, passing, err), 0);
    char wrong_setting[256];
    snprintf(wrong_setting, sizeof wrong_setting,
             "%sexpect setting srs.request_retention == 0.9\n", saved);
    CT_ASSERT_EQ(e2e_run(&app, wrong_setting, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "setting is [0.850000], expected == [0.9]") != NULL);
    static const char bad_moves[] = "expect cfg gen.preferred_moves == ZQ\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_moves, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "cfg value is not a move set") != NULL);
    static const char bad_double[] = "expect cfg anim.move_seconds == abc\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_double, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "cfg value is not a number") != NULL);
    static const char bad_key[] = "expect cfg bogus.key == 1\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_key, err), CT_SCRIPT_EXIT_EXPECT);
    CT_ASSERT(strstr(err, "unknown cfg key") != NULL);
    ct_app_shutdown(&app);
    CT_ASSERT_EQ(CT_E2E_PUTENV("CT_DATA_DIR="), 0);
    CT_ASSERT_EQ(CT_E2E_PUTENV("CT_MIGRATIONS_DIR="), 0);
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
                               "expect setting gen.preferred_moves == UDF\n"
                               "expect cfg srs.request_retention >= 0.9\n"
                               "expect cfg srs.request_retention <= 0.9\n"
                               "expect cfg srs.request_retention != 0.5\n";
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

CT_SUITE(e2e_run_trainer_edge_generate) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char text[] = "trainer_list 3bld 3style 1\n"
                               "trainer_case 0\n"
                               "trainer_generate\n"
                               "expect algcount == 1\n"
                               "expect count generated_alg == 1\n"
                               "expect genalg AB sequence == \"R2 U S' U2 S U R2\"\n"
                               "expect genalg AB classification == \"SP7\"\n"
                               "expect genalg AB qtm == 10\n"
                               "expect algcount >= 1\n"
                               "expect algcount <= 1\n"
                               "expect algcount != 2\n";
    CT_ASSERT_EQ(e2e_run(&app, text, err), 0);
    ct_trainer_form f;
    CT_ASSERT_EQ(ct_trainer_form_open(&f, db), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_event(&f, CT_EVENT_3BLD), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_list(&f, 1), CT_REPO_OK);
    CT_ASSERT_EQ(ct_trainer_form_select_case(&f, 0), CT_REPO_OK);
    CT_ASSERT_EQ(f.alg_status, CT_REPO_OK);
    CT_ASSERT_STR_EQ(f.alg_text, "R2 U S' U2 S U R2");
    ct_db_close(db);
}

CT_SUITE(e2e_run_script_guards) {
    sqlite3 *db = e2e_db();
    ct_app app;
    e2e_app(&app, db);
    char err[CT_SCRIPT_ERR_MAX];
    static const char unknown_event[] = "trainer_list zzz cfop 0\n";
    CT_ASSERT_EQ(e2e_run(&app, unknown_event, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "unknown event or method") != NULL);
    static const char unknown_method[] = "trainer_list 3bld zzz 0\n";
    CT_ASSERT_EQ(e2e_run(&app, unknown_method, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "unknown event or method") != NULL);
    static const char list_high[] = "trainer_list 3bld 3style 3\n";
    CT_ASSERT_EQ(e2e_run(&app, list_high, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "list id out of range") != NULL);
    static const char list_neg[] = "trainer_list 3bld 3style -1\n";
    CT_ASSERT_EQ(e2e_run(&app, list_neg, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "list id out of range") != NULL);
    static const char save_after_list[] = "trainer_list 3bld 3style 0\ntrainer_save d\n";
    CT_ASSERT_EQ(e2e_run(&app, save_after_list, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "no generated alg in scope") != NULL);
    static const char save_after_case[] = "trainer_list 3bld 3style 0\ntrainer_case 0\n"
                                          "trainer_save d\n";
    CT_ASSERT_EQ(e2e_run(&app, save_after_case, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "no generated alg in scope") != NULL);
    static const char two_face[] = "settings_open\nsettings_move AB on\n";
    CT_ASSERT_EQ(e2e_run(&app, two_face, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "one letter") != NULL);
    static const char bad_stage[] = "cfop_populate bogus\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_stage, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "unknown cfop stage") != NULL);
    static const char bad_grade[] = "review_begin\nreview_grade bogus\n";
    CT_ASSERT_EQ(e2e_run(&app, bad_grade, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "unknown grade") != NULL);
    static const char case_neg[] = "trainer_list 3bld 3style 0\ntrainer_case -1\n";
    CT_ASSERT_EQ(e2e_run(&app, case_neg, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "case index out of range") != NULL);
    static const char case_high[] = "trainer_list 3bld 3style 0\ntrainer_case 99999\n";
    CT_ASSERT_EQ(e2e_run(&app, case_high, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "case index out of range") != NULL);
    static const char wait_flow[] = "review_begin\nwait 1\nwait 100000\nreview_end\n"
                                    "expect count session_log == 1\n";
    CT_ASSERT_EQ(e2e_run(&app, wait_flow, err), 0);
    static const char wait_zero[] = "review_begin\nwait 0\n";
    CT_ASSERT_EQ(e2e_run(&app, wait_zero, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "wait out of range") != NULL);
    static const char wait_high[] = "review_begin\nwait 100001\n";
    CT_ASSERT_EQ(e2e_run(&app, wait_high, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "wait out of range") != NULL);
    static const char stages[] = "cfop_populate oll\n"
                                 "expect count flashcard == 57\n"
                                 "cfop_populate f2l\n"
                                 "expect count flashcard == 221\n";
    CT_ASSERT_EQ(e2e_run(&app, stages, err), 0);
    static const char fields[] = "cfop_populate pll\n"
                                 "cards_open cfop_pll\n"
                                 "cards_select 0\n"
                                 "cards_edit\n"
                                 "cards_field algbig \"R U\"\n"
                                 "cards_field algoh \"R U'\"\n"
                                 "cards_field front plainfront\n"
                                 "cards_save\n"
                                 "expect card cfop_pll pll.Aa algbig == \"R U\"\n"
                                 "expect card cfop_pll pll.Aa algoh == \"R U'\"\n"
                                 "expect card cfop_pll pll.Aa front == plainfront\n";
    CT_ASSERT_EQ(e2e_run(&app, fields, err), 0);
    char long_value[CT_CARD_ALG_MAX + 1];
    memset(long_value, 'R', sizeof long_value - 1);
    long_value[sizeof long_value - 1] = '\0';
    char field_text[sizeof long_value + 160];
    snprintf(field_text, sizeof field_text,
             "cfop_populate pll\ncards_open cfop_pll\ncards_select 0\ncards_edit\n"
             "cards_field algbig %s\n",
             long_value);
    CT_ASSERT_EQ(e2e_run(&app, field_text, err), CT_SCRIPT_EXIT_RUN);
    CT_ASSERT(strstr(err, "too long") != NULL);
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
