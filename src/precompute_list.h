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

// ════════════════════════════════════════════════════════════════════════
// STRUCTURE POUR LE PRÉCOMPUTING DU COMPTEUR DE RESPIRATIONS
// ════════════════════════════════════════════════════════════════════════
// Stocke les données précalculées pour chaque frame du compteur :
// - Flag indiquant si on est au scale_min (inspire)
// - Le scale pour l'effet fish-eye (synchronisé avec l'hexagone)
typedef struct {
    bool is_at_scale_min;  // 🚩 true si cette frame est au scale_min (inspire)
    bool is_at_scale_max;  // 🚩 true si cette frame est au scale_max (expire)
    double text_scale;     // Scale du texte (suit le scale de l'hexagone)
} CounterFrame;

// Pointeur de fonction type
typedef void (*SinusoidalMovementFunc)(double frame_time, const SinusoidalConfig* config, SinusoidalResult* result);

typedef struct HexagoneNode {
    Hexagon* data;
    Animation* animation;

    // Coordonnées précalculées pour chaque frame
    Sint16* precomputed_vx;
    Sint16* precomputed_vy;

    // 🆕 Frames précalculées pour le compteur de respirations
    CounterFrame* precomputed_counter_frames;  // Tableau synchronisé avec les frames de l'hexagone
    double current_scale;         // Scale actuel

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

// 🆕 PRÉCOMPUTING DU COMPTEUR DE RESPIRATIONS (génère les flags scale_min)
void precompute_counter_frames(HexagoneNode* node, int total_frames, int fps,
                               float breath_duration, int max_breaths);

/*------------------------- Fonctions utiles ----------------------------------*/
int gcd(int a, int b);
int lcm(int a, int b);
double gcd_fractional(double a, double b);

#endif
