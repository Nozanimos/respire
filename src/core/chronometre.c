// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chronometre.h"
#include "debug.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "core/memory/memory.h"

// CRÉATION DU CHRONOMÈTRE
StopwatchState* stopwatch_create(const char* font_path, int font_size) {
    // Allocation de la structure
    StopwatchState* stopwatch = SAFE_MALLOC(sizeof(StopwatchState));
    if (!stopwatch) {
        fprintf(stderr, "❌ Erreur allocation StopwatchState\n");
        return NULL;
    }

    // Initialisation des valeurs
    stopwatch->elapsed_seconds = 0;
    stopwatch->is_active = false;
    stopwatch->is_stopped = false;
    stopwatch->start_time = 0;
    stopwatch->last_update_time = 0;

    // Couleur bleu-nuit cendré (même couleur que timer et counter)
    stopwatch->text_color.r = 70;   // Teinte bleue faible
    stopwatch->text_color.g = 85;   // Un peu de vert pour le côté cendré
    stopwatch->text_color.b = 110;  // Dominante bleue
    stopwatch->text_color.a = 255;  // Opaque

    // Charger la police TTF
    stopwatch->font = TTF_OpenFont(font_path, font_size);
    if (!stopwatch->font) {
        fprintf(stderr, "❌ Erreur chargement police: %s\n", TTF_GetError());
        SAFE_FREE(stopwatch);
        return NULL;
    }
    stopwatch->font_size = font_size;

    // Sauvegarder le chemin de la police pour pouvoir la recharger avec différentes tailles
    stopwatch->font_path = strdup(font_path);
    if (!stopwatch->font_path) {
        fprintf(stderr, "❌ Erreur allocation font_path\n");
        TTF_CloseFont(stopwatch->font);
        SAFE_FREE(stopwatch);
        return NULL;
    }

    debug_printf("✅ Chronomètre créé: police %s taille %d\n", font_path, font_size);

    return stopwatch;
}

// DÉMARRER LE CHRONOMÈTRE
void stopwatch_start(StopwatchState* stopwatch) {
    if (!stopwatch) return;

    stopwatch->is_active = true;
    stopwatch->is_stopped = false;
    stopwatch->elapsed_seconds = 0;
    stopwatch->start_time = SDL_GetTicks();
    stopwatch->last_update_time = stopwatch->start_time;

    debug_printf("⏱️  Chronomètre démarré à 00:00\n");
}

// ARRÊTER LE CHRONOMÈTRE (garde le temps final)
void stopwatch_stop(StopwatchState* stopwatch) {
    if (!stopwatch || !stopwatch->is_active) return;

    stopwatch->is_active = false;
    stopwatch->is_stopped = true;

    debug_printf("⏹️  Chronomètre arrêté à %d secondes\n", stopwatch->elapsed_seconds);
}

// MISE À JOUR DU CHRONOMÈTRE (appelé à chaque frame)
// Le chronomètre INCRÉMENTE (contrairement au timer qui décrémente)
bool stopwatch_update(StopwatchState* stopwatch) {
    if (!stopwatch || !stopwatch->is_active || stopwatch->is_stopped) {
        return false;
    }

    // Calculer le temps écoulé depuis le démarrage
    Uint32 current_time = SDL_GetTicks();
    Uint32 elapsed_ms = current_time - stopwatch->last_update_time;

    // Mettre à jour toutes les 1000ms (1 seconde)
    if (elapsed_ms >= 1000) {
        stopwatch->elapsed_seconds++;
        stopwatch->last_update_time = current_time;

        debug_printf("⏱️  Chronomètre: %d secondes écoulées\n", stopwatch->elapsed_seconds);
    }

    return true;  // Chronomètre toujours actif
}

// RÉCUPÉRER LE TEMPS ÉCOULÉ
int stopwatch_get_elapsed_seconds(StopwatchState* stopwatch) {
    if (!stopwatch) return 0;
    return stopwatch->elapsed_seconds;
}

// FORMATER LE TEMPS AU FORMAT mm:ss
void stopwatch_format(StopwatchState* stopwatch, char* buffer) {
    if (!stopwatch || !buffer) return;

    // Extraire minutes et secondes
    int total_seconds = (stopwatch->elapsed_seconds < 0) ? 0 : stopwatch->elapsed_seconds;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;

    // Limiter minutes à 99 max pour tenir dans le format mm:ss
    if (minutes > 99) minutes = 99;

    // Limiter seconds entre 0 et 59 (sécurité supplémentaire pour le compilateur)
    if (seconds < 0) seconds = 0;
    if (seconds > 59) seconds = 59;

    // Format: "00:00" - buffer doit faire minimum 6 caractères
    snprintf(buffer, 6, "%02d:%02d", minutes, seconds);
}

// RENDU DU CHRONOMÈTRE CENTRÉ SUR L'HEXAGONE (avec Cairo - effet métallisé)
void stopwatch_render(StopwatchState* stopwatch, SDL_Renderer* renderer,
                      int center_x, int center_y, int hex_radius) {
    if (!stopwatch || !renderer || !stopwatch->font_path) return;

    // Formater le texte
    char time_text[6];
    stopwatch_format(stopwatch, time_text);

    // Calculer la taille de police proportionnelle au rayon de l'hexagone
    int adaptive_font_size = (int)(hex_radius * 0.35f);
    if (adaptive_font_size < 12) adaptive_font_size = 12;

    debug_printf("🔍 STOPWATCH CAIRO: hex_radius=%d → adaptive_font_size=%d (font_path=%s)\n",
                 hex_radius, adaptive_font_size, stopwatch->font_path);

    // Initialiser FreeType
    FT_Library ft_library;
    FT_Face ft_face;
    if (FT_Init_FreeType(&ft_library) != 0) {
        fprintf(stderr, "❌ Erreur init FreeType\n");
        return;
    }

    if (FT_New_Face(ft_library, stopwatch->font_path, 0, &ft_face) != 0) {
        fprintf(stderr, "❌ Erreur chargement police FreeType: %s\n", stopwatch->font_path);
        FT_Done_FreeType(ft_library);
        return;
    }

    // Définir la taille de police en pixels
    FT_Set_Pixel_Sizes(ft_face, 0, adaptive_font_size);

    // Créer surface Cairo dimensionnée selon la taille de police
    // (au lieu d'une taille fixe 300x100 qui coupe le texte)
    int surface_width = adaptive_font_size * 5;   // Largeur pour "00:00"
    int surface_height = adaptive_font_size * 2;  // Hauteur suffisante pour le texte

    cairo_font_face_t* cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surface_width, surface_height);
    cairo_t* cr = cairo_create(surface);

    // Configurer la police
    cairo_set_font_face(cr, cairo_face);
    cairo_set_font_size(cr, adaptive_font_size);

    // Mesurer le texte pour centrage
    cairo_text_extents_t extents;
    cairo_text_extents(cr, time_text, &extents);

    debug_printf("📏 STOPWATCH CAIRO: text='%s', width=%.2f, height=%.2f, x_bearing=%.2f, y_bearing=%.2f\n",
                 time_text, extents.width, extents.height, extents.x_bearing, extents.y_bearing);

    // Calculer position de départ (centré dans la nouvelle surface)
    double text_x = (surface_width / 2.0) - (extents.width / 2.0) - extents.x_bearing;
    double text_y = (surface_height / 2.0) - (extents.height / 2.0) - extents.y_bearing;

    // EFFET MÉTALLISÉ ANTHRACITE (gris-bleu foncé avec reflets brillants)
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
    cairo_show_text(cr, time_text);

    debug_printf("✍️ STOPWATCH CAIRO: texte dessiné à position (%.2f, %.2f)\n", text_x, text_y);

    cairo_pattern_destroy(gradient);

    // Convertir Cairo surface en SDL texture
    cairo_surface_flush(surface);
    unsigned char* cairo_data = cairo_image_surface_get_data(surface);
    int cairo_width = cairo_image_surface_get_width(surface);
    int cairo_height = cairo_image_surface_get_height(surface);
    int cairo_stride = cairo_image_surface_get_stride(surface);

    debug_printf("🖼️ STOPWATCH CAIRO: surface %dx%d, stride=%d\n", cairo_width, cairo_height, cairo_stride);

    // Créer surface SDL depuis les données Cairo
    SDL_Surface* sdl_surface = SDL_CreateRGBSurfaceFrom(
        cairo_data, cairo_width, cairo_height, 32, cairo_stride,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    if (!sdl_surface) {
        fprintf(stderr, "❌ Erreur création SDL surface: %s\n", SDL_GetError());
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        cairo_font_face_destroy(cairo_face);
        FT_Done_Face(ft_face);
        FT_Done_FreeType(ft_library);
        return;
    }

    // Créer texture SDL
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
    if (!texture) {
        fprintf(stderr, "❌ Erreur création texture: %s\n", SDL_GetError());
        SDL_FreeSurface(sdl_surface);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        cairo_font_face_destroy(cairo_face);
        FT_Done_Face(ft_face);
        FT_Done_FreeType(ft_library);
        return;
    }

    // Calculer rectangle de destination centré (utiliser toute la surface Cairo)
    SDL_Rect dest_rect = {
        center_x - (cairo_width / 2),
        center_y - (cairo_height / 2),
        cairo_width,
        cairo_height
    };

    debug_printf("📍 STOPWATCH CAIRO: dest_rect=(%d,%d,%d,%d), surface=%dx%d\n",
                 dest_rect.x, dest_rect.y, dest_rect.w, dest_rect.h,
                 cairo_width, cairo_height);

    // Dessiner la texture (NULL pour src_rect = toute la surface)
    SDL_RenderCopy(renderer, texture, NULL, &dest_rect);

    // Libérer les ressources
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(sdl_surface);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(cairo_face);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);
}

// RÉINITIALISER LE CHRONOMÈTRE
void stopwatch_reset(StopwatchState* stopwatch) {
    if (!stopwatch) return;

    stopwatch->elapsed_seconds = 0;
    stopwatch->is_active = false;
    stopwatch->is_stopped = false;
    stopwatch->start_time = 0;
    stopwatch->last_update_time = 0;

    debug_printf("🔄 Chronomètre réinitialisé à 00:00\n");
}

// DESTRUCTION DU CHRONOMÈTRE
void stopwatch_destroy(StopwatchState* stopwatch) {
    if (!stopwatch) return;

    // Libérer la police TTF si elle existe
    if (stopwatch->font) {
        TTF_CloseFont(stopwatch->font);
        stopwatch->font = NULL;
    }

    // Libérer le chemin de la police
    if (stopwatch->font_path) {
        SAFE_FREE(stopwatch->font_path);
        stopwatch->font_path = NULL;
    }

    // Libérer la structure
    SAFE_FREE(stopwatch);

    debug_printf("🗑️  Chronomètre détruit\n");
}
