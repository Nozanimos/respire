// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __COUNTER_CACHE_H__
#define __COUNTER_CACHE_H__

#include <SDL2/SDL.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════════════════
// SYSTÈME DE CACHE DE TEXTURES POUR LE COMPTEUR
// ════════════════════════════════════════════════════════════════════════
// Ce système précharge toutes les textures de chiffres au démarrage avec
// différents niveaux de scale pour l'effet breathing.
//
// Avantages :
// - Cairo s'exécute UNE FOIS au démarrage (pas à chaque frame)
// - Runtime ultra-léger : simple blit de texture
// - Antialiasing premium maintenu
// - Responsive garanti (multiplication des échelles)
// ════════════════════════════════════════════════════════════════════════

// Nombre de niveaux de scale prérendus (10 niveaux = transitions fluides)
#define CACHE_SCALE_LEVELS 10

typedef struct {
    SDL_Texture*** textures;     // [number][scale_level] -> texture
    int max_numbers;              // Nombre max de respirations (depuis config)
    int scale_levels;             // Nombre de niveaux de scale précalculés

    double min_scale;             // Scale minimum (breathing)
    double max_scale;             // Scale maximum (breathing)

    const char* font_path;        // Police utilisée
    int base_font_size;           // Taille de base de la police
    SDL_Color text_color;         // Couleur du texte

    SDL_Renderer* renderer;       // Renderer pour créer les textures
} CounterTextureCache;

// ════════════════════════════════════════════════════════════════════════
// PROTOTYPES
// ════════════════════════════════════════════════════════════════════════

/**
 * Créer et initialiser le cache de textures
 * Prérendere toutes les combinaisons (nombre × scale_level) avec Cairo
 *
 * @param renderer SDL renderer
 * @param max_numbers Nombre maximum de respirations (typiquement config.Nb_respiration)
 * @param font_path Chemin vers la police TTF
 * @param base_font_size Taille de base de la police (avant scaling)
 * @param text_color Couleur du texte
 * @param min_scale Scale minimum de l'animation breathing
 * @param max_scale Scale maximum de l'animation breathing
 * @return Pointeur vers le cache créé, NULL si erreur
 */
CounterTextureCache* counter_cache_create(SDL_Renderer* renderer,
                                          int max_numbers,
                                          const char* font_path,
                                          int base_font_size,
                                          SDL_Color text_color,
                                          double min_scale,
                                          double max_scale);

/**
 * Récupérer une texture du cache
 * Sélectionne automatiquement le niveau de scale le plus proche
 *
 * @param cache Pointeur vers le cache
 * @param number Numéro à afficher (1, 2, 3...)
 * @param relative_breath_scale Scale relatif du breathing (0.0 à 1.0)
 * @param texture_width [OUT] Largeur de la texture (pour centrage)
 * @param texture_height [OUT] Hauteur de la texture (pour centrage)
 * @return Texture prérendue, NULL si erreur
 */
SDL_Texture* counter_cache_get(CounterTextureCache* cache,
                               int number,
                               double relative_breath_scale,
                               int* texture_width,
                               int* texture_height);

/**
 * Détruire le cache et libérer toutes les textures
 *
 * @param cache Pointeur vers le cache à détruire
 */
void counter_cache_destroy(CounterTextureCache* cache);

#endif
