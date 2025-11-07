// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __PRECOMPUTE_LIST_H__
#define __PRECOMPUTE_LIST_H__


#include <stdbool.h>
#include <SDL2/SDL.h>
#include "animation.h"
#include "geometry.h"
#include "config.h"

// Déclaration anticipée
typedef struct Hexagon Hexagon;

// STRUCTURES GÉNÉRIQUES POUR LE MOUVEMENT SINUSOÏDAL
typedef struct {
    double angle_per_cycle;
    double scale_min;
    double scale_max;
    bool clockwise;
    double breath_duration;
} SinusoidalConfig;

typedef struct {
    double rotation;
    double scale;
} SinusoidalResult;

// Pointeur de fonction type
typedef void (*SinusoidalMovementFunc)(double frame_time, const SinusoidalConfig* config, SinusoidalResult* result);

typedef struct HexagoneNode {
    Hexagon* data;
    Animation* animation;

    // Coordonnées précalculées pour chaque frame
    Sint16* precomputed_vx;
    Sint16* precomputed_vy;

    // 🆕 Scales précalculés pour chaque frame (utilisé par le compteur)
    double* precomputed_scales;  // Tableau des scales pour effet fish-eye
    double current_scale;         // Scale actuel (mis à jour par apply_precomputed_frame)

    int total_cycles;
    int current_cycle;

    // 🆕 Flag pour figer l'animation
    bool is_frozen;               // Si true, apply_precomputed_frame ne fait rien

    struct HexagoneNode* prev;
    struct HexagoneNode* next;
} HexagoneNode;

typedef struct HexagoneList {
    HexagoneNode* first;
    HexagoneNode* last;
    int count;
} HexagoneList;

/*----- Prototypes -----*/
HexagoneList* new_hexagone_list(void);
bool is_empty_hexagone_list(HexagoneList *li);
int hexagone_list_count(HexagoneList *li);
void add_hexagone(HexagoneList* list, Hexagon* hex, Animation* anim);
bool are_coordinates_identical(Sint16* vx1, Sint16* vy1, Sint16* vx2, Sint16* vy2);
void free_hexagone_list(HexagoneList* list);

// NOUVELLE VERSION AVEC STRUCTURE GÉNÉRIQUE
void sinusoidal_movement(double frame_time, const SinusoidalConfig* config, SinusoidalResult* result);

void precompute_all_cycles(HexagoneList* list, int fps, float breath_duration);
void apply_precomputed_frame(HexagoneNode* node);
int calculate_alignment_cycles(void);
void debug_print_list_order(HexagoneList* list);
void print_rotation_frame_requirements(HexagoneList* list, int fps, float breath_duration);

/*------------------------- Fonctions utiles ----------------------------------*/
int gcd(int a, int b);
int lcm(int a, int b);
double gcd_fractional(double a, double b);

#endif
