// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include "chronometre.h"
#include "debug.h"
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════
// CRÉATION DU CHRONOMÈTRE
// ════════════════════════════════════════════════════════════════════════
StopwatchState* stopwatch_create(const char* font_path, int font_size) {
    // Allocation de la structure
    StopwatchState* stopwatch = malloc(sizeof(StopwatchState));
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
        free(stopwatch);
        return NULL;
    }
    stopwatch->font_size = font_size;

    debug_printf("✅ Chronomètre créé: police %s taille %d\n", font_path, font_size);

    return stopwatch;
}

// ════════════════════════════════════════════════════════════════════════
// DÉMARRER LE CHRONOMÈTRE
// ════════════════════════════════════════════════════════════════════════
void stopwatch_start(StopwatchState* stopwatch) {
    if (!stopwatch) return;

    stopwatch->is_active = true;
    stopwatch->is_stopped = false;
    stopwatch->elapsed_seconds = 0;
    stopwatch->start_time = SDL_GetTicks();
    stopwatch->last_update_time = stopwatch->start_time;

    debug_printf("⏱️  Chronomètre démarré à 00:00\n");
}

// ════════════════════════════════════════════════════════════════════════
// ARRÊTER LE CHRONOMÈTRE (garde le temps final)
// ════════════════════════════════════════════════════════════════════════
void stopwatch_stop(StopwatchState* stopwatch) {
    if (!stopwatch || !stopwatch->is_active) return;

    stopwatch->is_active = false;
    stopwatch->is_stopped = true;

    debug_printf("⏹️  Chronomètre arrêté à %d secondes\n", stopwatch->elapsed_seconds);
}

// ════════════════════════════════════════════════════════════════════════
// MISE À JOUR DU CHRONOMÈTRE (appelé à chaque frame)
// ════════════════════════════════════════════════════════════════════════
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

// ════════════════════════════════════════════════════════════════════════
// RÉCUPÉRER LE TEMPS ÉCOULÉ
// ════════════════════════════════════════════════════════════════════════
int stopwatch_get_elapsed_seconds(StopwatchState* stopwatch) {
    if (!stopwatch) return 0;
    return stopwatch->elapsed_seconds;
}

// ════════════════════════════════════════════════════════════════════════
// FORMATER LE TEMPS AU FORMAT mm:ss
// ════════════════════════════════════════════════════════════════════════
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

// ════════════════════════════════════════════════════════════════════════
// RENDU DU CHRONOMÈTRE CENTRÉ SUR L'HEXAGONE
// ════════════════════════════════════════════════════════════════════════
// Code identique à timer_render() mais utilise stopwatch_format()
void stopwatch_render(StopwatchState* stopwatch, SDL_Renderer* renderer,
                      int center_x, int center_y, int hex_radius) {
    if (!stopwatch || !renderer || !stopwatch->font) return;

    // Formater le texte
    char time_text[6];
    stopwatch_format(stopwatch, time_text);

    // Mesurer la largeur du texte avec TTF pour centrage précis
    int text_width, text_height;
    if (TTF_SizeUTF8(stopwatch->font, time_text, &text_width, &text_height) != 0) {
        fprintf(stderr, "❌ Erreur mesure texte: %s\n", TTF_GetError());
        return;
    }

    // Calculer la largeur maximale disponible dans l'hexagone
    // (diamètre du plus petit hexagone - 10px de marge totale)
    int max_width = (hex_radius * 2) - 10;

    // Ajuster la taille de police si le texte dépasse
    TTF_Font* render_font = stopwatch->font;
    int adjusted_font_size = stopwatch->font_size;

    if (text_width > max_width) {
        // Réduire la taille de police proportionnellement
        adjusted_font_size = (stopwatch->font_size * max_width) / text_width;
        render_font = TTF_OpenFont(TTF_FontFaceFamilyName(stopwatch->font), adjusted_font_size);
        if (!render_font) {
            render_font = stopwatch->font;  // Fallback si erreur
        } else {
            // Re-mesurer avec la nouvelle taille
            TTF_SizeUTF8(render_font, time_text, &text_width, &text_height);
        }
    }

    // Calculer position pour centrer le texte
    int text_x = center_x - (text_width / 2);
    int text_y = center_y - (text_height / 2);

    // Créer la surface avec le texte
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(
        render_font,
        time_text,
        stopwatch->text_color
    );

    if (!text_surface) {
        fprintf(stderr, "❌ Erreur rendu texte: %s\n", TTF_GetError());
        if (render_font != stopwatch->font) TTF_CloseFont(render_font);
        return;
    }

    // Créer texture depuis la surface
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);

    if (!text_texture) {
        fprintf(stderr, "❌ Erreur création texture: %s\n", SDL_GetError());
        SDL_FreeSurface(text_surface);
        if (render_font != stopwatch->font) TTF_CloseFont(render_font);
        return;
    }

    // Rectangle de destination
    SDL_Rect dest_rect = {text_x, text_y, text_width, text_height};

    // Dessiner
    SDL_RenderCopy(renderer, text_texture, NULL, &dest_rect);

    // Libération
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
    if (render_font != stopwatch->font) {
        TTF_CloseFont(render_font);
    }
}

// ════════════════════════════════════════════════════════════════════════
// RÉINITIALISER LE CHRONOMÈTRE
// ════════════════════════════════════════════════════════════════════════
void stopwatch_reset(StopwatchState* stopwatch) {
    if (!stopwatch) return;

    stopwatch->elapsed_seconds = 0;
    stopwatch->is_active = false;
    stopwatch->is_stopped = false;
    stopwatch->start_time = 0;
    stopwatch->last_update_time = 0;

    debug_printf("🔄 Chronomètre réinitialisé à 00:00\n");
}

// ════════════════════════════════════════════════════════════════════════
// DESTRUCTION DU CHRONOMÈTRE
// ════════════════════════════════════════════════════════════════════════
void stopwatch_destroy(StopwatchState* stopwatch) {
    if (!stopwatch) return;

    // Libérer la police TTF si elle existe
    if (stopwatch->font) {
        TTF_CloseFont(stopwatch->font);
        stopwatch->font = NULL;
    }

    // Libérer la structure
    free(stopwatch);

    debug_printf("🗑️  Chronomètre détruit\n");
}
