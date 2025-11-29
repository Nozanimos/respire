// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "precompute_list.h"
#include "counter_cache.h"

// ════════════════════════════════════════════════════════════════════════
// STRUCTURE COUNTER STATE
// ════════════════════════════════════════════════════════════════════════
// Gère l'affichage du compteur de respirations au centre de l'hexagone
// Les données (numéro de respiration, scale) viennent du précomputing
typedef struct CounterState {
    int total_breaths;          // Total de cycles configuré (pour vérification)
    bool is_active;             // Compteur actif ou non
    int retention_type;         // Type de rétention: 0=poumons pleins, 1=poumons vides

    // 🆕 État du compteur (persiste même quand le précomputing reboucle)
    int current_breath;         // Numéro actuel (0 au départ, puis 1, 2, 3...)
    bool was_at_min_last_frame; // État du flag à la frame précédente (pour détecter transitions)

    // 🆕 Gestion de la fin de session (attendre le scale_max après la dernière respiration)
    bool waiting_for_scale_min;  // Attend le scale_min final (poumons vides) après la dernière respiration
    bool was_at_max_last_frame; // État du flag scale_max à la frame précédente

    // Couleur du texte (bleu-nuit cendré - même couleur que le timer)
    SDL_Color text_color;

    // Police TTF et taille de base
    const char* font_path;
    int base_font_size;

    // 🎨 Cache de textures prérendues (Cairo au démarrage, blit au runtime)
    CounterTextureCache* cache;
} CounterState;

// Alias pour BreathCounter (utilisé dans d'autres modules)
typedef CounterState BreathCounter;

// ════════════════════════════════════════════════════════════════════════
// PROTOTYPES
// ════════════════════════════════════════════════════════════════════════

/**
 * Créer et initialiser un nouveau compteur de respirations
 * @param renderer SDL renderer (pour créer le cache de textures)
 * @param total_breaths Nombre total de cycles à compter (depuis config.Nb_respiration)
 * @param retention_type Type de rétention (0=poumons pleins, 1=poumons vides)
 * @param font_path Chemin vers la police TTF
 * @param base_font_size Taille de base de la police (sera scalée dynamiquement)
 * @param scale_min Scale minimum de l'animation breathing (pour le cache)
 * @param scale_max Scale maximum de l'animation breathing (pour le cache)
 * @param fps Frames par seconde (TARGET_FPS)
 * @param breath_duration Durée d'un cycle en secondes (config.breath_duration)
 * @return Pointeur vers le CounterState créé, NULL si erreur
 */
CounterState* counter_create(SDL_Renderer* renderer, int total_breaths, int retention_type,
                             const char* font_path, int base_font_size,
                             double scale_min, double scale_max,
                             int fps, float breath_duration);

/**
 * Dessiner le compteur centré sur l'hexagone avec effet fish-eye
 * La taille du texte varie en fonction du scale de l'hexagone (poumon qui se remplit/vide)
 * @param counter Pointeur vers le compteur
 * @param renderer Renderer SDL2
 * @param center_x Position X du centre de l'hexagone
 * @param center_y Position Y du centre de l'hexagone
 * @param hex_radius Rayon de l'hexagone (pour calculer la largeur max du texte)
 * @param hex_node Noeud de l'hexagone (contient données précomputées)
 * @param scale_factor Facteur d'échelle de la fenêtre (responsive)
 */
void counter_render(CounterState* counter, SDL_Renderer* renderer,
                    int center_x, int center_y, int hex_radius, HexagoneNode* hex_node,
                    float scale_factor);

/**
 * Libérer la mémoire du compteur
 * @param counter Pointeur vers le compteur à détruire
 */
void counter_destroy(CounterState* counter);

#endif
