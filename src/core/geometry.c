// SPDX-License-Identifier: GPL-3.0-or-later
// geometry.c - VERSION CAIRO (avec antialiasing)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include "geometry.h"
#include "precompute_list.h"
#include "animation.h"
#include "config.h"
#include "debug.h"

#define NB_SIDE 6
#define ADJUST 0.05f
#define PI 3.14159265358979323846

/*----------------------------------------------------*/
/* FONCTIONS UTILITAIRES CAIRO */
/*----------------------------------------------------*/

// Crée une texture SDL depuis une surface Cairo
static SDL_Texture* texture_from_cairo(SDL_Renderer* renderer, cairo_surface_t* surface) {
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);

    SDL_Surface* sdl_surface = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_ARGB8888
    );

    if (!sdl_surface) return NULL;

    // Copier les données
    memcpy(sdl_surface->pixels,
           cairo_image_surface_get_data(surface),
           height * cairo_image_surface_get_stride(surface));

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
    SDL_FreeSurface(sdl_surface);

    return texture;
}

/*----------------------------------------------------*/

void make_hexagone(SDL_Renderer *renderer, Hexagon* hex) {
    if (!renderer || !hex) return;

    // Calculer la taille nécessaire pour la surface
    int max_x = 0, max_y = 0, min_x = 0, min_y = 0;

    for (int i = 0; i < NB_SIDE; i++) {
        Sint16 x = (Sint16)(hex->vx[i] * hex->current_scale);
        Sint16 y = (Sint16)(hex->vy[i] * hex->current_scale);
        if (x > max_x) max_x = x;
        if (x < min_x) min_x = x;
        if (y > max_y) max_y = y;
        if (y < min_y) min_y = y;
    }

    int width = max_x - min_x + 4;  // +4 pour marge antialiasing
    int height = max_y - min_y + 4;

    if (width <= 0 || height <= 0) return;

    // Créer une surface Cairo
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    // Activer l'antialiasing de haute qualité
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // Translater pour centrer l'hexagone dans la surface
    cairo_translate(cr, -min_x + 2, -min_y + 2);

    // Dessiner l'hexagone
    for (int i = 0; i < NB_SIDE; i++) {
        double x = hex->vx[i] * hex->current_scale;
        double y = hex->vy[i] * hex->current_scale;

        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_close_path(cr);

    // Remplir avec la couleur et alpha
    cairo_set_source_rgba(cr,
                          hex->color.r / 255.0,
                          hex->color.g / 255.0,
                          hex->color.b / 255.0,
                          hex->color.a / 255.0);
    cairo_fill(cr);

    // Finaliser la surface Cairo
    cairo_surface_flush(surface);

    // Créer une texture SDL depuis la surface Cairo
    SDL_Texture* texture = texture_from_cairo(renderer, surface);

    if (texture) {
        // Activer le blend mode pour la transparence
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        // Calculer la position de rendu
        SDL_Rect dest_rect = {
            hex->center_x + min_x - 2,
            hex->center_y + min_y - 2,
            width,
            height
        };

        // Rendre la texture
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
        SDL_DestroyTexture(texture);
    }

    // Nettoyer Cairo
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
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

    // Stocker la position et l'échelle
    hex->center_x = center_x;
    hex->center_y = center_y;
    hex->current_scale = 1.0f;

    // Calcul du rayon basé sur le container et ratio
    int base_radius = (int)(container_size * size_ratio / 2);
    int current_radius = (int) (base_radius * (1.0f - element_id * ADJUST));

    // Créer des points RELATIFS (centrés sur 0,0)
    for(int i = 0; i < NB_SIDE; i++) {
        hex->vx[i] = (Sint16)(current_radius * cos(2*i*PI/NB_SIDE));  // RELATIF
        hex->vy[i] = (Sint16)(current_radius * sin(2*i*PI/NB_SIDE));  // RELATIF
    }

    SDL_Color colors[] = {
        {160, 50, 170, 180},   // Violet foncé (plus lumineux, plus opaque)
        {235, 80, 245, 180},   // Violet (plus vif, plus opaque)
        {245, 150, 250, 180},  // Violet pâle (plus lumineux, plus opaque)
        {255, 255, 255, 200}   // Blanc (plus opaque pour rester blanc)
    };
    hex->color = colors[element_id % 4];

    debug_printf("✅ Hexagone %d créé (Cairo) - Centre: (%d,%d), Points relatifs\n",
           element_id, center_x, center_y);

    return hex;
}

/*----------------------------------------------------*/

HexagoneList* create_all_hexagones(int center_x, int center_y, int container_size, float size_ratio) {
    HexagoneList* list = new_hexagone_list();
    if (!list) return NULL;

    for (int i = 0; i < NB_HX; i++) {
        Hexagon* hex = create_single_hexagon(center_x, center_y, container_size, size_ratio, i);
        bool clockwise = (i % 2 != 0);
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
// ═══════════════════════════════════════════════════════════════════════════════
// RECALCUL DES SOMMETS D'UN HEXAGONE APRÈS REDIMENSIONNEMENT
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// Cette fonction recalcule les sommets relatifs d'un hexagone en fonction
// du nouveau container_size. Les points vx/vy sont recalculés comme des
// coordonnées RELATIVES au centre (0, 0).
//
// IMPORTANT : Les hexagones utilisent des coordonnées relatives qui sont
// ensuite transformées en absolues lors du rendu dans make_hexagone().
// On doit donc recréer les points relatifs avec le nouveau rayon.
// ═══════════════════════════════════════════════════════════════════════════════

void recalculer_sommets(Hexagon* hex, int container_size) {
    if (!hex) return;

    // 1. Calculer le rayon de base pour ce container
    //    (en utilisant le même ratio que lors de la création)
    float size_ratio = 0.75f;  // Même valeur que dans main.c ligne 70
    int base_radius = (int)(container_size * size_ratio / 2);

    // 2. Ajuster le rayon selon l'élément (comme dans create_single_hexagon)
    //    Les hexagones intérieurs sont légèrement plus petits
    int current_radius = (int)(base_radius * (1.0f - hex->element_id * ADJUST));

    // 3. Recalculer les 6 sommets en coordonnées RELATIVES
    //    Un hexagone régulier a des angles de 0°, 60°, 120°, 180°, 240°, 300°
    //    (ou en radians : 0, π/3, 2π/3, π, 4π/3, 5π/3)
    for (int i = 0; i < NB_SIDE; i++) {
        // Calculer l'angle du sommet i (en radians)
        double angle_rad = 2.0 * i * PI / NB_SIDE;  // 0, 60°, 120°, etc.

        // Calculer les coordonnées relatives (centrées sur 0,0)
        hex->vx[i] = (Sint16)(current_radius * cos(angle_rad));
        hex->vy[i] = (Sint16)(current_radius * sin(angle_rad));
    }

    debug_printf("🔄 Sommets recalculés pour hexagone %d - Nouveau rayon: %d\n",
                 hex->element_id, current_radius);
}

/*----------------------------------------------------*/

void free_hexagon(Hexagon* hex) {
    if (!hex) return;
    free(hex->vx);
    free(hex->vy);
    free(hex);
}

/*----------------------------------------------------*/

// Crée un triangle isocèle générique avec Cairo
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

    debug_printf("🔺 Triangle créé (Cairo) - Centre: (%d,%d), Hauteur: %d, Orientation: %s\n",
                 center_x, center_y, height, points_up ? "haut" : "bas");

    return tri;
}

/*----------------------------------------------------*/

// Dessine un triangle avec Cairo pour l'antialiasing
void draw_triangle(SDL_Renderer *renderer, Triangle* tri) {
    if (!renderer || !tri) return;

    // Calculer les bounds du triangle
    int min_x = tri->vx[0], max_x = tri->vx[0];
    int min_y = tri->vy[0], max_y = tri->vy[0];

    for (int i = 1; i < 3; i++) {
        if (tri->vx[i] < min_x) min_x = tri->vx[i];
        if (tri->vx[i] > max_x) max_x = tri->vx[i];
        if (tri->vy[i] < min_y) min_y = tri->vy[i];
        if (tri->vy[i] > max_y) max_y = tri->vy[i];
    }

    int width = max_x - min_x + 4;
    int height = max_y - min_y + 4;

    if (width <= 0 || height <= 0) return;

    // Créer surface Cairo
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    cairo_translate(cr, -min_x + 2, -min_y + 2);

    // Dessiner le triangle
    cairo_move_to(cr, tri->vx[0], tri->vy[0]);
    cairo_line_to(cr, tri->vx[1], tri->vy[1]);
    cairo_line_to(cr, tri->vx[2], tri->vy[2]);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr,
                          tri->color.r / 255.0,
                          tri->color.g / 255.0,
                          tri->color.b / 255.0,
                          tri->color.a / 255.0);
    cairo_fill(cr);

    cairo_surface_flush(surface);

    // Convertir en texture SDL
    SDL_Texture* texture = texture_from_cairo(renderer, surface);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_Rect dest_rect = {min_x - 2, min_y - 2, width, height};
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
        SDL_DestroyTexture(texture);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

/*----------------------------------------------------*/

// Dessine un triangle avec offset
void draw_triangle_with_offset(SDL_Renderer *renderer, Triangle* tri, int offset_x, int offset_y) {
    if (!renderer || !tri) return;

    // Créer un triangle temporaire avec les coordonnées décalées
    Triangle temp_tri = *tri;
    for (int i = 0; i < 3; i++) {
        temp_tri.vx[i] = tri->vx[i] + offset_x;
        temp_tri.vy[i] = tri->vy[i] + offset_y;
    }

    draw_triangle(renderer, &temp_tri);
}
/*----------------------------------------------------*/

// Fonctions spécifiques pour flèches (utilisent create_triangle)
Triangle* create_up_arrow(int center_x, int center_y, int size, SDL_Color color) {
    return create_triangle(center_x, center_y, size, true, color);
}

Triangle* create_down_arrow(int center_x, int center_y, int size, SDL_Color color) {
    return create_triangle(center_x, center_y, size, false, color);
}

// Crée une flèche vers la gauche (triangle pointant à gauche)
Triangle* create_left_arrow(int center_x, int center_y, int size, SDL_Color color) {
    Triangle* tri = malloc(sizeof(Triangle));
    if (!tri) {
        debug_printf("❌ Erreur allocation triangle\n");
        return NULL;
    }

    int base_half = size;
    tri->center_x = center_x;
    tri->center_y = center_y;
    tri->color = color;

    // Flèche vers la gauche - sommet à gauche
    tri->vx[0] = center_x - size/2;    // Sommet gauche
    tri->vy[0] = center_y;              // Au centre

    tri->vx[1] = center_x + size/2;    // Base droite haut
    tri->vy[1] = center_y - base_half; // En haut

    tri->vx[2] = center_x + size/2;    // Base droite bas
    tri->vy[2] = center_y + base_half; // En bas

    debug_printf("◀ Flèche gauche créée (Cairo) - Centre: (%d,%d), Taille: %d\n",
                 center_x, center_y, size);

    return tri;
}

// Crée une flèche vers la droite (triangle pointant à droite)
Triangle* create_right_arrow(int center_x, int center_y, int size, SDL_Color color) {
    Triangle* tri = malloc(sizeof(Triangle));
    if (!tri) {
        debug_printf("❌ Erreur allocation triangle\n");
        return NULL;
    }

    int base_half = size;
    tri->center_x = center_x;
    tri->center_y = center_y;
    tri->color = color;

    // Flèche vers la droite - sommet à droite
    tri->vx[0] = center_x + size/2;    // Sommet droit
    tri->vy[0] = center_y;              // Au centre

    tri->vx[1] = center_x - size/2;    // Base gauche haut
    tri->vy[1] = center_y - base_half; // En haut

    tri->vx[2] = center_x - size/2;    // Base gauche bas
    tri->vy[2] = center_y + base_half; // En bas

    debug_printf("▶ Flèche droite créée (Cairo) - Centre: (%d,%d), Taille: %d\n",
                 center_x, center_y, size);

    return tri;
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

// ═══════════════════════════════════════════════════════════════════════════════
// PRIMITIVES DE DESSIN POUR L'UI - Versions Cairo pour l'antialiasing
// ═══════════════════════════════════════════════════════════════════════════════

// Dessine une ligne horizontale de séparation
void draw_separator_line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2,
                         int thickness, SDL_Color color) {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Dessiner plusieurs lignes pour l'épaisseur
    for (int i = 0; i < thickness; i++) {
        SDL_RenderDrawLine(renderer, x1, y1 + i, x2, y2 + i);
    }
}

// Dessine un cadre rectangulaire (pour le preview) avec Cairo
void draw_frame_rect(SDL_Renderer* renderer, int x, int y, int width, int height,
                     SDL_Color color) {
    if (!renderer) return;

    // Créer surface Cairo
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    cairo_set_line_width(cr, 1.0);

    // Dessiner le rectangle (stroke seulement)
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_set_source_rgba(cr,
                          color.r / 255.0,
                          color.g / 255.0,
                          color.b / 255.0,
                          color.a / 255.0);
    cairo_stroke(cr);

    cairo_surface_flush(surface);

    SDL_Texture* texture = texture_from_cairo(renderer, surface);
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_Rect dest_rect = {x, y, width, height};
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
        SDL_DestroyTexture(texture);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

// Dessine un rectangle arrondi rempli (pour les boutons) avec Cairo
void draw_rounded_rect(SDL_Renderer* renderer, int x, int y, int width, int height,
                       int radius, SDL_Color bg_color, SDL_Color border_color) {
    if (!renderer) return;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // Créer le chemin arrondi
    double r = radius;
    double degrees = PI / 180.0;

    cairo_new_sub_path(cr);
    cairo_arc(cr, width - r, r, r, -90 * degrees, 0 * degrees);
    cairo_arc(cr, width - r, height - r, r, 0 * degrees, 90 * degrees);
    cairo_arc(cr, r, height - r, r, 90 * degrees, 180 * degrees);
    cairo_arc(cr, r, r, r, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);

    // Remplir
    cairo_set_source_rgba(cr,
                          bg_color.r / 255.0,
                          bg_color.g / 255.0,
                          bg_color.b / 255.0,
                          bg_color.a / 255.0);
    cairo_fill_preserve(cr);

    // Bordure
    cairo_set_source_rgba(cr,
                          border_color.r / 255.0,
                          border_color.g / 255.0,
                          border_color.b / 255.0,
                          border_color.a / 255.0);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    cairo_surface_flush(surface);

    SDL_Texture* texture = texture_from_cairo(renderer, surface);
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_Rect dest_rect = {x, y, width, height};
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
        SDL_DestroyTexture(texture);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

// Dessine le fond du toggle (rectangle arrondi) avec Cairo
void draw_toggle_background(SDL_Renderer* renderer, int x, int y, int width, int height,
                            int radius, bool is_on) {
    if (!renderer) return;

    // Couleur selon l'état : vert si ON, gris si OFF
    SDL_Color bg_color = is_on ?
        (SDL_Color){100, 200, 100, 255} :  // Vert pâle
        (SDL_Color){150, 150, 150, 255};   // Gris

    SDL_Color border_color = {0, 0, 0, 255};

    draw_rounded_rect(renderer, x, y, width, height, radius, bg_color, border_color);
}

// Dessine le cercle mobile du toggle avec Cairo
void draw_toggle_circle(SDL_Renderer* renderer, int center_x, int center_y,
                        int radius, SDL_Color color) {
    if (!renderer) return;

    int size = radius * 2 + 4;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // Dessiner le cercle rempli
    cairo_arc(cr, radius + 2, radius + 2, radius, 0, 2 * PI);
    cairo_set_source_rgba(cr,
                          color.r / 255.0,
                          color.g / 255.0,
                          color.b / 255.0,
                          color.a / 255.0);
    cairo_fill_preserve(cr);

    // Bordure noire
    cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    cairo_surface_flush(surface);

    SDL_Texture* texture = texture_from_cairo(renderer, surface);
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_Rect dest_rect = {center_x - radius - 2, center_y - radius - 2, size, size};
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
        SDL_DestroyTexture(texture);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}
