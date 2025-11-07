// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "precompute_list.h"

// ════════════════════════════════════════════════════════════════════════
// STRUCTURE COUNTER STATE
// ════════════════════════════════════════════════════════════════════════
// Gère le compteur de respirations qui s'affiche au centre de l'hexagone
typedef struct {
    int current_breath;         // Cycle actuel (commence à 0, puis 1 au premier inspire)
    int total_breaths;          // Total de cycles à atteindre (depuis config.breath_cycles)
    bool is_active;             // Compteur actif ou non
    bool is_finished;           // Compteur terminé (atteint total_breaths)

    // 🆕 Calcul en temps réel (pas de precomputing)
    float breath_duration;      // Durée d'un cycle complet (inspire + expire) en secondes
    Uint32 start_time;          // Timestamp de démarrage (SDL_GetTicks)
    bool first_min_reached;     // true quand on a atteint le premier scale_min
    bool was_at_min;            // true si on était au scale_min à la frame précédente (pour détecter les transitions)

    // Configuration pour le calcul sinusoïdal
    SinusoidalConfig sin_config;

    // Couleur du texte (bleu-nuit cendré - même couleur que le timer)
    SDL_Color text_color;

    // Police TTF et taille de base
    TTF_Font* font;
    const char* font_path;
    int base_font_size;

} CounterState;

// ════════════════════════════════════════════════════════════════════════
// PROTOTYPES
// ════════════════════════════════════════════════════════════════════════

/**
 * Créer et initialiser un nouveau compteur de respirations
 * @param total_breaths Nombre total de cycles à compter (depuis config.Nb_respiration)
 * @param breath_duration Durée d'un cycle complet en secondes
 * @param sin_config Configuration sinusoïdale (scale_min, scale_max, etc.)
 * @param font_path Chemin vers la police TTF
 * @param base_font_size Taille de base de la police (sera scalée dynamiquement)
 * @return Pointeur vers le CounterState créé, NULL si erreur
 */
CounterState* counter_create(int total_breaths, float breath_duration,
                             const SinusoidalConfig* sin_config,
                             const char* font_path, int base_font_size);

/**
 * Démarrer le compteur (appelé quand le timer se termine)
 * @param counter Pointeur vers le compteur à démarrer
 */
void counter_start(CounterState* counter);

/**
 * Mettre à jour le compteur en fonction du temps écoulé
 * Calcule automatiquement le cycle de respiration actuel et le scale
 * @param counter Pointeur vers le compteur
 * @return true si le compteur est toujours actif, false s'il est terminé
 */
bool counter_update(CounterState* counter);

/**
 * Dessiner le compteur centré sur l'hexagone avec effet fish-eye
 * La taille du texte varie en fonction du scale de l'hexagone (poumon qui se remplit/vide)
 * @param counter Pointeur vers le compteur
 * @param renderer Renderer SDL2
 * @param center_x Position X du centre de l'hexagone
 * @param center_y Position Y du centre de l'hexagone
 * @param hex_radius Rayon de l'hexagone (pour calculer la largeur max du texte)
 * @param current_scale Scale actuel de l'hexagone (pour l'effet fish-eye)
 */
void counter_render(CounterState* counter, SDL_Renderer* renderer,
                    int center_x, int center_y, int hex_radius, double current_scale);

/**
 * Réinitialiser le compteur à sa valeur initiale
 * @param counter Pointeur vers le compteur
 */
void counter_reset(CounterState* counter);

/**
 * Libérer la mémoire du compteur
 * @param counter Pointeur vers le compteur à détruire
 */
void counter_destroy(CounterState* counter);

#endif
