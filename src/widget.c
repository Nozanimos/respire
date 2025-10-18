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
                                   int start_val, int increment, int arrow_size, int text_size) {
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
    widget->color = (SDL_Color){200, 200, 200, 255};       // Gris clair
    widget->hover_color = (SDL_Color){255, 255, 100, 255}; // Jaune
    widget->text_color = (SDL_Color){255, 255, 255, 255};  // Blanc

    // Création des flèches - positionnées à droite du texte
    int arrow_spacing = 15; // Espace entre les flèches
    int text_width = 150;   // Largeur estimée du texte

    widget->up_arrow = create_up_arrow(x + text_width, y + text_size/2, arrow_size, widget->color);
    widget->down_arrow = create_down_arrow(x + text_width + arrow_size*2 + arrow_spacing,
                                           y + text_size/2, arrow_size, widget->color);

    widget->up_hovered = false;
    widget->down_hovered = false;
    widget->on_value_changed = NULL;

    debug_printf("✅ Widget créé: %s (valeur: %d, min: %d, max: %d)\n",
                 name, start_val, min_val, max_val);
    return widget;
}

void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget, TTF_Font* font) {
    if (!widget || !renderer) return;

    // 1. Afficher le nom de l'option
    render_text(renderer, font, widget->option_name, widget->x, widget->y,
                (widget->text_color.r << 16) | (widget->text_color.g << 8) | widget->text_color.b);

    // 2. Afficher la valeur actuelle
    char value_str[20];
    snprintf(value_str, sizeof(value_str), "%d", widget->value);

    // Position de la valeur (au milieu entre le texte et les flèches)
    int value_x = widget->x + 120;
    render_text(renderer, font, value_str, value_x, widget->y,
                (widget->text_color.r << 16) | (widget->text_color.g << 8) | widget->text_color.b);

    // 3. Afficher les flèches avec la couleur appropriée
    SDL_Color up_color = widget->up_hovered ? widget->hover_color : widget->color;
    SDL_Color down_color = widget->down_hovered ? widget->hover_color : widget->color;

    // Mettre à jour les couleurs des triangles
    if (widget->up_arrow) {
        widget->up_arrow->color = up_color;
        draw_triangle(renderer, widget->up_arrow);
    }
    if (widget->down_arrow) {
        widget->down_arrow->color = down_color;
        draw_triangle(renderer, widget->down_arrow);
    }

    // Debug: afficher les zones de collision en mode debug
    #ifdef DEBUG_MODE
    if (widget->up_arrow) {
        SDL_Rect up_bounds = get_arrow_bounds(widget->up_arrow, widget->arrow_size);
        rectangleColor(renderer, up_bounds.x, up_bounds.y,
                        up_bounds.x + up_bounds.w, up_bounds.y + up_bounds.h, 0x00FF00FF);
    }
    if (widget->down_arrow) {
        SDL_Rect down_bounds = get_arrow_bounds(widget->down_arrow, widget->arrow_size);
        rectangleColor(renderer, down_bounds.x, down_bounds.y,
                        down_bounds.x + down_bounds.w, down_bounds.y + down_bounds.h, 0x00FF00FF);
    }
    #endif
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
