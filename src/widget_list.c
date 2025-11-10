// SPDX-License-Identifier: GPL-3.0-or-later
#include "widget_list.h"
#include "selector_widget.h"
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
        set_config_value_changed_callback(node->widget.increment_widget, callback);     // Appeler le callback pour synchroniser la valeur initiale
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
            // Utiliser local_arrows_x (qui inclut texte + espace après texte)
            // + taille des flèches + espace + largeur estimée valeur
            container_width = longest_widget->local_arrows_x +
                            longest_widget->arrow_size +
                            60;  // Espace après flèches + largeur valeur estimée
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

            case WIDGET_TYPE_SELECTOR:
                if (node->widget.selector_widget) {
                    handle_selector_widget_events(node->widget.selector_widget,
                                                 event, offset_x, offset_y);
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

            case WIDGET_TYPE_SELECTOR:
                // Le selector a une animation de roulette
                if (node->widget.selector_widget) {
                    update_selector_animation(node->widget.selector_widget, delta_time);
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
                       SDL_Color bg_color, ButtonYAnchor y_anchor, void (*callback)(void)) {
    if (!list || !id || !display_name) return false;

    WidgetNode* node = malloc(sizeof(WidgetNode));
    if (!node) return false;

    node->type = WIDGET_TYPE_BUTTON;
    node->id = strdup(id);
    node->display_name = strdup(display_name);
    node->widget.button_widget = create_button_widget(display_name, x, y, width, height, text_size, bg_color, y_anchor);

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
//  AJOUT D'UN WIDGET SELECTOR (liste avec flèches)
// ════════════════════════════════════════════════════════════════════════════
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
    if (!list || !id || !display_name) {
        debug_printf("❌ Paramètres invalides pour add_selector_widget\n");
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
    node->type = WIDGET_TYPE_SELECTOR;
    node->id = strdup(id);
    node->display_name = strdup(display_name);

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DU WIDGET CONCRET
    // ─────────────────────────────────────────────────────────────────────────
    node->widget.selector_widget = create_selector_widget(
        display_name, x, y,
        default_index,
        arrow_size, text_size,
        font
    );

    if (!node->widget.selector_widget) {
        debug_printf("❌ Échec création SelectorWidget '%s'\n", id);
        free((void*)node->id);
        free((void*)node->display_name);
        free(node);
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // ASSIGNATION DES CALLBACKS (NULL pour le selector car chaque option a son callback)
    // ─────────────────────────────────────────────────────────────────────────
    node->on_int_value_changed = NULL;
    node->on_bool_value_changed = NULL;
    node->on_float_value_changed = NULL;
    node->on_void_callback = NULL;

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

    debug_printf("✅ Widget SELECTOR '%s' (%s) ajouté à la liste (total: %d)\n",
                 id, display_name, list->count);

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
        free((void*)current->id);
        free((void*)current->display_name);
        free(current);

        current = next;
    }

    free(list);
    debug_printf("🗑️ Liste de widgets libérée\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  SCALING ET POSITIONNEMENT CENTRALISÉ
// ════════════════════════════════════════════════════════════════════════════

void rescale_and_layout_widgets(WidgetList* list, int panel_width,
                                 int screen_width, int screen_height) {
    if (!list) return;
    (void)screen_width;   // Non utilisé dans la nouvelle logique

    const int MARGIN_LEFT = 20;     // Marge gauche
    const int MARGIN_RIGHT = 20;    // Marge droite
    const int PANEL_WIDTH_BASE = 500;  // Largeur de base du panneau
    float panel_ratio = (float)panel_width / (float)PANEL_WIDTH_BASE;

    debug_printf("🔄 Layout widgets - panel_width: %d, screen_height: %d, ratio: %.2f\n",
                 panel_width, screen_height, panel_ratio);

    // ═════════════════════════════════════════════════════════════════════════
    // PHASE 0 : RESCALER INDIVIDUELLEMENT CHAQUE WIDGET
    // ═════════════════════════════════════════════════════════════════════════
    WidgetNode* node = list->first;
    while (node) {
        switch (node->type) {
            case WIDGET_TYPE_SELECTOR:
                if (node->widget.selector_widget) {
                    rescale_selector_widget(node->widget.selector_widget, panel_ratio);
                }
                break;

            // Les autres widgets utilisent rescale_widget_base appelé dans leur propre fonction
            // mais le selector a besoin d'un rescale complet pour recalculer le layout
            default:
                break;
        }
        node = node->next;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // PHASE 1 : CALCULER LA LARGEUR MAXIMALE DES WIDGETS (sans la barre)
    // ═════════════════════════════════════════════════════════════════════════
    int max_widget_width = 0;
    node = list->first;

    while (node) {
        int widget_width = 0;

        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            // Pour INCREMENT : largeur totale = du début du texte jusqu'à la fin de la valeur
            ConfigWidget* w = node->widget.increment_widget;
            widget_width = w->local_value_x + 50;  // +50 pour la largeur de la valeur
        }
        else if (node->type == WIDGET_TYPE_TOGGLE && node->widget.toggle_widget) {
            ToggleWidget* w = node->widget.toggle_widget;
            widget_width = w->base.width;
        }
        else if (node->type == WIDGET_TYPE_BUTTON && node->widget.button_widget) {
            ButtonWidget* w = node->widget.button_widget;
            widget_width = w->base.width;
        }
        else if (node->type == WIDGET_TYPE_PREVIEW && node->widget.preview_widget) {
            PreviewWidget* w = node->widget.preview_widget;
            widget_width = w->base.width;
        }
        else if (node->type == WIDGET_TYPE_LABEL && node->widget.label_widget) {
            LabelWidget* w = node->widget.label_widget;
            widget_width = w->base.width;
        }
        else if (node->type == WIDGET_TYPE_SELECTOR && node->widget.selector_widget) {
            SelectorWidget* w = node->widget.selector_widget;
            widget_width = w->base.width;
        }
        // Ignorer les séparateurs pour le calcul de largeur max

        if (widget_width > max_widget_width) {
            max_widget_width = widget_width;
        }

        node = node->next;
    }

    debug_printf("📏 Largeur max widget: %d px\n", max_widget_width);

    // ═════════════════════════════════════════════════════════════════════════
    // PHASE 2 : REPOSITIONNER LES WIDGETS EN CAS DE COLLISION
    // ═════════════════════════════════════════════════════════════════════════
    // Un widget est en collision si : base_x + width + MARGIN_RIGHT > panel_width

    node = list->first;
    while (node) {
        bool has_collision = false;
        int widget_x = 0;
        int widget_width = 0;

        // Obtenir position et largeur actuelle
        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            ConfigWidget* w = node->widget.increment_widget;
            widget_x = w->base.base_x;
            widget_width = w->local_value_x + 50;
            has_collision = (widget_x + widget_width + MARGIN_RIGHT > panel_width);

            if (has_collision) {
                // Centrer le widget
                int new_x = (panel_width - widget_width) / 2;
                w->base.x = new_x;
                debug_printf("🔄 INCREMENT centré: %d → %d (collision)\n", widget_x, new_x);
            } else {
                // Garder position de base
                w->base.x = w->base.base_x;
            }
        }
        else if (node->type == WIDGET_TYPE_TOGGLE && node->widget.toggle_widget) {
            ToggleWidget* w = node->widget.toggle_widget;
            widget_x = w->base.base_x;
            widget_width = w->base.base_width;
            has_collision = (widget_x + widget_width + MARGIN_RIGHT > panel_width);

            if (has_collision) {
                int new_x = (panel_width - widget_width) / 2;
                w->base.x = new_x;
                debug_printf("🔄 TOGGLE centré: %d → %d (collision)\n", widget_x, new_x);
            } else {
                w->base.x = w->base.base_x;
            }
        }
        else if (node->type == WIDGET_TYPE_BUTTON && node->widget.button_widget) {
            ButtonWidget* w = node->widget.button_widget;
            widget_x = w->base_x - w->base_width / 2;  // Position de base (du JSON)
            widget_width = w->base_width;
            has_collision = (widget_x + widget_width + MARGIN_RIGHT > panel_width);

            if (has_collision) {
                // Centrer le widget
                int new_x = (panel_width - widget_width) / 2;
                w->base.x = new_x;
                debug_printf("🔄 BUTTON '%s' centré: %d → %d (collision)\n",
                            node->id, widget_x, new_x);
            } else {
                // Garder position de base
                w->base.x = w->base_x - w->base_width / 2;
                debug_printf("🔄 BUTTON '%s' position normale: x = %d\n",
                            node->id, w->base.x);
            }

            // ═════════════════════════════════════════════════════════════════════
            // CALCUL DE LA POSITION Y EN FONCTION DE L'ANCRAGE
            // ═════════════════════════════════════════════════════════════════════
            if (w->y_anchor == BUTTON_ANCHOR_BOTTOM) {
                // Position relative au bas du panneau
                w->base.y = screen_height - w->base_y - w->base_height / 2;
                debug_printf("🔽 BUTTON '%s' ancré en BAS: y = %d (base_y=%d)\n",
                            node->id, w->base.y, w->base_y);
            } else {
                // Position relative au haut du panneau (comportement par défaut)
                w->base.y = w->base_y - w->base_height / 2;
                debug_printf("🔼 BUTTON '%s' ancré en HAUT: y = %d\n",
                            node->id, w->base.y);
            }
        }
        else if (node->type == WIDGET_TYPE_PREVIEW && node->widget.preview_widget) {
            PreviewWidget* w = node->widget.preview_widget;
            widget_x = w->base.x;
            widget_width = w->base.width;
            has_collision = (widget_x + widget_width + MARGIN_RIGHT > panel_width);

            if (has_collision) {
                int new_x = (panel_width - widget_width) / 2;
                w->base.x = new_x;
                debug_printf("🔄 PREVIEW centré: %d → %d (collision)\n", widget_x, new_x);
            }
        }
        else if (node->type == WIDGET_TYPE_LABEL && node->widget.label_widget) {
            LabelWidget* w = node->widget.label_widget;
            widget_x = w->base.x;
            widget_width = w->base.width;
            has_collision = (widget_x + widget_width + MARGIN_RIGHT > panel_width);

            if (has_collision) {
                int new_x = (panel_width - widget_width) / 2;
                w->base.x = new_x;
                debug_printf("🔄 LABEL centré: %d → %d (collision)\n", widget_x, new_x);
            }
        }
        else if (node->type == WIDGET_TYPE_SELECTOR && node->widget.selector_widget) {  // ← AJOUTER CE BLOC
            SelectorWidget* w = node->widget.selector_widget;
            widget_x = w->base.x;
            widget_width = w->base.width;
            has_collision = (widget_x + widget_width + MARGIN_RIGHT > panel_width);

            if (has_collision) {
                int new_x = (panel_width - widget_width) / 2;
                w->base.x = new_x;
                debug_printf("🔄 SELECTOR centré: %d → %d (collision)\n", widget_x, new_x);
            }
        }
        else if (node->type == WIDGET_TYPE_SEPARATOR && node->widget.separator_widget) {
            // La barre de séparation : rester centrée avec marges constantes
            SeparatorWidget* w = node->widget.separator_widget;
            w->base.x = MARGIN_LEFT;
            w->base.width = panel_width - MARGIN_LEFT - MARGIN_RIGHT;
            debug_printf("🔄 SEPARATOR ajusté: largeur = %d\n", w->base.width);
        }

        node = node->next;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // PHASE 3 : DÉTECTER ET RÉORGANISER LES BOUTONS QUI SE CHEVAUCHENT
    // ═════════════════════════════════════════════════════════════════════════
    // Collecter les boutons ancrés en bas et vérifier s'ils se chevauchent
    typedef struct {
        WidgetNode* node;
        ButtonWidget* button;
        int abs_x1, abs_x2;  // Coordonnées horizontales absolues
    } ButtonInfo;

    ButtonInfo bottom_buttons[32];  // Max 32 boutons
    int bottom_count = 0;

    node = list->first;
    while (node && bottom_count < 32) {
        if (node->type == WIDGET_TYPE_BUTTON && node->widget.button_widget) {
            ButtonWidget* btn = node->widget.button_widget;
            if (btn->y_anchor == BUTTON_ANCHOR_BOTTOM) {
                bottom_buttons[bottom_count].node = node;
                bottom_buttons[bottom_count].button = btn;
                bottom_buttons[bottom_count].abs_x1 = btn->base.x;
                bottom_buttons[bottom_count].abs_x2 = btn->base.x + btn->base.width;
                bottom_count++;
            }
        }
        node = node->next;
    }

    // Vérifier si des boutons se chevauchent
    bool has_overlap = false;
    for (int i = 0; i < bottom_count - 1 && !has_overlap; i++) {
        for (int j = i + 1; j < bottom_count && !has_overlap; j++) {
            // Deux rectangles se chevauchent si :
            // x1_a < x2_b ET x2_a > x1_b
            if (bottom_buttons[i].abs_x1 < bottom_buttons[j].abs_x2 &&
                bottom_buttons[i].abs_x2 > bottom_buttons[j].abs_x1) {
                has_overlap = true;
                debug_printf("⚠️ Chevauchement détecté entre '%s' et '%s'\n",
                            bottom_buttons[i].node->id,
                            bottom_buttons[j].node->id);
            }
        }
    }

    // Si chevauchement, réorganiser verticalement
    if (has_overlap && bottom_count > 0) {
        debug_printf("📐 Réorganisation verticale de %d boutons\n", bottom_count);

        const int BUTTON_SPACING = 10;  // Espacement entre boutons

        // Positionner les boutons depuis le bas avec l'espacement
        int current_y = screen_height - bottom_buttons[0].button->base_y;

        for (int i = 0; i < bottom_count; i++) {
            ButtonWidget* btn = bottom_buttons[i].button;

            // Centrer horizontalement
            btn->base.x = (panel_width - btn->base.width) / 2;

            // Positionner verticalement (de bas en haut)
            current_y -= btn->base.height;
            btn->base.y = current_y;
            current_y -= BUTTON_SPACING;

            debug_printf("📍 BUTTON '%s' empilé: x=%d, y=%d\n",
                        bottom_buttons[i].node->id, btn->base.x, btn->base.y);
        }
    }

    debug_printf("✅ %d widgets repositionnés\n", list->count);
}

// ════════════════════════════════════════════════════════════════════════════
//  CALCUL DE LA LARGEUR MINIMALE DU PANNEAU
// ════════════════════════════════════════════════════════════════════════════

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
            widget_width = w->local_value_x + 50;  // Largeur totale
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
