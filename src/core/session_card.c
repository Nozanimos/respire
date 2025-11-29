// SPDX-License-Identifier: GPL-3.0-or-later
// session_card.c - Carte de session animée avec Cairo
#include "session_card.h"
#include "debug.h"
#include "paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL_image.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// CONSTANTES
#define CARD_WIDTH_BASE  200    // Largeur de base (ratio carte de poker ~2.5:3.5)
#define CARD_HEIGHT_BASE 280    // Hauteur de base
#define CARD_CORNER_RADIUS 15   // Rayon des coins arrondis (comme une carte de poker)
#define CARD_MIN_SCALE 0.67f    // Scale minimum de la carte (2/3 de la taille de base)

#define MARGIN_SIDES  10        // Marge gauche/droite
#define MARGIN_TOP    10        // Marge du haut
#define MARGIN_BOTTOM 10        // Marge du bas

// CRÉATION DE LA TEXTURE DE LA CARTE
// Crée la texture complète : background + texte Cairo avec effet vitré
static SDL_Texture* create_card_texture(SDL_Renderer* renderer, int session_number,
                                       int width, int height,
                                       const char* font_path) {
    // 1. Charger l'image de fond (vert.jpg)
    SDL_Surface* bg_surface = IMG_Load(IMG_VERT);
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

    // 2. Créer une surface Cairo pour dessiner
    cairo_surface_t* cairo_surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32,
        width, height
    );

    cairo_t* cr = cairo_create(cairo_surface);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // 3. Créer un masque avec coins arrondis (rounded rectangle)
    double radius = CARD_CORNER_RADIUS;
    double x = 0, y = 0;
    double w = width, h = height;

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius, radius, -M_PI/2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI/2);
    cairo_arc(cr, x + radius, y + h - radius, radius, M_PI/2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_clip(cr);

    // 4. Dessiner le background dans le masque arrondi
    // Créer une surface temporaire du background
    cairo_surface_t* bg_pattern_surface = cairo_image_surface_create_for_data(
        (unsigned char*)scaled_bg->pixels,
        CAIRO_FORMAT_ARGB32,
        width, height,
        scaled_bg->pitch
    );
    cairo_set_source_surface(cr, bg_pattern_surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(bg_pattern_surface);

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

    // 5. Dessiner "Session" en haut avec effet vitré
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

    // Créer le path du texte
    cairo_move_to(cr, title_x, title_y);
    cairo_text_path(cr, title_text);

    // Contour blanc brillant (stroke)
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8);  // Blanc semi-transparent
    cairo_set_line_width(cr, 3.0);
    cairo_stroke_preserve(cr);

    // Remplissage noir
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);  // Noir
    cairo_fill(cr);

    // Effet vitré (reflet lumineux en haut)
    cairo_pattern_t* glass_pattern = cairo_pattern_create_linear(
        title_x, title_y - title_extents.height,
        title_x, title_y - title_extents.height/3
    );
    cairo_pattern_add_color_stop_rgba(glass_pattern, 0.0, 1.0, 1.0, 1.0, 0.4);
    cairo_pattern_add_color_stop_rgba(glass_pattern, 1.0, 1.0, 1.0, 1.0, 0.0);

    cairo_move_to(cr, title_x, title_y);
    cairo_text_path(cr, title_text);
    cairo_set_source(cr, glass_pattern);
    cairo_fill(cr);
    cairo_pattern_destroy(glass_pattern);

    // 6. Dessiner le numéro de session avec effet vitré
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

    // Créer le path du numéro
    cairo_move_to(cr, number_x, number_y);
    cairo_text_path(cr, number_text);

    // Contour blanc brillant (stroke)
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8);  // Blanc semi-transparent
    cairo_set_line_width(cr, 5.0);  // Plus épais pour le gros chiffre
    cairo_stroke_preserve(cr);

    // Remplissage noir
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);  // Noir
    cairo_fill(cr);

    // Effet vitré (reflet lumineux en haut)
    cairo_pattern_t* number_glass_pattern = cairo_pattern_create_linear(
        number_x, number_y - number_extents.height,
        number_x, number_y - number_extents.height/2.5
    );
    cairo_pattern_add_color_stop_rgba(number_glass_pattern, 0.0, 1.0, 1.0, 1.0, 0.5);
    cairo_pattern_add_color_stop_rgba(number_glass_pattern, 1.0, 1.0, 1.0, 1.0, 0.0);

    cairo_move_to(cr, number_x, number_y);
    cairo_text_path(cr, number_text);
    cairo_set_source(cr, number_glass_pattern);
    cairo_fill(cr);
    cairo_pattern_destroy(number_glass_pattern);

    // 7. Nettoyer Cairo et FreeType
    cairo_surface_flush(cairo_surface);
    cairo_font_face_destroy(cairo_face);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);

    // 8. Convertir la surface Cairo en SDL_Surface
    SDL_Surface* final_surface = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_ARGB8888
    );

    if (final_surface) {
        memcpy(final_surface->pixels,
               cairo_image_surface_get_data(cairo_surface),
               height * cairo_image_surface_get_stride(cairo_surface));
    }

    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);
    SDL_FreeSurface(scaled_bg);

    if (!final_surface) {
        debug_printf("❌ Erreur création surface finale\n");
        return NULL;
    }

    // 9. Créer la texture SDL
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, final_surface);
    SDL_FreeSurface(final_surface);

    if (!texture) {
        debug_printf("❌ Erreur création texture carte: %s\n", SDL_GetError());
    }

    return texture;
}

// CRÉATION ET INITIALISATION
SessionCardState* session_card_create(int session_number, int screen_width,
                                      int screen_height, const char* font_path,
                                      float scale_factor) {
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

    // 🆕 APPLIQUER LE SCALE_FACTOR AUX DIMENSIONS DE LA CARTE
    // Pour que la carte s'adapte au redimensionnement de la fenêtre
    // Limiter le scale minimum à 2/3 pour éviter que la carte devienne trop petite
    float card_scale = fmaxf(CARD_MIN_SCALE, scale_factor);
    card->scale_factor = card_scale;  // 🆕 Stocker pour utilisation ultérieure
    card->card_width = (int)(CARD_WIDTH_BASE * card_scale);
    card->card_height = (int)(CARD_HEIGHT_BASE * card_scale);

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

// DÉMARRAGE DE L'ANIMATION
void session_card_start(SessionCardState* card) {
    if (!card) return;

    card->phase = CARD_ENTERING;
    card->elapsed_time = 0.0f;
    card->current_x = -card->card_width;  // Commence hors écran à gauche

    debug_printf("🎬 Animation carte de session démarrée (session %d)\n", card->session_number);
}

// MISE À JOUR DE L'ANIMATION
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

// RENDU DE LA CARTE
void session_card_render(SessionCardState* card, SDL_Renderer* renderer) {
    if (!card || card->phase == CARD_FINISHED) return;

    // Créer la texture si elle n'existe pas encore
    if (!card->card_texture) {
        card->card_texture = create_card_texture(
            renderer,
            card->session_number,
            card->card_width,
            card->card_height,
            card->font_path
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

// MISE À JOUR DES DIMENSIONS D'ÉCRAN (lors du redimensionnement)
void session_card_update_screen_size(SessionCardState* card, int new_screen_width,
                                      int new_screen_height, float scale_factor) {
    if (!card) return;

    // Sauvegarder l'ancienne position center_x pour ajuster current_x si en animation
    int old_center_x = card->center_x;

    // 🆕 Mettre à jour les dimensions de la carte avec le nouveau scale_factor
    // Limiter le scale minimum à 2/3 pour éviter que la carte devienne trop petite
    float card_scale = fmaxf(CARD_MIN_SCALE, scale_factor);
    card->scale_factor = card_scale;  // 🆕 Mettre à jour le scale_factor stocké
    int new_card_width = (int)(CARD_WIDTH_BASE * card_scale);
    int new_card_height = (int)(CARD_HEIGHT_BASE * card_scale);

    // Si les dimensions ont changé, détruire la texture pour qu'elle soit recréée
    if (new_card_width != card->card_width || new_card_height != card->card_height) {
        if (card->card_texture) {
            SDL_DestroyTexture(card->card_texture);
            card->card_texture = NULL;
            debug_printf("🔄 Texture de la carte détruite pour recréation avec nouvelles dimensions\n");
        }
    }

    card->card_width = new_card_width;
    card->card_height = new_card_height;

    // Mettre à jour la largeur d'écran
    card->screen_width = new_screen_width;

    // Recalculer la position centrale (horizontale)
    card->center_x = new_screen_width / 2 - card->card_width / 2;

    // Recalculer la position verticale (toujours centrée)
    card->current_y = (new_screen_height - card->card_height) / 2;

    // 🎯 AJUSTER current_x si l'animation est en cours
    // Si la carte est en pause (au centre), la garder au centre
    if (card->phase == CARD_PAUSED) {
        card->current_x = card->center_x;
    }
    // Si la carte est en mouvement, ajuster proportionnellement
    else if (card->phase == CARD_ENTERING || card->phase == CARD_EXITING) {
        // Calculer l'offset relatif par rapport à l'ancien centre
        int offset_from_old_center = card->current_x - old_center_x;
        // Appliquer le même offset au nouveau centre
        card->current_x = card->center_x + offset_from_old_center;
    }

    debug_printf("🔄 Carte session mise à jour: screen=%dx%d, card=%dx%d, scale=%.2f→%.2f, center_x=%d, current_y=%d\n",
                 new_screen_width, new_screen_height, card->card_width, card->card_height,
                 scale_factor, card_scale, card->center_x, card->current_y);
}

// RÉINITIALISATION POUR UNE NOUVELLE SESSION
void session_card_reset(SessionCardState* card, int session_number, SDL_Renderer* renderer) {
    if (!card) return;

    // Détruire l'ancienne texture
    if (card->card_texture) {
        SDL_DestroyTexture(card->card_texture);
        card->card_texture = NULL;
    }

    // Mettre à jour le numéro
    card->session_number = session_number;

    // 🆕 RECALCULER LES DIMENSIONS avec le scale_factor actuel
    // Cela garantit que la police sera toujours à la bonne taille,
    // même si la fenêtre a été redimensionnée depuis la création initiale
    card->card_width = (int)(CARD_WIDTH_BASE * card->scale_factor);
    card->card_height = (int)(CARD_HEIGHT_BASE * card->scale_factor);

    // Recréer la texture avec le nouveau numéro ET les dimensions à jour
    card->card_texture = create_card_texture(
        renderer,
        session_number,
        card->card_width,
        card->card_height,
        card->font_path
    );

    // Réinitialiser l'animation
    card->phase = CARD_FINISHED;
    card->elapsed_time = 0.0f;
    card->current_x = -card->card_width;

    debug_printf("🔄 Carte réinitialisée pour session %d (dimensions: %dx%d, scale: %.2f)\n",
                 session_number, card->card_width, card->card_height, card->scale_factor);
}

// VÉRIFICATION FIN D'ANIMATION
bool session_card_is_finished(SessionCardState* card) {
    return (card && card->phase == CARD_FINISHED);
}

// LIBÉRATION MÉMOIRE
void session_card_destroy(SessionCardState* card) {
    if (!card) return;

    if (card->card_texture) {
        SDL_DestroyTexture(card->card_texture);
    }

    free(card);
    debug_printf("🗑️ Carte de session détruite\n");
}
