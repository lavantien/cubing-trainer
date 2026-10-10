#include "engine/notation.h"
#include "gen/classify.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        ct_classify_result r;
        memset(&r, 0, sizeof r);
        int rc = ct_classify_alg(argv[i], &r);
        printf("alg=%s rc=%d cls=%s moves=%d setup_len=%d inter_len=%d insert_len=%d\n", argv[i],
               rc, ct_classification_label(r.cls), r.moves, r.setup_len, r.interchange_len,
               r.insertion_len);
    }
    return 0;
}
