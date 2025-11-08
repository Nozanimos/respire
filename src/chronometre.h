// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __CHRONOMETRE_H__
#define __CHRONOMETRE_H__

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "geometry.h"

// ════════════════════════════════════════════════════════════════════════
// STRUCTURE STOPWATCH STATE (CHRONOMÈTRE)
// ════════════════════════════════════════════════════════════════════════
// Gère le chronomètre qui INCRÉMENTE le temps après la session de respiration
// Contrairement au timer (countdown), le chronomètre part de 00:00 et monte
typedef struct {
    int elapsed_seconds;        // Nombre de secondes écoulées depuis le départ
    bool is_active;             // Chronomètre actif ou non
    bool is_stopped;            // Chronomètre arrêté (par ESPACE ou CLIC)

    // Pour le calcul du temps écoulé
    Uint32 start_time;          // Temps SDL au démarrage (en ms)
    Uint32 last_update_time;    // Dernier tick SDL en ms

    // Couleur du texte (bleu-nuit cendré - même couleur que timer/counter)
    SDL_Color text_color;

    // Police TTF pour le rendu
    TTF_Font* font;
    int font_size;

} StopwatchState;

// ════════════════════════════════════════════════════════════════════════
// PROTOTYPES
// ════════════════════════════════════════════════════════════════════════

/**
 * Créer et initialiser un nouveau chronomètre
 * @param font_path Chemin vers la police TTF
 * @param font_size Taille de la police
 * @return Pointeur vers le StopwatchState créé, NULL si erreur
 */
StopwatchState* stopwatch_create(const char* font_path, int font_size);

/**
 * Démarrer le chronomètre (commence à compter depuis 00:00)
 * @param stopwatch Pointeur vers le chronomètre à démarrer
 */
void stopwatch_start(StopwatchState* stopwatch);

/**
 * Arrêter le chronomètre (garde le temps final)
 * @param stopwatch Pointeur vers le chronomètre
 */
void stopwatch_stop(StopwatchState* stopwatch);

/**
 * Mettre à jour le chronomètre (incrémenter les secondes)
 * @param stopwatch Pointeur vers le chronomètre
 * @return true si le chronomètre est toujours actif, false s'il est arrêté
 */
bool stopwatch_update(StopwatchState* stopwatch);

/**
 * Récupérer le temps écoulé en secondes
 * @param stopwatch Pointeur vers le chronomètre
 * @return Nombre de secondes écoulées
 */
int stopwatch_get_elapsed_seconds(StopwatchState* stopwatch);

/**
 * Formater le temps écoulé au format mm:ss
 * @param stopwatch Pointeur vers le chronomètre
 * @param buffer Buffer de sortie (EXACTEMENT 6 caractères : "00:00\0")
 *               Les minutes sont plafonnées à 99
 */
void stopwatch_format(StopwatchState* stopwatch, char* buffer);

/**
 * Dessiner le chronomètre centré sur un hexagone
 * @param stopwatch Pointeur vers le chronomètre
 * @param renderer Renderer SDL2
 * @param center_x Position X du centre de l'hexagone
 * @param center_y Position Y du centre de l'hexagone
 * @param hex_radius Rayon de l'hexagone (pour calculer la largeur max du texte)
 */
void stopwatch_render(StopwatchState* stopwatch, SDL_Renderer* renderer,
                      int center_x, int center_y, int hex_radius);

/**
 * Réinitialiser le chronomètre à 00:00
 * @param stopwatch Pointeur vers le chronomètre
 */
void stopwatch_reset(StopwatchState* stopwatch);

/**
 * Libérer la mémoire du chronomètre
 * @param stopwatch Pointeur vers le chronomètre à détruire
 */
void stopwatch_destroy(StopwatchState* stopwatch);

#endif
