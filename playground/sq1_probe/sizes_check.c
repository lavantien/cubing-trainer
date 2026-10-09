#include "engine/geometry_sq1.h"
#include "engine/geometry_clock.h"
#include <stdio.h>
int main(void) {
    printf("units=%d pieces=%d layers=%d hours=%d clocks=%d forms=%d stickers=%d moves=%d zero=%d\n",
           CT_SQ1_UNITS, CT_SQ1_PIECES, CT_SQ1_LAYERS, CT_CLOCK_HOURS, CT_CLOCK_CLOCKS,
           CT_CLOCK_FORMS, CT_SQ1_STICKERS, CT_SQ1_MOVES, CT_SQ1_ZERO_POS);
    printf("state bytes=%d clock bytes=%d\n", (int)sizeof(ct_sq1_state), (int)sizeof(ct_clock_state));
    return 0;
}
