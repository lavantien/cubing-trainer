#ifndef CT_APP_SCRIPT_H
#define CT_APP_SCRIPT_H

#include <stdbool.h>

#include "app/app.h"
#include "gen/classify.h"
#include "gen/score.h"
#include "storage/repo_algs.h"
#include "ui/ui.h"

enum {
    CT_SCRIPT_ARG_TEXT = 0,
    CT_SCRIPT_ARG_A,
    CT_SCRIPT_ARG_B,
    CT_SCRIPT_ARG_C,
    CT_SCRIPT_ARG_D,
    CT_SCRIPT_ARG_E
};

typedef enum ct_script_cmd {
    CT_SCRIPT_RESTART = 0,
    CT_SCRIPT_CARDS_OPEN,
    CT_SCRIPT_CARDS_NEW,
    CT_SCRIPT_CARDS_SELECT,
    CT_SCRIPT_CARDS_EDIT,
    CT_SCRIPT_CARDS_FIELD,
    CT_SCRIPT_CARDS_SAVE,
    CT_SCRIPT_CARDS_DELETE,
    CT_SCRIPT_CFOP_POPULATE,
    CT_SCRIPT_DUE,
    CT_SCRIPT_REVIEW_BEGIN,
    CT_SCRIPT_REVIEW_REVEAL,
    CT_SCRIPT_REVIEW_GRADE,
    CT_SCRIPT_REVIEW_END,
    CT_SCRIPT_WAIT,
    CT_SCRIPT_SETTINGS_OPEN,
    CT_SCRIPT_SETTINGS_RETENTION,
    CT_SCRIPT_SETTINGS_MOVE,
    CT_SCRIPT_SETTINGS_SAVE,
    CT_SCRIPT_TRAINER_LIST,
    CT_SCRIPT_TRAINER_CASE,
    CT_SCRIPT_TRAINER_GENERATE,
    CT_SCRIPT_TRAINER_SAVE,
    CT_SCRIPT_EXPECT_COUNT,
    CT_SCRIPT_EXPECT_CARD,
    CT_SCRIPT_EXPECT_SRS,
    CT_SCRIPT_EXPECT_REVIEWS,
    CT_SCRIPT_EXPECT_SETTING,
    CT_SCRIPT_EXPECT_CFG,
    CT_SCRIPT_EXPECT_ALGCOUNT,
    CT_SCRIPT_EXPECT_GENALG
} ct_script_cmd;

typedef struct {
    ct_script_cmd kind;
    int line;
    int argc;
    unsigned off[CT_SCRIPT_ARGS_MAX];
    unsigned len[CT_SCRIPT_ARGS_MAX];
} ct_script_row;

typedef struct {
    char text[CT_SCRIPT_TEXT_MAX];
    ct_script_row cmds[CT_SCRIPT_CMDS_MAX];
    int count;
} ct_script;

typedef struct {
    ct_cards_form cards;
    ct_settings_form settings;
    ct_review_form review;
    bool review_open;
    ct_event event;
    ct_method method;
    int list_id;
    int orbit;
    char target_key[CT_CARD_CASE_KEY_MAX];
    char alg_text[CT_CARD_ALG_MAX];
    ct_classify_result cls;
    ct_score_parts parts;
    char params_hash[CT_ALG_HASH_HEX_MAX];
    bool generated;
    bool synth_ready;
} ct_script_rt;

int ct_script_parse(const char *src, ct_script *out, char *err, unsigned cap);
const char *ct_script_arg(const ct_script *s, const ct_script_row *c, int i);
int ct_script_run(ct_app *app, ct_script_rt *rt, const ct_script *s, char *err, unsigned cap);
int ct_app_run_scripted(ct_app *app, const char *path);

#endif
