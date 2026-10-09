#include "engine/geometry_clock.h"
#include <stdio.h>

int main(void) {
    ct_clock_state a;
    ct_clock_state_solved(&a);
    ct_clock_apply(&a, ct_clock_move_id(CT_CLOCK_UR, 1));
    ct_clock_apply(&a, ct_clock_move_id(CT_CLOCK_UL, 1));
    ct_clock_state b;
    ct_clock_state_solved(&b);
    ct_clock_apply(&b, ct_clock_move_id(CT_CLOCK_U, 1));
    printf("ur+ul:");
    for (int i = 0; i < 18; i++) printf(" %d", a.clocks[i]);
    printf("\nu     :");
    for (int i = 0; i < 18; i++) printf(" %d", b.clocks[i]);
    printf("\nids: ur1=%d ul1=%d u1=%d\n", ct_clock_move_id(CT_CLOCK_UR, 1),
           ct_clock_move_id(CT_CLOCK_UL, 1), ct_clock_move_id(CT_CLOCK_U, 1));
    return 0;
}
