#include <stdio.h>
#include <string.h>

#include "engine/notation.h"
#include "gen/classify.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        return 2;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        return 2;
    }
    char line[512];
    while (fgets(line, sizeof line, f) != NULL) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = '\0';
        }
        char *bar = strchr(line, '|');
        const char *want = "";
        if (bar != NULL) {
            *bar = '\0';
            want = bar + 1;
        }
        ct_classify_result r;
        int rc = ct_classify_alg(line, &r);
        if (rc != CT_CLASSIFY_OK) {
            printf("alg=%s rc=%d\n", line, rc);
            continue;
        }
        char setup[160];
        char inter[160];
        char insert[160];
        ct_notation_format(r.setup, r.setup_len, 3, setup, sizeof setup);
        ct_notation_format(r.interchange, r.interchange_len, 3, inter, sizeof inter);
        ct_notation_format(r.insertion, r.insertion_len, 3, insert, sizeof insert);
        printf("label=%s alias=%s want=%s moves=%d qtm=%d setup=[%s] inter=[%s] insert=[%s]"
               " cyclic=%d\n",
               ct_classification_label(r.cls), ct_classification_alias(r.cls), want, r.moves,
               r.qtm, setup, inter, insert, r.cyclic);
    }
    fclose(f);
    return 0;
}
