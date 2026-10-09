#include <stdio.h>
#include <string.h>

#include "config/config.h"

static const char *keys[] = {
    "srs.request_retention", "srs.max_ivl_days",  "srs.w0",          "srs.w18",
    "srs.w19",               "srs.w-1",           "srs.w 1",         "srs.w01",
    "gen.preferred_moves",   "gen.weight_len",    "gen.weight_subset", "gen.weight_regrip",
    "gen.weight_finger",     "gen.penalty_R",     "gen.penalty_u",   "gen.penalty_M",
    "gen.finger_cost_R_cw",  "gen.finger_cost_R_ccw", "gen.finger_cost_x_cw", "gen.finger_cost_R",
    "gen.setup_bfs_depth",   "gen.fallback_depth", "gen.straggler_depth",
    "anim.move_seconds",     "window.width",      "window.height",   "window.target_fps",
    "db.filename",           "fuzz.iterations",   "nope",            "db.dir_name",
    "srs.request_retentionx"
};
static const char *values[] = {
    "", " ", "  ", "\t", "+1.5", " 1.5", "1.5 ", "0x1.8p3", "1e999", "-1e999",
    "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009",
    "0.0000000000000000000000000000000000000000000000000001", "-0", "1,5", "1.", ".5", "inf",
    "INF", "nan", "1e-400", "0", "0.5", "1000000", "2"
};

int main(void) {
    size_t nk = sizeof keys / sizeof keys[0];
    size_t nv = sizeof values / sizeof values[0];
    for (size_t k = 0; k < nk; k++) {
        int accepts = 0;
        for (size_t v = 0; v < nv; v++) {
            ct_config c = ct_config_defaults();
            int rc = ct_config_set(&c, keys[k], values[v]);
            if (rc == CT_CFG_OK) {
                accepts++;
                ct_config d = ct_config_defaults();
                double after = 0;
                double before = 0;
                if (strcmp(keys[k], "srs.request_retention") == 0) {
                    before = d.fsrs.request_retention;
                    after = c.fsrs.request_retention;
                } else if (strcmp(keys[k], "anim.move_seconds") == 0) {
                    before = d.anim.move_seconds;
                    after = c.anim.move_seconds;
                } else if (strcmp(keys[k], "window.target_fps") == 0) {
                    before = d.window.target_fps;
                    after = c.window.target_fps;
                } else if (strcmp(keys[k], "srs.max_ivl_days") == 0) {
                    before = d.fsrs.max_ivl_days;
                    after = c.fsrs.max_ivl_days;
                } else if (strcmp(keys[k], "gen.setup_bfs_depth") == 0) {
                    before = d.gen.setup_bfs_depth;
                    after = c.gen.setup_bfs_depth;
                }
                printf("ACCEPT %-24s [%s] rc=0%s\n", keys[k], values[v],
                       after != before ? "" : " (no-op value)");
            } else if (rc != CT_CFG_BAD_VALUE && rc != CT_CFG_UNKNOWN_KEY) {
                printf("WEIRD-RC %s [%s] rc=%d\n", keys[k], values[v], rc);
            }
        }
        printf("key %-24s accepts %zu/%zu\n", keys[k], accepts, nv);
    }

    struct {
        const char *key;
        const char *value;
        double expect;
    } clamp_cases[] = {
        {"srs.request_retention", "0.5", 0.7},
        {"srs.request_retention", "1.0", 0.99},
        {"srs.request_retention", "0.85", 0.85},
        {"srs.max_ivl_days", "-5", 0.1},
        {"anim.move_seconds", "0", 0.01},
        {"anim.move_seconds", "-0", 0.01},
        {"window.target_fps", "-9", 1},
        {"gen.setup_bfs_depth", "0", 1},
    };
    for (size_t i = 0; i < sizeof clamp_cases / sizeof clamp_cases[0]; i++) {
        ct_config c = ct_config_defaults();
        int rc = ct_config_set(&c, clamp_cases[i].key, clamp_cases[i].value);
        double got = -1;
        if (strcmp(clamp_cases[i].key, "srs.request_retention") == 0) {
            got = c.fsrs.request_retention;
        } else if (strcmp(clamp_cases[i].key, "srs.max_ivl_days") == 0) {
            got = c.fsrs.max_ivl_days;
        } else if (strcmp(clamp_cases[i].key, "anim.move_seconds") == 0) {
            got = c.anim.move_seconds;
        } else if (strcmp(clamp_cases[i].key, "window.target_fps") == 0) {
            got = c.window.target_fps;
        } else {
            got = c.gen.setup_bfs_depth;
        }
        printf("clamp %-24s [%s] rc=%d got=%.6g want=%.6g %s\n", clamp_cases[i].key,
               clamp_cases[i].value, rc, got, clamp_cases[i].expect,
               got == clamp_cases[i].expect ? "ok" : "DIFF");
    }

    ct_config c = ct_config_defaults();
    printf("null value rc=%d null key rc=%d\n", ct_config_set(&c, "srs.w0", NULL),
           ct_config_set(&c, NULL, "1"));
    char long_name[300];
    memset(long_name, 'a', sizeof long_name - 1);
    long_name[sizeof long_name - 1] = '\0';
    ct_config c2 = ct_config_defaults();
    printf("db.filename 250a rc=%d\n", ct_config_set(&c2, "db.filename", long_name));
    char name128[128];
    memset(name128, 'n', sizeof name128 - 1);
    name128[sizeof name128 - 1] = '\0';
    ct_config c3 = ct_config_defaults();
    printf("db.filename 127 chars rc=%d stored=%zu\n", ct_config_set(&c3, "db.filename", name128),
           strlen(c3.db.filename));
    ct_config c4 = ct_config_defaults();
    printf("db.filename traversal rc=%d stored=[%s]\n",
           ct_config_set(&c4, "db.filename", "..\\..\\evil.db"), c4.db.filename);
    ct_config c5 = ct_config_defaults();
    printf("preferred_moves [RUDxyz] rc=%d mask=%x\n",
           ct_config_set(&c5, "gen.preferred_moves", "RUDxyz"), c5.gen.preferred_moves);
    ct_config c6 = ct_config_defaults();
    printf("preferred_moves [R U] rc=%d\n", ct_config_set(&c6, "gen.preferred_moves", "R U"));
    ct_config c7 = ct_config_defaults();
    printf("preferred_moves empty rc=%d mask=%x\n", ct_config_set(&c7, "gen.preferred_moves", ""),
           c7.gen.preferred_moves);
    ct_config c8 = ct_config_defaults();
    printf("fuzz.iterations -5 rc=%d got=%d\n", ct_config_set(&c8, "fuzz.iterations", "-5"),
           c8.fuzz.iterations);
    ct_config c9 = ct_config_defaults();
    printf("window.width -100 rc=%d got=%d\n", ct_config_set(&c9, "window.width", "-100"),
           c9.window.width);
    return 0;
}
