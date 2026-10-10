#include "framework.h"

#include <stdio.h>

int main(int argc, char **argv) {
    int rc = ct_run_all(argc > 1 ? argv[1] : NULL);
    return rc;
}
