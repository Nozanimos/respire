// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "geometry.h"

// ════════════════════════════════════════════════════════════════════════
// STRUCTURE TIMER STATE
// ════════════════════════════════════════════════════════════════════════
// Gère l'état du countdown avant le démarrage de la session
typedef struct TimerState {
    int total_seconds;          // Durée totale en secondes (ex: 10s)
    int remaining_seconds;      // Secondes restantes
    bool is_active;             // Timer actif ou non
    bool is_finished;           // Timer terminé

    // Pour le calcul du temps écoulé
    Uint32 last_update_time;    // Dernier tick SDL en ms

    // Couleur du texte (bleu-nuit cendré)
    SDL_Color text_color;

    // Police TTF pour le rendu
    TTF_Font* font;
    int font_size;
    char* font_path;  // Chemin vers le fichier .ttf (pour recharger avec différentes tailles)

} TimerState;

// ════════════════════════════════════════════════════════════════════════
// PROTOTYPES
// ════════════════════════════════════════════════════════════════════════

/**
 * Créer et initialiser un nouveau timer
 * @param duration_seconds Durée du countdown en secondes
 * @param font_path Chemin vers la police TTF
 * @param font_size Taille de la police
 * @return Pointeur vers le TimerState créé, NULL si erreur
 */
TimerState* breathing_timer_create(int duration_seconds, const char* font_path, int font_size);

/**
 * Démarrer le timer
 * @param timer Pointeur vers le timer à démarrer
 */
void timer_start(TimerState* timer);

/**
 * Mettre à jour le timer (décrémenter les secondes)
 * @param timer Pointeur vers le timer
 * @return true si le timer est toujours actif, false s'il est terminé
 */
bool timer_update(TimerState* timer);

/**
 * Formater le temps restant au format mm:ss
 * @param timer Pointeur vers le timer
 * @param buffer Buffer de sortie (EXACTEMENT 6 caractères : "00:00\0")
 *               Les minutes sont plafonnées à 99
 */
void timer_format(TimerState* timer, char* buffer);

/**
 * Dessiner le timer centré sur un hexagone
 * @param timer Pointeur vers le timer
 * @param renderer Renderer SDL2
 * @param center_x Position X du centre de l'hexagone
 * @param center_y Position Y du centre de l'hexagone
 * @param hex_radius Rayon de l'hexagone (la taille de police s'adapte automatiquement)
 */
void timer_render(TimerState* timer, SDL_Renderer* renderer,
                  int center_x, int center_y, int hex_radius);

/**
 * Réinitialiser le timer à sa valeur initiale
 * @param timer Pointeur vers le timer
 */
void timer_reset(TimerState* timer);

/**
 * Libérer la mémoire du timer
 * @param timer Pointeur vers le timer à détruire
 */
void timer_destroy(TimerState* timer);

#endif
