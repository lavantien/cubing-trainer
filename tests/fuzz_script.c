#include <string.h>

#include "../src/app/script.h"
#include "framework.h"

int ct_fuzz_iterations(void);

enum { CT_SF_TEXT_MAX = CT_SCRIPT_TEXT_MAX / 2, CT_SF_ALPHABET = 96, CT_SF_NEWLINE_ODDS = 16 };

CT_SUITE(fuzz_script_parse) {
    unsigned long long seed = 1;
    for (int i = 0; i < ct_fuzz_iterations(); i++) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        ct_sfc64 s;
        ct_sfc64_init(&s, seed);
        char text[CT_SF_TEXT_MAX];
        unsigned at = 0;
        while (at + 1 < sizeof text) {
            text[at++] = (char)(' ' + (int)ct_sfc64_range(&s, CT_SF_ALPHABET));
            if (at + 1 < sizeof text && ct_sfc64_range(&s, CT_SF_NEWLINE_ODDS) == 0) {
                text[at++] = '\n';
            }
        }
        text[at] = '\0';
        ct_script script;
        char err[CT_SCRIPT_ERR_MAX];
        int rc = ct_script_parse(text, &script, err, (unsigned)sizeof err);
        if (rc == 0) {
            CT_ASSERT(script.count > 0);
            CT_ASSERT(script.count <= CT_SCRIPT_CMDS_MAX);
            for (int k = 0; k < script.count; k++) {
                CT_ASSERT(script.cmds[k].line >= 1);
                CT_ASSERT(script.cmds[k].argc >= 0);
                for (int a = 0; a < script.cmds[k].argc; a++) {
                    const char *arg = ct_script_arg(&script, &script.cmds[k], a);
                    CT_ASSERT(arg != NULL);
                    CT_ASSERT(arg >= script.text);
                    CT_ASSERT(arg < script.text + sizeof script.text);
                    CT_ASSERT(strlen(arg) == script.cmds[k].len[a]);
                }
            }
        } else {
            CT_ASSERT_EQ(rc, CT_SCRIPT_EXIT_PARSE);
            CT_ASSERT(err[0] != '\0');
        }
        ct_count_pass();
    }
}
