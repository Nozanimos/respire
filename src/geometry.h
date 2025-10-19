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
void draw_triangle(SDL_Renderer *renderer, Triangle* tri);
void draw_triangle_with_offset(SDL_Renderer *renderer, Triangle* tri, int offset_x, int offset_y);
void free_triangle(Triangle* tri);

#endif
