#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include <stdbool.h>

typedef struct {
    double angle_per_cycle;    // 15° ou 30° selon l'hexagone
    double scale_min;          // 0.1
    double scale_max;          // 1.0
    bool clockwise;            // sens rotation
} Animation;

// Prototypes
Animation* create_animation(bool clockwise, double angle_per_cycle);
void free_animation(Animation* anim);

#endif
