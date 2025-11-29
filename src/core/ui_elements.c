// SPDX-License-Identifier: GPL-3.0-or-later
// ui_elements.c
#include "settings_panel.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdio.h>
#include "debug.h"

// Créer un slider
Slider create_slider(int x, int y, int width, int min_val, int max_val, int start_val) {
    Slider slider;
    slider.min_value = min_val;
    slider.max_value = max_val;
    slider.current_value = start_val;
    slider.track_rect = (SDL_Rect){x, y, width, 10};
    slider.thumb_rect = (SDL_Rect){0, y - 5, 20, 20};
    slider.is_dragging = false;

    update_slider_thumb_position(&slider);
    return slider;
}

void update_slider_thumb_position(Slider* slider) {
    if (!slider) return;

    float ratio = (float)(slider->current_value - slider->min_value) /
    (float)(slider->max_value - slider->min_value);

    slider->thumb_rect.x = slider->track_rect.x + (int)(ratio * slider->track_rect.w) - 10;
    slider->thumb_rect.y = slider->track_rect.y - 5;
}

void render_slider(SDL_Renderer* renderer, Slider* slider, TTF_Font* font, int offset_x, int offset_y) {
    if (!slider || !renderer) return;

    // === AJOUTER LE DÉCALAGE ===
    SDL_Rect track_rect = {
        slider->track_rect.x + offset_x,
        slider->track_rect.y + offset_y,
        slider->track_rect.w,
        slider->track_rect.h
    };

    SDL_Rect thumb_rect = {
        slider->thumb_rect.x + offset_x,
        slider->thumb_rect.y + offset_y,
        slider->thumb_rect.w,
        slider->thumb_rect.h
    };

    // Piste du slider (gris)
    boxColor(renderer,
             track_rect.x, track_rect.y,
             track_rect.x + track_rect.w,
             track_rect.y + track_rect.h,
             0xFF888888);

    // Curseur (bleu quand drag, blanc sinon)
    Uint32 thumb_color = slider->is_dragging ? 0xFF0000FF : 0xFFFFFFFF;
    filledCircleColor(renderer,
                      thumb_rect.x + thumb_rect.w/2,
                      thumb_rect.y + thumb_rect.h/2,
                      thumb_rect.w/2, thumb_color);

    // Affichage de la valeur
    char value_text[20];
    snprintf(value_text, sizeof(value_text), "%d", slider->current_value);

    // Afficher le texte de la valeur
    if (font) {
        render_text(renderer, font, value_text,
                    track_rect.x + track_rect.w + 10,
                    track_rect.y - 5, 0xFFFFFFFF);
    }
}

bool handle_slider_event(Slider* slider, SDL_Event* event) {
    if (!slider || !event) return false;

    switch(event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (is_point_in_rect(event->button.x, event->button.y, slider->thumb_rect)) {
                slider->is_dragging = true;
                return true;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            slider->is_dragging = false;
            break;

        case SDL_MOUSEMOTION:
            if (slider->is_dragging) {
                int mouse_x = event->motion.x;
                float ratio = (float)(mouse_x - slider->track_rect.x) / slider->track_rect.w;
                ratio = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);

                slider->current_value = slider->min_value +
                (int)(ratio * (slider->max_value - slider->min_value));

                update_slider_thumb_position(slider);
                return true;
            }
            break;
    }
    return false;
}

// Fonction pour afficher du texte
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, Uint32 color) {
    if (!font || !text) return;

    SDL_Color sdl_color = {
        (color >> 16) & 0xFF,   // R
        (color >> 8) & 0xFF,    // G
        color & 0xFF,           // B
        255                     // A
    };

    // Utiliser UTF-8 pour supporter les accents
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text, sdl_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect dest_rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    } else {
        debug_printf("Erreur rendu texte: %s\n", TTF_GetError());
    }
}

// Fonction pour créer un bouton
UIButton create_button(const char* text, int x, int y, int width, int height) {
    UIButton button;
    snprintf(button.text, sizeof(button.text), "%s", text);
    button.rect = (SDL_Rect){x, y, width, height};
    button.texture = NULL;
    button.is_hovered = false;
    return button;
}

void render_button(SDL_Renderer* renderer, UIButton* button, TTF_Font* font, int offset_x, int offset_y) {
    if (!button || !renderer) return;

    SDL_Rect rect = {
        button->rect.x + offset_x,
        button->rect.y + offset_y,
        button->rect.w,
        button->rect.h
    };

    // Couleurs des boutons
    Uint32 bg_color;
    if (strstr(button->text, "Appliquer")) {
        bg_color = 0xFF4CAF50; // Vert
    } else if (strstr(button->text, "Annuler")) {
        bg_color = 0xFFF44336; // Rouge
    } else {
        bg_color = 0xFF888888; // Gris par défaut
    }

    // Dessiner le fond du bouton
    boxColor(renderer,
             rect.x, rect.y,
             rect.x + rect.w,
             rect.y + rect.h,
             bg_color);

    // Bordure
    rectangleColor(renderer,
                   rect.x, rect.y,
                   rect.x + rect.w,
                   rect.y + rect.h,
                   0xFF000000);

    // Texte centré avec UTF-8
    if (font) {
        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, button->text, (SDL_Color){255, 255, 255, 255});
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect text_rect = {
                rect.x + (rect.w - surface->w) / 2,
                rect.y + (rect.h - surface->h) / 2,
                surface->w,
                surface->h
            };
            SDL_RenderCopy(renderer, texture, NULL, &text_rect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
    }
}

/*----------------------------------------------------------------*/

// Fonction helper pour détecter les clics dans un rectangle
bool is_point_in_rect(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w &&
    y >= rect.y && y <= rect.y + rect.h);
}
