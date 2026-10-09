#include "framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CT_SPAWN_CMD_MAX 1024

static int ct_selftest_spawn(const char *exe) {
    char cmd[CT_SPAWN_CMD_MAX];
    int n = snprintf(cmd, sizeof(cmd), "set \"CT_EXPECTED_FAIL=1\" && \"%s\" expected_fail", exe);
    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "selftest_spawn: command truncated for %s\n", exe);
        return 1;
    }
    int rc = system(cmd);
    if (rc == 0) {
        fprintf(stderr, "selftest_spawn: child exited 0, expected nonzero\n");
        return 1;
    }
    printf("selftest_spawn: child exit code %d\n", rc);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "selftest_spawn") == 0) {
        return ct_selftest_spawn(argv[0]);
    }
    return ct_run_all(argc > 1 ? argv[1] : NULL);
}
