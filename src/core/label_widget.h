// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __LABEL_WIDGET_H__
#define __LABEL_WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════════════════════
//  ALIGNEMENT DU LABEL
// ════════════════════════════════════════════════════════════════════════════
typedef enum {
    LABEL_ALIGN_LEFT,      // Aligné à gauche
    LABEL_ALIGN_CENTER,    // Centré dans le panneau
    LABEL_ALIGN_RIGHT      // Aligné à droite
} LabelAlignment;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE DU WIDGET LABEL
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    // ─────────────────────────────────────────────────────────────────────────
    // BASE (position et dimensions relatives)
    // ─────────────────────────────────────────────────────────────────────────
    struct {
        int x, y;              // Position actuelle (après scaling/empilement)
        int base_x, base_y;    // Position de référence originale (depuis JSON)
        int width, height;     // Dimensions (calculées lors du rendu)
        bool is_hovered;       // Toujours false (non interactif)
    } base;

    // ─────────────────────────────────────────────────────────────────────────
    // TEXTE ET STYLE
    // ─────────────────────────────────────────────────────────────────────────
    char text[256];                  // Texte à afficher
    SDL_Color color;                 // Couleur du texte
    bool underlined;                 // Si true, souligne le texte

    // ─────────────────────────────────────────────────────────────────────────
    // POLICE
    // ─────────────────────────────────────────────────────────────────────────
    int base_text_size;              // Taille de police de base
    int current_text_size;           // Taille de police actuelle (après scaling)

    // ─────────────────────────────────────────────────────────────────────────
    // ALIGNEMENT
    // ─────────────────────────────────────────────────────────────────────────
    LabelAlignment alignment;        // Alignement du label (left/center/right)

} LabelWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES
// ════════════════════════════════════════════════════════════════════════════

// Crée un nouveau label
// x, y : position relative au panneau
// text_size : taille de la police
// color : couleur du texte
// underlined : si true, souligne le texte
// alignment : alignement du label (left/center/right)
LabelWidget* create_label_widget(const char* text, int x, int y,
                                 int text_size, SDL_Color color, bool underlined,
                                 LabelAlignment alignment);

// Rendu du label
// offset_x, offset_y : offset du panneau parent
void render_label_widget(SDL_Renderer* renderer, LabelWidget* label,
                         int offset_x, int offset_y);

// Rescaling du label selon le ratio du panneau
void rescale_label_widget(LabelWidget* label, float panel_ratio);

// Change le texte du label
void set_label_text(LabelWidget* label, const char* new_text);

// Libère le label
void free_label_widget(LabelWidget* label);

#endif // __LABEL_WIDGET_H__
