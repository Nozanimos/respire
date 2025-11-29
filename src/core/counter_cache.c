// SPDX-License-Identifier: GPL-3.0-or-later
// counter_cache.c - Système de cache de textures pour le compteur
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "counter_cache.h"
#include "precompute_list.h"  // Pour sinusoidal_movement
#include "debug.h"
#include "core/memory/memory.h"

// UTILITAIRE : Convertir surface Cairo vers texture SDL
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

// UTILITAIRE : Rendre un chiffre avec Cairo à un scale donné
static SDL_Texture* render_number_with_cairo(SDL_Renderer* renderer,
                                              int number,
                                              const char* font_path,
                                              double font_size,
                                              SDL_Color color) {
    // Paramètre color non utilisé car on utilise le gradient métallisé
    (void)color;

    // Initialiser FreeType
    FT_Library ft_library;
    FT_Face ft_face;

    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "❌ Erreur initialisation FreeType pour cache\n");
        return NULL;
    }

    if (FT_New_Face(ft_library, font_path, 0, &ft_face)) {
        fprintf(stderr, "❌ Erreur chargement police: %s\n", font_path);
        FT_Done_FreeType(ft_library);
        return NULL;
    }

    // Formater le texte
    char text[8];
    snprintf(text, sizeof(text), "%d", number);

    // Créer une surface Cairo temporaire pour mesurer le texte
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);

    // Créer une fonte Cairo depuis FreeType
    cairo_font_face_t* cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    cairo_set_font_face(temp_cr, cairo_face);
    cairo_set_font_size(temp_cr, font_size);

    // Mesurer le texte
    cairo_text_extents_t extents;
    cairo_text_extents(temp_cr, text, &extents);

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

    // Mesurer à nouveau avec le contexte final pour obtenir les bonnes dimensions
    cairo_text_extents(cr, text, &extents);

    // Position du texte
    double text_x = 5 - extents.x_bearing;
    double text_y = 5 - extents.y_bearing;

    // EFFET MÉTALLISÉ ANTHRACITE (gris-bleu foncé avec reflets brillants)
    // Même gradient que timer et chronomètre
    cairo_pattern_t* gradient = cairo_pattern_create_linear(0, text_y + extents.y_bearing,
                                                             0, text_y + extents.y_bearing + extents.height);

    // Couleurs plus foncées avec reflets brillants
    cairo_pattern_add_color_stop_rgba(gradient, 0.0,  0.40, 0.45, 0.52, 1.0);  // Haut: gris-bleu un peu plus foncé
    cairo_pattern_add_color_stop_rgba(gradient, 0.25, 0.60, 0.65, 0.72, 1.0);  // Reflet brillant (plus de contraste)
    cairo_pattern_add_color_stop_rgba(gradient, 0.7,  0.30, 0.35, 0.42, 1.0);  // Milieu sombre
    cairo_pattern_add_color_stop_rgba(gradient, 1.0,  0.24, 0.28, 0.36, 1.0);  // Bas: anthracite très foncé

    // Dessiner le texte avec le gradient
    cairo_move_to(cr, text_x, text_y);
    cairo_set_source(cr, gradient);
    cairo_show_text(cr, text);

    // Libérer le gradient
    cairo_pattern_destroy(gradient);

    cairo_surface_flush(surface);

    // Convertir en texture SDL
    SDL_Texture* texture = texture_from_cairo_surface(renderer, surface);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    // Nettoyage
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(cairo_face);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);

    return texture;
}

// CRÉATION DU CACHE
CounterTextureCache* counter_cache_create(SDL_Renderer* renderer,
                                          int max_numbers,
                                          const char* font_path,
                                          int base_font_size,
                                          SDL_Color text_color,
                                          double min_scale,
                                          double max_scale,
                                          int fps,
                                          float breath_duration) {
    if (!renderer || !font_path || max_numbers <= 0 || fps <= 0 || breath_duration <= 0.0f) {
        fprintf(stderr, "❌ Paramètres invalides pour counter_cache_create\n");
        return NULL;
    }

    // Allouer la structure
    CounterTextureCache* cache = SAFE_MALLOC(sizeof(CounterTextureCache));
    if (!cache) {
        fprintf(stderr, "❌ Erreur allocation CounterTextureCache\n");
        return NULL;
    }

    // Initialiser les champs
    cache->renderer = renderer;
    cache->max_numbers = max_numbers;
    cache->fps = fps;
    cache->breath_duration = breath_duration;
    cache->frames_per_cycle = (int)(fps * breath_duration);  // Ex: 60 × 3.0 = 180 frames
    cache->min_scale = min_scale;
    cache->max_scale = max_scale;
    cache->font_path = font_path;
    cache->base_font_size = base_font_size;
    cache->text_color = text_color;

    debug_printf("🎨 PRÉCALCUL COMPLET DES TEXTURES DU COMPTEUR (1 texture par frame)...\n");
    debug_printf("   Chiffres : 1 à %d\n", max_numbers);
    debug_printf("   Frames par cycle : %d (fps=%d × breath_duration=%.1fs)\n",
                 cache->frames_per_cycle, fps, breath_duration);
    debug_printf("   Scale breathing : %.2f → %.2f\n", min_scale, max_scale);
    debug_printf("   Police : %s (taille base %d)\n", font_path, base_font_size);

    // Allouer le tableau 2D de textures [number][frame_index]
    cache->textures = SAFE_MALLOC(max_numbers * sizeof(SDL_Texture**));
    if (!cache->textures) {
        fprintf(stderr, "❌ Erreur allocation tableau de textures\n");
        SAFE_FREE(cache);
        return NULL;
    }

    for (int i = 0; i < max_numbers; i++) {
        cache->textures[i] = SAFE_MALLOC(cache->frames_per_cycle * sizeof(SDL_Texture*));
        if (!cache->textures[i]) {
            fprintf(stderr, "❌ Erreur allocation sous-tableau de textures\n");
            // Libérer les tableaux déjà alloués
            for (int j = 0; j < i; j++) {
                SAFE_FREE(cache->textures[j]);
            }
            SAFE_FREE(cache->textures);
            SAFE_FREE(cache);
            return NULL;
        }
    }

    // Configurer la structure pour sinusoidal_movement
    SinusoidalConfig sinusoidal_config = {
        .angle_per_cycle = 0.0,           // Pas utilisé pour le scale
        .scale_min = min_scale,
        .scale_max = max_scale,
        .clockwise = true,                // Pas utilisé pour le scale
        .breath_duration = breath_duration
    };

    // Précalculer TOUTES les textures pour chaque frame du cycle
    int total_textures = 0;
    for (int number = 1; number <= max_numbers; number++) {
        for (int frame = 0; frame < cache->frames_per_cycle; frame++) {
            // Calculer le temps de cette frame
            double frame_time = (double)frame / fps;

            // Calculer le scale exact pour cette frame avec la sinusoïdale
            SinusoidalResult sinusoidal_result;
            sinusoidal_movement(frame_time, &sinusoidal_config, &sinusoidal_result);

            // Calculer la taille de police pour cette frame
            double font_size = base_font_size * sinusoidal_result.scale;
            if (font_size < 12.0) font_size = 12.0;  // Minimum lisible

            // Rendre la texture avec Cairo à la taille exacte
            SDL_Texture* texture = render_number_with_cairo(renderer, number, font_path,
                                                            font_size, text_color);

            if (!texture) {
                fprintf(stderr, "❌ Erreur rendu texture pour %d (frame %d, scale %.2f)\n",
                        number, frame, sinusoidal_result.scale);
                // Continuer quand même (texture NULL sera gérée dans get)
            } else {
                total_textures++;
            }

            cache->textures[number - 1][frame] = texture;
        }
    }

    double memory_mb = (total_textures * 50.0) / 1024.0;  // ~50KB par texture (estimation)
    debug_printf("✅ Cache créé: %d textures précalculées (%.1f MB estimé)\n",
                 total_textures, memory_mb);

    return cache;
}

// RÉCUPÉRATION D'UNE TEXTURE DU CACHE
SDL_Texture* counter_cache_get(CounterTextureCache* cache,
                               int number,
                               int frame_index,
                               int* texture_width,
                               int* texture_height) {
    if (!cache || number < 1 || number > cache->max_numbers) {
        return NULL;
    }

    // Wrap le frame_index dans le cycle (modulo)
    // Ex: frame_index = 185, frames_per_cycle = 180 → frame_in_cycle = 5
    int frame_in_cycle = frame_index % cache->frames_per_cycle;

    // Récupérer la texture pour cette frame exacte
    SDL_Texture* texture = cache->textures[number - 1][frame_in_cycle];

    // Récupérer les dimensions si demandées
    if (texture && (texture_width || texture_height)) {
        int w, h;
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        if (texture_width) *texture_width = w;
        if (texture_height) *texture_height = h;
    }

    return texture;
}

// DESTRUCTION DU CACHE
void counter_cache_destroy(CounterTextureCache* cache) {
    if (!cache) return;

    // Libérer toutes les textures SDL
    for (int i = 0; i < cache->max_numbers; i++) {
        for (int j = 0; j < cache->frames_per_cycle; j++) {
            if (cache->textures[i][j]) {
                SDL_DestroyTexture(cache->textures[i][j]);
            }
        }
        SAFE_FREE(cache->textures[i]);
    }
    SAFE_FREE(cache->textures);

    // Libérer la structure
    SAFE_FREE(cache);

    debug_printf("🧹 Cache de textures du compteur détruit\n");
}
