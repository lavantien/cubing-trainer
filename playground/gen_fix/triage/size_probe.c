#include "gen/synth.h"
#include <stdio.h>

int main(void) {
    int size = ct_synth_init();
    printf("size=%d corner=%d edge=%d\n", size, CT_SYNTH_CORNER_KEYS, CT_SYNTH_EDGE_KEYS);
    return 0;
}
