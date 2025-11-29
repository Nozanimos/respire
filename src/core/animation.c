// SPDX-License-Identifier: GPL-3.0-or-later
#include "animation.h"
#include <stdio.h>
#include <stdlib.h>
#include "core/memory/memory.h"

Animation* create_animation(bool clockwise, double angle_per_cycle) {
    Animation* anim = SAFE_MALLOC(sizeof(Animation));
    if (!anim) {
        fprintf(stderr,"Problème d'allocation dynamique (create_animation)\n");
        return NULL;

    }

    *anim = (Animation){
        .angle_per_cycle = angle_per_cycle,
        .scale_min = 0.1,
        .scale_max = 1.0,
        .clockwise = clockwise
    };
    return anim;
}

void free_animation(Animation* anim) {
    SAFE_FREE(anim);
}
