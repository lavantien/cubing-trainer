#include "framework.h"
#include "gen/score.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static ct_move mv(int face, int quarter) {
    ct_move m = {face, 1, 1, quarter};
    return m;
}

static int t_bit_index(unsigned bit) {
    for (int i = 0; i < CT_FACE_COUNT; i++) {
        if ((1u << i) == bit) {
            return i;
        }
    }
    return -1;
}

static unsigned t_face_bit(int face) {
    switch (face) {
    case CT_FACE_U:
        return ct_move_face_from_char('U');
    case CT_FACE_L:
        return ct_move_face_from_char('L');
    case CT_FACE_F:
        return ct_move_face_from_char('F');
    case CT_FACE_R:
        return ct_move_face_from_char('R');
    case CT_FACE_B:
        return ct_move_face_from_char('B');
    case CT_FACE_D:
        return ct_move_face_from_char('D');
    default:
        return 0;
    }
}

static double t_finger_cost(const ct_config *c, int face, int dir) {
    return c->gen.finger_cost[t_bit_index(t_face_bit(face))][dir];
}

static double t_subset_of(const ct_config *c, int face) {
    unsigned bit = t_face_bit(face);
    if ((c->gen.preferred_moves & bit) != 0) {
        return 0.0;
    }
    return c->gen.penalty[t_bit_index(bit)];
}

CT_SUITE(score_qtm) {
    ct_move r = mv(CT_FACE_R, CT_SCORE_QUARTER_CW);
    ct_move r2 = mv(CT_FACE_R, CT_SCORE_QUARTER_DOUBLE);
    ct_move rp = mv(CT_FACE_R, CT_SCORE_QUARTER_CCW);
    CT_ASSERT_EQ(ct_count_qtm(&r, 1), 1);
    CT_ASSERT_EQ(ct_count_qtm(&r2, 1), 2);
    CT_ASSERT_EQ(ct_count_qtm(&rp, 1), 1);

    ct_move mixed[4] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_U, CT_SCORE_QUARTER_DOUBLE),
                        mv(CT_FACE_F, CT_SCORE_QUARTER_CCW), mv(CT_FACE_L, CT_SCORE_QUARTER_CW)};
    CT_ASSERT_EQ(ct_count_qtm(mixed, 4), 5);

    ct_move long_seq[5] = {mv(CT_FACE_U, CT_SCORE_QUARTER_CW), mv(CT_FACE_D, CT_SCORE_QUARTER_CCW),
                           mv(CT_FACE_L, CT_SCORE_QUARTER_CW),
                           mv(CT_FACE_R, CT_SCORE_QUARTER_DOUBLE),
                           mv(CT_FACE_B, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_count_qtm(long_seq, 5), 6);

    CT_ASSERT_EQ(ct_count_qtm(long_seq, 0), 0);
    CT_ASSERT_EQ(ct_count_qtm(NULL, 0), 0);
}

CT_SUITE(score_subset) {
    ct_config c = ct_config_defaults();
    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "RUD"), 0);

    ct_move seq[3] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_L, CT_SCORE_QUARTER_CW),
                      mv(CT_FACE_F, CT_SCORE_QUARTER_CW)};
    ct_score_parts p;
    CT_ASSERT_EQ(ct_score_alg(seq, 3, &c, &p), CT_SCORE_OK);
    double want = t_subset_of(&c, CT_FACE_L) + t_subset_of(&c, CT_FACE_F);
    CT_ASSERT_DBL_EQ(p.subset_penalty, want, 1e-12);

    ct_move preferred[3] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW),
                            mv(CT_FACE_U, CT_SCORE_QUARTER_DOUBLE),
                            mv(CT_FACE_D, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_score_alg(preferred, 3, &c, &p), CT_SCORE_OK);
    CT_ASSERT_DBL_EQ(p.subset_penalty, 0.0, 1e-12);

    CT_ASSERT_EQ(ct_config_set(&c, "gen.preferred_moves", "RU"), 0);
    CT_ASSERT_EQ(ct_score_alg(seq, 3, &c, &p), CT_SCORE_OK);
    want = t_subset_of(&c, CT_FACE_L) + t_subset_of(&c, CT_FACE_F);
    CT_ASSERT_DBL_EQ(p.subset_penalty, want, 1e-12);

    CT_ASSERT_EQ(ct_score_alg(seq, 0, &c, &p), CT_SCORE_OK);
    CT_ASSERT_DBL_EQ(p.subset_penalty, 0.0, 1e-12);
}

CT_SUITE(score_finger) {
    ct_config c = ct_config_defaults();
    ct_score_parts p;

    int faces[6] = {CT_FACE_U, CT_FACE_D, CT_FACE_L, CT_FACE_R, CT_FACE_F, CT_FACE_B};
    for (int i = 0; i < 6; i++) {
        ct_move cw = mv(faces[i], CT_SCORE_QUARTER_CW);
        CT_ASSERT_EQ(ct_score_alg(&cw, 1, &c, &p), CT_SCORE_OK);
        CT_ASSERT_DBL_EQ(p.finger_penalty, t_finger_cost(&c, faces[i], CT_FINGER_DIR_CW), 1e-12);

        ct_move ccw = mv(faces[i], CT_SCORE_QUARTER_CCW);
        CT_ASSERT_EQ(ct_score_alg(&ccw, 1, &c, &p), CT_SCORE_OK);
        CT_ASSERT_DBL_EQ(p.finger_penalty, t_finger_cost(&c, faces[i], CT_FINGER_DIR_CCW), 1e-12);

        ct_move dbl = mv(faces[i], CT_SCORE_QUARTER_DOUBLE);
        CT_ASSERT_EQ(ct_score_alg(&dbl, 1, &c, &p), CT_SCORE_OK);
        CT_ASSERT_DBL_EQ(p.finger_penalty, t_finger_cost(&c, faces[i], CT_FINGER_DIR_CW), 1e-12);
    }

    CT_ASSERT_EQ(ct_config_set(&c, "gen.finger_cost_U_cw", "0.1"), 0);
    CT_ASSERT_EQ(ct_config_set(&c, "gen.finger_cost_U_ccw", "0.3"), 0);
    ct_move u = mv(CT_FACE_U, CT_SCORE_QUARTER_CW);
    ct_move up = mv(CT_FACE_U, CT_SCORE_QUARTER_CCW);
    ct_move u2 = mv(CT_FACE_U, CT_SCORE_QUARTER_DOUBLE);
    CT_ASSERT_EQ(ct_score_alg(&u, 1, &c, &p), CT_SCORE_OK);
    CT_ASSERT_DBL_EQ(p.finger_penalty, 0.1, 1e-12);
    CT_ASSERT_EQ(ct_score_alg(&up, 1, &c, &p), CT_SCORE_OK);
    CT_ASSERT_DBL_EQ(p.finger_penalty, 0.3, 1e-12);
    CT_ASSERT_EQ(ct_score_alg(&u2, 1, &c, &p), CT_SCORE_OK);
    CT_ASSERT_DBL_EQ(p.finger_penalty, 0.1, 1e-12);

    ct_move seq[3] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_U, CT_SCORE_QUARTER_CCW),
                      mv(CT_FACE_F, CT_SCORE_QUARTER_DOUBLE)};
    CT_ASSERT_EQ(ct_score_alg(seq, 3, &c, &p), CT_SCORE_OK);
    double want = t_finger_cost(&c, CT_FACE_R, CT_FINGER_DIR_CW) + 0.3 +
                  t_finger_cost(&c, CT_FACE_F, CT_FINGER_DIR_CW);
    CT_ASSERT_DBL_EQ(p.finger_penalty, want, 1e-12);
}

CT_SUITE(score_regrips) {
    ct_move trigger[4] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_U, CT_SCORE_QUARTER_CW),
                          mv(CT_FACE_R, CT_SCORE_QUARTER_CCW), mv(CT_FACE_U, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_count_regrips(trigger, 4), 0);

    ct_move ends_f[4] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_U, CT_SCORE_QUARTER_CW),
                         mv(CT_FACE_R, CT_SCORE_QUARTER_CCW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW)};
    CT_ASSERT_EQ(ct_count_regrips(ends_f, 4), 0);

    ct_move after_f[3] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW),
                          mv(CT_FACE_R, CT_SCORE_QUARTER_CW)};
    CT_ASSERT_EQ(ct_count_regrips(after_f, 3), 1);

    ct_move left_f[2] = {mv(CT_FACE_L, CT_SCORE_QUARTER_CW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW)};
    CT_ASSERT_EQ(ct_count_regrips(left_f, 2), 0);

    ct_move left_ff[3] = {mv(CT_FACE_L, CT_SCORE_QUARTER_CW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW),
                          mv(CT_FACE_F, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_count_regrips(left_ff, 3), 0);

    ct_move ff[2] = {mv(CT_FACE_F, CT_SCORE_QUARTER_CW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW)};
    CT_ASSERT_EQ(ct_count_regrips(ff, 2), 0);

    ct_move ffff[4] = {mv(CT_FACE_F, CT_SCORE_QUARTER_CW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW),
                       mv(CT_FACE_F, CT_SCORE_QUARTER_CCW), mv(CT_FACE_F, CT_SCORE_QUARTER_DOUBLE)};
    CT_ASSERT_EQ(ct_count_regrips(ffff, 4), 0);

    ct_move rffr[4] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW),
                       mv(CT_FACE_F, CT_SCORE_QUARTER_CCW), mv(CT_FACE_R, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_count_regrips(rffr, 4), 1);

    ct_move left_home[3] = {mv(CT_FACE_L, CT_SCORE_QUARTER_CW), mv(CT_FACE_F, CT_SCORE_QUARTER_CW),
                            mv(CT_FACE_L, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_count_regrips(left_home, 3), 1);

    ct_move fb[3] = {mv(CT_FACE_F, CT_SCORE_QUARTER_CW), mv(CT_FACE_B, CT_SCORE_QUARTER_CW),
                     mv(CT_FACE_F, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_count_regrips(fb, 3), 2);

    ct_move sides[4] = {mv(CT_FACE_L, CT_SCORE_QUARTER_CW), mv(CT_FACE_R, CT_SCORE_QUARTER_CW),
                        mv(CT_FACE_L, CT_SCORE_QUARTER_CCW), mv(CT_FACE_R, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_count_regrips(sides, 4), 0);

    ct_move d_only[3] = {mv(CT_FACE_D, CT_SCORE_QUARTER_CW), mv(CT_FACE_D, CT_SCORE_QUARTER_CCW),
                         mv(CT_FACE_D, CT_SCORE_QUARTER_DOUBLE)};
    CT_ASSERT_EQ(ct_count_regrips(d_only, 3), 0);

    CT_ASSERT_EQ(ct_count_regrips(sides, 0), 0);
    CT_ASSERT_EQ(ct_count_regrips(NULL, 0), 0);
}

CT_SUITE(score_total) {
    ct_config c = ct_config_defaults();
    ct_score_parts p;

    ct_move seq[6] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW),  mv(CT_FACE_L, CT_SCORE_QUARTER_CW),
                      mv(CT_FACE_F, CT_SCORE_QUARTER_CW),  mv(CT_FACE_F, CT_SCORE_QUARTER_CW),
                      mv(CT_FACE_U, CT_SCORE_QUARTER_CCW), mv(CT_FACE_L, CT_SCORE_QUARTER_CCW)};
    CT_ASSERT_EQ(ct_score_alg(seq, 6, &c, &p), CT_SCORE_OK);
    CT_ASSERT_EQ(p.qtm, 6);
    double subset = 2 * t_subset_of(&c, CT_FACE_L) + 2 * t_subset_of(&c, CT_FACE_F);
    CT_ASSERT_DBL_EQ(p.subset_penalty, subset, 1e-12);
    CT_ASSERT_EQ(p.regrips, 1);
    double finger = t_finger_cost(&c, CT_FACE_R, CT_FINGER_DIR_CW) +
                    t_finger_cost(&c, CT_FACE_L, CT_FINGER_DIR_CW) +
                    2 * t_finger_cost(&c, CT_FACE_F, CT_FINGER_DIR_CW) +
                    t_finger_cost(&c, CT_FACE_U, CT_FINGER_DIR_CCW) +
                    t_finger_cost(&c, CT_FACE_L, CT_FINGER_DIR_CCW);
    CT_ASSERT_DBL_EQ(p.finger_penalty, finger, 1e-12);
    double total = c.gen.weight_len * 6 + c.gen.weight_subset * subset + c.gen.weight_regrip * 1 +
                   c.gen.weight_finger * finger;
    CT_ASSERT_DBL_EQ(p.total, total, 1e-9);
    double base_total = p.total;

    CT_ASSERT_EQ(ct_score_alg(seq, 0, &c, &p), CT_SCORE_OK);
    CT_ASSERT_EQ(p.qtm, 0);
    CT_ASSERT_EQ(p.regrips, 0);
    CT_ASSERT_DBL_EQ(p.subset_penalty, 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(p.finger_penalty, 0.0, 1e-12);
    CT_ASSERT_DBL_EQ(p.total, 0.0, 1e-12);

    const char *keys[4] = {"gen.weight_len", "gen.weight_subset", "gen.weight_regrip",
                           "gen.weight_finger"};
    double comps[4] = {6, subset, 1, finger};
    char buf[16];
    double delta = 0.25;
    for (int w = 0; w < 4; w++) {
        ct_config c2 = c;
        double orig = w == 0   ? c.gen.weight_len
                      : w == 1 ? c.gen.weight_subset
                      : w == 2 ? c.gen.weight_regrip
                               : c.gen.weight_finger;
        snprintf(buf, sizeof buf, "%.2f", orig + delta);
        CT_ASSERT_EQ(ct_config_set(&c2, keys[w], buf), 0);
        ct_score_parts q;
        CT_ASSERT_EQ(ct_score_alg(seq, 6, &c2, &q), CT_SCORE_OK);
        CT_ASSERT_DBL_EQ(q.total - base_total, delta * comps[w], 1e-9);
    }
}

CT_SUITE(score_json) {
    char out[128];
    ct_score_parts p = {5, 0.0, 1, 0.6, 6.4};
    const char *full =
        "{\"qtm\":5,\"subset\":0.0000,\"regrip\":1,\"finger\":0.6000,\"total\":6.4000}";
    CT_ASSERT_EQ(ct_score_parts_json(&p, out, sizeof out), (int)strlen(full));
    CT_ASSERT_STR_EQ(out, full);

    ct_score_parts n = {2, -1.5, 0, 0.0, 0.25};
    const char *neg =
        "{\"qtm\":2,\"subset\":-1.5000,\"regrip\":0,\"finger\":0.0000,\"total\":0.2500}";
    CT_ASSERT_EQ(ct_score_parts_json(&n, out, sizeof out), (int)strlen(neg));
    CT_ASSERT_STR_EQ(out, neg);

    ct_score_parts z = {0, 0.0, 0, 0.0, 0.0};
    const char *zero =
        "{\"qtm\":0,\"subset\":0.0000,\"regrip\":0,\"finger\":0.0000,\"total\":0.0000}";
    CT_ASSERT_EQ(ct_score_parts_json(&z, out, sizeof out), (int)strlen(zero));
    CT_ASSERT_STR_EQ(out, zero);

    unsigned need = (unsigned)strlen(full) + 1;
    CT_ASSERT_EQ(ct_score_parts_json(&p, out, need), (int)strlen(full));
    CT_ASSERT_STR_EQ(out, full);
    CT_ASSERT_EQ(ct_score_parts_json(&p, out, need - 1), CT_SCORE_E_CAP);
    CT_ASSERT_EQ(ct_score_parts_json(&p, out, 8), CT_SCORE_E_CAP);

    ct_score_parts inf = {4, 0.0, 0, 0.0, INFINITY};
    CT_ASSERT_EQ(ct_score_parts_json(&inf, out, sizeof out), CT_SCORE_E_ARG);
    ct_score_parts nanp = {4, 0.0, 0, NAN, 0.0};
    CT_ASSERT_EQ(ct_score_parts_json(&nanp, out, sizeof out), CT_SCORE_E_ARG);
    ct_score_parts inf_subset = {4, INFINITY, 0, 0.0, 0.0};
    CT_ASSERT_EQ(ct_score_parts_json(&inf_subset, out, sizeof out), CT_SCORE_E_ARG);

    ct_config c = ct_config_defaults();
    ct_score_parts live;
    ct_move seq[2] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW), mv(CT_FACE_U, CT_SCORE_QUARTER_DOUBLE)};
    CT_ASSERT_EQ(ct_score_alg(seq, 2, &c, &live), CT_SCORE_OK);
    const char *live_str =
        "{\"qtm\":3,\"subset\":0.0000,\"regrip\":0,\"finger\":0.6000,\"total\":3.3000}";
    CT_ASSERT_EQ(ct_score_parts_json(&live, out, sizeof out), (int)strlen(live_str));
    CT_ASSERT_STR_EQ(out, live_str);
}

CT_SUITE(score_errors) {
    ct_config c = ct_config_defaults();
    ct_score_parts p;
    ct_move bad_face_lo[1] = {mv(-1, CT_SCORE_QUARTER_CW)};
    ct_move bad_face_hi[1] = {mv(6, CT_SCORE_QUARTER_CW)};
    ct_move bad_quarter_lo[1] = {mv(CT_FACE_R, 0)};
    ct_move bad_quarter_hi[1] = {mv(CT_FACE_R, CT_MOVE_QUARTERS)};
    ct_move ok[1] = {mv(CT_FACE_R, CT_SCORE_QUARTER_CW)};

    CT_ASSERT_EQ(ct_count_qtm(NULL, 1), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_count_qtm(ok, -1), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_count_qtm(bad_face_lo, 1), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_count_qtm(bad_face_hi, 1), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_count_qtm(bad_quarter_lo, 1), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_count_qtm(bad_quarter_hi, 1), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_count_regrips(NULL, 1), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_count_regrips(ok, -1), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_count_regrips(bad_face_lo, 1), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_count_regrips(bad_quarter_lo, 1), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_count_regrips(bad_quarter_hi, 1), CT_SCORE_E_MOVE);

    CT_ASSERT_EQ(ct_score_alg(ok, 1, NULL, &p), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_score_alg(ok, 1, &c, NULL), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_score_alg(NULL, 1, &c, &p), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_score_alg(ok, -1, &c, &p), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_score_alg(bad_face_hi, 1, &c, &p), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_score_alg(bad_quarter_lo, 1, &c, &p), CT_SCORE_E_MOVE);
    CT_ASSERT_EQ(ct_score_alg(bad_quarter_hi, 1, &c, &p), CT_SCORE_E_MOVE);

    char out[8];
    CT_ASSERT_EQ(ct_score_parts_json(NULL, out, sizeof out), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_score_parts_json(&p, NULL, sizeof out), CT_SCORE_E_ARG);
    CT_ASSERT_EQ(ct_score_parts_json(&p, out, 0), CT_SCORE_E_ARG);
}
