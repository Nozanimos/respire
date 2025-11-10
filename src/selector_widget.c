// SPDX-License-Identifier: GPL-3.0-or-later
#include "selector_widget.h"
#include "geometry.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ════════════════════════════════════════════════════════════════════════════
// CRÉATION DU WIDGET SELECTOR
// ════════════════════════════════════════════════════════════════════════════
SelectorWidget* create_selector_widget(const char* nom_affichage, int x, int y,
                                       int default_index, int arrow_size, int text_size,
                                       TTF_Font* font) {
    SelectorWidget* widget = malloc(sizeof(SelectorWidget));
    if (!widget) {
        fprintf(stderr, "❌ Erreur allocation SelectorWidget\n");
        return NULL;
    }

    // Initialisation de la base
    widget->base.x = x;
    widget->base.y = y;
    widget->base.base_x = x;
    widget->base.base_y = y;
    widget->base.width = 300;
    widget->base.height = 50;  // Plus grand pour layout vertical
    widget->base.base_width = 300;
    widget->base.base_height = 50;
    widget->base.enabled = true;
    widget->base.hovered = false;

    // Copier le nom d'affichage
    strncpy(widget->nom_affichage, nom_affichage, sizeof(widget->nom_affichage) - 1);
    widget->nom_affichage[sizeof(widget->nom_affichage) - 1] = '\0';

    // Initialisation des options
    widget->num_options = 0;
    widget->current_index = default_index;
    widget->previous_index = default_index;

    // Police
    widget->font = font;
    widget->base_text_size = text_size;
    widget->current_text_size = text_size;

    // Dimensions
    widget->base_arrow_size = arrow_size;
    widget->current_arrow_size = arrow_size;

    // Flèches
    widget->left_arrow = NULL;
    widget->right_arrow = NULL;

    // Zones
    widget->left_arrow_rect = (SDL_Rect){0, 0, 0, 0};
    widget->right_arrow_rect = (SDL_Rect){0, 0, 0, 0};
    widget->value_rect = (SDL_Rect){0, 0, 0, 0};

    // Couleurs
    widget->text_color = (SDL_Color){0, 0, 0, 255};
    widget->arrow_color = (SDL_Color){100, 100, 100, 255};
    widget->arrow_hover_color = (SDL_Color){50, 50, 50, 255};

    // État
    widget->left_arrow_hovered = false;
    widget->right_arrow_hovered = false;
    widget->value_hovered = false;

    // Animation
    widget->animation_progress = 0.0f;
    widget->animation_direction = 0;

    debug_printf("✅ SelectorWidget créé: '%s' à (%d, %d)\n", nom_affichage, x, y);

    return widget;
}

// ════════════════════════════════════════════════════════════════════════════
// AJOUTER UNE OPTION
// ════════════════════════════════════════════════════════════════════════════
bool add_selector_option(SelectorWidget* widget, const char* option_text, const char* callback_name) {
    if (!widget || !option_text || !callback_name) return false;

    if (widget->num_options >= MAX_SELECTOR_OPTIONS) {
        fprintf(stderr, "⚠️ Impossible d'ajouter l'option '%s': tableau plein\n", option_text);
        return false;
    }

    int index = widget->num_options;
    strncpy(widget->options[index].text, option_text, MAX_OPTION_TEXT_LENGTH - 1);
    widget->options[index].text[MAX_OPTION_TEXT_LENGTH - 1] = '\0';

    strncpy(widget->options[index].callback_name, callback_name, MAX_CALLBACK_NAME_LENGTH - 1);
    widget->options[index].callback_name[MAX_CALLBACK_NAME_LENGTH - 1] = '\0';

    widget->options[index].callback = NULL;

    widget->num_options++;

    debug_printf("✅ Option ajoutée: [%d] '%s' → %s\n", index, option_text, callback_name);

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// DÉFINIR LE CALLBACK D'UNE OPTION
// ════════════════════════════════════════════════════════════════════════════
void set_selector_option_callback(SelectorWidget* widget, int option_index, void (*callback)(void)) {
    if (!widget || option_index < 0 || option_index >= widget->num_options) {
        fprintf(stderr, "⚠️ Index d'option invalide: %d\n", option_index);
        return;
    }

    widget->options[option_index].callback = callback;
    debug_printf("✅ Callback défini pour option [%d] '%s'\n",
                 option_index, widget->options[option_index].text);
}

// ════════════════════════════════════════════════════════════════════════════
// CHANGER LA VALEUR SÉLECTIONNÉE (avec animation)
// ════════════════════════════════════════════════════════════════════════════
void set_selector_value(SelectorWidget* widget, int new_index) {
    if (!widget) return;

    if (new_index < 0 || new_index >= widget->num_options) {
        fprintf(stderr, "⚠️ Index invalide: %d\n", new_index);
        return;
    }

    if (new_index == widget->current_index) return;

    // Démarrer l'animation
    widget->previous_index = widget->current_index;
    widget->current_index = new_index;
    widget->animation_progress = 0.0f;
    widget->animation_direction = (new_index > widget->previous_index) ? 1 : -1;

    debug_printf("🔄 Selector '%s': option changée → [%d] '%s'\n",
                 widget->nom_affichage, new_index, widget->options[new_index].text);

    // Appeler le callback
    if (widget->options[new_index].callback) {
        widget->options[new_index].callback();
        debug_printf("✅ Callback appelé: %s\n", widget->options[new_index].callback_name);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// NAVIGUER VERS L'OPTION PRÉCÉDENTE
// ════════════════════════════════════════════════════════════════════════════
void selector_previous_option(SelectorWidget* widget) {
    if (!widget || widget->num_options == 0) return;

    int new_index = widget->current_index - 1;
    if (new_index < 0) {
        new_index = widget->num_options - 1;  // Bouclage
    }

    set_selector_value(widget, new_index);
}

// ════════════════════════════════════════════════════════════════════════════
// NAVIGUER VERS L'OPTION SUIVANTE
// ════════════════════════════════════════════════════════════════════════════
void selector_next_option(SelectorWidget* widget) {
    if (!widget || widget->num_options == 0) return;

    int new_index = widget->current_index + 1;
    if (new_index >= widget->num_options) {
        new_index = 0;  // Bouclage
    }

    set_selector_value(widget, new_index);
}

// ════════════════════════════════════════════════════════════════════════════
// MISE À JOUR DE L'ANIMATION
// ════════════════════════════════════════════════════════════════════════════
void update_selector_animation(SelectorWidget* widget, float delta_time) {
    if (!widget || widget->animation_direction == 0) return;

    // Progression de l'animation (rapide : 0.15s)
    widget->animation_progress += delta_time / 0.15f;

    if (widget->animation_progress >= 1.0f) {
        widget->animation_progress = 1.0f;
        widget->animation_direction = 0;  // Animation terminée
    }
}

// ════════════════════════════════════════════════════════════════════════════
// RENDU DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void render_selector_widget(SDL_Renderer* renderer, SelectorWidget* widget,
                            int offset_x, int offset_y) {
    if (!widget || !renderer || !widget->base.enabled) return;
    if (widget->num_options == 0) return;

    // Obtenir la police à la bonne taille
    TTF_Font* current_font = get_font_for_size(widget->current_text_size);
    if (!current_font) {
        debug_printf("❌ Police non disponible pour taille %d\n", widget->current_text_size);
        return;
    }

    // Position absolue du widget
    int widget_x = offset_x + widget->base.x;
    int widget_y = offset_y + widget->base.y;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. RENDU DU LABEL (au-dessus, aligné à gauche)
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Surface* label_surface = TTF_RenderUTF8_Blended(current_font,
                                                        widget->nom_affichage,
                                                        widget->text_color);
    if (label_surface) {
        SDL_Texture* label_texture = SDL_CreateTextureFromSurface(renderer, label_surface);
        SDL_Rect label_rect = {widget_x, widget_y, label_surface->w, label_surface->h};
        SDL_RenderCopy(renderer, label_texture, NULL, &label_rect);
        SDL_DestroyTexture(label_texture);
        SDL_FreeSurface(label_surface);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 2. FOND HOVER pour la zone de valeur
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->value_hovered) {
        SDL_Rect hover_bg = widget->value_rect;
        hover_bg.x += widget_x;
        hover_bg.y += widget_y;
        // Activation du blending
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
        SDL_RenderFillRect(renderer, &hover_bg);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. RENDU DE LA FLÈCHE GAUCHE ◀
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->left_arrow) {
        SDL_Color arrow_color = widget->left_arrow_hovered ?
                                widget->arrow_hover_color : widget->arrow_color;
        widget->left_arrow->color = arrow_color;
        draw_triangle_with_offset(renderer, widget->left_arrow, widget_x, widget_y);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 4. RENDU DU TEXTE DE L'OPTION ACTUELLE (avec animation)
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->current_index >= 0 && widget->current_index < widget->num_options) {
        const char* current_text = widget->options[widget->current_index].text;

        // Animation : décalage horizontal basé sur la progression
        float offset_factor = 0.0f;
        if (widget->animation_direction != 0) {
            // Easing out : décélération
            float t = widget->animation_progress;
            float eased = 1.0f - (1.0f - t) * (1.0f - t);
            offset_factor = (1.0f - eased) * widget->animation_direction * 20.0f;
        }

        SDL_Surface* value_surface = TTF_RenderUTF8_Blended(current_font,
                                                            current_text,
                                                            widget->text_color);
        if (value_surface) {
            SDL_Texture* value_texture = SDL_CreateTextureFromSurface(renderer, value_surface);

            // Centrer horizontalement et verticalement dans value_rect
            int value_x = widget_x + widget->value_rect.x +
                         (widget->value_rect.w - value_surface->w) / 2 + (int)offset_factor;
            int value_y = widget_y + widget->value_rect.y +
                         (widget->value_rect.h - value_surface->h) / 2;

            SDL_Rect value_render_rect = {value_x, value_y, value_surface->w, value_surface->h};

            // Alpha pour fade out pendant l'animation
            if (widget->animation_direction != 0) {
                SDL_SetTextureAlphaMod(value_texture, (Uint8)(255 * widget->animation_progress));
            }

            SDL_RenderCopy(renderer, value_texture, NULL, &value_render_rect);
            SDL_DestroyTexture(value_texture);
            SDL_FreeSurface(value_surface);
        }

        // Afficher l'option précédente en fade out
        if (widget->animation_direction != 0 && widget->previous_index >= 0) {
            const char* prev_text = widget->options[widget->previous_index].text;
            float prev_offset = -widget->animation_direction * 20.0f * widget->animation_progress;

            SDL_Surface* prev_surface = TTF_RenderUTF8_Blended(current_font, prev_text, widget->text_color);
            if (prev_surface) {
                SDL_Texture* prev_texture = SDL_CreateTextureFromSurface(renderer, prev_surface);

                int prev_x = widget_x + widget->value_rect.x +
                            (widget->value_rect.w - prev_surface->w) / 2 + (int)prev_offset;
                int prev_y = widget_y + widget->value_rect.y +
                            (widget->value_rect.h - prev_surface->h) / 2;

                SDL_Rect prev_rect = {prev_x, prev_y, prev_surface->w, prev_surface->h};
                SDL_SetTextureAlphaMod(prev_texture, (Uint8)(255 * (1.0f - widget->animation_progress)));
                SDL_RenderCopy(renderer, prev_texture, NULL, &prev_rect);
                SDL_DestroyTexture(prev_texture);
                SDL_FreeSurface(prev_surface);
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5. RENDU DE LA FLÈCHE DROITE ▶
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->right_arrow) {
        SDL_Color arrow_color = widget->right_arrow_hovered ?
                                widget->arrow_hover_color : widget->arrow_color;
        widget->right_arrow->color = arrow_color;
        draw_triangle_with_offset(renderer, widget->right_arrow, widget_x, widget_y);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════
void handle_selector_widget_events(SelectorWidget* widget, SDL_Event* event,
                                   int offset_x, int offset_y) {
    if (!widget || !event) return;

    int widget_x = offset_x + widget->base.x;
    int widget_y = offset_y + widget->base.y;

    switch (event->type) {
        case SDL_MOUSEMOTION: {
            int mouse_x = event->motion.x;
            int mouse_y = event->motion.y;

            // Vérifier survol flèche gauche
            SDL_Rect left_abs = widget->left_arrow_rect;
            left_abs.x += widget_x;
            left_abs.y += widget_y;
            widget->left_arrow_hovered = (mouse_x >= left_abs.x &&
                                         mouse_x <= left_abs.x + left_abs.w &&
                                         mouse_y >= left_abs.y &&
                                         mouse_y <= left_abs.y + left_abs.h);

            // Vérifier survol flèche droite
            SDL_Rect right_abs = widget->right_arrow_rect;
            right_abs.x += widget_x;
            right_abs.y += widget_y;
            widget->right_arrow_hovered = (mouse_x >= right_abs.x &&
                                          mouse_x <= right_abs.x + right_abs.w &&
                                          mouse_y >= right_abs.y &&
                                          mouse_y <= right_abs.y + right_abs.h);

            // Vérifier survol de la zone de valeur
            SDL_Rect value_abs = widget->value_rect;
            value_abs.x += widget_x;
            value_abs.y += widget_y;
            widget->value_hovered = (mouse_x >= value_abs.x &&
                                    mouse_x <= value_abs.x + value_abs.w &&
                                    mouse_y >= value_abs.y &&
                                    mouse_y <= value_abs.y + value_abs.h);
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            if (event->button.button != SDL_BUTTON_LEFT) break;

            int mouse_x = event->button.x;
            int mouse_y = event->button.y;

            // Clic sur flèche gauche
            SDL_Rect left_abs = widget->left_arrow_rect;
            left_abs.x += widget_x;
            left_abs.y += widget_y;
            if (mouse_x >= left_abs.x && mouse_x <= left_abs.x + left_abs.w &&
                mouse_y >= left_abs.y && mouse_y <= left_abs.y + left_abs.h) {
                selector_previous_option(widget);
                break;
            }

            // Clic sur flèche droite
            SDL_Rect right_abs = widget->right_arrow_rect;
            right_abs.x += widget_x;
            right_abs.y += widget_y;
            if (mouse_x >= right_abs.x && mouse_x <= right_abs.x + right_abs.w &&
                mouse_y >= right_abs.y && mouse_y <= right_abs.y + right_abs.h) {
                selector_next_option(widget);
                break;
            }
            break;
        }

        case SDL_MOUSEWHEEL: {
            // Molette de souris : changer l'option si survol de la zone
            if (widget->value_hovered) {
                if (event->wheel.y > 0) {
                    selector_previous_option(widget);  // Molette vers le haut
                } else if (event->wheel.y < 0) {
                    selector_next_option(widget);  // Molette vers le bas
                }
            }
            break;
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// RESCALING DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void rescale_selector_widget(SelectorWidget* widget, float panel_ratio) {
    if (!widget) return;

    // Appeler le rescaling de base
    rescale_widget_base(&widget->base, panel_ratio);

    // Recalculer les dimensions
    widget->current_arrow_size = (int)(widget->base_arrow_size * panel_ratio);
    if (widget->current_arrow_size < 6) widget->current_arrow_size = 6;

    // NE PAS rescaler la taille de police - garder la taille de base
    widget->current_text_size = widget->base_text_size;

    // Obtenir la police à la bonne taille
    TTF_Font* current_font = get_font_for_size(widget->current_text_size);
    if (!current_font) {
        debug_printf("❌ Police non disponible pour taille %d\n", widget->current_text_size);
        return;
    }

    // Layout VERTICAL : [Label]
    //                   [◀] [Texte] [▶]
    const int SPACING_ARROW = 5;  // 5px entre texte et flèches
    const int VERTICAL_GAP = 5;   // Gap entre label et sélecteur

    // Mesurer le label avec UTF8
    int label_width = 0;
    int label_height = 0;
    TTF_SizeUTF8(current_font, widget->nom_affichage, &label_width, &label_height);

    // Trouver la largeur maximale des options
    int max_option_width = 100;
    for (int i = 0; i < widget->num_options; i++) {
        int w = 0;
        TTF_SizeUTF8(current_font, widget->options[i].text, &w, NULL);
        if (w > max_option_width) max_option_width = w;
    }

    // Hauteur du sélecteur (ligne avec flèches)
    int selector_height = label_height;

    // Y du sélecteur (en dessous du label)
    int selector_y = label_height + VERTICAL_GAP;

    // Zone cliquable des flèches
    int arrow_click_area = widget->current_arrow_size * 2;

    // Flèche gauche (alignée à gauche)
    widget->left_arrow_rect.x = 0;
    widget->left_arrow_rect.y = selector_y;
    widget->left_arrow_rect.w = arrow_click_area;
    widget->left_arrow_rect.h = selector_height;

    // Zone de valeur (entre les flèches)
    int value_x = arrow_click_area + SPACING_ARROW;
    int value_width = max_option_width + 5;  // +5 pour padding
    widget->value_rect.x = value_x;
    widget->value_rect.y = selector_y;
    widget->value_rect.w = value_width;
    widget->value_rect.h = selector_height;

    // Flèche droite (après la zone de valeur)
    widget->right_arrow_rect.x = value_x + value_width + SPACING_ARROW;
    widget->right_arrow_rect.y = selector_y;
    widget->right_arrow_rect.w = arrow_click_area;
    widget->right_arrow_rect.h = selector_height;

    // Recréer les flèches
    if (widget->left_arrow) free_triangle(widget->left_arrow);
    if (widget->right_arrow) free_triangle(widget->right_arrow);

    int arrow_y_center = selector_y + selector_height / 2;
    int left_arrow_x = widget->left_arrow_rect.x + arrow_click_area / 2;
    int right_arrow_x = widget->right_arrow_rect.x + arrow_click_area / 2;

    widget->left_arrow = create_left_arrow(left_arrow_x, arrow_y_center,
                                           widget->current_arrow_size,
                                           widget->arrow_color);
    widget->right_arrow = create_right_arrow(right_arrow_x, arrow_y_center,
                                             widget->current_arrow_size,
                                             widget->arrow_color);

    // Recalculer les dimensions totales du widget
    widget->base.width = widget->right_arrow_rect.x + widget->right_arrow_rect.w;
    widget->base.height = selector_y + selector_height;

    debug_printf("🔄 Selector '%s' rescalé (vertical):\n", widget->nom_affichage);
    debug_printf("   Largeur: %d, Hauteur: %d\n", widget->base.width, widget->base.height);
    debug_printf("   Label: %dpx, Selector Y: %d\n", label_height, selector_y);
}

// ════════════════════════════════════════════════════════════════════════════
// LIBÉRATION DE LA MÉMOIRE
// ════════════════════════════════════════════════════════════════════════════
void free_selector_widget(SelectorWidget* widget) {
    if (!widget) return;

    if (widget->left_arrow) free_triangle(widget->left_arrow);
    if (widget->right_arrow) free_triangle(widget->right_arrow);

    debug_printf("🗑️ Libération SelectorWidget '%s'\n", widget->nom_affichage);
    free(widget);
}
