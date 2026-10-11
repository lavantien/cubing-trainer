#include "config/config.h"
#include "engine/cube.h"
#include "engine/notation.h"
#include "gen/score.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void show(const char *tag, const ct_move *seq, int count) {
    ct_config c = ct_config_defaults();
    ct_score_parts p;
    int rc = ct_score_alg(seq, count, &c, &p);
    char js[512];
    int jrc = ct_score_parts_json(&p, js, sizeof js);
    printf("%s rc=%d qtm=%d subset=%.4f regrip=%d finger=%.4f total=%.4f json_rc=%d js=%s\n", tag,
           rc, p.qtm, p.subset_penalty, p.regrips, p.finger_penalty, p.total, jrc, js);
}

int main(void) {
    ct_move m;
    ct_move seq[8];
    int n = ct_notation_parse("M", 3, &m, 1);
    printf("parse M rc=%d face=%d quarter=%d\n", n, m.face, m.quarter);
    seq[0] = m;
    show("M as move", seq, 1);

    n = ct_notation_parse("x y' z2", 3, seq, 8);
    printf("parse xyz rc=%d\n", n);
    show("x y' z2", seq, 3);

    ct_config c2 = ct_config_defaults();
    printf("set MES rc=%d\n", ct_config_set(&c2, "gen.preferred_moves", "MES"));
    printf("mask=%u\n", c2.gen.preferred_moves);
    ct_score_parts p;
    n = ct_notation_parse("M", 3, &m, 1);
    seq[0] = m;
    ct_score_alg(seq, 1, &c2, &p);
    printf("M under MES mask subset=%.4f\n", p.subset_penalty);

    ct_config big = ct_config_defaults();
    printf("set weight_len 1e308 rc=%d\n", ct_config_set(&big, "gen.weight_len", "1e308"));
    n = ct_notation_parse("R U R' U'", 3, seq, 8);
    printf("parse rc=%d\n", n);
    ct_score_alg(seq, 4, &big, &p);
    char js[512];
    int jrc = ct_score_parts_json(&p, js, sizeof js);
    printf("inf case total=%f json_rc=%d js=%s\n", p.total, jrc, js);

    ct_score_parts q = {1, 0.1, 2, 2.6, 1e18};
    char b[512];
    int r = ct_score_parts_json(&q, b, sizeof b);
    printf("fmt probe rc=%d js=%s\n", r, b);
    ct_score_parts neg = {0, -0.00001, 0, 0.0, 0.0};
    r = ct_score_parts_json(&neg, b, sizeof b);
    printf("tiny neg rc=%d js=%s\n", r, b);

    ct_move weird[4];
    weird[0] = (ct_move){CT_FACE_R, 1, 1, 0};
    weird[1] = (ct_move){CT_FACE_U, 1, 1, 4};
    weird[2] = (ct_move){CT_FACE_F, 1, 1, -1};
    weird[3] = (ct_move){CT_FACE_L, 1, 1, 2147483647};
    show("quarter 0/4/-1/max", weird, 4);

    ct_move bad[1] = {{6, 1, 1, 1}};
    printf("face 6 rc=%d\n", ct_score_alg(bad, 1, &c2, &p));
    printf("neg count rc=%d\n", ct_score_alg(seq, -3, &c2, &p));

    for (int i = 0; i < 6; i++) {
        char key[32];
        snprintf(key, sizeof key, "gen.finger_cost_%c_cw", "UDLRFB"[i]);
        ct_config cc = ct_config_defaults();
        printf("key %s rc=%d row=%d cost=%.2f\n", key, ct_config_set(&cc, key, "1.5"), i,
               cc.gen.finger_cost[i][0]);
    }
    return 0;
}
