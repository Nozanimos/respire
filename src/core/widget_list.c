// SPDX-License-Identifier: GPL-3.0-or-later
#include "widget_list.h"
#include "selector_widget.h"
#include "debug.h"
#include "core/error/error.h"
#include "core/memory/memory.h"
#include <stdlib.h>
#include <string.h>

//  CRÉATION D'UNE LISTE DE WIDGETS VIDE
// Alloue une nouvelle liste vide prête à recevoir des widgets
WidgetList* create_widget_list(void) {
    Error err;
    error_init(&err);
    WidgetList* list = NULL;

    list = SAFE_MALLOC(sizeof(WidgetList));
    CHECK_ALLOC(list, &err, "Erreur allocation liste de widgets");

    list->first = NULL;
    list->last = NULL;
    list->count = 0;

    debug_printf("✅ Liste de widgets créée\n");
    return list;

cleanup:
    error_print(&err);
    SAFE_FREE(list);
    return NULL;
}

//  VÉRIFICATION SI LA LISTE EST VIDE
bool is_widget_list_empty(WidgetList* list) {
    return (list == NULL || list->first == NULL);
}

// ═════════════════════════════════════════════════════════════════════════════
//  HELPERS DE FACTORISATION POUR LES FONCTIONS ADD_*_WIDGET
// ═════════════════════════════════════════════════════════════════════════════

// Initialise un WidgetNode de base avec id et display_name
// Retourne le node alloué ou NULL en cas d'erreur
static WidgetNode* init_widget_node_base(WidgetList* list, const char* id,
                                         const char* display_name,
                                         WidgetType type, Error* err) {
    CHECK_PTR(list, err, "Liste de widgets NULL");
    CHECK_PTR(id, err, "ID widget NULL");
    CHECK_PTR(display_name, err, "Nom d'affichage NULL");

    WidgetNode* node = SAFE_MALLOC(sizeof(WidgetNode));
    CHECK_ALLOC(node, err, "Erreur allocation nœud widget");

    // Initialiser tous les pointeurs à NULL pour cleanup sécurisé
    node->id = NULL;
    node->display_name = NULL;
    memset(&node->widget, 0, sizeof(node->widget));

    // Configuration du nœud
    node->type = type;

    node->id = strdup(id);
    CHECK_ALLOC(node->id, err, "Échec allocation id widget");

    node->display_name = strdup(display_name);
    CHECK_ALLOC(node->display_name, err, "Échec allocation nom d'affichage widget");

    // Initialiser les callbacks à NULL
    node->on_int_value_changed = NULL;
    node->on_bool_value_changed = NULL;
    node->on_float_value_changed = NULL;

    return node;

cleanup:
    // En cas d'erreur, libérer les allocations partielles
    if (node) {
        if (node->id) SAFE_FREE(node->id);
        if (node->display_name) SAFE_FREE(node->display_name);
        SAFE_FREE(node);
    }
    return NULL;
}

// Ajoute un node initialisé à la fin de la liste
static void add_node_to_list(WidgetList* list, WidgetNode* node,
                             const char* id, const char* display_name,
                             const char* type_name) {
    node->next = NULL;
    node->prev = list->last;

    if (list->last) {
        list->last->next = node;
    } else {
        list->first = node;  // Premier élément
    }
    list->last = node;
    list->count++;

    debug_printf("✅ Widget %s '%s' (%s) ajouté à la liste (total: %d)\n",
                 type_name, id, display_name, list->count);
}

// Macro pour cleanup d'un WidgetNode en cas d'erreur
#define CLEANUP_WIDGET_NODE(node, widget_field, free_func) \
    do { \
        if (node) { \
            if (node->id) SAFE_FREE(node->id); \
            if (node->display_name) SAFE_FREE(node->display_name); \
            if (node->widget.widget_field) free_func(node->widget.widget_field); \
            SAFE_FREE(node); \
        } \
    } while(0)

// ═════════════════════════════════════════════════════════════════════════════

//  AJOUT D'UN WIDGET INCREMENT (avec flèches ↑↓)
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
                         int text_size,
                         TTF_Font* font,
                         void (*callback)(int),
                         const char* display_type) {
    Error err;
    error_init(&err);

    // Initialiser le node de base
    WidgetNode* node = init_widget_node_base(list, id, display_name, WIDGET_TYPE_INCREMENT, &err);

    // Créer le widget concret
    node->widget.increment_widget = create_config_widget(
        display_name, x, y,
        min_val, max_val, start_val, increment,
        text_size, font, display_type
    );
    CHECK_PTR(node->widget.increment_widget, &err, "Échec création ConfigWidget");

    // Assigner le callback
    node->on_int_value_changed = callback;
    if (callback) {
        set_config_value_changed_callback(node->widget.increment_widget, callback);
        callback(start_val);
    }

    // Ajouter à la liste
    add_node_to_list(list, node, id, display_name, "INCREMENT");
    return true;

cleanup:
    error_print(&err);
    CLEANUP_WIDGET_NODE(node, increment_widget, free_config_widget);
    return false;
}

//  AJOUT D'UN WIDGET TOGGLE (interrupteur ON/OFF)
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
    Error err;
    error_init(&err);

    // Initialiser le node de base
    WidgetNode* node = init_widget_node_base(list, id, display_name, WIDGET_TYPE_TOGGLE, &err);

    // Créer le widget concret
    node->widget.toggle_widget = create_toggle_widget(
        display_name, x, y,
        start_state,
        toggle_width, toggle_height, thumb_size,
        text_size, &err
    );
    PROPAGATE_ERROR(&err, false);

    // Assigner le callback
    node->on_bool_value_changed = callback;
    if (callback) {
        set_toggle_value_changed_callback(node->widget.toggle_widget, callback);
        callback(start_state);
    }

    // Ajouter à la liste
    add_node_to_list(list, node, id, display_name, "TOGGLE");
    return true;

cleanup:
    error_print(&err);
    CLEANUP_WIDGET_NODE(node, toggle_widget, free_toggle_widget);
    return false;
}

//  RENDU DE TOUS LES WIDGETS (FACTORISATION ✨)
// Parcourt toute la liste et appelle la fonction de rendu appropriée
// selon le type de chaque widget
//
// PARAMÈTRES :
//   - renderer : Le renderer SDL
//   - list : La liste de widgets à afficher
//   - offset_x, offset_y : Offset du conteneur parent (panneau)
//   - panel_width : Largeur actuelle du panneau (pour separator responsive)
void render_all_widgets(SDL_Renderer* renderer, WidgetList* list,
                       int offset_x, int offset_y, int panel_width, int scroll_offset) {
    if (!renderer || is_widget_list_empty(list)) return;

    // Appliquer le scroll_offset au offset_y
    int adjusted_offset_y = offset_y - scroll_offset;

    // ═════════════════════════════════════════════════════════════════════════
    // ÉTAPE 1 : PRÉ-CALCUL DES GROUPES DE WIDGETS INCREMENT
    // ═════════════════════════════════════════════════════════════════════════
    // On regroupe les widgets INCREMENT proches verticalement
    // et on trouve le plus long nom dans chaque groupe pour aligner les flèches

    // Si l'écart entre deux widgets > seuil, c'est un nouveau groupe
    // Espacement dans JSON = 30px, donc on utilise ce seuil
    const int GROUP_SPACING_THRESHOLD = 30;

    // Structure pour stocker les infos des widgets INCREMENT
    typedef struct {
        ConfigWidget* widget;
        int y_position;
        int text_width;
        int group_id;
        int max_text_width_in_group;
        int container_width_for_group;  // Largeur totale du widget le plus long
    } IncrementInfo;

    IncrementInfo increment_infos[50];  // Max 50 widgets INCREMENT
    int increment_count = 0;

    // Premier passage : collecter tous les widgets INCREMENT
    WidgetNode* node = list->first;
    while (node && increment_count < 50) {
        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            ConfigWidget* w = node->widget.increment_widget;

            // Mesurer la largeur du texte actuel (avec taille de police actuelle)
            TTF_Font* font = get_font_for_size(w->current_text_size);
            int text_width = 0;
            if (font) {
                TTF_SizeUTF8(font, w->option_name, &text_width, NULL);
            }

            increment_infos[increment_count].widget = w;
            increment_infos[increment_count].y_position = w->base.y;
            increment_infos[increment_count].text_width = text_width;
            increment_infos[increment_count].group_id = -1;
            increment_infos[increment_count].max_text_width_in_group = 0;
            increment_infos[increment_count].container_width_for_group = 0;
            increment_count++;
        }
        node = node->next;
    }

    // Deuxième passage : trier par position Y pour faciliter le regroupement
    for (int i = 0; i < increment_count - 1; i++) {
        for (int j = i + 1; j < increment_count; j++) {
            if (increment_infos[j].y_position < increment_infos[i].y_position) {
                IncrementInfo temp = increment_infos[i];
                increment_infos[i] = increment_infos[j];
                increment_infos[j] = temp;
            }
        }
    }

    // Troisième passage : regrouper par proximité verticale
    int current_group = 0;
    for (int i = 0; i < increment_count; i++) {
        if (increment_infos[i].group_id == -1) {
            // Nouveau groupe
            increment_infos[i].group_id = current_group;
            int last_y = increment_infos[i].y_position;

            // Trouver tous les widgets proches qui suivent (maintenant triés par Y)
            for (int j = i + 1; j < increment_count; j++) {
                int y_diff = increment_infos[j].y_position - last_y;
                if (y_diff > 0 && y_diff <= GROUP_SPACING_THRESHOLD &&
                    increment_infos[j].group_id == -1) {
                    increment_infos[j].group_id = current_group;
                    last_y = increment_infos[j].y_position;
                }
            }
            current_group++;
        }
    }

    // Quatrième passage : calculer container_width pour chaque groupe
    // On utilise le local_arrows_x + arrow_size + espace + valeur du widget le plus long
    for (int g = 0; g < current_group; g++) {
        int max_text_width = 0;
        ConfigWidget* longest_widget = NULL;

        // Trouver le widget avec le texte le plus long dans ce groupe
        for (int i = 0; i < increment_count; i++) {
            if (increment_infos[i].group_id == g && increment_infos[i].text_width > max_text_width) {
                max_text_width = increment_infos[i].text_width;
                longest_widget = increment_infos[i].widget;
            }
        }

        // Calculer le container_width basé sur le widget le plus long
        int container_width = 0;
        if (longest_widget) {
            // Utiliser local_roller_x (qui inclut texte + espace après texte)
            // + largeur du roller + RIGHT_MARGIN pour alignement à droite
            container_width = longest_widget->local_roller_x +
                            longest_widget->roller_width +
                            10;  // RIGHT_MARGIN (cohérent avec calculate_roller_x_offset)
        }

        // Appliquer cette largeur et max_text_width à tous les widgets du groupe
        for (int i = 0; i < increment_count; i++) {
            if (increment_infos[i].group_id == g) {
                increment_infos[i].max_text_width_in_group = max_text_width;
                increment_infos[i].container_width_for_group = container_width;
            }
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // ÉTAPE 2 : RENDU DE TOUS LES WIDGETS
    // ═════════════════════════════════════════════════════════════════════════

    node = list->first;
    while (node) {
        // ─────────────────────────────────────────────────────────────────────
        // SWITCH sur le type de widget
        // ─────────────────────────────────────────────────────────────────────
        switch (node->type) {
            case WIDGET_TYPE_INCREMENT:
                if (node->widget.increment_widget) {
                    // Trouver le container_width pour ce widget (basé sur son groupe)
                    int container_width = 0;
                    for (int i = 0; i < increment_count; i++) {
                        if (increment_infos[i].widget == node->widget.increment_widget) {
                            container_width = increment_infos[i].container_width_for_group;
                            break;
                        }
                    }

                    render_config_widget(renderer, node->widget.increment_widget,
                                       offset_x, adjusted_offset_y, container_width);
                }
                break;

            case WIDGET_TYPE_TOGGLE:
                if (node->widget.toggle_widget) {
                    render_toggle_widget(renderer, node->widget.toggle_widget,
                                       offset_x, adjusted_offset_y);
                }
                break;

            case WIDGET_TYPE_LABEL:
                if (node->widget.label_widget) {
                    render_label_widget(renderer, node->widget.label_widget,
                                       offset_x, adjusted_offset_y);
                }
                break;

            case WIDGET_TYPE_SEPARATOR:
                if (node->widget.separator_widget) {
                    // Utiliser la largeur dynamique du panneau pour le responsive
                    render_separator_widget(renderer, node->widget.separator_widget,
                                          offset_x, adjusted_offset_y, panel_width);
                }
                break;

            case WIDGET_TYPE_PREVIEW:
                if (node->widget.preview_widget) {
                    render_preview_widget(renderer, node->widget.preview_widget,
                                        offset_x, adjusted_offset_y);
                }
                break;

            case WIDGET_TYPE_BUTTON:
                if (node->widget.button_widget) {
                    render_button_widget(renderer, node->widget.button_widget,
                                       offset_x, adjusted_offset_y);
                }
                break;

            case WIDGET_TYPE_SLIDER:
                // TODO: À implémenter plus tard
                break;

            case WIDGET_TYPE_SELECTOR:
                if (node->widget.selector_widget) {
                    render_selector_widget(renderer, node->widget.selector_widget,
                                         offset_x, adjusted_offset_y);
                }
                break;

            default:
                debug_printf("❌ Type de widget inconnu: %d\n", node->type);
                break;
        }

        node = node->next;
    }
}

//  GESTION DES ÉVÉNEMENTS POUR TOUS LES WIDGETS (FACTORISATION ✨)
// Parcourt toute la liste et transmet l'événement à chaque widget
// selon son type
//
// PARAMÈTRES :
//   - list : La liste de widgets
//   - event : L'événement SDL à traiter
//   - offset_x, offset_y : Offset du conteneur parent
//   - scroll_offset : Décalage vertical du scroll (pour ajuster les collisions)
void handle_widget_list_events(WidgetList* list, SDL_Event* event,
                               int offset_x, int offset_y, int scroll_offset) {
    if (is_widget_list_empty(list) || !event) return;

    // ═════════════════════════════════════════════════════════════════════════
    // APPLIQUER LE SCROLL À L'OFFSET VERTICAL
    // ═════════════════════════════════════════════════════════════════════════
    // IMPORTANT : Le rendu utilise (offset_y - scroll_offset), donc la détection
    // de collision doit utiliser la MÊME formule pour que les zones cliquables
    // correspondent exactement aux zones rendues à l'écran !
    int adjusted_offset_y = offset_y - scroll_offset;

    // ═════════════════════════════════════════════════════════════════════════
    // ÉTAPE 1 : PRÉ-CALCUL DES GROUPES DE WIDGETS INCREMENT (même logique que render)
    // ═════════════════════════════════════════════════════════════════════════
    const int GROUP_SPACING_THRESHOLD = 30;

    typedef struct {
        ConfigWidget* widget;
        int y_position;
        int text_width;
        int group_id;
        int max_text_width_in_group;
        int container_width_for_group;
    } IncrementInfo;

    IncrementInfo increment_infos[50];
    int increment_count = 0;

    // Premier passage : collecter tous les widgets INCREMENT
    WidgetNode* node = list->first;
    while (node && increment_count < 50) {
        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            ConfigWidget* w = node->widget.increment_widget;

            TTF_Font* font = get_font_for_size(w->current_text_size);
            int text_width = 0;
            if (font) {
                TTF_SizeUTF8(font, w->option_name, &text_width, NULL);
            }

            increment_infos[increment_count].widget = w;
            increment_infos[increment_count].y_position = w->base.y;
            increment_infos[increment_count].text_width = text_width;
            increment_infos[increment_count].group_id = -1;
            increment_infos[increment_count].max_text_width_in_group = 0;
            increment_infos[increment_count].container_width_for_group = 0;
            increment_count++;
        }
        node = node->next;
    }

    // Deuxième passage : trier par position Y
    for (int i = 0; i < increment_count - 1; i++) {
        for (int j = i + 1; j < increment_count; j++) {
            if (increment_infos[j].y_position < increment_infos[i].y_position) {
                IncrementInfo temp = increment_infos[i];
                increment_infos[i] = increment_infos[j];
                increment_infos[j] = temp;
            }
        }
    }

    // Troisième passage : regrouper par proximité verticale
    int current_group = 0;
    for (int i = 0; i < increment_count; i++) {
        if (increment_infos[i].group_id == -1) {
            increment_infos[i].group_id = current_group;
            int last_y = increment_infos[i].y_position;

            for (int j = i + 1; j < increment_count; j++) {
                int y_diff = increment_infos[j].y_position - last_y;
                if (y_diff > 0 && y_diff <= GROUP_SPACING_THRESHOLD &&
                    increment_infos[j].group_id == -1) {
                    increment_infos[j].group_id = current_group;
                    last_y = increment_infos[j].y_position;
                }
            }
            current_group++;
        }
    }

    // Quatrième passage : calculer container_width pour chaque groupe
    for (int g = 0; g < current_group; g++) {
        int max_text_width = 0;
        ConfigWidget* longest_widget = NULL;

        for (int i = 0; i < increment_count; i++) {
            if (increment_infos[i].group_id == g && increment_infos[i].text_width > max_text_width) {
                max_text_width = increment_infos[i].text_width;
                longest_widget = increment_infos[i].widget;
            }
        }

        int container_width = 0;
        if (longest_widget) {
            container_width = longest_widget->local_roller_x +
                            longest_widget->roller_width +
                            10;  // RIGHT_MARGIN (cohérent avec render_all_widgets)
        }

        for (int i = 0; i < increment_count; i++) {
            if (increment_infos[i].group_id == g) {
                increment_infos[i].max_text_width_in_group = max_text_width;
                increment_infos[i].container_width_for_group = container_width;
            }
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // ÉTAPE 2 : TRAITEMENT DES ÉVÉNEMENTS
    // ═════════════════════════════════════════════════════════════════════════

    node = list->first;
    while (node) {
        // ─────────────────────────────────────────────────────────────────────
        // SWITCH sur le type de widget
        // ─────────────────────────────────────────────────────────────────────
        switch (node->type) {
            case WIDGET_TYPE_INCREMENT:
                if (node->widget.increment_widget) {
                    // Trouver le container_width pour ce widget (basé sur son groupe)
                    int container_width = 0;
                    for (int i = 0; i < increment_count; i++) {
                        if (increment_infos[i].widget == node->widget.increment_widget) {
                            container_width = increment_infos[i].container_width_for_group;
                            break;
                        }
                    }

                    handle_config_widget_events(node->widget.increment_widget,
                                              event, offset_x, adjusted_offset_y, container_width);
                }
                break;

            case WIDGET_TYPE_TOGGLE:
                if (node->widget.toggle_widget) {
                    handle_toggle_widget_events(node->widget.toggle_widget,
                                              event, offset_x, adjusted_offset_y);
                }
                break;

            case WIDGET_TYPE_BUTTON:
                if (node->widget.button_widget) {
                    handle_button_widget_events(node->widget.button_widget,
                                              event, offset_x, adjusted_offset_y);
                }
                break;

            case WIDGET_TYPE_SELECTOR:
                if (node->widget.selector_widget) {
                    handle_selector_widget_events(node->widget.selector_widget,
                                                 event, offset_x, adjusted_offset_y);
                }
                break;

            // Les autres widgets ne gèrent pas d'événements
            case WIDGET_TYPE_LABEL:
            case WIDGET_TYPE_SEPARATOR:
            case WIDGET_TYPE_PREVIEW:
            case WIDGET_TYPE_SLIDER:
                break;

            default:
                break;
        }

        node = node->next;
    }
}

//  MISE À JOUR DES ANIMATIONS DE TOUS LES WIDGETS
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

            case WIDGET_TYPE_SELECTOR:
                // Le selector a une animation de roulette + animation du sous-menu
                if (node->widget.selector_widget) {
                    update_selector_animation(node->widget.selector_widget, delta_time);
                    update_selector_submenu_animation(node->widget.selector_widget, delta_time);
                }
                break;

            // Les autres widgets n'ont pas d'animation
            default:
                break;
        }

        node = node->next;
    }
}

//  RECHERCHE D'UN WIDGET PAR SON ID
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

//  RÉCUPÉRATION DE LA VALEUR INT D'UN WIDGET
// Récupère la valeur actuelle d'un widget INCREMENT
//
// RETOURNE :
//   - true si succès (valeur stockée dans out_value)
//   - false si échec (widget non trouvé ou mauvais type)
bool get_widget_int_value(WidgetList* list, const char* id, int* out_value) {
    if (!out_value) return false;

    WidgetNode* node = find_widget_by_id(list, id);
    if (!node) return false;

    // Gérer les widgets INCREMENT
    if (node->type == WIDGET_TYPE_INCREMENT) {
        if (!node->widget.increment_widget) return false;
        *out_value = node->widget.increment_widget->value;
        return true;
    }

    // Gérer les widgets SELECTOR (retourne l'index actuel)
    if (node->type == WIDGET_TYPE_SELECTOR) {
        if (!node->widget.selector_widget) return false;
        *out_value = node->widget.selector_widget->current_index;
        return true;
    }

    debug_printf("❌ Widget '%s' n'est ni INCREMENT ni SELECTOR\n", id);
    return false;
}

//  RÉCUPÉRATION DE LA VALEUR BOOL D'UN WIDGET
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

//  MODIFICATION DE LA VALEUR INT D'UN WIDGET
// Change la valeur d'un widget INCREMENT par programmation
// (sans interaction utilisateur)
//
// RETOURNE :
//   - true si succès
//   - false si échec
bool set_widget_int_value(WidgetList* list, const char* id, int new_value) {
    WidgetNode* node = find_widget_by_id(list, id);
    if (!node) return false;

    // Gérer les widgets INCREMENT
    if (node->type == WIDGET_TYPE_INCREMENT) {
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

    // Gérer les widgets SELECTOR (change l'index actuel)
    if (node->type == WIDGET_TYPE_SELECTOR) {
        if (!node->widget.selector_widget) return false;

        SelectorWidget* widget = node->widget.selector_widget;

        // Vérifier que l'index est valide
        if (new_value < 0 || new_value >= widget->num_options) {
            debug_printf("⚠️ Index %d hors limites pour selector '%s' [0, %d]\n",
                         new_value, id, widget->num_options - 1);
            return false;
        }

        widget->current_index = new_value;
        debug_printf("🔧 Selector '%s' mis à jour: index %d (%s)\n",
                     id, new_value, widget->options[new_value].text);
        return true;
    }

    debug_printf("❌ Widget '%s' n'est ni INCREMENT ni SELECTOR\n", id);
    return false;
}

//  MODIFICATION DE LA VALEUR BOOL D'UN WIDGET
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

//  AFFICHAGE DEBUG DE LA LISTE
//  AJOUT D'UN WIDGET LABEL (texte/titre)
bool add_label_widget(WidgetList* list,
                      const char* id,
                      const char* display_name,
                      int x, int y,
                      int text_size,
                      SDL_Color color,
                      bool underlined,
                      LabelAlignment alignment) {
    Error err;
    error_init(&err);

    // Initialiser le node de base
    WidgetNode* node = init_widget_node_base(list, id, display_name, WIDGET_TYPE_LABEL, &err);

    // Créer le widget concret
    node->widget.label_widget = create_label_widget(display_name, x, y, text_size, color, underlined, alignment, &err);
    PROPAGATE_ERROR(&err, false);

    // Pas de callback pour les labels
    node->on_void_callback = NULL;

    // Ajouter à la liste
    add_node_to_list(list, node, id, display_name, "LABEL");
    return true;

cleanup:
    error_print(&err);
    CLEANUP_WIDGET_NODE(node, label_widget, free_label_widget);
    return false;
}

//  AJOUT D'UN WIDGET SEPARATOR (barre de séparation)
bool add_separator_widget(WidgetList* list, const char* id, int y,
                          int start_margin, int end_margin, int thickness, SDL_Color color) {
    Error err;
    error_init(&err);

    // Initialiser le node de base
    WidgetNode* node = init_widget_node_base(list, id, "separator", WIDGET_TYPE_SEPARATOR, &err);

    // Créer le widget concret
    node->widget.separator_widget = create_separator_widget(y, start_margin, end_margin, thickness, color, &err);
    PROPAGATE_ERROR(&err, false);

    // Pas de callback pour les separators
    node->on_void_callback = NULL;

    // Ajouter à la liste
    add_node_to_list(list, node, id, "separator", "SEPARATOR");
    return true;

cleanup:
    error_print(&err);
    CLEANUP_WIDGET_NODE(node, separator_widget, free_separator_widget);
    return false;
}

//  AJOUT D'UN WIDGET PREVIEW (zone d'animation)
bool add_preview_widget(WidgetList* list, const char* id, int x, int y,
                        int frame_size, float size_ratio, float breath_duration) {
    Error err;
    error_init(&err);

    // Initialiser le node de base
    WidgetNode* node = init_widget_node_base(list, id, "preview", WIDGET_TYPE_PREVIEW, &err);

    // Créer le widget concret
    node->widget.preview_widget = create_preview_widget(x, y, frame_size, size_ratio, breath_duration, &err);
    PROPAGATE_ERROR(&err, false);

    // Pas de callback pour les previews
    node->on_void_callback = NULL;

    // Ajouter à la liste
    add_node_to_list(list, node, id, "preview", "PREVIEW");
    return true;

cleanup:
    error_print(&err);
    CLEANUP_WIDGET_NODE(node, preview_widget, free_preview_widget);
    return false;
}

//  AJOUT D'UN WIDGET BUTTON (bouton cliquable)
bool add_button_widget(WidgetList* list, const char* id, const char* display_name,
                       int x, int y, int width, int height, int text_size,
                       SDL_Color bg_color, ButtonYAnchor y_anchor, void (*callback)(void)) {
    Error err;
    error_init(&err);

    // Initialiser le node de base
    WidgetNode* node = init_widget_node_base(list, id, display_name, WIDGET_TYPE_BUTTON, &err);

    // Créer le widget concret
    node->widget.button_widget = create_button_widget(display_name, x, y, width, height, text_size, bg_color, y_anchor, &err);
    PROPAGATE_ERROR(&err, false);

    // Assigner le callback
    node->on_void_callback = callback;
    if (callback) {
        set_button_click_callback(node->widget.button_widget, callback);
    }

    // Ajouter à la liste
    add_node_to_list(list, node, id, display_name, "BUTTON");
    return true;

cleanup:
    error_print(&err);
    CLEANUP_WIDGET_NODE(node, button_widget, free_button_widget);
    return false;
}

//  AJOUT D'UN WIDGET SELECTOR (liste avec flèches)
// Crée un widget selector et l'ajoute à la fin de la liste
//
// PARAMÈTRES :
//   - list : La liste où ajouter le widget
//   - id : Identifiant unique (ex: "retention_type")
//   - display_name : Nom affiché (ex: "Type de rétention")
//   - x, y : Position RELATIVE au conteneur parent
//   - default_index : Index de l'option sélectionnée par défaut
//   - arrow_size : Taille des flèches en pixels
//   - text_size : Taille de la police
//   - font : Police TTF pour le rendu du texte
bool add_selector_widget(WidgetList* list, const char* id, const char* display_name,
                         int x, int y, int default_index, int arrow_size, int text_size,
                         TTF_Font* font) {
    Error err;
    error_init(&err);

    // Initialiser le node de base
    WidgetNode* node = init_widget_node_base(list, id, display_name, WIDGET_TYPE_SELECTOR, &err);

    // Créer le widget concret
    node->widget.selector_widget = create_selector_widget(
        display_name, x, y,
        default_index,
        arrow_size, text_size,
        font, &err
    );
    PROPAGATE_ERROR(&err, false);

    // Pas de callback global pour le selector (chaque option a son callback)
    node->on_void_callback = NULL;

    // Ajouter à la liste
    add_node_to_list(list, node, id, display_name, "SELECTOR");
    return true;

cleanup:
    error_print(&err);
    CLEANUP_WIDGET_NODE(node, selector_widget, free_selector_widget);
    return false;
}

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

//  LIBÉRATION DE LA LISTE ET DE TOUS SES WIDGETS
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
            case WIDGET_TYPE_SELECTOR:
                if (current->widget.selector_widget) {
                    free_selector_widget(current->widget.selector_widget);
                }
                break;

            default:
                break;
        }

        // ─────────────────────────────────────────────────────────────────────
        // LIBÉRATION DES CHAÎNES ET DU NŒUD
        // ─────────────────────────────────────────────────────────────────────
        SAFE_FREE(current->id);
        SAFE_FREE(current->display_name);
        SAFE_FREE(current);

        current = next;
    }

    SAFE_FREE(list);
    debug_printf("🗑️ Liste de widgets libérée\n");
}

//  CALCUL DE LA LARGEUR MINIMALE DU PANNEAU

int calculate_min_panel_width(WidgetList* list) {
    if (!list) return 100;  // Valeur par défaut sécurisée

    const int MARGIN_LEFT = 20;
    const int MARGIN_RIGHT = 20;

    int max_widget_width = 0;
    WidgetNode* node = list->first;

    while (node) {
        int widget_width = 0;

        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            ConfigWidget* w = node->widget.increment_widget;
            widget_width = w->local_roller_x + w->roller_width + 10;  // Largeur totale
        }
        else if (node->type == WIDGET_TYPE_TOGGLE && node->widget.toggle_widget) {
            widget_width = node->widget.toggle_widget->base.base_width;
        }
        else if (node->type == WIDGET_TYPE_BUTTON && node->widget.button_widget) {
            widget_width = node->widget.button_widget->base_width;
        }
        else if (node->type == WIDGET_TYPE_PREVIEW && node->widget.preview_widget) {
            widget_width = node->widget.preview_widget->base_frame_size;
        }
        else if (node->type == WIDGET_TYPE_LABEL && node->widget.label_widget) {
            widget_width = node->widget.label_widget->base.width;
        }
        else if (node->type == WIDGET_TYPE_SELECTOR && node->widget.selector_widget) {
            widget_width = node->widget.selector_widget->base.width;
        }
        // Ignorer les séparateurs

        if (widget_width > max_widget_width) {
            max_widget_width = widget_width;
        }

        node = node->next;
    }

    int min_width = MARGIN_LEFT + max_widget_width + MARGIN_RIGHT;
    debug_printf("📐 Largeur minimale panneau: %d px (widget max: %d)\n",
                 min_width, max_widget_width);

    return min_width;
}
