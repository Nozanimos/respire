#include "toggle_widget.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION D'UN WIDGET TOGGLE
// ════════════════════════════════════════════════════════════════════════════
ToggleWidget* create_toggle_widget(const char* name, int x, int y, bool start_state,
                                   int toggle_width, int toggle_height, int thumb_size,
                                   int text_size) {
    ToggleWidget* widget = malloc(sizeof(ToggleWidget));
    if (!widget) {
        debug_printf("❌ Erreur allocation ToggleWidget: %s\n", name);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DES VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    snprintf(widget->option_name, sizeof(widget->option_name), "%s", name);
    widget->value = start_state;
    widget->toggle_width = toggle_width;
    widget->toggle_height = toggle_height;
    widget->thumb_size = thumb_size;
    widget->base_toggle_width = toggle_width;
    widget->base_toggle_height = toggle_height;
    widget->base_thumb_size = thumb_size;
    widget->animation_progress = start_state ? 1.0f : 0.0f;
    widget->is_animating = false;
    widget->toggle_hovered = false;

    // Stocker la taille de police de base
    widget->base_text_size = text_size;
    widget->current_text_size = text_size;

    // Espacement de base
    widget->base_espace_apres_texte = 20;

    // ─────────────────────────────────────────────────────────────────────────
    // COULEURS DU WIDGET (style moderne)
    // ─────────────────────────────────────────────────────────────────────────
    widget->bg_off_color = (SDL_Color){181, 181, 181, 255};      // Gris
    widget->bg_on_color = (SDL_Color){76, 175, 80, 255};         // Vert
    widget->thumb_color = (SDL_Color){255, 255, 255, 255};       // Blanc
    widget->text_color = (SDL_Color){0, 0, 0, 255};              // Texte noir
    widget->bg_hover_color = (SDL_Color){0, 0, 0, 50};           // Fond transparent

    // ─────────────────────────────────────────────────────────────────────────
    // MESURE DU TEXTE (pour calculer les dimensions)
    // ─────────────────────────────────────────────────────────────────────────
    int text_width = 0;
    int text_height = 0;

    TTF_Font* correct_font = get_font_for_size(widget->current_text_size);
    if (correct_font) {
        TTF_SizeUTF8(correct_font, name, &text_width, &text_height);
    } else {
        text_width = strlen(name) * (text_size / 2);
        text_height = text_size;
    }

    widget->text_height = text_height;

    // ─────────────────────────────────────────────────────────────────────────
    // LAYOUT INTERNE (coordonnées LOCALES au widget)
    // ─────────────────────────────────────────────────────────────────────────
    widget->local_text_x = 0;
    widget->local_text_y = 0;

    widget->local_toggle_x = text_width + widget->base_espace_apres_texte;
    widget->local_toggle_y = (text_height - toggle_height) / 2;

    // ─────────────────────────────────────────────────────────────────────────
    // POSITION DU THUMB DANS LE TOGGLE (coordonnées LOCALES au toggle)
    // ─────────────────────────────────────────────────────────────────────────
    int thumb_offset = (toggle_height - thumb_size) / 2;

    if (start_state) {
        widget->thumb_local_x = toggle_width - thumb_size - thumb_offset;
    } else {
        widget->thumb_local_x = thumb_offset;
    }
    widget->thumb_local_y = thumb_offset;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DE LA BOUNDING BOX TOTALE
    // ─────────────────────────────────────────────────────────────────────────
    int total_width = widget->local_toggle_x + toggle_width + 10;
    int total_height = text_height > toggle_height ? text_height : toggle_height;

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DE LA BASE (WidgetBase)
    // ─────────────────────────────────────────────────────────────────────────
    widget->base.x = x;
    widget->base.y = y;
    widget->base.base_x = x;
    widget->base.base_y = y;
    widget->base.width = total_width;
    widget->base.height = total_height;
    widget->base.base_width = total_width;
    widget->base.base_height = total_height;
    widget->base.hovered = false;
    widget->base.enabled = true;

    widget->on_value_changed = NULL;

    debug_subsection("Création ToggleWidget");
    debug_printf("  Nom : %s\n", name);
    debug_printf("  Position : (%d, %d)\n", x, y);
    debug_printf("  Taille : %dx%d\n", total_width, total_height);
    debug_printf("  Police : %dpx\n", text_size);
    debug_printf("  Largeur texte mesuré : %dpx\n", text_width);
    debug_printf("  État : %s\n", start_state ? "ON" : "OFF");
    debug_blank_line();

    return widget;
}

// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DE L'ANIMATION
// ════════════════════════════════════════════════════════════════════════════
void update_toggle_widget(ToggleWidget* widget, float delta_time) {
    if (!widget || !widget->is_animating) return;

    float animation_speed = 6.0f;

    if (widget->value) {
        widget->animation_progress += animation_speed * delta_time;
        if (widget->animation_progress >= 1.0f) {
            widget->animation_progress = 1.0f;
            widget->is_animating = false;
        }
    } else {
        widget->animation_progress -= animation_speed * delta_time;
        if (widget->animation_progress <= 0.0f) {
            widget->animation_progress = 0.0f;
            widget->is_animating = false;
        }
    }

    // Calculer la position du thumb selon l'animation
    int thumb_offset = (widget->toggle_height - widget->thumb_size) / 2;
    int thumb_min = thumb_offset;
    int thumb_max = widget->toggle_width - widget->thumb_size - thumb_offset;

    widget->thumb_local_x = thumb_min + (int)((thumb_max - thumb_min) * widget->animation_progress);
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void render_toggle_widget(SDL_Renderer* renderer, ToggleWidget* widget,
                          int offset_x, int offset_y) {
    if (!widget || !renderer) return;

    int widget_screen_x = offset_x + widget->base.x;
    int widget_screen_y = offset_y + widget->base.y;

    // ─────────────────────────────────────────────────────────────────────────
    // FOND AU SURVOL (rectangle arrondi)
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->base.hovered) {
        roundedBoxRGBA(renderer,
                       widget_screen_x - 5,
                       widget_screen_y - 5,
                       widget_screen_x + widget->base.width + 5,
                       widget_screen_y + widget->base.height + 5,
                       5,
                       widget->bg_hover_color.r,
                       widget->bg_hover_color.g,
                       widget->bg_hover_color.b,
                       widget->bg_hover_color.a);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DU TEXTE (nom de l'option)
    // ─────────────────────────────────────────────────────────────────────────
    TTF_Font* correct_font = get_font_for_size(widget->current_text_size);
    if (correct_font) {
        SDL_Surface* text_surface = TTF_RenderUTF8_Blended(correct_font, widget->option_name, widget->text_color);
        if (text_surface) {
            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            if (text_texture) {
                SDL_Rect text_rect = {
                    widget_screen_x + widget->local_text_x,
                    widget_screen_y + widget->local_text_y,
                    text_surface->w,
                    text_surface->h
                };
                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DU TOGGLE (rectangle avec bords arrondis)
    // ─────────────────────────────────────────────────────────────────────────
    int toggle_screen_x = widget_screen_x + widget->local_toggle_x;
    int toggle_screen_y = widget_screen_y + widget->local_toggle_y;

    // Interpoler la couleur de fond selon l'animation
    SDL_Color bg_color;
    if (widget->animation_progress == 0.0f) {
        bg_color = widget->bg_off_color;
    } else if (widget->animation_progress == 1.0f) {
        bg_color = widget->bg_on_color;
    } else {
        float t = widget->animation_progress;
        bg_color.r = (Uint8)(widget->bg_off_color.r * (1 - t) + widget->bg_on_color.r * t);
        bg_color.g = (Uint8)(widget->bg_off_color.g * (1 - t) + widget->bg_on_color.g * t);
        bg_color.b = (Uint8)(widget->bg_off_color.b * (1 - t) + widget->bg_on_color.b * t);
        bg_color.a = 255;
    }

    int radius = widget->toggle_height / 2;
    roundedBoxRGBA(renderer,
                   toggle_screen_x,
                   toggle_screen_y,
                   toggle_screen_x + widget->toggle_width,
                   toggle_screen_y + widget->toggle_height,
                   radius,
                   bg_color.r, bg_color.g, bg_color.b, bg_color.a);

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DU THUMB (cercle blanc)
    // ─────────────────────────────────────────────────────────────────────────
    int thumb_screen_x = toggle_screen_x + widget->thumb_local_x + widget->thumb_size / 2;
    int thumb_screen_y = toggle_screen_y + widget->thumb_local_y + widget->thumb_size / 2;

    filledCircleRGBA(renderer,
                     thumb_screen_x,
                     thumb_screen_y,
                     widget->thumb_size / 2,
                     widget->thumb_color.r,
                     widget->thumb_color.g,
                     widget->thumb_color.b,
                     widget->thumb_color.a);
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════
void handle_toggle_widget_events(ToggleWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y) {
    if (!widget || !event) return;

    int widget_screen_x = offset_x + widget->base.x;
    int widget_screen_y = offset_y + widget->base.y;

    if (event->type == SDL_MOUSEMOTION) {
        int mx = event->motion.x;
        int my = event->motion.y;

        widget->base.hovered = widget_contains_point(&widget->base, mx, my, offset_x, offset_y);

        int toggle_screen_x = widget_screen_x + widget->local_toggle_x;
        int toggle_screen_y = widget_screen_y + widget->local_toggle_y;

        widget->toggle_hovered = (mx >= toggle_screen_x &&
                                  mx < toggle_screen_x + widget->toggle_width &&
                                  my >= toggle_screen_y &&
                                  my < toggle_screen_y + widget->toggle_height);
    }
    else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        if (widget->base.hovered) {
            toggle_widget_value(widget);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  BASCULER LA VALEUR DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void toggle_widget_value(ToggleWidget* widget) {
    if (!widget) return;

    widget->value = !widget->value;
    widget->is_animating = true;

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
//  RESPONSIVE : RESCALE DU WIDGET (INTELLIGENT)
// ════════════════════════════════════════════════════════════════════════════
void rescale_toggle_widget(ToggleWidget* widget, float panel_ratio) {
    if (!widget) return;

    debug_subsection("Rescale TOGGLE (intelligent)");
    debug_printf("  Widget : %s\n", widget->option_name);
    debug_printf("  Ratio : %.2f\n", panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 1. SCALER LA BASE (position du widget)
    // ─────────────────────────────────────────────────────────────────────────
    rescale_widget_base(&widget->base, panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 2. CALCULER LA NOUVELLE TAILLE DE POLICE
    // ─────────────────────────────────────────────────────────────────────────
    int new_text_size = (int)(widget->base_text_size * panel_ratio);
    widget->current_text_size = new_text_size;

    debug_printf("  Police : %dpx → %dpx\n", widget->base_text_size, new_text_size);

    // ─────────────────────────────────────────────────────────────────────────
    // 3. OBTENIR LA POLICE À CETTE TAILLE (avec minimum garanti)
    // ─────────────────────────────────────────────────────────────────────────
    TTF_Font* scaled_font = get_font_for_size(new_text_size);
    if (!scaled_font) {
        debug_printf("  ⚠️ Impossible d'obtenir police\n");
        debug_blank_line();
        return;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 4. REMESURER LE TEXTE AVEC LA NOUVELLE POLICE
    // ─────────────────────────────────────────────────────────────────────────
    int new_text_width = 0;
    int new_text_height = 0;
    TTF_SizeUTF8(scaled_font, widget->option_name, &new_text_width, &new_text_height);

    widget->text_height = new_text_height;

    debug_printf("  Texte remesuré : %dpx × %dpx\n", new_text_width, new_text_height);

    // ─────────────────────────────────────────────────────────────────────────
    // 5. SCALER L'ESPACEMENT
    // ─────────────────────────────────────────────────────────────────────────
    int scaled_espace_texte = (int)(widget->base_espace_apres_texte * panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 6. RECALCULER LES OFFSETS AVEC LES VRAIES DIMENSIONS
    // ─────────────────────────────────────────────────────────────────────────
    widget->local_toggle_x = new_text_width + scaled_espace_texte;

    // Scaler les dimensions du toggle
    widget->toggle_width = (int)(widget->base_toggle_width * panel_ratio);
    widget->toggle_height = (int)(widget->base_toggle_height * panel_ratio);
    widget->thumb_size = (int)(widget->base_thumb_size * panel_ratio);

    if (widget->toggle_width < 20) widget->toggle_width = 20;
    if (widget->toggle_height < 10) widget->toggle_height = 10;
    if (widget->thumb_size < 8) widget->thumb_size = 8;

    // Recentrer le toggle verticalement
    widget->local_toggle_y = (widget->text_height - widget->toggle_height) / 2;

    // ─────────────────────────────────────────────────────────────────────────
    // 7. RECALCULER LA POSITION DU THUMB
    // ─────────────────────────────────────────────────────────────────────────
    int thumb_offset = (widget->toggle_height - widget->thumb_size) / 2;
    int thumb_min = thumb_offset;
    int thumb_max = widget->toggle_width - widget->thumb_size - thumb_offset;

    widget->thumb_local_x = thumb_min + (int)((thumb_max - thumb_min) * widget->animation_progress);
    widget->thumb_local_y = thumb_offset;

    debug_printf("  ✓ Toggle : %dx%d, thumb : %d\n",
                 widget->toggle_width, widget->toggle_height, widget->thumb_size);
    debug_printf("  ✓ Offset toggle : %d\n", widget->local_toggle_x);
    debug_blank_line();
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void free_toggle_widget(ToggleWidget* widget) {
    if (!widget) return;
    free(widget);
}
