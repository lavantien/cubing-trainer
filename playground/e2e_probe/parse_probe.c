#include <stdio.h>
#include <string.h>

#include "app/script.h"

int main(void) {
    static const char text[] = "# leading comment\n"
                               "\n"
                               "restart\n"
                               "   # indented comment\n"
                               "cards_field notes \"two words\" \n";
    ct_script s;
    char err[CT_SCRIPT_ERR_MAX];
    int rc = ct_script_parse(text, &s, err, sizeof err);
    printf("rc %d err [%s] count %d\n", rc, err, s.count);
    for (int i = 0; i < s.count; i++) {
        printf("cmd %d kind %d argc %d line %d:", i, (int)s.cmds[i].kind, s.cmds[i].argc,
               s.cmds[i].line);
        for (int a = 0; a < s.cmds[i].argc; a++) {
            printf(" [%s]", ct_script_arg(&s, &s.cmds[i], a));
        }
        printf("\n");
    }
    static const char two[] = "restart\nsettings_open\n";
    rc = ct_script_parse(two, &s, err, sizeof err);
    printf("rc %d err [%s] count %d\n", rc, err, s.count);
    static const char nope[] = "nope unfiled\n";
    rc = ct_script_parse(nope, &s, err, sizeof err);
    printf("rc %d err [%s]\n", rc, err);
    static const char review[] =
        "cards_open unfiled\n"
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
        "expect srs unfiled new-1 due == today\n"
        "expect reviews unfiled new-1 == 3,4\n"
        "expect count session_log == 2\n";
    rc = ct_script_parse(review, &s, err, sizeof err);
    printf("rc %d err [%s] count %d\n", rc, err, s.count);
    return 0;
}
