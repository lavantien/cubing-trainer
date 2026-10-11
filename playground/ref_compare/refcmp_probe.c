#include "config/config.h"
#include "engine/notation.h"
#include "gen/classify.h"
#include "gen/score.h"
#include "gen/synth.h"
#include "gen/targets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int run_score(void) {
    char line[4096];
    printf("id\tparse_rc\tn_tokens\tn_rot\tn_slice\tn_face\tn_wide\tqtm_probe\tscore_qtm\tsubset"
           "\tregrip\tfinger\ttotal\tcls_rc\tcls\tsetup\tinter\tinsert\tinsert_first\tcyclic\tcls_"
           "moves\tcls_qtm\tfaces\n");
    while (fgets(line, sizeof line, stdin)) {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = '\0';
        char *tab = strchr(line, '\t');
        if (!tab)
            continue;
        *tab = '\0';
        char *tab2 = strchr(tab + 1, '\t');
        if (!tab2)
            continue;
        *tab2 = '\0';
        const char *id = line;
        int orbit = atoi(tab + 1);
        const char *alg = tab2 + 1;
        ct_move seq[64];
        int n = ct_notation_parse(alg, 3, seq, 64);
        if (n < 0) {
            printf("%s\t%d\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\n", id, n);
            continue;
        }
        int n_rot = 0;
        int n_slice = 0;
        int n_face = 0;
        int n_wide = 0;
        for (int i = 0; i < n; i++) {
            if (seq[i].depth_from == 1 && seq[i].depth_to == 3)
                n_rot++;
            else if (seq[i].depth_from == 2 && seq[i].depth_to == 2)
                n_slice++;
            else if (seq[i].depth_to > seq[i].depth_from)
                n_wide++;
            else
                n_face++;
        }
        int qtm = ct_count_qtm(seq, n);
        ct_config cfg = ct_config_defaults();
        ct_score_parts parts;
        int src_rc = ct_score_alg(seq, n, &cfg, orbit, &parts);
        if (src_rc != CT_SCORE_OK) {
            printf("%s\tSCORE_RC_%d\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\n", id, src_rc);
            continue;
        }
        char faces[65];
        static const char face_letters[CT_CUBE_FACE_COUNT] = {
            [CT_FACE_U] = 'U', [CT_FACE_L] = 'L', [CT_FACE_F] = 'F',
            [CT_FACE_R] = 'R', [CT_FACE_B] = 'B', [CT_FACE_D] = 'D',
        };
        for (int i = 0; i < n && i < 64; i++) {
            faces[i] = face_letters[seq[i].face];
            if (seq[i].depth_from == CT_GEN_SLICE_DEPTH && seq[i].depth_to == CT_GEN_SLICE_DEPTH) {
                if (seq[i].face == CT_FACE_L)
                    faces[i] = 'M';
                else if (seq[i].face == CT_FACE_D)
                    faces[i] = 'E';
                else if (seq[i].face == CT_FACE_F)
                    faces[i] = 'S';
            }
        }
        faces[n > 64 ? 64 : n] = '\0';
        ct_classify_result cls;
        memset(&cls, 0, sizeof cls);
        int cls_rc = ct_classify_alg(alg, &cls);
        if (cls_rc < 0) {
            printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.4f\t%d\t%.4f\t%.4f\t%d\t\t\t\t\t\t\t\t"
                   "\t\t%s\n",
                   id, n, n, n_rot, n_slice, n_face, n_wide, qtm, parts.qtm, parts.subset_penalty,
                   parts.regrips, parts.finger_penalty, parts.total, cls_rc, faces);
            continue;
        }
        printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.4f\t%d\t%.4f\t%.4f\t%d\t%s\t%d\t%d\t%d\t%d"
               "\t%d\t%d\t%d\t%s\n",
               id, n, n, n_rot, n_slice, n_face, n_wide, qtm, parts.qtm, parts.subset_penalty,
               parts.regrips, parts.finger_penalty, parts.total, cls_rc,
               ct_classification_label(cls.cls), cls.setup_len, cls.interchange_len,
               cls.insertion_len, cls.insert_first ? 1 : 0, cls.cyclic ? 1 : 0, cls.moves,
               cls.qtm, faces);
    }
    return 0;
}

static int run_synth(void) {
    if (ct_synth_init() < 0) {
        printf("SYNTH_INIT_FAIL\n");
        return 1;
    }
    ct_config cfg = ct_config_defaults();
    char line[256];
    printf("orbit\tt1\tt2\trc\tfound\tsetup\tfallback\tlen\tqtm\talg\n");
    while (fgets(line, sizeof line, stdin)) {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = '\0';
        int orbit;
        int t1;
        int t2;
        if (sscanf(line, "%d\t%d\t%d", &orbit, &t1, &t2) != 3)
            continue;
        int buffer = orbit == CT_ORBIT_CORNER ? CT_3STYLE_CORNER_BUFFER : CT_3STYLE_EDGE_BUFFER;
        ct_synth_result res;
        memset(&res, 0, sizeof res);
        int rc = ct_synth_target(orbit, buffer, t1, t2, &cfg, &res);
        char text[256] = "";
        if (rc == 0 && res.found) {
            if (ct_notation_format(res.seq, res.len, 3, text, (unsigned)sizeof text) < 0)
                snprintf(text, sizeof text, "FMT_FAIL");
        }
        printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n", orbit, t1, t2, rc, res.found ? 1 : 0,
               res.used_setup ? 1 : 0, res.used_fallback ? 1 : 0, res.len, res.qtm, text);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: refcmp_probe score|synth\n");
        return 1;
    }
    if (strcmp(argv[1], "score") == 0)
        return run_score();
    if (strcmp(argv[1], "synth") == 0)
        return run_synth();
    fprintf(stderr, "unknown mode %s\n", argv[1]);
    return 1;
}
