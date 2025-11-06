// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
#include "timer.h"
#include "debug.h"
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════
// CRÉATION DU TIMER
// ════════════════════════════════════════════════════════════════════════
TimerState* breathing_timer_create(int duration_seconds, const char* font_path, int font_size) {
    // Allocation de la structure
    TimerState* timer = malloc(sizeof(TimerState));
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
        free(timer);
        return NULL;
    }
    timer->font_size = font_size;

    debug_printf("✅ Timer créé: %d secondes, police %s taille %d\n",
                 duration_seconds, font_path, font_size);

    return timer;
}

// ════════════════════════════════════════════════════════════════════════
// DÉMARRER LE TIMER
// ════════════════════════════════════════════════════════════════════════
void timer_start(TimerState* timer) {
    if (!timer) return;

    timer->is_active = true;
    timer->is_finished = false;
    timer->last_update_time = SDL_GetTicks();

    debug_printf("⏱️  Timer démarré: %d secondes\n", timer->total_seconds);
}

// ════════════════════════════════════════════════════════════════════════
// MISE À JOUR DU TIMER (appelé à chaque frame)
// ════════════════════════════════════════════════════════════════════════
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

// ════════════════════════════════════════════════════════════════════════
// FORMATER LE TEMPS AU FORMAT mm:ss
// ════════════════════════════════════════════════════════════════════════
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

// ════════════════════════════════════════════════════════════════════════
// RENDU DU TIMER CENTRÉ SUR L'HEXAGONE (avec SDL2_gfx)
// ════════════════════════════════════════════════════════════════════════
void timer_render(TimerState* timer, SDL_Renderer* renderer,
                  int center_x, int center_y, int hex_radius) {
    if (!timer || !renderer || !timer->font) return;

    // Formater le texte
    char time_text[6];
    timer_format(timer, time_text);

    // Mesurer la largeur du texte avec TTF pour centrage précis
    int text_width, text_height;
    if (TTF_SizeUTF8(timer->font, time_text, &text_width, &text_height) != 0) {
        fprintf(stderr, "❌ Erreur mesure texte: %s\n", TTF_GetError());
        return;
    }

    // Calculer la largeur maximale disponible dans l'hexagone
    // (diamètre du plus petit hexagone - 10px de marge totale)
    int max_width = (hex_radius * 2) - 10;

    // Ajuster la taille de police si le texte dépasse
    TTF_Font* render_font = timer->font;
    int adjusted_font_size = timer->font_size;

    if (text_width > max_width) {
        // Réduire la taille de police proportionnellement
        adjusted_font_size = (timer->font_size * max_width) / text_width;
        render_font = TTF_OpenFont(TTF_FontFaceFamilyName(timer->font), adjusted_font_size);
        if (!render_font) {
            render_font = timer->font;  // Fallback si erreur
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
        timer->text_color
    );

    if (!text_surface) {
        fprintf(stderr, "❌ Erreur rendu texte: %s\n", TTF_GetError());
        if (render_font != timer->font) TTF_CloseFont(render_font);
        return;
    }

    // Créer texture depuis la surface
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);

    if (!text_texture) {
        fprintf(stderr, "❌ Erreur création texture: %s\n", SDL_GetError());
        SDL_FreeSurface(text_surface);
        if (render_font != timer->font) TTF_CloseFont(render_font);
        return;
    }

    // Rectangle de destination
    SDL_Rect dest_rect = {text_x, text_y, text_width, text_height};

    // Dessiner
    SDL_RenderCopy(renderer, text_texture, NULL, &dest_rect);

    // Libération
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
    if (render_font != timer->font) {
        TTF_CloseFont(render_font);
    }
}

// ════════════════════════════════════════════════════════════════════════
// RÉINITIALISER LE TIMER
// ════════════════════════════════════════════════════════════════════════
void timer_reset(TimerState* timer) {
    if (!timer) return;

    timer->remaining_seconds = timer->total_seconds;
    timer->is_active = false;
    timer->is_finished = false;
    timer->last_update_time = 0;

    debug_printf("🔄 Timer réinitialisé à %d secondes\n", timer->total_seconds);
}

// ════════════════════════════════════════════════════════════════════════
// DESTRUCTION DU TIMER
// ════════════════════════════════════════════════════════════════════════
void timer_destroy(TimerState* timer) {
    if (!timer) return;

    // Libérer la police TTF si elle existe
    if (timer->font) {
        TTF_CloseFont(timer->font);
        timer->font = NULL;
    }

    // Libérer la structure
    free(timer);

    debug_printf("🗑️  Timer détruit\n");
}
