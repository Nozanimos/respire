// SPDX-License-Identifier: GPL-3.0-or-later
// session_card.c - Carte de session animée avec Cairo
#include "session_card.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL_image.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// ════════════════════════════════════════════════════════════════════════
// CONSTANTES
// ════════════════════════════════════════════════════════════════════════
#define CARD_WIDTH_BASE  200    // Largeur de base (ratio carte de poker ~2.5:3.5)
#define CARD_HEIGHT_BASE 280    // Hauteur de base

#define MARGIN_SIDES  10        // Marge gauche/droite
#define MARGIN_TOP    10        // Marge du haut
#define MARGIN_BOTTOM 10        // Marge du bas

// ════════════════════════════════════════════════════════════════════════
// CRÉATION DE LA TEXTURE DE LA CARTE
// ════════════════════════════════════════════════════════════════════════
// Crée la texture complète : background + texte Cairo
static SDL_Texture* create_card_texture(SDL_Renderer* renderer, int session_number,
                                       int width, int height,
                                       const char* font_path, SDL_Color text_color) {
    // 1. Charger l'image de fond (vert.jpg)
    SDL_Surface* bg_surface = IMG_Load("../img/vert.jpg");
    if (!bg_surface) {
        debug_printf("❌ Erreur chargement vert.jpg: %s\n", IMG_GetError());
        return NULL;
    }

    // Redimensionner le background à la taille de la carte
    SDL_Surface* scaled_bg = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_ARGB8888
    );
    if (!scaled_bg) {
        SDL_FreeSurface(bg_surface);
        return NULL;
    }

    // Blit avec mise à l'échelle
    SDL_BlitScaled(bg_surface, NULL, scaled_bg, NULL);
    SDL_FreeSurface(bg_surface);

    // 2. Créer une surface Cairo pour le texte
    cairo_surface_t* cairo_surface = cairo_image_surface_create_for_data(
        (unsigned char*)scaled_bg->pixels,
        CAIRO_FORMAT_ARGB32,
        width, height,
        scaled_bg->pitch
    );

    cairo_t* cr = cairo_create(cairo_surface);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // 3. Initialiser FreeType et charger la police
    FT_Library ft_library;
    FT_Face ft_face;

    if (FT_Init_FreeType(&ft_library) != 0) {
        debug_printf("❌ Erreur init FreeType\n");
        cairo_destroy(cr);
        cairo_surface_destroy(cairo_surface);
        SDL_FreeSurface(scaled_bg);
        return NULL;
    }

    if (FT_New_Face(ft_library, font_path, 0, &ft_face) != 0) {
        debug_printf("❌ Erreur chargement police: %s\n", font_path);
        FT_Done_FreeType(ft_library);
        cairo_destroy(cr);
        cairo_surface_destroy(cairo_surface);
        SDL_FreeSurface(scaled_bg);
        return NULL;
    }

    cairo_font_face_t* cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);

    // 4. Dessiner "Session" en haut
    cairo_set_font_face(cr, cairo_face);

    // Calculer la taille de police dynamique pour "Session"
    // Elle doit occuper la largeur - 2*MARGIN_SIDES
    int available_width_title = width - 2 * MARGIN_SIDES;
    int title_font_size = available_width_title / 4;  // Approximation: 4 pixels par caractère

    cairo_set_font_size(cr, title_font_size);

    const char* title_text = "Session";
    cairo_text_extents_t title_extents;
    cairo_text_extents(cr, title_text, &title_extents);

    // Centrer horizontalement
    int title_x = (width - title_extents.width) / 2;
    int title_y = MARGIN_TOP + title_extents.height;

    // Couleur du texte (opaque, pas d'alpha)
    cairo_set_source_rgb(cr,
                         text_color.r / 255.0,
                         text_color.g / 255.0,
                         text_color.b / 255.0);

    cairo_move_to(cr, title_x, title_y);
    cairo_show_text(cr, title_text);

    // 5. Dessiner le numéro de session (prend l'espace restant)
    char number_text[16];
    snprintf(number_text, sizeof(number_text), "%d", session_number);

    // La taille du numéro doit remplir l'espace vertical restant
    int available_height = height - title_y - MARGIN_BOTTOM - 10;
    int number_font_size = available_height * 0.8;  // 80% de l'espace disponible

    cairo_set_font_size(cr, number_font_size);

    cairo_text_extents_t number_extents;
    cairo_text_extents(cr, number_text, &number_extents);

    // Centrer horizontalement et verticalement dans l'espace restant
    // Tenir compte du x_bearing pour un centrage correct (surtout pour le chiffre "1")
    int number_x = (width - number_extents.width) / 2 - number_extents.x_bearing;
    int number_y = title_y + 10 + (available_height + number_extents.height) / 2;

    cairo_move_to(cr, number_x, number_y);
    cairo_show_text(cr, number_text);

    // 6. Nettoyer Cairo et FreeType
    cairo_surface_flush(cairo_surface);
    cairo_font_face_destroy(cairo_face);
    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);

    // 7. Créer la texture SDL
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, scaled_bg);
    SDL_FreeSurface(scaled_bg);

    if (!texture) {
        debug_printf("❌ Erreur création texture carte: %s\n", SDL_GetError());
    }

    return texture;
}

// ════════════════════════════════════════════════════════════════════════
// CRÉATION ET INITIALISATION
// ════════════════════════════════════════════════════════════════════════
SessionCardState* session_card_create(int session_number, int screen_width,
                                      int screen_height, const char* font_path) {
    SessionCardState* card = malloc(sizeof(SessionCardState));
    if (!card) {
        debug_printf("❌ Erreur allocation SessionCardState\n");
        return NULL;
    }

    // Initialiser l'état
    card->phase = CARD_FINISHED;  // Pas démarrée
    card->elapsed_time = 0.0f;

    // Timings
    card->enter_duration = 0.5f;  // 0.5 seconde pour entrer
    card->pause_duration = 3.0f;  // 3 secondes au centre
    card->exit_duration = 0.5f;   // 0.5 seconde pour sortir

    // Dimensions de la carte
    card->card_width = CARD_WIDTH_BASE;
    card->card_height = CARD_HEIGHT_BASE;

    // Position et écran
    card->screen_width = screen_width;
    card->center_x = screen_width / 2 - card->card_width / 2;
    card->current_x = -card->card_width;  // Commence hors écran à gauche
    card->current_y = (screen_height - card->card_height) / 2;  // Centré verticalement

    // Données
    card->session_number = session_number;

    // Police et couleur (même que chrono: bleu-nuit cendré)
    card->font_path = font_path;
    card->text_color = (SDL_Color){123, 140, 153, 255};

    // Texture (sera créée plus tard avec le renderer)
    card->card_texture = NULL;

    debug_printf("✅ Carte de session créée (session %d, taille %dx%d)\n",
                 session_number, card->card_width, card->card_height);

    return card;
}

// ════════════════════════════════════════════════════════════════════════
// DÉMARRAGE DE L'ANIMATION
// ════════════════════════════════════════════════════════════════════════
void session_card_start(SessionCardState* card) {
    if (!card) return;

    card->phase = CARD_ENTERING;
    card->elapsed_time = 0.0f;
    card->current_x = -card->card_width;  // Commence hors écran à gauche

    debug_printf("🎬 Animation carte de session démarrée (session %d)\n", card->session_number);
}

// ════════════════════════════════════════════════════════════════════════
// MISE À JOUR DE L'ANIMATION
// ════════════════════════════════════════════════════════════════════════
bool session_card_update(SessionCardState* card, float delta_time) {
    if (!card || card->phase == CARD_FINISHED) return false;

    card->elapsed_time += delta_time;

    switch (card->phase) {
        case CARD_ENTERING: {
            // Animation: -card_width → center_x en 1 seconde
            float progress = card->elapsed_time / card->enter_duration;
            if (progress >= 1.0f) {
                progress = 1.0f;
                card->phase = CARD_PAUSED;
                card->elapsed_time = 0.0f;
            }

            // Interpolation linéaire
            int start_x = -card->card_width;
            int end_x = card->center_x;
            card->current_x = start_x + (int)((end_x - start_x) * progress);
            break;
        }

        case CARD_PAUSED: {
            // Reste au centre pendant 3 secondes
            card->current_x = card->center_x;

            if (card->elapsed_time >= card->pause_duration) {
                card->phase = CARD_EXITING;
                card->elapsed_time = 0.0f;
            }
            break;
        }

        case CARD_EXITING: {
            // Animation: center_x → screen_width en 1 seconde
            float progress = card->elapsed_time / card->exit_duration;
            if (progress >= 1.0f) {
                progress = 1.0f;
                card->phase = CARD_FINISHED;
                debug_printf("✅ Animation carte terminée (session %d)\n", card->session_number);
            }

            // Interpolation linéaire
            int start_x = card->center_x;
            int end_x = card->screen_width;
            card->current_x = start_x + (int)((end_x - start_x) * progress);
            break;
        }

        case CARD_FINISHED:
            return false;
    }

    return true;
}

// ════════════════════════════════════════════════════════════════════════
// RENDU DE LA CARTE
// ════════════════════════════════════════════════════════════════════════
void session_card_render(SessionCardState* card, SDL_Renderer* renderer) {
    if (!card || card->phase == CARD_FINISHED) return;

    // Créer la texture si elle n'existe pas encore
    if (!card->card_texture) {
        card->card_texture = create_card_texture(
            renderer,
            card->session_number,
            card->card_width,
            card->card_height,
            card->font_path,
            card->text_color
        );

        if (!card->card_texture) {
            debug_printf("❌ Impossible de créer la texture de la carte\n");
            return;
        }
    }

    // Dessiner la carte à sa position actuelle
    SDL_Rect dest_rect = {
        card->current_x,
        card->current_y,
        card->card_width,
        card->card_height
    };

    SDL_RenderCopy(renderer, card->card_texture, NULL, &dest_rect);
}

// ════════════════════════════════════════════════════════════════════════
// RÉINITIALISATION POUR UNE NOUVELLE SESSION
// ════════════════════════════════════════════════════════════════════════
void session_card_reset(SessionCardState* card, int session_number, SDL_Renderer* renderer) {
    if (!card) return;

    // Détruire l'ancienne texture
    if (card->card_texture) {
        SDL_DestroyTexture(card->card_texture);
        card->card_texture = NULL;
    }

    // Mettre à jour le numéro
    card->session_number = session_number;

    // Recréer la texture avec le nouveau numéro
    card->card_texture = create_card_texture(
        renderer,
        session_number,
        card->card_width,
        card->card_height,
        card->font_path,
        card->text_color
    );

    // Réinitialiser l'animation
    card->phase = CARD_FINISHED;
    card->elapsed_time = 0.0f;
    card->current_x = -card->card_width;

    debug_printf("🔄 Carte réinitialisée pour session %d\n", session_number);
}

// ════════════════════════════════════════════════════════════════════════
// VÉRIFICATION FIN D'ANIMATION
// ════════════════════════════════════════════════════════════════════════
bool session_card_is_finished(SessionCardState* card) {
    return (card && card->phase == CARD_FINISHED);
}

// ════════════════════════════════════════════════════════════════════════
// LIBÉRATION MÉMOIRE
// ════════════════════════════════════════════════════════════════════════
void session_card_destroy(SessionCardState* card) {
    if (!card) return;

    if (card->card_texture) {
        SDL_DestroyTexture(card->card_texture);
    }

    free(card);
    debug_printf("🗑️ Carte de session détruite\n");
}
