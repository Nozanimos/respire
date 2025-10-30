#include "widget_list.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION D'UNE LISTE DE WIDGETS VIDE
// ════════════════════════════════════════════════════════════════════════════
// Alloue une nouvelle liste vide prête à recevoir des widgets
WidgetList* create_widget_list(void) {
    WidgetList* list = malloc(sizeof(WidgetList));
    if (!list) {
        debug_printf("❌ Erreur allocation liste de widgets\n");
        return NULL;
    }

    list->first = NULL;
    list->last = NULL;
    list->count = 0;

    debug_printf("✅ Liste de widgets créée\n");
    return list;
}

// ════════════════════════════════════════════════════════════════════════════
//  VÉRIFICATION SI LA LISTE EST VIDE
// ════════════════════════════════════════════════════════════════════════════
bool is_widget_list_empty(WidgetList* list) {
    return (list == NULL || list->first == NULL);
}

// ════════════════════════════════════════════════════════════════════════════
//  COMPTEUR DE WIDGETS DANS LA LISTE
// ════════════════════════════════════════════════════════════════════════════
int widget_list_count(WidgetList* list) {
    if (is_widget_list_empty(list)) return 0;
    return list->count;
}

// ════════════════════════════════════════════════════════════════════════════
//  AJOUT D'UN WIDGET INCREMENT (avec flèches ↑↓)
// ════════════════════════════════════════════════════════════════════════════
// Crée un widget numérique et l'ajoute à la fin de la liste
//
// PARAMÈTRES :
//   - list : La liste où ajouter le widget
//   - id : Identifiant unique (ex: "breath_duration")
//   - display_name : Nom affiché à l'écran (ex: "Durée respiration")
//   - x, y : Position RELATIVE au conteneur parent
//   - min_val, max_val : Limites de la valeur
//   - start_val : Valeur initiale
//   - increment : Pas d'incrémentation
//   - arrow_size, text_size : Dimensions visuelles
//   - font : Police TTF pour le rendu
//   - callback : Fonction appelée quand la valeur change
bool add_increment_widget(WidgetList* list,
                         const char* id,
                         const char* display_name,
                         int x, int y,
                         int min_val, int max_val, int start_val, int increment,
                         int arrow_size, int text_size,
                         TTF_Font* font,
                         void (*callback)(int)) {
    if (!list || !id || !display_name) {
        debug_printf("❌ Paramètres invalides pour add_increment_widget\n");
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DU NŒUD
    // ─────────────────────────────────────────────────────────────────────────
    WidgetNode* node = malloc(sizeof(WidgetNode));
    if (!node) {
        debug_printf("❌ Erreur allocation nœud widget\n");
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CONFIGURATION DU NŒUD
    // ─────────────────────────────────────────────────────────────────────────
    node->type = WIDGET_TYPE_INCREMENT;
    node->id = strdup(id);              // Copie de la chaîne
    node->display_name = strdup(display_name);

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DU WIDGET CONCRET
    // ─────────────────────────────────────────────────────────────────────────
    node->widget.increment_widget = create_config_widget(
        display_name, x, y,
        min_val, max_val, start_val, increment,
        arrow_size, text_size, font
    );

    if (!node->widget.increment_widget) {
        debug_printf("❌ Échec création ConfigWidget '%s'\n", id);
        free((void*)node->id);
        free((void*)node->display_name);
        free(node);
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // ASSIGNATION DU CALLBACK
    // ─────────────────────────────────────────────────────────────────────────
    node->on_int_value_changed = callback;
    node->on_bool_value_changed = NULL;
    node->on_float_value_changed = NULL;

    // Attacher le callback au widget concret aussi
    if (callback) {
        set_config_value_changed_callback(node->widget.increment_widget, callback);     // ✅ NOUVEAU : Appeler le callback pour synchroniser la valeur initiale
        callback(start_val);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // AJOUT À LA LISTE (en fin)
    // ─────────────────────────────────────────────────────────────────────────
    node->next = NULL;
    node->prev = list->last;

    if (list->last) {
        list->last->next = node;
    } else {
        list->first = node;  // Premier élément
    }
    list->last = node;
    list->count++;

    debug_printf("✅ Widget INCREMENT '%s' (%s) ajouté à la liste (total: %d)\n",
                 id, display_name, list->count);

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  AJOUT D'UN WIDGET TOGGLE (interrupteur ON/OFF)
// ════════════════════════════════════════════════════════════════════════════
// Crée un widget toggle et l'ajoute à la fin de la liste
//
// PARAMÈTRES :
//   - list : La liste où ajouter le widget
//   - id : Identifiant unique (ex: "alternate_cycles")
//   - display_name : Nom affiché (ex: "Cycles alternés")
//   - x, y : Position RELATIVE au conteneur parent
//   - start_state : État initial (true = ON, false = OFF)
//   - toggle_width, toggle_height : Dimensions du bouton
//   - thumb_size : Diamètre du curseur circulaire
//   - text_size : Taille de référence du texte
//   - font : Police TTF
//   - callback : Fonction appelée quand l'état change
bool add_toggle_widget(WidgetList* list,
                      const char* id,
                      const char* display_name,
                      int x, int y,
                      bool start_state,
                      int toggle_width, int toggle_height, int thumb_size,
                      int text_size,
                      void (*callback)(bool)) {
    if (!list || !id || !display_name) {
        debug_printf("❌ Paramètres invalides pour add_toggle_widget\n");
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DU NŒUD
    // ─────────────────────────────────────────────────────────────────────────
    WidgetNode* node = malloc(sizeof(WidgetNode));
    if (!node) {
        debug_printf("❌ Erreur allocation nœud widget\n");
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CONFIGURATION DU NŒUD
    // ─────────────────────────────────────────────────────────────────────────
    node->type = WIDGET_TYPE_TOGGLE;
    node->id = strdup(id);
    node->display_name = strdup(display_name);

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DU WIDGET CONCRET
    // ─────────────────────────────────────────────────────────────────────────
    node->widget.toggle_widget = create_toggle_widget(
        display_name, x, y,
        start_state,
        toggle_width, toggle_height, thumb_size,
        text_size
    );

    if (!node->widget.toggle_widget) {
        debug_printf("❌ Échec création ToggleWidget '%s'\n", id);
        free((void*)node->id);
        free((void*)node->display_name);
        free(node);
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // ASSIGNATION DU CALLBACK
    // ─────────────────────────────────────────────────────────────────────────
    node->on_int_value_changed = NULL;
    node->on_bool_value_changed = callback;
    node->on_float_value_changed = NULL;

    // Attacher le callback au widget concret aussi
    if (callback) {
        set_toggle_value_changed_callback(node->widget.toggle_widget, callback);
        // ✅ NOUVEAU : Appeler le callback pour synchroniser l'état initial
        callback(start_state);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // AJOUT À LA LISTE (en fin)
    // ─────────────────────────────────────────────────────────────────────────
    node->next = NULL;
    node->prev = list->last;

    if (list->last) {
        list->last->next = node;
    } else {
        list->first = node;
    }
    list->last = node;
    list->count++;

    debug_printf("✅ Widget TOGGLE '%s' (%s) ajouté à la liste (total: %d)\n",
                 id, display_name, list->count);

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DE TOUS LES WIDGETS (FACTORISATION ✨)
// ════════════════════════════════════════════════════════════════════════════
// Parcourt toute la liste et appelle la fonction de rendu appropriée
// selon le type de chaque widget
//
// PARAMÈTRES :
//   - renderer : Le renderer SDL
//   - list : La liste de widgets à afficher
//   - offset_x, offset_y : Offset du conteneur parent (panneau)
void render_all_widgets(SDL_Renderer* renderer, WidgetList* list,
                       int offset_x, int offset_y) {
    if (!renderer || is_widget_list_empty(list)) return;

    WidgetNode* node = list->first;
    while (node) {
        // ─────────────────────────────────────────────────────────────────────
        // SWITCH sur le type de widget
        // ─────────────────────────────────────────────────────────────────────
        switch (node->type) {
            case WIDGET_TYPE_INCREMENT:
                if (node->widget.increment_widget) {
                    render_config_widget(renderer, node->widget.increment_widget,
                                       offset_x, offset_y);
                }
                break;

            case WIDGET_TYPE_TOGGLE:
                if (node->widget.toggle_widget) {
                    render_toggle_widget(renderer, node->widget.toggle_widget,
                                       offset_x, offset_y);
                }
                break;

            case WIDGET_TYPE_LABEL:
                if (node->widget.label_widget) {
                    render_label_widget(renderer, node->widget.label_widget,
                                       offset_x, offset_y);
                }
                break;

            case WIDGET_TYPE_SEPARATOR:
                if (node->widget.separator_widget) {
                    // Pour le separator on a besoin de la largeur du panneau
                    // On va utiliser une valeur par défaut de 500px pour l'instant
                    // TODO: Passer panel_width en paramètre
                    render_separator_widget(renderer, node->widget.separator_widget,
                                          offset_x, offset_y, 500);
                }
                break;

            case WIDGET_TYPE_PREVIEW:
                if (node->widget.preview_widget) {
                    render_preview_widget(renderer, node->widget.preview_widget,
                                        offset_x, offset_y);
                }
                break;

            case WIDGET_TYPE_BUTTON:
                if (node->widget.button_widget) {
                    render_button_widget(renderer, node->widget.button_widget,
                                       offset_x, offset_y);
                }
                break;

            case WIDGET_TYPE_SLIDER:
                // TODO: À implémenter plus tard
                break;

            case WIDGET_TYPE_SELECTOR:
                // TODO: À implémenter plus tard
                break;

            default:
                debug_printf("❌ Type de widget inconnu: %d\n", node->type);
                break;
        }

        node = node->next;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS POUR TOUS LES WIDGETS (FACTORISATION ✨)
// ════════════════════════════════════════════════════════════════════════════
// Parcourt toute la liste et transmet l'événement à chaque widget
// selon son type
//
// PARAMÈTRES :
//   - list : La liste de widgets
//   - event : L'événement SDL à traiter
//   - offset_x, offset_y : Offset du conteneur parent
void handle_widget_list_events(WidgetList* list, SDL_Event* event,
                               int offset_x, int offset_y) {
    if (is_widget_list_empty(list) || !event) return;

    WidgetNode* node = list->first;
    while (node) {
        // ─────────────────────────────────────────────────────────────────────
        // SWITCH sur le type de widget
        // ─────────────────────────────────────────────────────────────────────
        switch (node->type) {
            case WIDGET_TYPE_INCREMENT:
                if (node->widget.increment_widget) {
                    handle_config_widget_events(node->widget.increment_widget,
                                              event, offset_x, offset_y);
                }
                break;

            case WIDGET_TYPE_TOGGLE:
                if (node->widget.toggle_widget) {
                    handle_toggle_widget_events(node->widget.toggle_widget,
                                              event, offset_x, offset_y);
                }
                break;

            case WIDGET_TYPE_BUTTON:
                if (node->widget.button_widget) {
                    handle_button_widget_events(node->widget.button_widget,
                                              event, offset_x, offset_y);
                }
                break;

            // Les autres widgets ne gèrent pas d'événements
            case WIDGET_TYPE_LABEL:
            case WIDGET_TYPE_SEPARATOR:
            case WIDGET_TYPE_PREVIEW:
            case WIDGET_TYPE_SLIDER:
            case WIDGET_TYPE_SELECTOR:
                break;

            default:
                break;
        }

        node = node->next;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DES ANIMATIONS DE TOUS LES WIDGETS
// ════════════════════════════════════════════════════════════════════════════
// Certains widgets ont des animations (comme le toggle). Cette fonction
// parcourt la liste et met à jour toutes les animations en cours.
//
// PARAMÈTRES :
//   - list : La liste de widgets
//   - delta_time : Temps écoulé depuis la dernière frame (en secondes)
void update_widget_list_animations(WidgetList* list, float delta_time) {
    if (is_widget_list_empty(list)) return;

    WidgetNode* node = list->first;
    while (node) {
        switch (node->type) {
            case WIDGET_TYPE_TOGGLE:
                // Les toggles ont une animation de glissement
                if (node->widget.toggle_widget) {
                    update_toggle_widget(node->widget.toggle_widget, delta_time);
                }
                break;

            case WIDGET_TYPE_PREVIEW:
                // Le preview a une animation d'hexagones
                if (node->widget.preview_widget) {
                    update_preview_widget(node->widget.preview_widget, delta_time);
                }
                break;

            // Les autres widgets n'ont pas d'animation
            default:
                break;
        }

        node = node->next;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  RECHERCHE D'UN WIDGET PAR SON ID
// ════════════════════════════════════════════════════════════════════════════
// Parcourt la liste pour trouver un widget avec l'ID donné
//
// RETOURNE :
//   - Le nœud trouvé, ou NULL si non trouvé
WidgetNode* find_widget_by_id(WidgetList* list, const char* id) {
    if (is_widget_list_empty(list) || !id) return NULL;

    WidgetNode* node = list->first;
    while (node) {
        if (strcmp(node->id, id) == 0) {
            return node;  // Trouvé !
        }
        node = node->next;
    }

    debug_printf("⚠️ Widget '%s' non trouvé dans la liste\n", id);
    return NULL;
}

// ════════════════════════════════════════════════════════════════════════════
//  RÉCUPÉRATION DE LA VALEUR INT D'UN WIDGET
// ════════════════════════════════════════════════════════════════════════════
// Récupère la valeur actuelle d'un widget INCREMENT
//
// RETOURNE :
//   - true si succès (valeur stockée dans out_value)
//   - false si échec (widget non trouvé ou mauvais type)
bool get_widget_int_value(WidgetList* list, const char* id, int* out_value) {
    if (!out_value) return false;

    WidgetNode* node = find_widget_by_id(list, id);
    if (!node) return false;

    if (node->type != WIDGET_TYPE_INCREMENT) {
        debug_printf("❌ Widget '%s' n'est pas de type INCREMENT\n", id);
        return false;
    }

    if (!node->widget.increment_widget) return false;

    *out_value = node->widget.increment_widget->value;
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  RÉCUPÉRATION DE LA VALEUR BOOL D'UN WIDGET
// ════════════════════════════════════════════════════════════════════════════
// Récupère l'état actuel d'un widget TOGGLE
//
// RETOURNE :
//   - true si succès (état stocké dans out_value)
//   - false si échec
bool get_widget_bool_value(WidgetList* list, const char* id, bool* out_value) {
    if (!out_value) return false;

    WidgetNode* node = find_widget_by_id(list, id);
    if (!node) return false;

    if (node->type != WIDGET_TYPE_TOGGLE) {
        debug_printf("❌ Widget '%s' n'est pas de type TOGGLE\n", id);
        return false;
    }

    if (!node->widget.toggle_widget) return false;

    *out_value = node->widget.toggle_widget->value;
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  MODIFICATION DE LA VALEUR INT D'UN WIDGET
// ════════════════════════════════════════════════════════════════════════════
// Change la valeur d'un widget INCREMENT par programmation
// (sans interaction utilisateur)
//
// RETOURNE :
//   - true si succès
//   - false si échec
bool set_widget_int_value(WidgetList* list, const char* id, int new_value) {
    WidgetNode* node = find_widget_by_id(list, id);
    if (!node) return false;

    if (node->type != WIDGET_TYPE_INCREMENT) {
        debug_printf("❌ Widget '%s' n'est pas de type INCREMENT\n", id);
        return false;
    }

    if (!node->widget.increment_widget) return false;

    ConfigWidget* widget = node->widget.increment_widget;

    // Vérifier les limites
    if (new_value < widget->min_value || new_value > widget->max_value) {
        debug_printf("⚠️ Valeur %d hors limites pour '%s' [%d, %d]\n",
                     new_value, id, widget->min_value, widget->max_value);
        return false;
    }

    widget->value = new_value;
    debug_printf("🔧 Widget '%s' mis à jour: %d\n", id, new_value);

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  MODIFICATION DE LA VALEUR BOOL D'UN WIDGET
// ════════════════════════════════════════════════════════════════════════════
// Change l'état d'un widget TOGGLE par programmation
//
// RETOURNE :
//   - true si succès
//   - false si échec
bool set_widget_bool_value(WidgetList* list, const char* id, bool new_value) {
    WidgetNode* node = find_widget_by_id(list, id);
    if (!node) return false;

    if (node->type != WIDGET_TYPE_TOGGLE) {
        debug_printf("❌ Widget '%s' n'est pas de type TOGGLE\n", id);
        return false;
    }

    if (!node->widget.toggle_widget) return false;

    node->widget.toggle_widget->value = new_value;
    node->widget.toggle_widget->animation_progress = new_value ? 1.0f : 0.0f;
    debug_printf("🔧 Widget '%s' mis à jour: %s\n", id, new_value ? "ON" : "OFF");

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  AFFICHAGE DEBUG DE LA LISTE
// ════════════════════════════════════════════════════════════════════════════
//  AJOUT D'UN WIDGET LABEL (texte/titre)
// ════════════════════════════════════════════════════════════════════════════
bool add_label_widget(WidgetList* list,
                      const char* id,
                      const char* display_name,
                      int x, int y,
                      int text_size,
                      SDL_Color color,
                      bool underlined) {
    if (!list || !id || !display_name) return false;

    WidgetNode* node = malloc(sizeof(WidgetNode));
    if (!node) return false;

    node->type = WIDGET_TYPE_LABEL;
    node->id = strdup(id);
    node->display_name = strdup(display_name);
    node->widget.label_widget = create_label_widget(display_name, x, y, text_size, color, underlined);

    if (!node->widget.label_widget) {
        free((void*)node->id);
        free((void*)node->display_name);
        free(node);
        return false;
    }

    node->on_int_value_changed = NULL;
    node->on_bool_value_changed = NULL;
    node->on_float_value_changed = NULL;
    node->on_void_callback = NULL;
    node->next = NULL;
    node->prev = list->last;

    if (list->last) list->last->next = node;
    else list->first = node;
    list->last = node;
    list->count++;

    debug_printf("✅ Widget LABEL '%s' ajouté (total: %d)\n", id, list->count);
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  AJOUT D'UN WIDGET SEPARATOR (barre de séparation)
// ════════════════════════════════════════════════════════════════════════════
bool add_separator_widget(WidgetList* list, const char* id, int y,
                          int start_margin, int end_margin, int thickness, SDL_Color color) {
    if (!list || !id) return false;

    WidgetNode* node = malloc(sizeof(WidgetNode));
    if (!node) return false;

    node->type = WIDGET_TYPE_SEPARATOR;
    node->id = strdup(id);
    node->display_name = strdup("separator");
    node->widget.separator_widget = create_separator_widget(y, start_margin, end_margin, thickness, color);

    if (!node->widget.separator_widget) {
        free((void*)node->id);
        free((void*)node->display_name);
        free(node);
        return false;
    }

    node->on_int_value_changed = NULL;
    node->on_bool_value_changed = NULL;
    node->on_float_value_changed = NULL;
    node->on_void_callback = NULL;
    node->next = NULL;
    node->prev = list->last;

    if (list->last) list->last->next = node;
    else list->first = node;
    list->last = node;
    list->count++;

    debug_printf("✅ Widget SEPARATOR '%s' ajouté (total: %d)\n", id, list->count);
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  AJOUT D'UN WIDGET PREVIEW (zone d'animation)
// ════════════════════════════════════════════════════════════════════════════
bool add_preview_widget(WidgetList* list, const char* id, int x, int y,
                        int frame_size, float size_ratio, float breath_duration) {
    if (!list || !id) return false;

    WidgetNode* node = malloc(sizeof(WidgetNode));
    if (!node) return false;

    node->type = WIDGET_TYPE_PREVIEW;
    node->id = strdup(id);
    node->display_name = strdup("preview");
    node->widget.preview_widget = create_preview_widget(x, y, frame_size, size_ratio, breath_duration);

    if (!node->widget.preview_widget) {
        free((void*)node->id);
        free((void*)node->display_name);
        free(node);
        return false;
    }

    node->on_int_value_changed = NULL;
    node->on_bool_value_changed = NULL;
    node->on_float_value_changed = NULL;
    node->on_void_callback = NULL;
    node->next = NULL;
    node->prev = list->last;

    if (list->last) list->last->next = node;
    else list->first = node;
    list->last = node;
    list->count++;

    debug_printf("✅ Widget PREVIEW '%s' ajouté (total: %d)\n", id, list->count);
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  AJOUT D'UN WIDGET BUTTON (bouton cliquable)
// ════════════════════════════════════════════════════════════════════════════
bool add_button_widget(WidgetList* list, const char* id, const char* display_name,
                       int x, int y, int width, int height, int text_size,
                       SDL_Color bg_color, void (*callback)(void)) {
    if (!list || !id || !display_name) return false;

    WidgetNode* node = malloc(sizeof(WidgetNode));
    if (!node) return false;

    node->type = WIDGET_TYPE_BUTTON;
    node->id = strdup(id);
    node->display_name = strdup(display_name);
    node->widget.button_widget = create_button_widget(display_name, x, y, width, height, text_size, bg_color);

    if (!node->widget.button_widget) {
        free((void*)node->id);
        free((void*)node->display_name);
        free(node);
        return false;
    }

    node->on_int_value_changed = NULL;
    node->on_bool_value_changed = NULL;
    node->on_float_value_changed = NULL;
    node->on_void_callback = callback;

    if (callback) set_button_click_callback(node->widget.button_widget, callback);

    node->next = NULL;
    node->prev = list->last;

    if (list->last) list->last->next = node;
    else list->first = node;
    list->last = node;
    list->count++;

    debug_printf("✅ Widget BUTTON '%s' ajouté (total: %d)\n", id, list->count);
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// Affiche le contenu de la liste pour debug
void debug_print_widget_list(WidgetList* list) {
    if (is_widget_list_empty(list)) {
        debug_printf("📋 Liste de widgets VIDE\n");
        return;
    }

    debug_printf("📋 LISTE DE WIDGETS (%d éléments):\n", list->count);

    WidgetNode* node = list->first;
    int index = 0;
    while (node) {
        const char* type_name;
        switch (node->type) {
            case WIDGET_TYPE_INCREMENT: type_name = "INCREMENT"; break;
            case WIDGET_TYPE_TOGGLE:    type_name = "TOGGLE";    break;
            case WIDGET_TYPE_LABEL:     type_name = "LABEL";     break;
            case WIDGET_TYPE_SEPARATOR: type_name = "SEPARATOR"; break;
            case WIDGET_TYPE_PREVIEW:   type_name = "PREVIEW";   break;
            case WIDGET_TYPE_BUTTON:    type_name = "BUTTON";    break;
            case WIDGET_TYPE_SLIDER:    type_name = "SLIDER";    break;
            case WIDGET_TYPE_SELECTOR:  type_name = "SELECTOR";  break;
            default:                    type_name = "UNKNOWN";   break;
        }

        debug_printf("  [%d] %s - ID:'%s' - Nom:'%s'\n",
                     index, type_name, node->id, node->display_name);

        // Afficher la valeur actuelle selon le type
        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            debug_printf("      Valeur: %d\n", node->widget.increment_widget->value);
        } else if (node->type == WIDGET_TYPE_TOGGLE && node->widget.toggle_widget) {
            debug_printf("      État: %s\n", node->widget.toggle_widget->value ? "ON" : "OFF");
        }

        index++;
        node = node->next;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION DE LA LISTE ET DE TOUS SES WIDGETS
// ════════════════════════════════════════════════════════════════════════════
// Parcourt la liste, libère chaque widget et chaque nœud
void free_widget_list(WidgetList* list) {
    if (!list) return;

    WidgetNode* current = list->first;
    while (current) {
        WidgetNode* next = current->next;

        // ─────────────────────────────────────────────────────────────────────
        // LIBÉRATION DU WIDGET CONCRET selon son type
        // ─────────────────────────────────────────────────────────────────────
        switch (current->type) {
            case WIDGET_TYPE_INCREMENT:
                if (current->widget.increment_widget) {
                    free_config_widget(current->widget.increment_widget);
                }
                break;

            case WIDGET_TYPE_TOGGLE:
                if (current->widget.toggle_widget) {
                    free_toggle_widget(current->widget.toggle_widget);
                }
                break;

            case WIDGET_TYPE_LABEL:
                if (current->widget.label_widget) {
                    free_label_widget(current->widget.label_widget);
                }
                break;

            case WIDGET_TYPE_SEPARATOR:
                if (current->widget.separator_widget) {
                    free_separator_widget(current->widget.separator_widget);
                }
                break;

            case WIDGET_TYPE_PREVIEW:
                if (current->widget.preview_widget) {
                    free_preview_widget(current->widget.preview_widget);
                }
                break;

            case WIDGET_TYPE_BUTTON:
                if (current->widget.button_widget) {
                    free_button_widget(current->widget.button_widget);
                }
                break;

            default:
                break;
        }

        // ─────────────────────────────────────────────────────────────────────
        // LIBÉRATION DES CHAÎNES ET DU NŒUD
        // ─────────────────────────────────────────────────────────────────────
        free((void*)current->id);
        free((void*)current->display_name);
        free(current);

        current = next;
    }

    free(list);
    debug_printf("🗑️ Liste de widgets libérée\n");
}
