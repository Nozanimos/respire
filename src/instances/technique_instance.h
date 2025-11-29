#ifndef TECHNIQUE_INSTANCE_H
#define TECHNIQUE_INSTANCE_H

#include <SDL2/SDL.h>
#include <stdbool.h>

// Forward declarations
typedef struct TechniqueInstance TechniqueInstance;

/**
 * Interface commune pour toutes les techniques de respiration/méditation
 * Chaque technique (WHM, Box Breathing, 4-7-8, etc.) implémente ces fonctions
 */
struct TechniqueInstance {
    // Nom de la technique (pour debug)
    const char* name;

    // Données spécifiques à la technique (AppState interne, etc.)
    void* technique_data;

    // Cycle de vie
    void (*init)(TechniqueInstance* self, SDL_Renderer* renderer);
    void (*handle_event)(TechniqueInstance* self, SDL_Event* event);
    void (*update)(TechniqueInstance* self, float delta_time);
    void (*render)(TechniqueInstance* self, SDL_Renderer* renderer);
    void (*cleanup)(TechniqueInstance* self);

    // État
    bool is_finished;  // true quand l'utilisateur veut retourner à l'accueil
    bool needs_high_fps;  // true si l'animation nécessite 60 FPS
};

/**
 * Créer une instance de technique
 * Cette fonction est un "factory" qui permet de créer différentes techniques
 */
TechniqueInstance* technique_create(const char* technique_name, SDL_Renderer* renderer);

/**
 * Détruire une instance de technique (appelle cleanup puis free)
 */
void technique_destroy(TechniqueInstance* instance);

#endif // TECHNIQUE_INSTANCE_H
