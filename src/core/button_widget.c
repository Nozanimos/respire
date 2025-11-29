// SPDX-License-Identifier: GPL-3.0-or-later
#include <string.h>
#include <stdlib.h>
#include "button_widget.h"
#include "geometry.h"
//#include "renderer.h"
#include "widget_base.h"
#include "debug.h"

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION DU WIDGET BUTTON
// ════════════════════════════════════════════════════════════════════════════
ButtonWidget* create_button_widget(const char* text, int x, int y,
                                   int width, int height, int text_size,
                                   SDL_Color bg_color, ButtonYAnchor y_anchor) {
    ButtonWidget* button = malloc(sizeof(ButtonWidget));
    if (!button) {
        debug_printf("❌ Erreur allocation ButtonWidget\n");
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISER LA BASE
    // ─────────────────────────────────────────────────────────────────────────
    // IMPORTANT : (x, y) représente le coin HAUT-GAUCHE du bouton, exactement
    // comme pour tous les autres widgets (LabelWidget, IncrementWidget, etc.).
    // Cela assure une cohérence totale dans le système de positionnement.
    // ─────────────────────────────────────────────────────────────────────────
    button->base.x = x;
    button->base.y = y;
    button->base.width = width;
    button->base.height = height;
    button->base.is_hovered = false;

    // Copier le texte
    strncpy(button->text, text, sizeof(button->text) - 1);
    button->text[sizeof(button->text) - 1] = '\0';

    // Style
    button->bg_color = bg_color;

    // Couleur hover = version plus claire
    button->hover_color.r = (bg_color.r + 40 > 255) ? 255 : bg_color.r + 40;
    button->hover_color.g = (bg_color.g + 40 > 255) ? 255 : bg_color.g + 40;
    button->hover_color.b = (bg_color.b + 40 > 255) ? 255 : bg_color.b + 40;
    button->hover_color.a = bg_color.a;

    button->text_color = (SDL_Color){255, 255, 255, 255};  // Blanc par défaut
    button->border_color = (SDL_Color){0, 0, 0, 255};      // Noir
    button->border_radius = 5;  // Coins légèrement arrondis

    // Police
    button->base_text_size = text_size;
    button->current_text_size = text_size;

    // Dimensions de base et ancrage
    button->base_width = width;
    button->base_height = height;
    button->base_x = x;
    button->base_y = y;
    button->y_anchor = y_anchor;

    // Callback
    button->on_click = NULL;

    const char* anchor_str = (y_anchor == BUTTON_ANCHOR_TOP) ? "TOP" : "BOTTOM";
    debug_printf("🔘 Bouton créé - Texte: \"%s\", Coin HG: (%d,%d), Taille: %dx%d, Anchor: %s\n",
                 text, x, y, width, height, anchor_str);

    return button;
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU BUTTON
// ════════════════════════════════════════════════════════════════════════════
void render_button_widget(SDL_Renderer* renderer, ButtonWidget* button,
                          int offset_x, int offset_y) {
    if (!renderer || !button) return;

    // ═════════════════════════════════════════════════════════════════════════
    // CALCULER LA POSITION ABSOLUE
    // ═════════════════════════════════════════════════════════════════════════
    int abs_x = offset_x + button->base.x;
    int abs_y = offset_y + button->base.y;

    // ═════════════════════════════════════════════════════════════════════════
    // DESSINER LE FOND DU BOUTON (utilise geometry.c)
    // ═════════════════════════════════════════════════════════════════════════
    SDL_Color current_color = button->base.is_hovered ?
                              button->hover_color : button->bg_color;

    draw_rounded_rect(renderer, abs_x, abs_y,
                      button->base.width, button->base.height,
                      button->border_radius, current_color, button->border_color);

    // ═════════════════════════════════════════════════════════════════════════
    // RENDRE LE TEXTE CENTRÉ
    // ═════════════════════════════════════════════════════════════════════════
    TTF_Font* font = get_font_for_size(button->current_text_size);
    if (!font) {
        debug_printf("❌ Impossible d'obtenir la police pour le bouton\n");
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, button->text, button->text_color);
    if (!surface) {
        debug_printf("❌ Erreur rendu texte bouton: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        debug_printf("❌ Erreur création texture bouton\n");
        SDL_FreeSurface(surface);
        return;
    }

    // Centrer le texte dans le bouton
    int text_x = abs_x + (button->base.width - surface->w) / 2;
    int text_y = abs_y + (button->base.height - surface->h) / 2;

    SDL_Rect text_rect = {text_x, text_y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);

    // Nettoyage
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════
void handle_button_widget_events(ButtonWidget* button, SDL_Event* event,
                                 int offset_x, int offset_y) {
    if (!button || !event) return;

    if (event->type == SDL_MOUSEMOTION) {
        // ═════════════════════════════════════════════════════════════════════
        // VÉRIFIER LE SURVOL
        // ═════════════════════════════════════════════════════════════════════
        int mx = event->motion.x;
        int my = event->motion.y;

        int abs_x = offset_x + button->base.x;
        int abs_y = offset_y + button->base.y;

        button->base.is_hovered = (mx >= abs_x && mx <= abs_x + button->base.width &&
                                   my >= abs_y && my <= abs_y + button->base.height);
    }
    else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        // ═════════════════════════════════════════════════════════════════════
        // CLIC SUR LE BOUTON
        // ═════════════════════════════════════════════════════════════════════
        if (button->base.is_hovered && button->on_click) {
            debug_printf("🖱️ Clic sur bouton: \"%s\"\n", button->text);
            button->on_click();
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  RESCALING DU BUTTON
// ════════════════════════════════════════════════════════════════════════════
void rescale_button_widget(ButtonWidget* button, float panel_ratio) {
    if (!button) return;

    // ═════════════════════════════════════════════════════════════════════════
    // RECALCULER LES DIMENSIONS
    // ═════════════════════════════════════════════════════════════════════════
    button->base.width = (int)(button->base_width * panel_ratio);
    button->base.height = (int)(button->base_height * panel_ratio);

    // ═════════════════════════════════════════════════════════════════════════
    // RECALCULER LA TAILLE DE POLICE
    // ═════════════════════════════════════════════════════════════════════════
    button->current_text_size = (int)(button->base_text_size * panel_ratio);

    // Minimum 12px
    if (button->current_text_size < 12) {
        button->current_text_size = 12;
    }

    debug_printf("🔄 Bouton rescalé - Taille: %dx%d, Police: %d (ratio: %.2f)\n",
                 button->base.width, button->base.height,
                 button->current_text_size, panel_ratio);
}

// ════════════════════════════════════════════════════════════════════════════
//  DÉFINIR LE CALLBACK
// ════════════════════════════════════════════════════════════════════════════
void set_button_click_callback(ButtonWidget* button, void (*callback)(void)) {
    if (button) {
        button->on_click = callback;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION
// ════════════════════════════════════════════════════════════════════════════
void free_button_widget(ButtonWidget* button) {
    if (button) {
        free(button);
        debug_printf("🗑️ Bouton libéré\n");
    }
}
