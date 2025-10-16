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

#endif
