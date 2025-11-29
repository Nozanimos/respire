// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "debug.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "core/memory/memory.h"

// CRÉATION DU TIMER
TimerState* breathing_timer_create(int duration_seconds, const char* font_path, int font_size) {
    // Allocation de la structure
    TimerState* timer = SAFE_MALLOC(sizeof(TimerState));
    if (!timer) {
        fprintf(stderr, "❌ Erreur allocation TimerState\n");
        return NULL;
    }

    // Initialisation des valeurs
    timer->total_seconds = duration_seconds;
    timer->remaining_seconds = duration_seconds;
    timer->is_active = false;
    timer->is_finished = false;
    timer->last_update_time = 0;

    // Couleur bleu-nuit cendré (nuance de bleu sombre avec un peu de gris)
    timer->text_color.r = 70;   // Teinte bleue faible
    timer->text_color.g = 85;   // Un peu de vert pour le côté cendré
    timer->text_color.b = 110;  // Dominante bleue
    timer->text_color.a = 255;  // Opaque

    // Charger la police TTF pour SDL2_gfx
    timer->font = TTF_OpenFont(font_path, font_size);
    if (!timer->font) {
        fprintf(stderr, "❌ Erreur chargement police: %s\n", TTF_GetError());
        SAFE_FREE(timer);
        return NULL;
    }
    timer->font_size = font_size;

    // Sauvegarder le chemin de la police pour pouvoir la recharger avec différentes tailles
    timer->font_path = strdup(font_path);
    if (!timer->font_path) {
        fprintf(stderr, "❌ Erreur allocation font_path\n");
        TTF_CloseFont(timer->font);
        SAFE_FREE(timer);
        return NULL;
    }

    debug_printf("✅ Timer créé: %d secondes, police %s taille %d\n",
                 duration_seconds, font_path, font_size);

    return timer;
}

// DÉMARRER LE TIMER
void timer_start(TimerState* timer) {
    if (!timer) return;

    timer->is_active = true;
    timer->is_finished = false;
    timer->last_update_time = SDL_GetTicks();

    debug_printf("⏱️  Timer démarré: %d secondes\n", timer->total_seconds);
}

// MISE À JOUR DU TIMER (appelé à chaque frame)
bool timer_update(TimerState* timer) {
    if (!timer || !timer->is_active || timer->is_finished) {
        return false;
    }

    // Calculer le temps écoulé depuis la dernière mise à jour
    Uint32 current_time = SDL_GetTicks();
    Uint32 elapsed_ms = current_time - timer->last_update_time;

    // Mettre à jour toutes les 1000ms (1 seconde)
    if (elapsed_ms >= 1000) {
        timer->remaining_seconds--;
        timer->last_update_time = current_time;

        debug_printf("⏱️  Timer: %d secondes restantes\n", timer->remaining_seconds);

        // Vérifier si le timer est terminé
        if (timer->remaining_seconds <= 0) {
            timer->remaining_seconds = 0;
            timer->is_finished = true;
            timer->is_active = false;

            debug_printf("✅ Timer terminé!\n");
            return false;
        }
    }

    return true;  // Timer toujours actif
}

// FORMATER LE TEMPS AU FORMAT mm:ss
void timer_format(TimerState* timer, char* buffer) {
    if (!timer || !buffer) return;

    // Limiter les valeurs pour éviter le débordement
    int total_seconds = (timer->remaining_seconds < 0) ? 0 : timer->remaining_seconds;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;

    // Limiter minutes à 99 max pour tenir dans le format mm:ss
    if (minutes > 99) minutes = 99;

    // Format: "00:00" - buffer doit faire minimum 6 caractères
    snprintf(buffer, 6, "%02d:%02d", minutes, seconds);
}

// RENDU DU TIMER CENTRÉ SUR L'HEXAGONE (avec Cairo - effet métallisé)
void timer_render(TimerState* timer, SDL_Renderer* renderer,
                  int center_x, int center_y, int hex_radius) {
    if (!timer || !renderer || !timer->font_path) return;

    // Formater le texte
    char time_text[6];
    timer_format(timer, time_text);

    // Calculer la taille de police proportionnelle au rayon de l'hexagone
    int adaptive_font_size = (int)(hex_radius * 0.35f);
    if (adaptive_font_size < 12) adaptive_font_size = 12;

    debug_printf("🔍 TIMER CAIRO: hex_radius=%d → adaptive_font_size=%d (font_path=%s)\n",
                 hex_radius, adaptive_font_size, timer->font_path);

    // Initialiser FreeType
    FT_Library ft_library;
    FT_Face ft_face;
    if (FT_Init_FreeType(&ft_library) != 0) {
        fprintf(stderr, "❌ Erreur init FreeType\n");
        return;
    }

    if (FT_New_Face(ft_library, timer->font_path, 0, &ft_face) != 0) {
        fprintf(stderr, "❌ Erreur chargement police FreeType: %s\n", timer->font_path);
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

    debug_printf("📏 TIMER CAIRO: text='%s', width=%.2f, height=%.2f, x_bearing=%.2f, y_bearing=%.2f\n",
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

    debug_printf("✍️ TIMER CAIRO: texte dessiné à position (%.2f, %.2f)\n", text_x, text_y);

    cairo_pattern_destroy(gradient);

    // Convertir Cairo surface en SDL texture
    cairo_surface_flush(surface);
    unsigned char* cairo_data = cairo_image_surface_get_data(surface);
    int cairo_width = cairo_image_surface_get_width(surface);
    int cairo_height = cairo_image_surface_get_height(surface);
    int cairo_stride = cairo_image_surface_get_stride(surface);

    debug_printf("🖼️ TIMER CAIRO: surface %dx%d, stride=%d\n", cairo_width, cairo_height, cairo_stride);

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

    debug_printf("📍 TIMER CAIRO: dest_rect=(%d,%d,%d,%d), surface=%dx%d\n",
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

// RÉINITIALISER LE TIMER
void timer_reset(TimerState* timer) {
    if (!timer) return;

    timer->remaining_seconds = timer->total_seconds;
    timer->is_active = false;
    timer->is_finished = false;
    timer->last_update_time = 0;

    debug_printf("🔄 Timer réinitialisé à %d secondes\n", timer->total_seconds);
}

// DESTRUCTION DU TIMER
void timer_destroy(TimerState* timer) {
    if (!timer) return;

    // Libérer la police TTF si elle existe
    if (timer->font) {
        TTF_CloseFont(timer->font);
        timer->font = NULL;
    }

    // Libérer le chemin de la police
    if (timer->font_path) {
        SAFE_FREE(timer->font_path);
        timer->font_path = NULL;
    }

    // Libérer la structure
    SAFE_FREE(timer);

    debug_printf("🗑️  Timer détruit\n");
}
