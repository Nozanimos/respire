#include "widget.h"
#include "geometry.h"
#include "debug.h"
#include <stdio.h>
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════════
//  CALCUL DU RECTANGLE ENGLOBANT D'UNE FLÈCHE
// ════════════════════════════════════════════════════════════════════════════
// IMPORTANT : Les triangles stockent leurs points dans vx[]/vy[] qui sont
// des coordonnées RELATIVES. Pour obtenir la vraie zone de clic, on doit
// ajouter l'offset du panneau à center_x/center_y (qui sont aussi relatifs).
SDL_Rect get_arrow_bounds(int arrow_center_x, int arrow_center_y, int base_size,
                          int offset_x, int offset_y) {
    int height = base_size;
    int base_half = height;

    return (SDL_Rect){
        arrow_center_x - base_half + offset_x,
        arrow_center_y - height + offset_y,
        base_half * 2,
        height * 2
    };
}

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION D'UN WIDGET DE CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════
// Crée un widget interactif avec flèches haut/bas pour modifier une valeur
//
// PARAMÈTRES :
//   - name : Texte affiché à gauche (ex: "Durée respiration")
//   - x, y : Position RELATIVE au conteneur parent (panneau)
//   - min_val, max_val : Limites de la valeur
//   - start_val : Valeur initiale
//   - increment : Pas d'incrémentation (ex: 1)
//   - arrow_size : Taille des flèches en pixels
//   - text_size : Taille du texte (référence, pas utilisée directement)
//   - font : Police TTF pour calculer les dimensions réelles
ConfigWidget* create_config_widget(const char* name, int x, int y, int min_val, int max_val,
                                   int start_val, int increment, int arrow_size, int text_size,
                                   TTF_Font* font) {
    ConfigWidget* widget = malloc(sizeof(ConfigWidget));
    if (!widget) {
        debug_printf("❌ Erreur allocation widget: %s\n", name);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // VALIDATION ET INITIALISATION DES VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    if (start_val < min_val) start_val = min_val;
    if (start_val > max_val) start_val = max_val;

    snprintf(widget->option_name, sizeof(widget->option_name), "%s", name);
    widget->value = start_val;
    widget->min_value = min_val;
    widget->max_value = max_val;
    widget->increment = increment;
    widget->x = x;
    widget->y = y;
    widget->arrow_size = arrow_size;
    widget->text_size = text_size;
    widget->whole_widget_hovered = false;

    // ─────────────────────────────────────────────────────────────────────────
    // COULEURS DU WIDGET
    // ─────────────────────────────────────────────────────────────────────────
    widget->bg_hover_color = (SDL_Color){0, 0, 0, 50};        // Fond noir transparent
    widget->color = (SDL_Color){0, 0, 0, 255};                // Flèches noires
    widget->hover_color = (SDL_Color){255, 255, 100, 255};    // Flèches jaunes au survol
    widget->text_color = (SDL_Color){0, 0, 0, 255};           // Texte noir

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DU LAYOUT HORIZONTAL
    // ─────────────────────────────────────────────────────────────────────────
    // Layout : [Nom] <espacement> [↑↓] <espacement> [Valeur]

    int text_width = 0, text_height = 0;
    if (font) {
        TTF_SizeUTF8(font, name, &text_width, &text_height);
    } else {
        text_width = strlen(name) * (text_size / 2);
        text_height = text_size;
    }

    int espace_apres_texte = 20;    // Espace entre le nom et les flèches
    int espace_apres_fleches = 10;  // Espace entre les flèches et la valeur

    widget->name_x = x;
    widget->arrows_x = x + text_width + espace_apres_texte;
    widget->value_x = widget->arrows_x + (2 * arrow_size) + espace_apres_fleches;
    widget->text_center_y = y + text_height / 2;
    widget->text_height = text_height;

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DES FLÈCHES (coordonnées RELATIVES)
    // ─────────────────────────────────────────────────────────────────────────
    // Les flèches sont créées avec les positions relatives au panneau
    widget->up_arrow_y = widget->text_center_y - 5;
    widget->down_arrow_y = widget->text_center_y + 5;

    widget->up_arrow = create_up_arrow(widget->arrows_x, widget->up_arrow_y, arrow_size, widget->color);
    widget->down_arrow = create_down_arrow(widget->arrows_x, widget->down_arrow_y, arrow_size, widget->color);

    widget->up_hovered = false;
    widget->down_hovered = false;
    widget->on_value_changed = NULL;

    debug_printf("✅ Widget '%s' créé - Layout: nom@%d, flèches@%d, valeur@%d\n",
                 name, widget->name_x, widget->arrows_x, widget->value_x);

    return widget;
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU D'UN WIDGET DE CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════
// Dessine le widget à l'écran en tenant compte de l'offset du panneau parent
void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget, TTF_Font* font,
                          int offset_x, int offset_y) {
    if (!widget || !renderer) return;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DES POSITIONS ABSOLUES
    // ─────────────────────────────────────────────────────────────────────────
    int absolute_name_x = widget->name_x + offset_x;
    int absolute_y = widget->y + offset_y;
    int absolute_value_x = widget->value_x + offset_x;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DE LA LARGEUR TOTALE DU WIDGET
    // ─────────────────────────────────────────────────────────────────────────
    // Pour calculer la largeur totale, on doit connaître la largeur de la valeur affichée
    char value_str[20];
    snprintf(value_str, sizeof(value_str), "%d", widget->value);
    int value_width = 0;

    // Calcul cohérent avec handle_config_widget_events
    if (widget->text_size > 0) {
        value_width = strlen(value_str) * (widget->text_size / 2);
    } else {
        value_width = strlen(value_str) * 8;
    }

    // Largeur totale = de name_x jusqu'à la fin de la valeur + marges
    int total_width = (widget->value_x + value_width) - widget->name_x;



    // ─────────────────────────────────────────────────────────────────────────
    // FOND AU SURVOL DU WIDGET COMPLET
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->whole_widget_hovered) {
        SDL_Rect bg_rect = {
            absolute_name_x - 10,                    // Marge gauche de 10px
            absolute_y - 5,                         // Marge haut de 5px
            total_width + 20,                       // Largeur totale + marges
            widget->text_height + 10                // Hauteur + marges
        };

        // Conversion de la couleur
        Uint32 bg_color =
        ((Uint32)widget->bg_hover_color.a << 24) |
        ((Uint32)widget->bg_hover_color.r << 16) |
        ((Uint32)widget->bg_hover_color.g << 8) |
        (Uint32)widget->bg_hover_color.b;

        roundedBoxColor(renderer, bg_rect.x, bg_rect.y,
                        bg_rect.x + bg_rect.w, bg_rect.y + bg_rect.h,
                        bg_rect.h/2,  // Rayon de courbure des coins
                        bg_color);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DU TEXTE DU NOM
    // ─────────────────────────────────────────────────────────────────────────
    render_text(renderer, font, widget->option_name, absolute_name_x, absolute_y,
                (widget->text_color.r << 16) | (widget->text_color.g << 8) | widget->text_color.b);

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DES FLÈCHES AVEC COULEUR DE SURVOL
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color up_color = widget->up_hovered ? widget->hover_color : widget->color;
    SDL_Color down_color = widget->down_hovered ? widget->hover_color : widget->color;

    if (widget->up_arrow) {
        widget->up_arrow->color = up_color;
        draw_triangle_with_offset(renderer, widget->up_arrow, offset_x, offset_y);
    }
    if (widget->down_arrow) {
        widget->down_arrow->color = down_color;
        draw_triangle_with_offset(renderer, widget->down_arrow, offset_x, offset_y);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DE LA VALEUR NUMÉRIQUE
    // ─────────────────────────────────────────────────────────────────────────
    //char value_str[20];
    snprintf(value_str, sizeof(value_str), "%d", widget->value);
    render_text(renderer, font, value_str, absolute_value_x, absolute_y,
                (widget->text_color.r << 16) | (widget->text_color.g << 8) | widget->text_color.b);
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y) {
    if (!widget || !event) return;

    switch (event->type) {
        // ═════════════════════════════════════════════════════════════════════
        // MOUVEMENT DE LA SOURIS (détection du survol)
        // ═════════════════════════════════════════════════════════════════════
        case SDL_MOUSEMOTION: {
            int mouse_x = event->motion.x;
            int mouse_y = event->motion.y;

            // ─────────────────────────────────────────────────────────────────
            // CALCUL PRÉCIS DE LA ZONE COMPLÈTE DU WIDGET
            // ─────────────────────────────────────────────────────────────────
            // Calculer la largeur de la valeur comme dans le rendu
            char value_str[20];
            snprintf(value_str, sizeof(value_str), "%d", widget->value);
            int value_width = 0;

            // Estimation de la largeur (similaire au calcul dans render_config_widget)
            if (widget->text_size > 0) {
                // Estimation basée sur la taille du texte
                value_width = strlen(value_str) * (widget->text_size / 2);
            } else {
                value_width = strlen(value_str) * 8; // Valeur par défaut
            }

            // Largeur totale = de name_x jusqu'à la fin de la valeur + marges
            int total_width = (widget->value_x + value_width) - widget->name_x;

            SDL_Rect widget_rect = {
                widget->name_x + offset_x - 10,      // Marge gauche
                widget->y + offset_y - 5,           // Marge haut
                total_width + 20,                   // Largeur totale + marges
                widget->text_height + 10            // Hauteur + marges
            };

            bool was_hovered = widget->whole_widget_hovered;
            widget->whole_widget_hovered = is_point_in_rect(mouse_x, mouse_y, widget_rect);

            // ✅ DEBUG : Afficher seulement quand l'état change
            if (was_hovered != widget->whole_widget_hovered) {
                debug_printf("🖱️ Widget '%s' - Survol: %s\n",
                             widget->option_name,
                             widget->whole_widget_hovered ? "ACTIF" : "inactif");
            }
            // ─────────────────────────────────────────────────────────────────
            // Détection du survol des flèches (utiliser les positions stockées)
            // ─────────────────────────────────────────────────────────────────
            SDL_Rect up_bounds = get_arrow_bounds(widget->arrows_x, widget->up_arrow_y,
                                                   widget->arrow_size, offset_x, offset_y);
            widget->up_hovered = is_point_in_rect(mouse_x, mouse_y, up_bounds);

            SDL_Rect down_bounds = get_arrow_bounds(widget->arrows_x, widget->down_arrow_y,
                                                     widget->arrow_size, offset_x, offset_y);
            widget->down_hovered = is_point_in_rect(mouse_x, mouse_y, down_bounds);

            break;
        }

        // ═════════════════════════════════════════════════════════════════════
        // CLIC SOURIS
        // ═════════════════════════════════════════════════════════════════════
        case SDL_MOUSEBUTTONDOWN: {
            if (event->button.button != SDL_BUTTON_LEFT) break;

            int mouse_x = event->button.x;
            int mouse_y = event->button.y;

            // ─────────────────────────────────────────────────────────────────
            // Clic sur flèche haut
            // ─────────────────────────────────────────────────────────────────
            SDL_Rect up_bounds = get_arrow_bounds(widget->arrows_x, widget->up_arrow_y,
                                                   widget->arrow_size, offset_x, offset_y);
            if (is_point_in_rect(mouse_x, mouse_y, up_bounds)) {
                int new_value = widget->value + widget->increment;
                if (new_value <= widget->max_value) {
                    widget->value = new_value;
                    debug_printf("⬆️ Widget '%s': %d -> %d\n",
                                widget->option_name, widget->value - widget->increment, widget->value);

                    if (widget->on_value_changed) {
                        widget->on_value_changed(widget->value);
                    }
                }
            }

            // ─────────────────────────────────────────────────────────────────
            // Clic sur flèche bas
            // ─────────────────────────────────────────────────────────────────
            SDL_Rect down_bounds = get_arrow_bounds(widget->arrows_x, widget->down_arrow_y,
                                                     widget->arrow_size, offset_x, offset_y);
            if (is_point_in_rect(mouse_x, mouse_y, down_bounds)) {
                int new_value = widget->value - widget->increment;
                if (new_value >= widget->min_value) {
                    widget->value = new_value;
                    debug_printf("⬇️ Widget '%s': %d -> %d\n",
                                widget->option_name, widget->value + widget->increment, widget->value);

                    if (widget->on_value_changed) {
                        widget->on_value_changed(widget->value);
                    }
                }
            }
            break;
        }

        // ═════════════════════════════════════════════════════════════════════
        // MOLETTE DE LA SOURIS
        // ═════════════════════════════════════════════════════════════════════
        case SDL_MOUSEWHEEL: {
            if (!widget->whole_widget_hovered) break;

            if (event->wheel.y > 0) {  // Molette vers le haut
                int new_value = widget->value + widget->increment;
                if (new_value <= widget->max_value) {
                    widget->value = new_value;
                    debug_printf("🖱️ Molette UP '%s': %d\n", widget->option_name, widget->value);
                    if (widget->on_value_changed) {
                        widget->on_value_changed(widget->value);
                    }
                }
            } else if (event->wheel.y < 0) {  // Molette vers le bas
                int new_value = widget->value - widget->increment;
                if (new_value >= widget->min_value) {
                    widget->value = new_value;
                    debug_printf("🖱️ Molette DOWN '%s': %d\n", widget->option_name, widget->value);
                    if (widget->on_value_changed) {
                        widget->on_value_changed(widget->value);
                    }
                }
            }
            break;
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACK DE CHANGEMENT DE VALEUR
// ════════════════════════════════════════════════════════════════════════════
void set_widget_value_changed_callback(ConfigWidget* widget, void (*callback)(int)) {
    if (widget) {
        widget->on_value_changed = callback;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION D'UN WIDGET
// ════════════════════════════════════════════════════════════════════════════
void free_config_widget(ConfigWidget* widget) {
    if (!widget) return;

    // Sauvegarder le nom avant de libérer
    char widget_name[50];
    snprintf(widget_name, sizeof(widget_name), "%s", widget->option_name);

    if (widget->up_arrow) {
        free_triangle(widget->up_arrow);
    }
    if (widget->down_arrow) {
        free_triangle(widget->down_arrow);
    }

    free(widget);
    debug_printf("🗑️ Widget '%s' libéré\n", widget_name);
}
