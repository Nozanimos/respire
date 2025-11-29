// SPDX-License-Identifier: GPL-3.0-or-later
#include <string.h>
#include <stdlib.h>
#include "label_widget.h"
#include "renderer.h"
#include "debug.h"

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION DU WIDGET LABEL
// ════════════════════════════════════════════════════════════════════════════
LabelWidget* create_label_widget(const char* text, int x, int y,
                                 int text_size, SDL_Color color, bool underlined,
                                 LabelAlignment alignment) {
    LabelWidget* label = malloc(sizeof(LabelWidget));
    if (!label) {
        debug_printf("❌ Erreur allocation LabelWidget\n");
        return NULL;
    }

    // Initialiser la base
    label->base.x = x;
    label->base.y = y;
    label->base.base_x = x;   // Position originale depuis JSON
    label->base.base_y = y;   // Position originale depuis JSON
    label->base.width = 0;    // Sera calculé lors du rendu
    label->base.height = 0;   // Sera calculé lors du rendu
    label->base.is_hovered = false;

    // Copier le texte
    strncpy(label->text, text, sizeof(label->text) - 1);
    label->text[sizeof(label->text) - 1] = '\0';

    // Style
    label->color = color;
    label->underlined = underlined;

    // Taille de police
    label->base_text_size = text_size;
    label->current_text_size = text_size;

    // Alignement
    label->alignment = alignment;

    const char* align_str = (alignment == LABEL_ALIGN_LEFT) ? "left" :
                           (alignment == LABEL_ALIGN_RIGHT) ? "right" : "center";
    debug_printf("📝 Label créé - Texte: \"%s\", Pos: (%d,%d), Taille: %d, Align: %s\n",
                 text, x, y, text_size, align_str);

    return label;
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU LABEL
// ════════════════════════════════════════════════════════════════════════════
void render_label_widget(SDL_Renderer* renderer, LabelWidget* label,
                         int offset_x, int offset_y) {
    if (!renderer || !label) return;

    // ═════════════════════════════════════════════════════════════════════════
    // CALCULER LA POSITION ABSOLUE
    // ═════════════════════════════════════════════════════════════════════════
    int abs_x = offset_x + label->base.x;
    int abs_y = offset_y + label->base.y;

    // ═════════════════════════════════════════════════════════════════════════
    // RENDRE LE TEXTE
    // ═════════════════════════════════════════════════════════════════════════
    TTF_Font* font = get_font_for_size(label->current_text_size);
    if (!font) {
        debug_printf("❌ Impossible d'obtenir la police pour le label\n");
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, label->text, label->color);
    if (!surface) {
        debug_printf("❌ Erreur rendu texte label: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        debug_printf("❌ Erreur création texture label\n");
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect text_rect = {abs_x, abs_y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);

    // ═════════════════════════════════════════════════════════════════════════
    // DESSINER LA LIGNE DE SOULIGNEMENT SI DEMANDÉ
    // ═════════════════════════════════════════════════════════════════════════
    if (label->underlined) {
        int underline_y = abs_y + surface->h + 2;  // 2px sous le texte
        SDL_SetRenderDrawColor(renderer, label->color.r, label->color.g,
                               label->color.b, label->color.a);
        SDL_RenderDrawLine(renderer, abs_x, underline_y,
                           abs_x + surface->w, underline_y);
    }

    // Mettre à jour les dimensions (pour usage futur)
    label->base.width = surface->w;
    label->base.height = surface->h;

    // Nettoyage
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// ════════════════════════════════════════════════════════════════════════════
//  RESCALING DU LABEL
// ════════════════════════════════════════════════════════════════════════════
void rescale_label_widget(LabelWidget* label, float panel_ratio) {
    if (!label) return;

    // ═════════════════════════════════════════════════════════════════════════
    // RECALCULER LA TAILLE DE POLICE
    // ═════════════════════════════════════════════════════════════════════════
    label->current_text_size = (int)(label->base_text_size * panel_ratio);

    // Minimum 10px
    if (label->current_text_size < 10) {
        label->current_text_size = 10;
    }

    debug_printf("🔄 Label rescalé - Police: %d (ratio: %.2f)\n",
                 label->current_text_size, panel_ratio);
}

// ════════════════════════════════════════════════════════════════════════════
//  CHANGER LE TEXTE DU LABEL
// ════════════════════════════════════════════════════════════════════════════
void set_label_text(LabelWidget* label, const char* new_text) {
    if (!label || !new_text) return;

    strncpy(label->text, new_text, sizeof(label->text) - 1);
    label->text[sizeof(label->text) - 1] = '\0';

    debug_printf("✏️ Texte du label changé: \"%s\"\n", new_text);
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION
// ════════════════════════════════════════════════════════════════════════════
void free_label_widget(LabelWidget* label) {
    if (label) {
        free(label);
        debug_printf("🗑️ Label libéré\n");
    }
}
