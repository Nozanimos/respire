#include "widget.h"
#include "geometry.h"  // Pour les triangles
#include "debug.h"
#include <stdio.h>
//#include <string.h>

// Fonction pour obtenir le rectangle de délimitation d'une flèche
SDL_Rect get_arrow_bounds(Triangle* arrow, int base_size) {
    if (!arrow) return (SDL_Rect){0,0,0,0};

    // Pour un triangle isocèle avec base = 2*height, on peut approximer le bounding box
    int height = base_size;
    int base_half = height; // car base = 2*height

    return (SDL_Rect){
        arrow->center_x - base_half,
        arrow->center_y - height,
        base_half * 2,  // largeur = base
        height * 2      // hauteur totale
    };
}

ConfigWidget* create_config_widget(const char* name, int x, int y, int min_val, int max_val,
                                   int start_val, int increment, int arrow_size, int text_size,
                                   TTF_Font* font) {
    ConfigWidget* widget = malloc(sizeof(ConfigWidget));
    if (!widget) {
        debug_printf("❌ Erreur allocation widget: %s\n", name);
        return NULL;
    }

    // Validation des paramètres
    if (start_val < min_val) start_val = min_val;
    if (start_val > max_val) start_val = max_val;

    // Initialisation des valeurs
    snprintf(widget->option_name, sizeof(widget->option_name), "%s", name);
    widget->value = start_val;
    widget->min_value = min_val;
    widget->max_value = max_val;
    widget->increment = increment;
    widget->x = x;
    widget->y = y;
    widget->arrow_size = arrow_size;
    widget->text_size = text_size;

    // Couleurs par défaut
    widget->color = (SDL_Color){0,0,0, 255};
    widget->hover_color = (SDL_Color){255, 255, 100, 255};
    widget->text_color = (SDL_Color){0,0,0, 255};

    // ✅ CALCUL EXACT DES DIMENSIONS DU TEXTE
    int text_width = 0, text_height = 0;
    if (font) {
        TTF_SizeText(font, name, &text_width, &text_height);
    } else {
        // Fallback si police non disponible
        text_width = strlen(name) * (text_size / 2);
        text_height = text_size;
    }

    // ✅ CALCUL DES POSITIONS AVEC ESPACEMENT OPTIMAL
    int espace_apres_texte = 15;  // Espace entre texte et flèches
    int espace_apres_fleches = 10; // Espace entre flèches et valeur

    widget->name_x = x;
    widget->arrows_x = x + text_width + espace_apres_texte;
    widget->value_x = widget->arrows_x + (2 * arrow_size) + espace_apres_fleches;
    widget->text_center_y = y + text_height / 2;

    // Création des flèches
    int up_arrow_y = widget->text_center_y - 5;
    int down_arrow_y = widget->text_center_y + 5;

    widget->up_arrow = create_up_arrow(widget->arrows_x, up_arrow_y, arrow_size, widget->color);
    widget->down_arrow = create_down_arrow(widget->arrows_x, down_arrow_y, arrow_size, widget->color);

    widget->up_hovered = false;
    widget->down_hovered = false;
    widget->on_value_changed = NULL;

    debug_printf("✅ Widget créé: %s - texte: %dpx, flèches: %d, valeur: %d\n",
                 name, text_width, widget->arrows_x, widget->value_x);
    return widget;
}

void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget, TTF_Font* font) {
    if (!widget || !renderer) return;

    // ✅ UTILISER LES POSITIONS PRÉCALCULÉES
    int text_height = TTF_FontHeight(font);
    int text_center_y = widget->y + text_height / 2;

    // 1. Afficher le nom de l'option
    render_text(renderer, font, widget->option_name, widget->name_x, widget->y,
                (widget->text_color.r << 16) | (widget->text_color.g << 8) | widget->text_color.b);

    // 2. Afficher les flèches
    SDL_Color up_color = widget->up_hovered ? widget->hover_color : widget->color;
    SDL_Color down_color = widget->down_hovered ? widget->hover_color : widget->color;

    // Mettre à jour les positions des triangles (au cas où)
    if (widget->up_arrow) {
        widget->up_arrow->center_x = widget->arrows_x;
        widget->up_arrow->center_y = text_center_y - 5;
        widget->up_arrow->color = up_color;
        draw_triangle(renderer, widget->up_arrow);
    }
    if (widget->down_arrow) {
        widget->down_arrow->center_x = widget->arrows_x;
        widget->down_arrow->center_y = text_center_y + 5;
        widget->down_arrow->color = down_color;
        draw_triangle(renderer, widget->down_arrow);
    }

    // 3. Afficher la valeur
    char value_str[20];
    snprintf(value_str, sizeof(value_str), "%d", widget->value);
    render_text(renderer, font, value_str, widget->value_x, widget->y,
                (widget->text_color.r << 16) | (widget->text_color.g << 8) | widget->text_color.b);
}

void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event) {
    if (!widget || !event) return;

    switch (event->type) {
        case SDL_MOUSEMOTION: {
            int mouse_x = event->motion.x;
            int mouse_y = event->motion.y;

            // Vérifier le survol des flèches
            if (widget->up_arrow) {
                SDL_Rect up_bounds = get_arrow_bounds(widget->up_arrow, widget->arrow_size);
                widget->up_hovered = is_point_in_rect(mouse_x, mouse_y, up_bounds);
            }

            if (widget->down_arrow) {
                SDL_Rect down_bounds = get_arrow_bounds(widget->down_arrow, widget->arrow_size);
                widget->down_hovered = is_point_in_rect(mouse_x, mouse_y, down_bounds);
            }
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            if (event->button.button != SDL_BUTTON_LEFT) break;

            int mouse_x = event->button.x;
            int mouse_y = event->button.y;

            // Clic sur flèche haut
            if (widget->up_arrow) {
                SDL_Rect up_bounds = get_arrow_bounds(widget->up_arrow, widget->arrow_size);
                if (is_point_in_rect(mouse_x, mouse_y, up_bounds)) {
                    int new_value = widget->value + widget->increment;
                    if (new_value <= widget->max_value) {
                        widget->value = new_value;
                        debug_printf("⬆️  Widget %s: %d -> %d\n",
                                    widget->option_name, widget->value - widget->increment, widget->value);

                        // Appeler le callback si défini
                        if (widget->on_value_changed) {
                            widget->on_value_changed(widget->value);
                        }
                    }
                }
            }

            // Clic sur flèche bas
            if (widget->down_arrow) {
                SDL_Rect down_bounds = get_arrow_bounds(widget->down_arrow, widget->arrow_size);
                if (is_point_in_rect(mouse_x, mouse_y, down_bounds)) {
                    int new_value = widget->value - widget->increment;
                    if (new_value >= widget->min_value) {
                        widget->value = new_value;
                        debug_printf("⬇️  Widget %s: %d -> %d\n",
                                    widget->option_name, widget->value + widget->increment, widget->value);

                        // Appeler le callback si défini
                        if (widget->on_value_changed) {
                            widget->on_value_changed(widget->value);
                        }
                    }
                }
            }
            break;
        }

        case SDL_MOUSEWHEEL: {
            // Support de la molette de souris (bonus!)
            if (event->wheel.y > 0) { // Molette vers le haut
                int new_value = widget->value + widget->increment;
                if (new_value <= widget->max_value) {
                    widget->value = new_value;
                    if (widget->on_value_changed) {
                        widget->on_value_changed(widget->value);
                    }
                }
            } else if (event->wheel.y < 0) { // Molette vers le bas
                int new_value = widget->value - widget->increment;
                if (new_value >= widget->min_value) {
                    widget->value = new_value;
                    if (widget->on_value_changed) {
                        widget->on_value_changed(widget->value);
                    }
                }
            }
            break;
        }
    }
}

void set_widget_value_changed_callback(ConfigWidget* widget, void (*callback)(int)) {
    if (widget) {
        widget->on_value_changed = callback;
    }
}

void free_config_widget(ConfigWidget* widget) {
    if (!widget) return;

    if (widget->up_arrow) {
        free_triangle(widget->up_arrow);
    }
    if (widget->down_arrow) {
        free_triangle(widget->down_arrow);
    }

    free(widget);
    debug_printf("🗑️  Widget libéré\n");
}
