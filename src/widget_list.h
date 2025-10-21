#ifndef __WIDGET_LIST_H__
#define __WIDGET_LIST_H__

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "widget_types.h"
#include "widget.h"
#include "toggle_widget.h"

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
        void* generic_widget;             // Pour les futurs types
    } widget;

    // ─────────────────────────────────────────────────────────────────────────
    // CALLBACK DE CHANGEMENT DE VALEUR
    // ─────────────────────────────────────────────────────────────────────────
    // Pointeurs de fonction génériques pour notifier les changements
    void (*on_int_value_changed)(int new_value);      // Pour les widgets numériques
    void (*on_bool_value_changed)(bool new_value);    // Pour les toggles
    void (*on_float_value_changed)(float new_value);  // Pour les futurs sliders

    // ─────────────────────────────────────────────────────────────────────────
    // CHAÎNAGE
    // ─────────────────────────────────────────────────────────────────────────
    struct WidgetNode* next;
    struct WidgetNode* prev;
} WidgetNode;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE DE LA LISTE DE WIDGETS
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
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
int widget_list_count(WidgetList* list);

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
                       TTF_Font* font,
                       void (*callback)(bool));

// ─────────────────────────────────────────────────────────────────────────
// RENDU ET ÉVÉNEMENTS (FACTORISATION MAGIQUE ✨)
// ─────────────────────────────────────────────────────────────────────────
// Ces fonctions parcourent TOUTE la liste et appellent les bonnes fonctions
// pour chaque widget selon son type
void render_all_widgets(SDL_Renderer* renderer, WidgetList* list, TTF_Font* font,
                        int offset_x, int offset_y);

void handle_widget_list_events(WidgetList* list, SDL_Event* event,
                               int offset_x, int offset_y);

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

#endif
