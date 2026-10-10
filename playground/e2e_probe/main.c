#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config/config.h"
#include "engine/notation.h"
#include "gen/classify.h"
#include "gen/score.h"
#include "gen/synth.h"
#include "storage/repo_algs.h"
#include "storage/repo_cards.h"

static double probe_seconds(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

int main(void) {
    ct_config cfg = ct_config_defaults();
    double t0 = probe_seconds();
    int size = ct_synth_init();
    double t1 = probe_seconds();
    printf("synth init size %d seconds %.3f\n", size, t1 - t0);

    ct_synth_result res;
    memset(&res, 0, sizeof res);
    double t2 = probe_seconds();
    int rc = ct_synth_target(CT_ORBIT_CORNER, 0, 1, 2, &cfg, &res);
    double t3 = probe_seconds();
    printf("corner BC rc %d found %d len %d qtm %d seconds %.3f\n", rc, res.found, res.len,
           res.qtm, t3 - t2);
    if (rc != 0 || !res.found) {
        return 1;
    }
    char seq[CT_CARD_ALG_MAX];
    ct_notation_format(res.seq, res.len, 3, seq, (unsigned)sizeof seq);
    printf("sequence [%s]\n", seq);
    ct_classify_result cls;
    memset(&cls, 0, sizeof cls);
    int crc = ct_classify(res.seq, res.len, &cls);
    printf("classify rc %d label %s moves %d qtm %d\n", crc, ct_classification_label(cls.cls),
           cls.moves, cls.qtm);
    char setup[CT_CARD_ALG_MAX];
    char inter[CT_CARD_ALG_MAX];
    char insert[CT_CARD_ALG_MAX];
    ct_notation_format(cls.setup, cls.setup_len, 3, setup, (unsigned)sizeof setup);
    ct_notation_format(cls.interchange, cls.interchange_len, 3, inter, (unsigned)sizeof inter);
    ct_notation_format(cls.insertion, cls.insertion_len, 3, insert, (unsigned)sizeof insert);
    printf("parts setup [%s] interchange [%s] insertion [%s]\n", setup, inter, insert);
    ct_score_parts parts;
    memset(&parts, 0, sizeof parts);
    int src = ct_score_alg(res.seq, res.len, &cfg, &parts);
    char js[CT_ALG_PARTS_MAX];
    ct_score_parts_json(&parts, js, (unsigned)sizeof js);
    printf("score rc %d qtm %d subset %g regrips %d finger %g total %g json %s\n", src, parts.qtm,
           parts.subset_penalty, parts.regrips, parts.finger_penalty, parts.total, js);
    return 0;
}
