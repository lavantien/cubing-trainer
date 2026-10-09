#include "config/config.h"
#include "engine/cube.h"

#include <stdio.h>

int main(void) {
    printf("%d %d %d %d %d\n", CT_MOVE_BIT_U == 1u << 0, CT_MOVE_BIT_Z == 1u << 11, CT_FACE_U == 0,
           CT_FACE_D == 5, CT_MOVE_QUARTERS == 4);
    return 0;
}
