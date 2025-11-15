// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __WIDGET_LIST_H__
#define __WIDGET_LIST_H__

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "widget_types.h"
#include "widget.h"
#include "toggle_widget.h"
#include "label_widget.h"
#include "separator_widget.h"
#include "preview_widget.h"
#include "button_widget.h"
#include "selector_widget.h"

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE D'UN NŒUD DE LA LISTE DE WIDGETS
// ════════════════════════════════════════════════════════════════════════════
// Chaque nœud contient :
//   - Le type de widget (INCREMENT, TOGGLE, etc.)
//   - Un pointeur vers le widget concret (ConfigWidget* ou ToggleWidget*)
//   - Les métadonnées (id, nom affiché, commande associée)
typedef struct WidgetNode {
    WidgetType type;              // Type du widget
    const char* id;               // Identifiant unique (ex: "breath_duration")
    const char* display_name;     // Nom affiché (ex: "Durée respiration")

    // ─────────────────────────────────────────────────────────────────────────
    // POINTEUR VERS LE WIDGET CONCRET
    // ─────────────────────────────────────────────────────────────────────────
    // On utilise un union pour stocker le bon type de widget
    union {
        ConfigWidget* increment_widget;   // Pour WIDGET_TYPE_INCREMENT
        ToggleWidget* toggle_widget;      // Pour WIDGET_TYPE_TOGGLE
        LabelWidget* label_widget;        // Pour WIDGET_TYPE_LABEL
        SeparatorWidget* separator_widget;// Pour WIDGET_TYPE_SEPARATOR
        PreviewWidget* preview_widget;    // Pour WIDGET_TYPE_PREVIEW
        ButtonWidget* button_widget;      // Pour WIDGET_TYPE_BUTTON
        SelectorWidget* selector_widget;  // Pour WIDGET_TYPE_SELECTOR  ← AJOUTER ICI
        void* generic_widget;             // Pour les futurs types
    } widget;

    // ─────────────────────────────────────────────────────────────────────────
    // CALLBACK DE CHANGEMENT DE VALEUR
    // ─────────────────────────────────────────────────────────────────────────
    // Pointeurs de fonction génériques pour notifier les changements
    void (*on_int_value_changed)(int new_value);      // Pour les widgets numériques
    void (*on_bool_value_changed)(bool new_value);    // Pour les toggles
    void (*on_float_value_changed)(float new_value);  // Pour les futurs sliders
    void (*on_void_callback)(void);                   // Pour les boutons

    // ─────────────────────────────────────────────────────────────────────────
    // CHAÎNAGE
    // ─────────────────────────────────────────────────────────────────────────
    struct WidgetNode* next;
    struct WidgetNode* prev;
} WidgetNode;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE DE LA LISTE DE WIDGETS
// ════════════════════════════════════════════════════════════════════════════
typedef struct WidgetList {
    WidgetNode* first;
    WidgetNode* last;
    int count;
} WidgetList;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS
// ════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────
// GESTION DE LA LISTE
// ─────────────────────────────────────────────────────────────────────────
WidgetList* create_widget_list(void);
void free_widget_list(WidgetList* list);
bool is_widget_list_empty(WidgetList* list);

// ─────────────────────────────────────────────────────────────────────────
// AJOUT DE WIDGETS
// ─────────────────────────────────────────────────────────────────────────
// Ajoute un widget INCREMENT (avec flèches)
bool add_increment_widget(WidgetList* list,
                          const char* id,
                          const char* display_name,
                          int x, int y,
                          int min_val, int max_val, int start_val, int increment,
                          int arrow_size, int text_size,
                          TTF_Font* font,
                          void (*callback)(int));

// Ajoute un widget TOGGLE (interrupteur)
bool add_toggle_widget(WidgetList* list,
                       const char* id,
                       const char* display_name,
                       int x, int y,
                       bool start_state,
                       int toggle_width, int toggle_height, int thumb_size,
                       int text_size,
                       void (*callback)(bool));

// Ajoute un widget LABEL (texte/titre)
bool add_label_widget(WidgetList* list,
                      const char* id,
                      const char* display_name,
                      int x, int y,
                      int text_size,
                      SDL_Color color,
                      bool underlined,
                      LabelAlignment alignment);

// Ajoute un widget SEPARATOR (ligne de séparation)
bool add_separator_widget(WidgetList* list,
                          const char* id,
                          int y,
                          int start_margin,
                          int end_margin,
                          int thickness,
                          SDL_Color color);

// Ajoute un widget PREVIEW (zone d'animation)
bool add_preview_widget(WidgetList* list,
                        const char* id,
                        int x, int y,
                        int frame_size,
                        float size_ratio,
                        float breath_duration);

// Ajoute un widget BUTTON (bouton cliquable)
bool add_button_widget(WidgetList* list,
                       const char* id,
                       const char* display_name,
                       int x, int y,
                       int width, int height,
                       int text_size,
                       SDL_Color bg_color,
                       ButtonYAnchor y_anchor,
                       void (*callback)(void));

// Ajoute un widget SELECTOR (liste avec flèches)
bool add_selector_widget(WidgetList* list,
                         const char* id,
                         const char* display_name,
                         int x, int y,
                         int default_index,
                         int arrow_size,
                         int text_size,
                         TTF_Font* font);

// ─────────────────────────────────────────────────────────────────────────
// RENDU ET ÉVÉNEMENTS (FACTORISATION MAGIQUE ✨)
// ─────────────────────────────────────────────────────────────────────────
// Ces fonctions parcourent TOUTE la liste et appellent les bonnes fonctions
// pour chaque widget selon son type
void render_all_widgets(SDL_Renderer* renderer, WidgetList* list,
                        int offset_x, int offset_y, int panel_width, int scroll_offset);

void handle_widget_list_events(WidgetList* list, SDL_Event* event,
                               int offset_x, int offset_y, int scroll_offset);

void update_widget_list_animations(WidgetList* list, float delta_time);

// ─────────────────────────────────────────────────────────────────────────
// UTILITAIRES
// ─────────────────────────────────────────────────────────────────────────
// Trouve un widget par son ID
WidgetNode* find_widget_by_id(WidgetList* list, const char* id);

// Récupère la valeur actuelle d'un widget
bool get_widget_int_value(WidgetList* list, const char* id, int* out_value);
bool get_widget_bool_value(WidgetList* list, const char* id, bool* out_value);

// Modifie la valeur d'un widget
bool set_widget_int_value(WidgetList* list, const char* id, int new_value);
bool set_widget_bool_value(WidgetList* list, const char* id, bool new_value);

// Debug
void debug_print_widget_list(WidgetList* list);

// ─────────────────────────────────────────────────────────────────────────
// SCALING ET POSITIONNEMENT CENTRALISÉ
// ─────────────────────────────────────────────────────────────────────────
// Calcule la largeur minimale requise pour le panneau
// Retourne : MARGIN_LEFT + plus_grande_largeur_widget + MARGIN_RIGHT
int calculate_min_panel_width(WidgetList* list);

#endif
