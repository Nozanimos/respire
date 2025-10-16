#include "animation.h"
#include <stdio.h>
#include <stdlib.h>

Animation* create_animation(bool clockwise, double angle_per_cycle) {
    Animation* anim = malloc(sizeof(Animation));
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
    free(anim);
}
