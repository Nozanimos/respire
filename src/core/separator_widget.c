// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdlib.h>
#include "separator_widget.h"
#include "geometry.h"
#include "debug.h"

//  CRÉATION DU WIDGET SEPARATOR
SeparatorWidget* create_separator_widget(int y, int start_margin, int end_margin,
                                         int thickness, SDL_Color color) {
    SeparatorWidget* sep = malloc(sizeof(SeparatorWidget));
    if (!sep) {
        debug_printf("❌ Erreur allocation SeparatorWidget\n");
        return NULL;
    }

    // Initialiser la base
    sep->base.x = start_margin;
    sep->base.y = y;
    sep->base.base_y = y;        // Position Y originale (ne change jamais)
    sep->base.width = 0;         // Sera calculé lors du rescale
    sep->base.height = thickness;
    sep->base.is_hovered = false;

    // Style
    sep->color = color;
    sep->thickness = thickness;

    // Dimensions de base
    sep->base_start_margin = start_margin;
    sep->base_end_margin = end_margin;
    sep->base_width = 0;  // Sera calculé selon la largeur du panneau

    debug_printf("📏 Separator créé - Y: %d, Marges: (%d, %d), Épaisseur: %d\n",
                 y, start_margin, end_margin, thickness);

    return sep;
}

//  RENDU DU SEPARATOR
void render_separator_widget(SDL_Renderer* renderer, SeparatorWidget* sep,
                             int offset_x, int offset_y, int panel_width) {
    if (!renderer || !sep) return;

    // ═════════════════════════════════════════════════════════════════════════
    // CALCULER LES POSITIONS ABSOLUES
    // ═════════════════════════════════════════════════════════════════════════
    int x1 = offset_x + sep->base.x;
    int y1 = offset_y + sep->base.y;
    int x2 = offset_x + panel_width - sep->base_end_margin;
    int y2 = y1;

    // ═════════════════════════════════════════════════════════════════════════
    // DESSINER LA LIGNE (utilise la fonction de geometry.c)
    // ═════════════════════════════════════════════════════════════════════════
    draw_separator_line(renderer, x1, y1, x2, y2, sep->thickness, sep->color);
}

//  RESCALING DU SEPARATOR
void rescale_separator_widget(SeparatorWidget* sep, float panel_ratio, int panel_width) {
    if (!sep) return;

    // ═════════════════════════════════════════════════════════════════════════
    // RECALCULER LES MARGES SELON LE RATIO
    // ═════════════════════════════════════════════════════════════════════════
    sep->base.x = (int)(sep->base_start_margin * panel_ratio);
    int end_margin_scaled = (int)(sep->base_end_margin * panel_ratio);

    // ═════════════════════════════════════════════════════════════════════════
    // CALCULER LA LARGEUR
    // ═════════════════════════════════════════════════════════════════════════
    sep->base.width = panel_width - sep->base.x - end_margin_scaled;

    // Épaisseur minimum 1px
    sep->thickness = (int)(sep->base.height * panel_ratio);
    if (sep->thickness < 1) sep->thickness = 1;

    debug_printf("🔄 Separator rescalé - Largeur: %d, Épaisseur: %d (ratio: %.2f)\n",
                 sep->base.width, sep->thickness, panel_ratio);
}

//  LIBÉRATION
void free_separator_widget(SeparatorWidget* sep) {
    if (sep) {
        free(sep);
        debug_printf("🗑️ Separator libéré\n");
    }
}
