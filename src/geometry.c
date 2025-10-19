// geometry.c - VERSION MODIFIÉE
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "geometry.h"
#include "hexagone_list.h"
#include "animation.h"
#include "config.h"
#include "debug.h"

#define NB_SIDE 6
#define ADJUST 0.05f
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
    int current_radius = (int) (base_radius * (1.0f - element_id * ADJUST));

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

    debug_printf("✅ Hexagone %d créé - Centre: (%d,%d), Points relatifs\n",
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
        debug_printf("Hexagone %d créé - Angle: %.1f° - Container: %d, Ratio: %.2f\n",
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

// Crée un triangle isocèle générique
Triangle* create_triangle(int center_x, int center_y, int height, bool points_up, SDL_Color color) {
    Triangle* tri = malloc(sizeof(Triangle));
    if (!tri) {
        debug_printf("❌ Erreur allocation triangle\n");
        return NULL;
    }

    // Selon ta spécification : base = 2 × hauteur
    int base_half = height;  // base/2 = hauteur, donc base = 2 × hauteur

    tri->center_x = center_x;
    tri->center_y = center_y;
    tri->color = color;

    if (points_up) {
        // Flèche vers le haut - sommet en haut
        tri->vx[0] = center_x;              // Sommet
        tri->vy[0] = center_y - height/2;   // Ajustement pour centrage

        tri->vx[1] = center_x - base_half;  // Base gauche
        tri->vy[1] = center_y + height/2;   // Ajustement pour centrage

        tri->vx[2] = center_x + base_half;  // Base droite
        tri->vy[2] = center_y + height/2;   // Ajustement pour centrage
    } else {
        // Flèche vers le bas - sommet en bas
        tri->vx[0] = center_x;              // Sommet
        tri->vy[0] = center_y + height/2;   // Ajustement pour centrage

        tri->vx[1] = center_x - base_half;  // Base gauche
        tri->vy[1] = center_y - height/2;   // Ajustement pour centrage

        tri->vx[2] = center_x + base_half;  // Base droite
        tri->vy[2] = center_y - height/2;   // Ajustement pour centrage
    }

    debug_printf("🔺 Triangle créé - Centre: (%d,%d), Hauteur: %d, Orientation: %s\n",
                 center_x, center_y, height, points_up ? "haut" : "bas");

    return tri;
}

/*----------------------------------------------------*/

// Dessine un triangle
void draw_triangle(SDL_Renderer *renderer, Triangle* tri) {
    if (!renderer || !tri) return;

    filledPolygonRGBA(renderer, tri->vx, tri->vy, 3,
                      tri->color.r, tri->color.g, tri->color.b, tri->color.a);
}

/*----------------------------------------------------*/

// Dessine un triangle avec offset
void draw_triangle_with_offset(SDL_Renderer *renderer, Triangle* tri, int offset_x, int offset_y) {
    if (!renderer || !tri) return;

    // Créer des tableaux temporaires avec les coordonnées décalées
    Sint16 vx_offset[3];
    Sint16 vy_offset[3];

    for (int i = 0; i < 3; i++) {
        vx_offset[i] = tri->vx[i] + offset_x;
        vy_offset[i] = tri->vy[i] + offset_y;
    }

    filledPolygonRGBA(renderer, vx_offset, vy_offset, 3,
                      tri->color.r, tri->color.g, tri->color.b, tri->color.a);
}
/*----------------------------------------------------*/

// Fonctions spécifiques pour flèches (utilisent create_triangle)
Triangle* create_up_arrow(int center_x, int center_y, int size, SDL_Color color) {
    return create_triangle(center_x, center_y, size, true, color);
}

Triangle* create_down_arrow(int center_x, int center_y, int size, SDL_Color color) {
    return create_triangle(center_x, center_y, size, false, color);
}

/*----------------------------------------------------*/

// Libère un triangle
void free_triangle(Triangle* tri) {
    if (tri) {
        free(tri);
        debug_printf("🔻 Triangle libéré\n");
    }
}

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/

/*----------------------------------------------------*/


