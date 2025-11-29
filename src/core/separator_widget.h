// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __SEPARATOR_WIDGET_H__
#define __SEPARATOR_WIDGET_H__

#include <SDL2/SDL.h>
#include <stdbool.h>

//  STRUCTURE DU WIDGET SEPARATOR
typedef struct {
    // ─────────────────────────────────────────────────────────────────────────
    // BASE (position et dimensions relatives)
    // ─────────────────────────────────────────────────────────────────────────
    struct {
        int x, y;              // Position actuelle (x = start_margin scalé, y fixe)
        int base_y;            // Position Y originale (depuis JSON, ne change jamais)
        int width, height;     // Dimensions de la ligne
        bool is_hovered;       // Toujours false (non interactif)
    } base;

    // ─────────────────────────────────────────────────────────────────────────
    // STYLE
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color color;                 // Couleur de la ligne
    int thickness;                   // Épaisseur en pixels

    // ─────────────────────────────────────────────────────────────────────────
    // MARGES DE BASE (pour rescaling)
    // ─────────────────────────────────────────────────────────────────────────
    int base_start_margin;           // Marge de départ (gauche)
    int base_end_margin;             // Marge de fin (droite)
    int base_width;                  // Largeur de base

} SeparatorWidget;

//  PROTOTYPES

// Crée un nouveau separator
// y : position verticale relative au panneau
// start_margin : marge à gauche
// end_margin : marge à droite
// thickness : épaisseur de la ligne
// color : couleur de la ligne
SeparatorWidget* create_separator_widget(int y, int start_margin, int end_margin,
                                         int thickness, SDL_Color color);

// Rendu du separator
// offset_x, offset_y : offset du panneau parent
// panel_width : largeur actuelle du panneau (pour calculer x2)
void render_separator_widget(SDL_Renderer* renderer, SeparatorWidget* sep,
                             int offset_x, int offset_y, int panel_width);

// Rescaling du separator selon le ratio du panneau
// panel_ratio : ratio de scaling
// panel_width : largeur actuelle du panneau
void rescale_separator_widget(SeparatorWidget* sep, float panel_ratio, int panel_width);

// Libère le separator
void free_separator_widget(SeparatorWidget* sep);

#endif // __SEPARATOR_WIDGET_H__
