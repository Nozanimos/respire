// SPDX-License-Identifier: GPL-3.0-or-later
// geometry.h
#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdbool.h>

// Déclaration anticipée
typedef struct HexagoneList HexagoneList;

typedef struct Hexagon {
    unsigned char element_id;
    Sint16* vx;
    Sint16* vy;
    SDL_Color color;
    int center_x;
    int center_y;
    float current_scale;
} Hexagon;

// Prototypes
void make_hexagone(SDL_Renderer *renderer, Hexagon* hex);
Hexagon* create_single_hexagon(int center_x, int center_y, int container_size, float size_ratio, unsigned char element_id);
void recalculer_sommets(Hexagon* hex, int container_size);
void free_hexagon(Hexagon* hex);
HexagoneList* create_all_hexagones(int center_x, int center_y, int container_size, float size_ratio);
void move_hexagon(Hexagon* hex, int new_center_x, int new_center_y);
void scale_hexagon(Hexagon* hex, float new_scale);
void transform_hexagon(Hexagon* hex, int new_center_x, int new_center_y, float new_scale);

// Structure pour un triangle (réutilisable)
typedef struct {
    Sint16 vx[3];
    Sint16 vy[3];
    SDL_Color color;
    int center_x;
    int center_y;
} Triangle;

// Prototypes pour triangles
Triangle* create_triangle(int center_x, int center_y, int height, bool points_up, SDL_Color color);
Triangle* create_up_arrow(int center_x, int center_y, int size, SDL_Color color);
Triangle* create_down_arrow(int center_x, int center_y, int size, SDL_Color color);
Triangle* create_left_arrow(int center_x, int center_y, int size, SDL_Color color);
Triangle* create_right_arrow(int center_x, int center_y, int size, SDL_Color color);
void draw_triangle(SDL_Renderer *renderer, Triangle* tri);
void draw_triangle_with_offset(SDL_Renderer *renderer, Triangle* tri, int offset_x, int offset_y);
void free_triangle(Triangle* tri);

// ═══════════════════════════════════════════════════════════════════════════════
// PRIMITIVES DE DESSIN POUR L'UI (dessin pur, sans logique métier)
// ═══════════════════════════════════════════════════════════════════════════════

// Dessine une ligne horizontale de séparation
// x1, y1 : point de départ (coordonnées absolues après offset)
// x2, y2 : point d'arrivée
// thickness : épaisseur en pixels
// color : couleur de la ligne
void draw_separator_line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2,
                         int thickness, SDL_Color color);

// Dessine un cadre rectangulaire (pour le preview)
// x, y : coin supérieur gauche (coordonnées absolues)
// width, height : dimensions
// color : couleur du cadre
void draw_frame_rect(SDL_Renderer* renderer, int x, int y, int width, int height,
                     SDL_Color color);

// Dessine un rectangle arrondi rempli (pour les boutons)
// x, y : coin supérieur gauche (coordonnées absolues)
// width, height : dimensions
// radius : rayon des coins arrondis
// bg_color : couleur de fond
// border_color : couleur de la bordure
void draw_rounded_rect(SDL_Renderer* renderer, int x, int y, int width, int height,
                       int radius, SDL_Color bg_color, SDL_Color border_color);

// Dessine le fond du toggle (rectangle arrondi)
// x, y : coin supérieur gauche (coordonnées absolues)
// width, height : dimensions du toggle
// radius : rayon des coins
// is_on : état du toggle (change la couleur)
void draw_toggle_background(SDL_Renderer* renderer, int x, int y, int width, int height,
                            int radius, bool is_on);

// Dessine le cercle mobile du toggle
// center_x, center_y : centre du cercle (coordonnées absolues)
// radius : rayon du cercle
// color : couleur du cercle
void draw_toggle_circle(SDL_Renderer* renderer, int center_x, int center_y,
                        int radius, SDL_Color color);

#endif
