// geometry.c - VERSION MODIFIÉE
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "geometry.h"
#include "hexagone_list.h"
#include "animation.h"
#include "config.h"

#define NB_SIDE 6
#define ADJUST 10
#define PI 3.14159265358979323846

/*----------------------------------------------------*/

void make_hexagone(SDL_Renderer *renderer, Hexagon* hex) {
    if (!renderer || !hex) return;

    // ✅ CALCUL DES COORDONNÉES ABSOLUES à la volée
    Sint16 absolute_vx[NB_SIDE];
    Sint16 absolute_vy[NB_SIDE];

    for (int i = 0; i < NB_SIDE; i++) {
        // ✅ Les points vx/vy peuvent être soit :
        // - Les points relatifs de base (sans animation)
        // - Les points transformés relatifs (avec animation précalculée)
        absolute_vx[i] = hex->center_x + (Sint16)(hex->vx[i] * hex->current_scale);
        absolute_vy[i] = hex->center_y + (Sint16)(hex->vy[i] * hex->current_scale);
    }

    filledPolygonRGBA(renderer, absolute_vx, absolute_vy, NB_SIDE,
                      hex->color.r, hex->color.g, hex->color.b, hex->color.a);
}

/*----------------------------------------------------*/

Hexagon* create_single_hexagon(int center_x, int center_y, int container_size, float size_ratio, unsigned char element_id) {
    Hexagon* hex = malloc(sizeof(Hexagon));
    if (!hex) {
        fprintf(stderr,"Problème d'allocation structure Hexagon\n");
        return NULL;
    }

    // Initialisation
    hex->element_id = element_id;
    hex->vx = malloc(NB_SIDE * sizeof(Sint16));
    hex->vy = malloc(NB_SIDE * sizeof(Sint16));
    if (!hex->vx || !hex->vy) {
        fprintf(stderr,"Problème d'allocation dynamique (create_single_hexagon)\n");
        free(hex->vx);
        free(hex->vy);
        free(hex);
        return NULL;
    }

    // ✅ NOUVEAU : Stocker la position et l'échelle
    hex->center_x = center_x;
    hex->center_y = center_y;
    hex->current_scale = 1.0f;

    // Calcul du rayon basé sur le container et ratio
    int base_radius = (int)(container_size * size_ratio / 2);
    int current_radius = base_radius - (element_id * ADJUST);

    // ✅ MODIFICATION : Créer des points RELATIFS (centrés sur 0,0)
    for(int i = 0; i < NB_SIDE; i++) {
        hex->vx[i] = (Sint16)(current_radius * cos(2*i*PI/NB_SIDE));  // RELATIF
        hex->vy[i] = (Sint16)(current_radius * sin(2*i*PI/NB_SIDE));  // RELATIF
    }

    SDL_Color colors[] = {
        {137, 36, 144, 150},   // Violet foncé
        {217, 61, 228, 150},   // Violet
        {228, 129, 235, 150},  // Violet pâle
        {255, 255, 255, 130}   // Blanc
    };
    hex->color = colors[element_id % 4];

    printf("✅ Hexagone %d créé - Centre: (%d,%d), Points relatifs\n",
           element_id, center_x, center_y);

    return hex;
}

/*----------------------------------------------------*/

HexagoneList* create_all_hexagones(int center_x, int center_y, int container_size, float size_ratio) {
    HexagoneList* list = new_hexagone_list();
    if (!list) return NULL;

    for (int i = 0; i < NB_HX; i++) {
        Hexagon* hex = create_single_hexagon(center_x, center_y, container_size, size_ratio, i);
        bool clockwise = (i % 2 == 0);
        double angle;

        // SWITCH identique à celui de main.c original
        switch(i) {
            case 0: angle = ANGLE_1; break;
            case 1: angle = ANGLE_2; break;
            case 2: angle = ANGLE_3; break;
            case 3: angle = ANGLE_4; break;
            default: angle = ANGLE_1;
        }

        Animation* anim = create_animation(clockwise, angle);
        add_hexagone(list, hex, anim);
        printf("Hexagone %d créé - Angle: %.1f° - Container: %d, Ratio: %.2f\n",
               i, angle, container_size, size_ratio);
    }

    return list;
}

/*----------------------------------------------------*/

// Déplacer un hexagone
void move_hexagon(Hexagon* hex, int new_center_x, int new_center_y) {
    if (!hex) return;
    hex->center_x = new_center_x;
    hex->center_y = new_center_y;
}

/*----------------------------------------------------*/

// Redimensionner un hexagone
void scale_hexagon(Hexagon* hex, float new_scale) {
    if (!hex) return;
    hex->current_scale = new_scale;
}

/*----------------------------------------------------*/

// Déplacer ET redimensionner un hexagone
void transform_hexagon(Hexagon* hex, int new_center_x, int new_center_y, float new_scale) {
    if (!hex) return;
    hex->center_x = new_center_x;
    hex->center_y = new_center_y;
    hex->current_scale = new_scale;
}

/*----------------------------------------------------*/

void free_hexagon(Hexagon* hex) {
    if (!hex) return;
    free(hex->vx);
    free(hex->vy);
    free(hex);
}

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/


