// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <math.h>
#include "counter.h"
#include "debug.h"

// ════════════════════════════════════════════════════════════════════════
// CRÉATION DU COMPTEUR
// ════════════════════════════════════════════════════════════════════════
CounterState* counter_create(int total_breaths, int retention_type, const char* font_path, int base_font_size) {
    // Allocation de la structure
    CounterState* counter = malloc(sizeof(CounterState));
    if (!counter) {
        fprintf(stderr, "❌ Erreur allocation CounterState\n");
        return NULL;
    }

    // Initialisation des valeurs
    counter->total_breaths = total_breaths;
    counter->retention_type = retention_type;
    counter->is_active = false;

    // 🆕 Initialiser l'état du compteur
    counter->current_breath = 0;
    counter->was_at_min_last_frame = false;
    counter->waiting_for_scale_min = false;
    counter->was_at_max_last_frame = false;

    // Couleur bleu-nuit cendré (même que le timer)
    counter->text_color.r = 70;
    counter->text_color.g = 85;
    counter->text_color.b = 110;
    counter->text_color.a = 255;

    // Sauvegarder le chemin de la police
    counter->font_path = font_path;
    counter->base_font_size = base_font_size;

    const char* retention_name = (retention_type == 0) ? "poumons pleins" : "poumons vides";
    debug_printf("✅ Compteur créé: %d respirations max (%s), police %s taille %d\n",
                 total_breaths, retention_name, font_path, base_font_size);

    return counter;
}



// ════════════════════════════════════════════════════════════════════════
// RENDU DU COMPTEUR AVEC EFFET FISH-EYE (DONNÉES PRÉCOMPUTÉES)
// ════════════════════════════════════════════════════════════════════════
// Le chiffre "respire" avec l'hexagone : sa taille vient du précomputing
// Scale max (inspire) = texte agrandi (poumons pleins)
// Scale min (expire) = texte réduit (poumons vides)
//
// Logique de fin de session :
// 1. Compter normalement jusqu'à total_breaths (ex: 10)
// 2. Incrémenter à chaque scale_max (inspire = poumons pleins)
// 3. Après le 10ème scale_max, passer en mode "attente scale_min"
// 4. Continuer à afficher le dernier chiffre jusqu'au prochain scale_min
// 5. Au scale_min, signaler que la session est terminée (poumons vides)
// ════════════════════════════════════════════════════════════════════════
void counter_render(CounterState* counter, SDL_Renderer* renderer,
                    int center_x, int center_y, int hex_radius, HexagoneNode* hex_node) {
    if (!counter || !renderer || !hex_node) return;

    // Supprimer le warning de paramètre inutilisé
    (void)hex_radius;

    // Ne rien afficher si le compteur n'est pas actif
    if (!counter->is_active) return;

    // 🎯 RÉCUPÉRER LES DONNÉES PRÉCOMPUTÉES depuis l'hexagone
    if (!hex_node->precomputed_counter_frames) return;

    // Vérifier que current_cycle est dans les limites
    if (hex_node->current_cycle < 0 || hex_node->current_cycle >= hex_node->total_cycles) {
        return;
    }

    CounterFrame* current_frame = &hex_node->precomputed_counter_frames[hex_node->current_cycle];
    bool is_at_min_now = current_frame->is_at_scale_min;
    bool is_at_max_now = current_frame->is_at_scale_max;
    double text_scale = current_frame->text_scale;

    // 🚩 LOGIQUE DE COMPTAGE SELON LE TYPE DE RÉTENTION
    // retention_type=0 (poumons pleins) : compter sur scale_MAX (inspire)
    // retention_type=1 (poumons vides) : compter sur scale_MIN (expire)

    if (counter->retention_type == 0) {
        // POUMONS PLEINS : compter sur scale_MAX
        if (is_at_max_now && !counter->was_at_max_last_frame) {
            if (!counter->waiting_for_scale_min) {  // Note: le flag est mal nommé, devrait être "waiting_for_target"
                counter->current_breath++;
                debug_printf("🫁 Respiration %d/%d détectée à scale_max (poumons pleins) (frame %d)\n",
                             counter->current_breath, counter->total_breaths, hex_node->current_cycle);

                if (counter->current_breath >= counter->total_breaths) {
                    counter->waiting_for_scale_min = true;  // On attend le dernier scale_max
                    debug_printf("✅ %d respirations complétées - attente du prochain scale_max...\n",
                                 counter->total_breaths);
                }
            } else {
                counter->is_active = false;
                debug_printf("🎯 Scale_max final atteint - session terminée (poumons pleins)\n");
            }
        }
    } else {
        // POUMONS VIDES : compter sur scale_MIN (comportement original)
        if (is_at_min_now && !counter->was_at_min_last_frame) {
            if (!counter->waiting_for_scale_min) {
                counter->current_breath++;
                debug_printf("🫁 Respiration %d/%d détectée à scale_min (poumons vides) (frame %d)\n",
                             counter->current_breath, counter->total_breaths, hex_node->current_cycle);

                if (counter->current_breath >= counter->total_breaths) {
                    counter->waiting_for_scale_min = true;
                    debug_printf("✅ %d respirations complétées - attente du prochain scale_min...\n",
                                 counter->total_breaths);
                }
            } else {
                counter->is_active = false;
                debug_printf("🎯 Scale_min final atteint - session terminée (poumons vides)\n");
            }
        }
    }

    // Sauvegarder les états pour la prochaine frame
    counter->was_at_min_last_frame = is_at_min_now;
    counter->was_at_max_last_frame = is_at_max_now;

    // Ne rien afficher si breath_number est 0 (pas encore démarré)
    if (counter->current_breath == 0) return;

    // Formater le texte (numéro du cycle actuel)
    char count_text[8];
    snprintf(count_text, sizeof(count_text), "%d", counter->current_breath);

    // 🎨 EFFET FISH-EYE : Calculer la taille de police avec le scale précomputé
    int font_size = (int)(counter->base_font_size * text_scale);
    if (font_size < 12) font_size = 12;  // Minimum lisible

    // Ouvrir la police à la taille calculée
    TTF_Font* render_font = TTF_OpenFont(counter->font_path, font_size);
    if (!render_font) {
        fprintf(stderr, "❌ Erreur chargement police taille %d: %s\n", font_size, TTF_GetError());
        return;
    }

    // Générer la surface avec le texte (rendu haute qualité)
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(
        render_font,
        count_text,
        counter->text_color
    );

    if (!text_surface) {
        fprintf(stderr, "❌ Erreur rendu texte: %s\n", TTF_GetError());
        TTF_CloseFont(render_font);
        return;
    }

    // Créer texture depuis la surface
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) {
        fprintf(stderr, "❌ Erreur création texture: %s\n", SDL_GetError());
        SDL_FreeSurface(text_surface);
        TTF_CloseFont(render_font);
        return;
    }

    // Calculer la position centrée
    int text_width = text_surface->w;
    int text_height = text_surface->h;
    int text_x = center_x - (text_width / 2);
    int text_y = center_y - (text_height / 2);

    // Rectangle de destination
    SDL_Rect dest_rect = {text_x, text_y, text_width, text_height};

    // Dessiner le texte
    SDL_RenderCopy(renderer, text_texture, NULL, &dest_rect);

    // Nettoyage
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
    TTF_CloseFont(render_font);
}

// ════════════════════════════════════════════════════════════════════════
// LIBÉRATION MÉMOIRE
// ════════════════════════════════════════════════════════════════════════
void counter_destroy(CounterState* counter) {
    if (!counter) return;

    free(counter);
    debug_printf("🧹 Compteur détruit\n");
}
