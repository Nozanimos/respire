// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __PREVIEW_WIDGET_H__
#define __PREVIEW_WIDGET_H__

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "preview_list.h"

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE DU WIDGET PREVIEW
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    // ─────────────────────────────────────────────────────────────────────────
    // BASE (position et dimensions relatives)
    // ─────────────────────────────────────────────────────────────────────────
    struct {
        int x, y;              // Position relative au panneau
        int width, height;     // Dimensions de la frame (carré)
        bool is_hovered;       // Toujours false (non interactif)
    } base;

    // ─────────────────────────────────────────────────────────────────────────
    // DIMENSIONS
    // ─────────────────────────────────────────────────────────────────────────
    int base_frame_size;             // Taille de base de la frame carrée
    int container_size;              // Taille du container hexagones (frame * size_ratio)
    float size_ratio;                // Ratio container/frame (ex: 5/6 = 0.833)

    // ─────────────────────────────────────────────────────────────────────────
    // CENTRE DU CONTAINER HEXAGONES (relatif au container)
    // ─────────────────────────────────────────────────────────────────────────
    int center_x;
    int center_y;

    // ─────────────────────────────────────────────────────────────────────────
    // LISTE DES HEXAGONES ANIMÉS
    // ─────────────────────────────────────────────────────────────────────────
    HexagoneList* hex_list;          // Liste de tous les hexagones

    // ─────────────────────────────────────────────────────────────────────────
    // ANIMATION
    // ─────────────────────────────────────────────────────────────────────────
    Uint32 last_update;              // Timestamp de la dernière mise à jour
    float current_time;              // Temps écoulé depuis le début

} PreviewWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES
// ════════════════════════════════════════════════════════════════════════════

// Crée un nouveau preview
// x, y : position relative au panneau
// frame_size : taille de la frame carrée
// size_ratio : ratio entre container et frame (ex: 5/6)
// breath_duration : durée d'un cycle de respiration en secondes
PreviewWidget* create_preview_widget(int x, int y, int frame_size,
                                     float size_ratio, float breath_duration);

// Rendu du preview
// offset_x, offset_y : offset du panneau parent
void render_preview_widget(SDL_Renderer* renderer, PreviewWidget* preview,
                           int offset_x, int offset_y);

// Mise à jour de l'animation
// delta_time : temps écoulé depuis la dernière frame (en secondes)
void update_preview_widget(PreviewWidget* preview, float delta_time);

// Rescaling du preview selon le ratio du panneau
// panel_ratio : ratio de scaling du panneau
// breath_duration : durée de respiration (pour recalculer les frames)
void rescale_preview_widget(PreviewWidget* preview, float panel_ratio,
                            float breath_duration);

// Change la durée de respiration
void preview_set_breath_duration(PreviewWidget* preview, float new_duration);

// Réinitialise l'animation
void reset_preview_widget(PreviewWidget* preview);

// Libère le preview
void free_preview_widget(PreviewWidget* preview);

#endif // __PREVIEW_WIDGET_H__
