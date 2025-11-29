// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "preview_widget.h"
#include "geometry.h"
//#include "animation.h"
#include "config.h"
#include "debug.h"
#include "core/memory/memory.h"

//  CRÉATION DU WIDGET PREVIEW
PreviewWidget* create_preview_widget(int x, int y, int frame_size,
                                     float size_ratio, float breath_duration) {
    PreviewWidget* preview = SAFE_MALLOC(sizeof(PreviewWidget));
    if (!preview) {
        debug_printf("❌ Erreur allocation PreviewWidget\n");
        return NULL;
    }

    // Initialiser la base
    preview->base.x = x;
    preview->base.y = y;
    preview->base.base_x = x;         // Position originale depuis JSON
    preview->base.base_y = y;         // Position originale depuis JSON
    preview->base.width = frame_size;
    preview->base.height = frame_size;
    preview->base.is_hovered = false;

    // Dimensions de base
    preview->base_frame_size = frame_size;
    preview->size_ratio = size_ratio;  // 5/6 = 0.833

    // ═════════════════════════════════════════════════════════════════════════
    // CALCUL DES DIMENSIONS DU CONTAINER HEXAGONES
    // ═════════════════════════════════════════════════════════════════════════
    // Le container fait size_ratio de la frame (ex: 83.3% pour 5/6)
    preview->container_size = (int)(frame_size * size_ratio);
    preview->center_x = preview->container_size / 2;
    preview->center_y = preview->container_size / 2;

    // ═════════════════════════════════════════════════════════════════════════
    // CRÉATION DES HEXAGONES
    // ═════════════════════════════════════════════════════════════════════════
    preview->hex_list = create_all_hexagones(
        preview->center_x,
        preview->center_y,
        preview->container_size,
        preview->size_ratio
    );

    if (!preview->hex_list) {
        debug_printf("❌ ERREUR: Impossible de créer les hexagones du preview\n");
        SAFE_FREE(preview);
        return NULL;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // PRÉ-CALCULER LES CYCLES D'ANIMATION
    // ═════════════════════════════════════════════════════════════════════════
    precompute_all_cycles(preview->hex_list, TARGET_FPS, breath_duration);

    // Animation
    preview->last_update = SDL_GetTicks();
    preview->current_time = 0.0;

    debug_printf("🎬 Preview créé - Pos: (%d,%d), Frame: %d, Container: %d, Ratio: %.3f\n",
                 x, y, frame_size, preview->container_size, size_ratio);

    return preview;
}

//  RENDU DU PREVIEW
void render_preview_widget(SDL_Renderer* renderer, PreviewWidget* preview,
                           int offset_x, int offset_y) {
    if (!renderer || !preview) return;

    // ═════════════════════════════════════════════════════════════════════════
    // CALCULER LA POSITION ABSOLUE DE LA FRAME
    // ═════════════════════════════════════════════════════════════════════════
    int frame_abs_x = offset_x + preview->base.x;
    int frame_abs_y = offset_y + preview->base.y;

    // ═════════════════════════════════════════════════════════════════════════
    // DESSINER LE CADRE BLANC (utilise geometry.c)
    // ═════════════════════════════════════════════════════════════════════════
    SDL_Color white = {255, 255, 255, 255};
    draw_frame_rect(renderer, frame_abs_x, frame_abs_y,
                    preview->base.width, preview->base.height, white);

    // ═════════════════════════════════════════════════════════════════════════
    // DESSINER LES HEXAGONES ANIMÉS
    // ═════════════════════════════════════════════════════════════════════════
    if (!preview->hex_list) return;

    // Calculer l'offset pour centrer le container dans la frame
    // Le container (83.3%) doit être centré dans la frame (100%)
    int offset_center_x = (preview->base.width - preview->container_size) / 2;
    int offset_center_y = (preview->base.height - preview->container_size) / 2;

    HexagoneNode* node = preview->hex_list->first;
    while (node && node->data) {
        Hexagon* hex = node->data;

        // ═════════════════════════════════════════════════════════════════════
        // POSITIONNER L'HEXAGONE EN COORDONNÉES ABSOLUES
        // ═════════════════════════════════════════════════════════════════════
        // Position absolue = offset_panneau + base_widget + offset_center + position_relative_hex
        int abs_x = frame_abs_x + offset_center_x + preview->center_x;
        int abs_y = frame_abs_y + offset_center_y + preview->center_y;

        transform_hexagon(hex, abs_x, abs_y, hex->current_scale);
        make_hexagone(renderer, hex);

        // Restaurer la position relative pour les prochains rendus
        transform_hexagon(hex, preview->center_x, preview->center_y, 1.0f);

        node = node->next;
    }
}

//  MISE À JOUR DE L'ANIMATION
void update_preview_widget(PreviewWidget* preview, float delta_time) {
    if (!preview || !preview->hex_list) return;

    // Note: delta_time est passé en paramètre mais on utilise SDL_GetTicks()
    // pour le calcul du temps réel (pour éviter les dérives de synchronisation)
    (void)delta_time;  // Marque explicitement comme inutilisé

    // ═════════════════════════════════════════════════════════════════════════
    // AVANCER DANS LE TEMPS
    // ═════════════════════════════════════════════════════════════════════════
    Uint32 current_ticks = SDL_GetTicks();
    float delta = (current_ticks - preview->last_update) / 1000.0f;
    preview->last_update = current_ticks;
    preview->current_time += delta;

    // ═════════════════════════════════════════════════════════════════════════
    // AVANCER D'UNE FRAME DANS LE PRÉCALCUL
    // ═════════════════════════════════════════════════════════════════════════
    HexagoneNode* node = preview->hex_list->first;
    while (node) {
        apply_precomputed_frame(node);
        node = node->next;
    }
}

//  RESCALING DU PREVIEW
void rescale_preview_widget(PreviewWidget* preview, float panel_ratio,
                            float breath_duration) {
    if (!preview) return;

    // ═════════════════════════════════════════════════════════════════════════
    // RECALCULER LES DIMENSIONS DE LA FRAME
    // ═════════════════════════════════════════════════════════════════════════
    preview->base.width = (int)(preview->base_frame_size * panel_ratio);
    preview->base.height = preview->base.width;  // Carré

    // ═════════════════════════════════════════════════════════════════════════
    // RECALCULER LE CONTAINER HEXAGONES (toujours 5/6 de la frame)
    // ═════════════════════════════════════════════════════════════════════════
    preview->container_size = (int)(preview->base.width * preview->size_ratio);
    preview->center_x = preview->container_size / 2;
    preview->center_y = preview->container_size / 2;

    debug_printf("🔄 Preview rescalé - Frame: %d, Container: %d, Ratio: %.2f\n",
                 preview->base.width, preview->container_size, panel_ratio);

    // ═════════════════════════════════════════════════════════════════════════
    // REDIMENSIONNER LES HEXAGONES
    // ═════════════════════════════════════════════════════════════════════════
    if (preview->hex_list) {
        HexagoneNode* node = preview->hex_list->first;
        int hex_count = 0;

        while (node && node->data) {
            Hexagon* hex = node->data;

            // Repositionner au nouveau centre (relatif)
            hex->center_x = preview->center_x;
            hex->center_y = preview->center_y;

            // Recalculer les sommets de base avec la nouvelle taille
            recalculer_sommets(hex, preview->container_size);
            hex->current_scale = 1.0f;

            hex_count++;
            node = node->next;
        }

        // ═════════════════════════════════════════════════════════════════════
        // RE-CALCULER TOUTES LES FRAMES ANIMÉES
        // ═════════════════════════════════════════════════════════════════════
        precompute_all_cycles(preview->hex_list, TARGET_FPS, breath_duration);

        debug_printf("✅ %d hexagones redimensionnés et frames recalculées\n", hex_count);
    }
}

//  CHANGER LA DURÉE DE RESPIRATION
void preview_set_breath_duration(PreviewWidget* preview, float new_duration) {
    if (!preview) return;

    debug_printf("🔄 Changement durée respiration preview: %.1fs\n", new_duration);

    // ═════════════════════════════════════════════════════════════════════════
    // LIBÉRER LES ANCIENS HEXAGONES
    // ═════════════════════════════════════════════════════════════════════════
    if (preview->hex_list) {
        free_hexagone_list(preview->hex_list);
        preview->hex_list = NULL;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // RECALCULER LES CENTRES RELATIFS
    // ═════════════════════════════════════════════════════════════════════════
    // ⚠️ IMPORTANT : Les centres doivent être recalculés à partir du container_size
    // pour rester au centre du cadre, sinon les hexagones disparaissent
    preview->center_x = preview->container_size / 2;
    preview->center_y = preview->container_size / 2;

    debug_printf("🔄 Recréation hexagones - Container: %d, Centre: (%d,%d), Ratio: %.2f\n",
                 preview->container_size, preview->center_x, preview->center_y,
                 preview->size_ratio);

    // ═════════════════════════════════════════════════════════════════════════
    // RECRÉER LES HEXAGONES AVEC LES BONS CENTRES
    // ═════════════════════════════════════════════════════════════════════════
    preview->hex_list = create_all_hexagones(
        preview->center_x,
        preview->center_y,
        preview->container_size,
        preview->size_ratio
    );

    if (!preview->hex_list) {
        debug_printf("❌ ERREUR: Impossible de recréer les hexagones\n");
        return;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // RE-PRÉCALCULER LES CYCLES AVEC LA NOUVELLE DURÉE
    // ═════════════════════════════════════════════════════════════════════════
    precompute_all_cycles(preview->hex_list, TARGET_FPS, new_duration);

    // Réinitialiser le temps
    preview->current_time = 0.0;
    preview->last_update = SDL_GetTicks();

    debug_printf("✅ Prévisualisation COMPLÈTEMENT réinitialisée avec nouvelle durée\n");
}

//  RÉINITIALISER LE PREVIEW
void reset_preview_widget(PreviewWidget* preview) {
    if (!preview) return;

    preview->current_time = 0.0;
    preview->last_update = SDL_GetTicks();

    debug_printf("🔄 Preview réinitialisé\n");
}

//  LIBÉRATION
void free_preview_widget(PreviewWidget* preview) {
    if (!preview) return;

    if (preview->hex_list) {
        free_hexagone_list(preview->hex_list);
    }

    SAFE_FREE(preview);
    debug_printf("🗑️ Preview libéré\n");
}
