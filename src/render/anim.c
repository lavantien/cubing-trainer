#include "render/anim.h"

static ct_move ct_anim_at(const ct_anim *a, int i) {
    return a->moves[(a->head + i) % CT_ANIM_QUEUE_CAP];
}

static ct_anim_step_result ct_anim_result(ct_move m, double progress, bool done) {
    ct_anim_step_result r;
    r.move = m;
    r.progress = progress;
    r.just_completed = done;
    return r;
}

static ct_anim_step_result ct_anim_pop(ct_anim *a) {
    ct_anim_step_result r = ct_anim_result(ct_anim_at(a, 0), 1.0, true);
    a->head = (a->head + 1) % CT_ANIM_QUEUE_CAP;
    a->count--;
    a->elapsed = 0.0;
    return r;
}

void ct_anim_init(ct_anim *a) {
    a->count = 0;
    a->head = 0;
    a->elapsed = 0.0;
    a->speed = 1.0;
    a->playing = true;
    for (int i = 0; i < CT_ANIM_QUEUE_CAP; i++) {
        a->moves[i] = (ct_move){0};
    }
}

int ct_anim_push(ct_anim *a, ct_move m) {
    if (a->count >= CT_ANIM_QUEUE_CAP) {
        return -1;
    }
    a->moves[(a->head + a->count) % CT_ANIM_QUEUE_CAP] = m;
    a->count++;
    return 0;
}

bool ct_anim_active(const ct_anim *a) {
    return a->count > 0;
}

ct_anim_step_result ct_anim_update(ct_anim *a, double dt, double move_seconds) {
    if (a->count == 0) {
        return ct_anim_result((ct_move){0}, 0.0, false);
    }
    if (a->playing) {
        a->elapsed += dt * a->speed;
        if (a->elapsed >= move_seconds) {
            return ct_anim_pop(a);
        }
    }
    return ct_anim_result(ct_anim_at(a, 0), ct_ease_cubic_in_out(a->elapsed / move_seconds), false);
}

ct_anim_step_result ct_anim_step(ct_anim *a) {
    if (a->count == 0) {
        return ct_anim_result((ct_move){0}, 0.0, false);
    }
    return ct_anim_pop(a);
}

void ct_anim_set_speed(ct_anim *a, double speed) {
    double minimum = 1.0 / (double)CT_ANIM_SPEED_EPS_SCALE;
    if (speed < minimum) {
        speed = minimum;
    }
    a->speed = speed;
}

void ct_anim_pause(ct_anim *a) {
    a->playing = false;
}

void ct_anim_resume(ct_anim *a) {
    a->playing = true;
}

int ct_anim_prefix(const ct_anim *a, int upto, ct_move *out, int cap) {
    int n = upto;
    if (n > a->count) {
        n = a->count;
    }
    if (n > cap) {
        n = cap;
    }
    if (n < 0) {
        n = 0;
    }
    for (int i = 0; i < n; i++) {
        out[i] = ct_anim_at(a, i);
    }
    return n;
}

double ct_ease_cubic_in_out(double t) {
    if (t < 0.0) {
        return 0.0;
    }
    if (t > 1.0) {
        return 1.0;
    }
    if (t < 0.5) {
        return 4.0 * t * t * t;
    }
    double u = 2.0 - 2.0 * t;
    return 1.0 - u * u * u / 2.0;
}
