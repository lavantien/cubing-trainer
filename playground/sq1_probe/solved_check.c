#include "engine/geometry_sq1.h"
#include <stdio.h>
int main(void) {
    ct_sq1_state s;
    fprintf(stderr, "before solved\n");
    ct_sq1_state_solved(&s);
    fprintf(stderr, "after solved\n");
    for (int i = 0; i < 24; i++) fprintf(stderr, "%d ", s.units[i]);
    fprintf(stderr, "\nslash=%d\n", ct_sq1_can_slash(&s));
    int id = ct_sq1_move_id(1, 0);
    fprintf(stderr, "id=%d\n", id);
    int rc = ct_sq1_apply(&s, id);
    fprintf(stderr, "applied rc=%d\n", rc);
    for (int i = 0; i < 12; i++) fprintf(stderr, "%d ", s.units[i]);
    fprintf(stderr, "\ncan=%d\n", ct_sq1_can_slash(&s));
    return 0;
}
