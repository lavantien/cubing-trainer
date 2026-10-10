#include "framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t hd_suite_count(void);
const char *hd_suite_name(size_t i);
uint64_t hd_run_one(size_t i);
int hd_run_named(const char *name);

#define HD_SKIP_PATH "playground/m6_heapdiag/hd_skip.txt"
#define HD_SKIP_LINE 128

static bool hd_should_skip(bool *flags) {
    FILE *f = fopen(HD_SKIP_PATH, "r");
    if (f == NULL) {
        return false;
    }
    char line[HD_SKIP_LINE];
    while (fgets(line, sizeof line, f) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            len--;
            line[len] = '\0';
        }
        for (size_t i = 0; i < hd_suite_count(); i++) {
            if (strcmp(line, hd_suite_name(i)) == 0) {
                flags[i] = true;
            }
        }
    }
    fclose(f);
    return true;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    if (argc < 2 || strcmp(argv[1], "list") == 0) {
        for (size_t i = 0; i < hd_suite_count(); i++) {
            printf("%zu %s\n", i, hd_suite_name(i));
        }
        return 0;
    }
    if (strcmp(argv[1], "all") == 0) {
        static bool skip[512];
        bool filtered = hd_should_skip(skip);
        for (size_t i = 0; i < hd_suite_count(); i++) {
            if (filtered && skip[i]) {
                continue;
            }
            hd_run_one(i);
        }
        return 0;
    }
    if (strcmp(argv[1], "upto") == 0 && argc == 3) {
        long end = strtol(argv[2], NULL, 10);
        if (end < 0 || (size_t)end >= hd_suite_count()) {
            fprintf(stderr, "hd: bad index %s\n", argv[2]);
            return 2;
        }
        for (long i = 0; i <= end; i++) {
            hd_run_one((size_t)i);
        }
        return 0;
    }
    for (int i = 1; i < argc; i++) {
        int rc = hd_run_named(argv[i]);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}
