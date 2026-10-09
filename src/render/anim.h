#ifndef CT_ANIM_H
#define CT_ANIM_H

#include <stdbool.h>

#include "config/config.h"
#include "engine/cube.h"

typedef struct {
    ct_move moves[CT_ANIM_QUEUE_CAP];
    int count;
    int head;
    double elapsed;
    double speed;
    bool playing;
} ct_anim;

typedef struct {
    ct_move move;
    double progress;
    bool just_completed;
} ct_anim_step_result;

void ct_anim_init(ct_anim *a);
int ct_anim_push(ct_anim *a, ct_move m);
bool ct_anim_active(const ct_anim *a);
ct_anim_step_result ct_anim_update(ct_anim *a, double dt, double move_seconds);
ct_anim_step_result ct_anim_step(ct_anim *a);
void ct_anim_set_speed(ct_anim *a, double speed);
void ct_anim_pause(ct_anim *a);
void ct_anim_resume(ct_anim *a);
int ct_anim_prefix(const ct_anim *a, int upto, ct_move *out, int cap);
double ct_ease_cubic_in_out(double t);

#endif
