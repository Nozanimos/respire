// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __COUNTER_CACHE_H__
#define __COUNTER_CACHE_H__

#include <SDL2/SDL.h>
#include <stdbool.h>

// SYSTÈME DE CACHE DE TEXTURES POUR LE COMPTEUR
// Ce système précharge TOUTES les textures pour chaque frame d'un cycle
// de respiration complet. Chaque texture est rendue à la taille exacte
// calculée par sinusoidal_movement.
//
// Avantages :
// - Cairo s'exécute UNE FOIS au démarrage (pas à chaque frame)
// - Qualité vectorielle parfaite (pas de scaling GPU)
// - Zéro saccade (texture exacte par frame, pas d'interpolation)
// - Tailles identiques garanties (même formule de scale pour tous)
// - Runtime ultra-léger : lookup O(1) + blit direct
// - Responsive garanti (multiplication du scale_factor uniquement)

typedef struct {
    SDL_Texture*** textures;      // [number][frame_index] -> texture
    int max_numbers;               // Nombre max de respirations (depuis config)
    int frames_per_cycle;          // Nombre de frames par cycle (fps × breath_duration)

    int fps;                       // FPS de l'animation (60)
    float breath_duration;         // Durée d'un cycle en secondes (3.0)

    double min_scale;              // Scale minimum (breathing)
    double max_scale;              // Scale maximum (breathing)

    const char* font_path;         // Police utilisée
    int base_font_size;            // Taille de base de la police
    SDL_Color text_color;          // Couleur du texte

    SDL_Renderer* renderer;        // Renderer pour créer les textures
} CounterTextureCache;

// PROTOTYPES

/**
 * Créer et initialiser le cache de textures
 * Précalcule TOUTES les textures pour un cycle complet de respiration
 * en utilisant sinusoidal_movement pour calculer le scale exact de chaque frame
 *
 * @param renderer SDL renderer
 * @param max_numbers Nombre maximum de respirations (typiquement config.Nb_respiration)
 * @param font_path Chemin vers la police TTF
 * @param base_font_size Taille de base de la police (avant scaling)
 * @param text_color Couleur du texte
 * @param min_scale Scale minimum de l'animation breathing
 * @param max_scale Scale maximum de l'animation breathing
 * @param fps Frames par seconde (TARGET_FPS, typiquement 60)
 * @param breath_duration Durée d'un cycle en secondes (config.breath_duration)
 * @return Pointeur vers le cache créé, NULL si erreur
 */
CounterTextureCache* counter_cache_create(SDL_Renderer* renderer,
                                          int max_numbers,
                                          const char* font_path,
                                          int base_font_size,
                                          SDL_Color text_color,
                                          double min_scale,
                                          double max_scale,
                                          int fps,
                                          float breath_duration);

/**
 * Récupérer une texture du cache pour une frame spécifique
 * Lookup direct O(1) : textures[number-1][frame_index % frames_per_cycle]
 *
 * @param cache Pointeur vers le cache
 * @param number Numéro à afficher (1, 2, 3...)
 * @param frame_index Index de la frame actuelle (hex_node->current_cycle)
 * @param texture_width [OUT] Largeur de la texture (pour centrage)
 * @param texture_height [OUT] Hauteur de la texture (pour centrage)
 * @return Texture prérendue à la taille exacte, NULL si erreur
 */
SDL_Texture* counter_cache_get(CounterTextureCache* cache,
                               int number,
                               int frame_index,
                               int* texture_width,
                               int* texture_height);

/**
 * Détruire le cache et libérer toutes les textures
 *
 * @param cache Pointeur vers le cache à détruire
 */
void counter_cache_destroy(CounterTextureCache* cache);

#endif
