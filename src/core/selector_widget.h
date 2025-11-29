// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __SELECTOR_WIDGET_H__
#define __SELECTOR_WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "widget_base.h"
#include "geometry.h"

// ════════════════════════════════════════════════════════════════════════════
//  CONSTANTES
// ════════════════════════════════════════════════════════════════════════════
#define MAX_SELECTOR_OPTIONS 20      // Maximum d'options dans une liste
#define MAX_OPTION_TEXT_LENGTH 50    // Longueur max du texte d'une option
#define MAX_CALLBACK_NAME_LENGTH 50  // Longueur max du nom de callback

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE D'UNE OPTION DU SÉLECTEUR
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    char text[MAX_OPTION_TEXT_LENGTH];           // Texte affiché (ex: "Poumons pleins")
    char callback_name[MAX_CALLBACK_NAME_LENGTH]; // Nom du callback associé
    void (*callback)(void);                       // Pointeur vers la fonction callback
} SelectorOption;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE DU WIDGET SELECTOR
// ════════════════════════════════════════════════════════════════════════════
// Layout visuel : [Label]  [◀] [Option actuelle] [▶]
typedef struct SelectorWidget {
    // ─────────────────────────────────────────────────────────────────────────
    // BASE DU WIDGET
    // ─────────────────────────────────────────────────────────────────────────
    WidgetBase base;

    // ─────────────────────────────────────────────────────────────────────────
    // LABEL ET OPTIONS
    // ─────────────────────────────────────────────────────────────────────────
    char nom_affichage[100];     // Label affiché (ex: "Type de rétention")
    SelectorOption options[MAX_SELECTOR_OPTIONS];
    int num_options;
    int current_index;

    // ─────────────────────────────────────────────────────────────────────────
    // POLICE ET DIMENSIONS
    // ─────────────────────────────────────────────────────────────────────────
    TTF_Font* font;              // Police pour le texte
    int base_text_size;          // Taille de police de référence
    int current_text_size;       // Taille actuelle après scaling
    int base_arrow_size;         // Taille de base des flèches
    int current_arrow_size;      // Taille actuelle des flèches

    // ─────────────────────────────────────────────────────────────────────────
    // FLÈCHES (utilisant geometry.c)
    // ─────────────────────────────────────────────────────────────────────────
    Triangle* left_arrow;        // Flèche gauche ◀
    Triangle* right_arrow;       // Flèche droite ▶

    // ─────────────────────────────────────────────────────────────────────────
    // ZONES CLIQUABLES (coordonnées LOCALES au widget)
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Rect left_arrow_rect;
    SDL_Rect right_arrow_rect;

    // ─────────────────────────────────────────────────────────────────────────
    // COULEURS
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color text_color;
    SDL_Color arrow_color;
    SDL_Color arrow_hover_color;

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION
    // ─────────────────────────────────────────────────────────────────────────
    bool left_arrow_hovered;
    bool right_arrow_hovered;
    bool value_hovered;          // Hover sur la zone de texte

    // ─────────────────────────────────────────────────────────────────────────
    // ANIMATION (pour effet roulette)
    // ─────────────────────────────────────────────────────────────────────────
    float animation_progress;    // 0.0 → 1.0
    int animation_direction;     // -1 = gauche, +1 = droite, 0 = pas d'animation
    int previous_index;          // Index précédent pour l'animation
    SDL_Rect value_rect;         // Zone du texte de valeur (pour hover et molette)

    // ─────────────────────────────────────────────────────────────────────────
    // SOUS-MENU ROLLER (OVERLAY) - Système de pattern personnalisé
    // ─────────────────────────────────────────────────────────────────────────
    bool submenu_enabled;        // Activé depuis le JSON
    bool submenu_open;           // Actuellement ouvert (true) ou fermé (false)
    float submenu_animation;     // 0.0 (fermé) → 1.0 (ouvert)
    bool submenu_animating;      // En cours d'animation

    SDL_Rect submenu_full_rect;  // Zone totale du sous-menu (pour détection hover globale)

    // Séquence 1 (première partie du pattern)
    int seq1_type;               // 0 = pleins, 1 = vides
    int seq1_count;              // Nombre de sessions (1-10)
    SDL_Rect seq1_type_rect;     // Zone cliquable du roller texte 1
    SDL_Rect seq1_count_rect;    // Zone cliquable du roller chiffre 1
    bool seq1_type_hovered;      // Hover sur roller texte 1
    bool seq1_count_hovered;     // Hover sur roller chiffre 1

    // Séquence 2 (deuxième partie du pattern)
    int seq2_type;               // 0 = pleins, 1 = vides
    int seq2_count;              // Nombre de sessions (1-10)
    SDL_Rect seq2_type_rect;     // Zone cliquable du roller texte 2
    SDL_Rect seq2_count_rect;    // Zone cliquable du roller chiffre 2
    bool seq2_type_hovered;      // Hover sur roller texte 2
    bool seq2_count_hovered;     // Hover sur roller chiffre 2

    // Callback spécial pour le mode roller (4 paramètres)
    void (*roller_callback)(int seq1_type, int seq1_count, int seq2_type, int seq2_count);

} SelectorWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES
// ════════════════════════════════════════════════════════════════════════════

/**
 * Crée un nouveau widget selector
 */
SelectorWidget* create_selector_widget(const char* nom_affichage, int x, int y,
                                       int default_index, int arrow_size, int text_size,
                                       TTF_Font* font);

/**
 * Ajoute une option au sélecteur
 */
bool add_selector_option(SelectorWidget* widget, const char* option_text, const char* callback_name);

/**
 * Définit le callback pour une option spécifique
 */
void set_selector_option_callback(SelectorWidget* widget, int option_index, void (*callback)(void));

/**
 * Change l'option sélectionnée (et appelle le callback associé)
 */
void set_selector_value(SelectorWidget* widget, int new_index);

/**
 * Navigue vers l'option précédente
 */
void selector_previous_option(SelectorWidget* widget);

/**
 * Navigue vers l'option suivante
 */
void selector_next_option(SelectorWidget* widget);

/**
 * Rend le widget à l'écran
 */
void render_selector_widget(SDL_Renderer* renderer, SelectorWidget* widget,
                            int offset_x, int offset_y);

/**
 * Gère les événements souris du widget
 */
void handle_selector_widget_events(SelectorWidget* widget, SDL_Event* event,
                                   int offset_x, int offset_y);

/**
 * Recalcule les positions du widget selon le ratio du panneau
 */
void rescale_selector_widget(SelectorWidget* widget, float panel_ratio);

/**
 * Met à jour l'animation du widget
 */
void update_selector_animation(SelectorWidget* widget, float delta_time);

/**
 * Met à jour l'animation du sous-menu (slide down/up)
 */
void update_selector_submenu_animation(SelectorWidget* widget, float delta_time);

/**
 * Libère la mémoire du widget
 */
void free_selector_widget(SelectorWidget* widget);

#endif // __SELECTOR_WIDGET_H__
