#include "toggle_widget.h"
#include "settings_panel.h"
#include "debug.h"
#include <stdio.h>
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION D'UN WIDGET TOGGLE
// ════════════════════════════════════════════════════════════════════════════
// Crée un widget interrupteur avec animation de transition
//
// PARAMÈTRES :
//   - name : Texte affiché à gauche (ex: "Cycles alternés")
//   - x, y : Position RELATIVE au conteneur parent
//   - start_state : État initial (true = ON, false = OFF)
//   - toggle_width, toggle_height : Dimensions du bouton toggle
//   - thumb_size : Diamètre du curseur circulaire
//   - text_size : Taille de référence du texte
//   - font : Police TTF pour calculer les dimensions
ToggleWidget* create_toggle_widget(const char* name, int x, int y, bool start_state,
                                   int toggle_width, int toggle_height, int thumb_size,
                                   int text_size, TTF_Font* font) {
    ToggleWidget* widget = malloc(sizeof(ToggleWidget));
    if (!widget) {
        debug_printf("❌ Erreur allocation toggle widget: %s\n", name);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DES VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    snprintf(widget->option_name, sizeof(widget->option_name), "%s", name);
    widget->value = start_state;
    widget->x = x;
    widget->y = y;
    widget->toggle_width = toggle_width;
    widget->toggle_height = toggle_height;
    widget->thumb_size = thumb_size;
    widget->animation_progress = start_state ? 1.0f : 0.0f;
    widget->is_animating = false;
    widget->whole_widget_hovered = false;
    widget->toggle_hovered = false;

    // ─────────────────────────────────────────────────────────────────────────
    // COULEURS DU WIDGET (style moderne)
    // ─────────────────────────────────────────────────────────────────────────
    widget->bg_off_color = (SDL_Color){181, 181, 181, 255};      // Gris
    widget->bg_on_color = (SDL_Color){76, 175, 80, 255};         // Vert
    widget->thumb_color = (SDL_Color){255, 255, 255, 255};       // Blanc
    widget->text_color = (SDL_Color){0, 0, 0, 255};              // Texte noir
    widget->bg_hover_color = (SDL_Color){0, 0, 0, 50};           // Fond transparent

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DU LAYOUT
    // ─────────────────────────────────────────────────────────────────────────
    int text_width = 0;
    if (font) {
        TTF_SizeUTF8(font, name, &text_width, &widget->text_height);
    } else {
        text_width = strlen(name) * (text_size / 2);
        widget->text_height = text_size;
    }

    int espace_apres_texte = 20;    // Espace entre le texte et le toggle

    widget->name_x = x;
    widget->toggle_x = x + text_width + espace_apres_texte;
    widget->text_center_y = y + widget->text_height / 2;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DES RECTANGLES (positions RELATIVES)
    // ─────────────────────────────────────────────────────────────────────────
    // Rectangle du bouton toggle
    widget->toggle_rect = (SDL_Rect){
        widget->toggle_x,
        widget->text_center_y - (toggle_height / 2),  // Centré verticalement
        toggle_width,
        toggle_height
    };

    // Rectangle du curseur (position initiale basée sur l'état)
    int thumb_offset = (toggle_height - thumb_size) / 2;
    int start_x = start_state ?
        (widget->toggle_rect.x + toggle_width - thumb_size - thumb_offset) :
        (widget->toggle_rect.x + thumb_offset);

    widget->thumb_rect = (SDL_Rect){
        start_x,
        widget->toggle_rect.y + thumb_offset,
        thumb_size,
        thumb_size
    };

    widget->on_value_changed = NULL;

    debug_printf("✅ Toggle widget '%s' créé - Layout: nom@%d, toggle@%d, état: %s\n",
                 name, widget->name_x, widget->toggle_x,
                 start_state ? "ON" : "OFF");

    return widget;
}

// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DE L'ANIMATION
// ════════════════════════════════════════════════════════════════════════════
// Gère l'interpolation du curseur pour un effet fluide
void update_toggle_widget(ToggleWidget* widget, float delta_time) {
    if (!widget || !widget->is_animating) return;

    // Vitesse d'animation (ajustable)
    float animation_speed = 8.0f;
    float delta = animation_speed * delta_time;

    if (widget->value) {
        // Animation vers ON
        widget->animation_progress += delta;
        if (widget->animation_progress >= 1.0f) {
            widget->animation_progress = 1.0f;
            widget->is_animating = false;
        }
    } else {
        // Animation vers OFF
        widget->animation_progress -= delta;
        if (widget->animation_progress <= 0.0f) {
            widget->animation_progress = 0.0f;
            widget->is_animating = false;
        }
    }

    // Mise à jour de la position du curseur
    int thumb_offset = (widget->toggle_height - widget->thumb_size) / 2;
    int min_x = widget->toggle_rect.x + thumb_offset;
    int max_x = widget->toggle_rect.x + widget->toggle_width - widget->thumb_size - thumb_offset;

    widget->thumb_rect.x = min_x + (int)((max_x - min_x) * widget->animation_progress);
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU WIDGET TOGGLE
// ════════════════════════════════════════════════════════════════════════════
void render_toggle_widget(SDL_Renderer* renderer, ToggleWidget* widget, TTF_Font* font,
                         int offset_x, int offset_y) {
    if (!widget || !renderer) {
        debug_printf("❌ Rendu toggle: widget ou renderer NULL\n");
        return;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DES POSITIONS ABSOLUES
    // ─────────────────────────────────────────────────────────────────────────
    int absolute_name_x = widget->name_x + offset_x;
    int absolute_y = widget->y + offset_y;

    SDL_Rect absolute_toggle_rect = {
        widget->toggle_rect.x + offset_x,
        widget->toggle_rect.y + offset_y,
        widget->toggle_rect.w,
        widget->toggle_rect.h
    };

    SDL_Rect absolute_thumb_rect = {
        widget->thumb_rect.x + offset_x,
        widget->thumb_rect.y + offset_y,
        widget->thumb_rect.w,
        widget->thumb_rect.h
    };

    debug_printf("🎨 Rendu toggle '%s' - Texte: (%d,%d), Toggle: (%d,%d %dx%d)\n",
                 widget->option_name,
                 absolute_name_x, absolute_y,
                 absolute_toggle_rect.x, absolute_toggle_rect.y,
                 absolute_toggle_rect.w, absolute_toggle_rect.h);

    // ─────────────────────────────────────────────────────────────────────────
    // FOND AU SURVOL DU WIDGET COMPLET
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->whole_widget_hovered) {
        // Calcul de la zone totale (texte + toggle)
        int total_width = (widget->toggle_x + widget->toggle_width) - widget->name_x;
        SDL_Rect bg_rect = {
            absolute_name_x - 10,
            absolute_y - 5,
            total_width + 20,
            widget->text_height + 10
        };

        Uint32 bg_color = (widget->bg_hover_color.a << 24) |
                         (widget->bg_hover_color.r << 16) |
                         (widget->bg_hover_color.g << 8) |
                         widget->bg_hover_color.b;

        roundedBoxColor(renderer, bg_rect.x, bg_rect.y,
                 bg_rect.x + bg_rect.w, bg_rect.y + bg_rect.h,
                 bg_rect.h/2,
                 bg_color);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DU TEXTE
    // ─────────────────────────────────────────────────────────────────────────
    if (font) {
        render_text(renderer, font, widget->option_name, absolute_name_x, absolute_y,
                    (widget->text_color.r << 16) | (widget->text_color.g << 8) | widget->text_color.b);
    } else {
        debug_printf("❌ Police non disponible pour le toggle widget\n");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DU BOUTON TOGGLE
    // ─────────────────────────────────────────────────────────────────────────
    // Fond du toggle (arrondi)
    SDL_Color current_bg_color = widget->value ? widget->bg_on_color : widget->bg_off_color;

    // Si survolé, éclaircir la couleur
    if (widget->toggle_hovered) {
        current_bg_color.r = SDL_min(255, current_bg_color.r + 30);
        current_bg_color.g = SDL_min(255, current_bg_color.g + 30);
        current_bg_color.b = SDL_min(255, current_bg_color.b + 30);
    }

    Uint32 bg_color_value = (current_bg_color.a << 24) |
                           (current_bg_color.r << 16) |
                            current_bg_color.g <<8 |
                            current_bg_color.b;

    // Fond arrondi du toggle
    roundedBoxColor(renderer,
                   absolute_toggle_rect.x, absolute_toggle_rect.y,
                   absolute_toggle_rect.x + absolute_toggle_rect.w,
                   absolute_toggle_rect.y + absolute_toggle_rect.h,
                   absolute_toggle_rect.h / 2,  // Rayon = moitié de la hauteur
                   bg_color_value);

    debug_printf("🎨 RENDU TOGGLE - Rect absolu: x=%d, y=%d, w=%d, h=%d\n",
                 absolute_toggle_rect.x, absolute_toggle_rect.y,
                 absolute_toggle_rect.w, absolute_toggle_rect.h);

    // Curseur (cercle blanc)
    int center_x = absolute_thumb_rect.x + (absolute_thumb_rect.w / 2);
    int center_y = absolute_thumb_rect.y + (absolute_thumb_rect.h / 2);
    int radius = absolute_thumb_rect.w / 2;

    filledCircleColor(renderer, center_x, center_y, radius,
                     (widget->thumb_color.a << 24) |
                     (widget->thumb_color.r << 16) |
                      widget->thumb_color.g << 8 |
                      widget->thumb_color.b);

    debug_printf("✅ Toggle '%s' rendu - État: %s\n",
                 widget->option_name, widget->value ? "ON" : "OFF");
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════
void handle_toggle_widget_events(ToggleWidget* widget, SDL_Event* event,
                                int offset_x, int offset_y) {
    if (!widget || !event) return;

    switch (event->type) {
        case SDL_MOUSEMOTION: {
            int mouse_x = event->motion.x;
            int mouse_y = event->motion.y;

            // Zone complète du widget
            int total_width = (widget->toggle_x + widget->toggle_width) - widget->name_x;
            SDL_Rect widget_rect = {
                widget->name_x + offset_x - 10,
                widget->y + offset_y - 5,
                total_width + 20,
                widget->text_height + 10
            };
            widget->whole_widget_hovered = is_point_in_rect(mouse_x, mouse_y, widget_rect);

            // Zone du bouton toggle
            SDL_Rect toggle_abs_rect = {
                widget->toggle_rect.x + offset_x,
                widget->toggle_rect.y + offset_y,
                widget->toggle_rect.w,
                widget->toggle_rect.h
            };
            widget->toggle_hovered = is_point_in_rect(mouse_x, mouse_y, toggle_abs_rect);
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            if (event->button.button != SDL_BUTTON_LEFT) break;

            int mouse_x = event->button.x;
            int mouse_y = event->button.y;

            SDL_Rect toggle_abs_rect = {
                widget->toggle_rect.x + offset_x,
                widget->toggle_rect.y + offset_y,
                widget->toggle_rect.w,
                widget->toggle_rect.h
            };

            if (is_point_in_rect(mouse_x, mouse_y, toggle_abs_rect)) {
                toggle_widget_value(widget);
            }
            break;
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  BASculement de la valeur
// ════════════════════════════════════════════════════════════════════════════
void toggle_widget_value(ToggleWidget* widget) {
    if (!widget) return;

    widget->value = !widget->value;
    widget->is_animating = true;

    debug_printf("🔄 Toggle '%s' basculé: %s\n",
                 widget->option_name,
                 widget->value ? "ON" : "OFF");

    if (widget->on_value_changed) {
        widget->on_value_changed(widget->value);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACK DE CHANGEMENT DE VALEUR
// ════════════════════════════════════════════════════════════════════════════
void set_toggle_value_changed_callback(ToggleWidget* widget, void (*callback)(bool)) {
    if (widget) {
        widget->on_value_changed = callback;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void free_toggle_widget(ToggleWidget* widget) {
    if (!widget) return;

    char widget_name[50];
    snprintf(widget_name, sizeof(widget_name), "%s", widget->option_name);

    free(widget);
    debug_printf("🗑️ Toggle widget '%s' libéré\n", widget_name);
}
