// SPDX-License-Identifier: GPL-3.0-or-later
// counter.c - VERSION CAIRO (avec antialiasing)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
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
// RENDU DU COMPTEUR AVEC EFFET FISH-EYE (DONNÉES PRÉCOMPUTÉES) - VERSION CAIRO
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

// Fonction utilitaire pour créer une texture SDL depuis une surface Cairo
static SDL_Texture* texture_from_cairo_surface(SDL_Renderer* renderer, cairo_surface_t* surface) {
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);

    SDL_Surface* sdl_surface = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_ARGB8888
    );

    if (!sdl_surface) return NULL;

    // Copier les données de Cairo vers SDL
    memcpy(sdl_surface->pixels,
           cairo_image_surface_get_data(surface),
           height * cairo_image_surface_get_stride(surface));

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
    SDL_FreeSurface(sdl_surface);

    return texture;
}

void counter_render(CounterState* counter, SDL_Renderer* renderer,
                    int center_x, int center_y, int hex_radius,
                    GlobalCounterFrames* counter_frames, int current_cycle,
                    float scale_factor) {
    if (!counter || !renderer || !counter_frames || !counter_frames->frames) return;

    // Supprimer le warning de paramètre inutilisé
    (void)hex_radius;

    // Ne rien afficher si le compteur n'est pas actif
    if (!counter->is_active) return;

    // Vérifier que current_cycle est dans les limites
    if (current_cycle < 0 || current_cycle >= counter_frames->total_frames) {
        return;
    }

    CounterFrame* current_frame = &counter_frames->frames[current_cycle];
    bool is_at_min_now = current_frame->is_at_scale_min;
    bool is_at_max_now = current_frame->is_at_scale_max;
    double text_scale = current_frame->text_scale;

    // Flag pour éviter de terminer dans le même frame où on atteint total_breaths
    bool just_completed = false;

    // 🚩 LOGIQUE DE COMPTAGE : TOUJOURS SUR SCALE_MIN
    // Le compteur COMPTE toujours sur scale_min (expire) : 1, 2, 3... 10
    // Seule la CONDITION DE FIN varie selon le type de rétention :
    //   - retention_type=0 (poumons pleins) : après le 10ème scale_min, attendre scale_MAX pour finir
    //   - retention_type=1 (poumons vides) : après le 10ème scale_min, faire le cycle complet puis attendre scale_MIN pour finir

    // 📊 COMPTAGE : toujours sur scale_min (expire)
    if (is_at_min_now && !counter->was_at_min_last_frame) {
        if (!counter->waiting_for_scale_min) {
            counter->current_breath++;
            debug_printf("🫁 Respiration %d/%d détectée à scale_min (expire) (frame %d)\n",
                         counter->current_breath, counter->total_breaths, current_cycle);

            if (counter->current_breath >= counter->total_breaths) {
                counter->waiting_for_scale_min = true;
                just_completed = true;  // On vient juste d'atteindre le total, ne pas terminer maintenant
                const char* target = (counter->retention_type == 0) ? "scale_max" : "scale_min";
                debug_printf("✅ %d respirations complétées - attente du prochain %s...\n",
                             counter->total_breaths, target);
            }
        }
    }

    // 🎯 CONDITION DE FIN : varie selon le type de rétention
    // Ne pas terminer dans le même frame où on vient d'atteindre total_breaths (just_completed)
    if (counter->waiting_for_scale_min && !just_completed) {
        if (counter->retention_type == 0) {
            // POUMONS PLEINS : terminer au scale_max (inspire final)
            if (is_at_max_now && !counter->was_at_max_last_frame) {
                counter->is_active = false;
                debug_printf("🎯 Scale_max final atteint - session terminée (poumons pleins)\n");
            }
        } else {
            // POUMONS VIDES : terminer au PROCHAIN scale_min (expire final après cycle complet)
            if (is_at_min_now && !counter->was_at_min_last_frame) {
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
    // 🆕 APPLIQUER LE SCALE_FACTOR pour le responsive
    double font_size = counter->base_font_size * text_scale * scale_factor;
    if (font_size < 12.0) font_size = 12.0;  // Minimum lisible

    // ═══════════════════════════════════════════════════════════════════════════
    // RENDU AVEC CAIRO pour bénéficier de l'antialiasing
    // ═══════════════════════════════════════════════════════════════════════════

    // Initialiser FreeType
    FT_Library ft_library;
    FT_Face ft_face;

    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "❌ Erreur initialisation FreeType\n");
        return;
    }

    if (FT_New_Face(ft_library, counter->font_path, 0, &ft_face)) {
        fprintf(stderr, "❌ Erreur chargement police: %s\n", counter->font_path);
        FT_Done_FreeType(ft_library);
        return;
    }

    // Créer une surface Cairo temporaire pour mesurer le texte
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);

    // Créer une fonte Cairo depuis FreeType
    cairo_font_face_t* cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    cairo_set_font_face(temp_cr, cairo_face);
    cairo_set_font_size(temp_cr, font_size);

    // Mesurer le texte
    cairo_text_extents_t extents;
    cairo_text_extents(temp_cr, count_text, &extents);

    int text_width = (int)(extents.width + 10);  // +10 pour marge
    int text_height = (int)(extents.height + 10);

    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);

    // Créer la surface finale pour le rendu
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, text_width, text_height);
    cairo_t* cr = cairo_create(surface);

    // Activer l'antialiasing de haute qualité
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // Configurer la police
    cairo_set_font_face(cr, cairo_face);
    cairo_set_font_size(cr, font_size);

    // Dessiner le texte avec la couleur
    cairo_set_source_rgba(cr,
                          counter->text_color.r / 255.0,
                          counter->text_color.g / 255.0,
                          counter->text_color.b / 255.0,
                          counter->text_color.a / 255.0);

    // Positionner le texte (centré dans la surface)
    cairo_move_to(cr, 5 - extents.x_bearing, 5 - extents.y_bearing);
    cairo_show_text(cr, count_text);

    cairo_surface_flush(surface);

    // Convertir en texture SDL
    SDL_Texture* text_texture = texture_from_cairo_surface(renderer, surface);

    if (text_texture) {
        SDL_SetTextureBlendMode(text_texture, SDL_BLENDMODE_BLEND);

        // Calculer la position centrée
        int text_x = center_x - (text_width / 2);
        int text_y = center_y - (text_height / 2);

        SDL_Rect dest_rect = {text_x, text_y, text_width, text_height};

        // Dessiner le texte
        SDL_RenderCopy(renderer, text_texture, NULL, &dest_rect);
        SDL_DestroyTexture(text_texture);
    }

    // Nettoyage
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(cairo_face);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);
}

// ════════════════════════════════════════════════════════════════════════
// LIBÉRATION MÉMOIRE
// ════════════════════════════════════════════════════════════════════════
void counter_destroy(CounterState* counter) {
    if (!counter) return;

    free(counter);
    debug_printf("🧹 Compteur détruit\n");
}
