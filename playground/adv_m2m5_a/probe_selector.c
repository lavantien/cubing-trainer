#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "gen/score.h"
#include "trainer/selector.h"

static void selector_dump(void) {
    ct_event events[32];
    int n = ct_selector_events(events, 32);
    printf("events=%d\n", n);
    int total_lists = 0;
    int dead_lists = 0;
    int reachable_cases = 0;
    for (int i = 0; i < n; i++) {
        ct_method methods[16];
        int m = ct_selector_methods(events[i], methods, 16);
        for (int j = 0; j < m; j++) {
            ct_stage stages[16];
            ct_orbit orbits[16];
            int ids[16];
            int lists = ct_selector_lists(events[i], methods[j], ids, stages, orbits, 16);
            printf("event=%d method=%d lists=%d:", events[i], methods[j], lists);
            int cases_here = 0;
            int dead_here = 0;
            for (int k = 0; k < lists; k++) {
                int count = ct_selector_case_count(events[i], methods[j], k);
                printf(" [stage=%d orbit=%d cases=%d]", stages[k], orbits[k], count);
                if (count <= 0) {
                    dead_here++;
                } else {
                    cases_here += count;
                }
            }
            printf(" dead=%d cases=%d\n", dead_here, cases_here);
            total_lists += lists;
            dead_lists += dead_here;
            reachable_cases += cases_here;
            for (int k = 0; k < lists; k++) {
                char key[8];
                int count = ct_selector_case_count(events[i], methods[j], k);
                for (int idx = 0; idx < count && idx < 3; idx++) {
                    if (ct_selector_case_key(events[i], methods[j], k, idx, key,
                                             sizeof key) == 0) {
                        printf("  key ev=%d m=%d list=%d idx=%d -> %s\n", events[i], methods[j],
                               k, idx, key);
                        break;
                    }
                }
            }
        }
    }
    printf("total_lists=%d dead_lists=%d reachable_cases=%d\n", total_lists, dead_lists,
           reachable_cases);
}

static void score_edges(void) {
    ct_config cfg = ct_config_defaults();
    ct_move seq[3] = {{0, 1, 1, 1}, {2, 1, 1, 2}, {4, 1, 1, 3}};
    ct_score_parts p;
    printf("set penalty_R nan: %d\n", ct_config_set(&cfg, "gen.penalty_R", "nan"));
    printf("set penalty_R inf: %d\n", ct_config_set(&cfg, "gen.penalty_R", "inf"));
    printf("set weight_len nan: %d\n", ct_config_set(&cfg, "gen.weight_len", "nan"));
    printf("set weight_finger -1: %d\n", ct_config_set(&cfg, "gen.weight_finger", "-1"));
    printf("set request_retention nan: %d\n", ct_config_set(&cfg, "srs.request_retention", "nan"));
    printf("set request_retention 0: %d\n", ct_config_set(&cfg, "srs.request_retention", "0"));
    printf("set request_retention inf: %d\n", ct_config_set(&cfg, "srs.request_retention", "inf"));
    printf("set preferred_moves 0: %d\n", ct_config_set(&cfg, "gen.preferred_moves", "0"));
    printf("set preferred_moves ffffffff: %d\n",
           ct_config_set(&cfg, "gen.preferred_moves", "ffffffff"));
    ct_score_alg(seq, 3, &cfg, &p);
    printf("score with cfg after sets: qtm=%d subset=%f regrip=%d finger=%f total=%f\n", p.qtm,
           p.subset_penalty, p.regrips, p.finger_penalty, p.total);
    char json[128];
    int jrc = ct_score_parts_json(&p, json, sizeof json);
    printf("json rc=%d json=%s\n", jrc, jrc >= 0 ? json : "(rejected)");
    ct_config clean = ct_config_defaults();
    ct_score_alg(seq, 3, &clean, &p);
    printf("clean defaults: qtm=%d subset=%f regrip=%d finger=%f total=%f\n", p.qtm,
           p.subset_penalty, p.regrips, p.finger_penalty, p.total);
    ct_move seq2[5] = {{0, 1, 1, 1}, {2, 1, 1, 1}, {0, 1, 1, 3}, {5, 1, 1, 1}, {2, 1, 1, 1}};
    ct_score_alg(seq2, 5, &clean, &p);
    printf("U F U' D F regrips=%d\n", p.regrips);
    ct_move seq3[2] = {{2, 1, 1, 1}, {0, 1, 1, 1}};
    ct_score_alg(seq3, 2, &clean, &p);
    printf("F U regrips=%d\n", p.regrips);
    ct_move seq4[3] = {{2, 1, 1, 1}, {0, 1, 1, 1}, {2, 1, 1, 3}};
    ct_score_alg(seq4, 3, &clean, &p);
    printf("F U F' regrips=%d\n", p.regrips);
    ct_move seq5[2] = {{2, 1, 1, 1}, {2, 1, 1, 1}};
    ct_score_alg(seq5, 2, &clean, &p);
    printf("F F regrips=%d\n", p.regrips);
    ct_move seq6[4] = {{2, 1, 1, 1}, {2, 1, 1, 1}, {2, 1, 1, 1}, {2, 1, 1, 1}};
    ct_score_alg(seq6, 4, &clean, &p);
    printf("F F F F regrips=%d\n", p.regrips);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    selector_dump();
    score_edges();
    return 0;
}
