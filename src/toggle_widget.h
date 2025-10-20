#ifndef __TOGGLE_WIDGET_H__
#define __TOGGLE_WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE D'UN WIDGET TOGGLE (ON/OFF)
// ════════════════════════════════════════════════════════════════════════════
// Widget interactif de type interrupteur avec animation visuelle
//
// LAYOUT VISUEL :
//   [option_name]  [◯---] → [option_name]  [---●]
//        ↑             ↑           ↑           ↑
//      name_x      toggle_x     name_x     toggle_x (actif)
//
// ANIMATION :
//   - Cercle blanc qui glisse de gauche à droite
//   - Fond vert quand actif, gris quand inactif
//   - Transition fluide avec interpolation
typedef struct ToggleWidget {
    // ─────────────────────────────────────────────────────────────────────────
    // IDENTITÉ ET VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    char option_name[50];        // Nom affiché (ex: "Cycles alternés")
    bool value;                  // Valeur actuelle (true = ON, false = OFF)

    // ─────────────────────────────────────────────────────────────────────────
    // ÉLÉMENTS GRAPHIQUES ET ANIMATION
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Rect toggle_rect;        // Rectangle du bouton toggle
    SDL_Rect thumb_rect;         // Rectangle du curseur (cercle)
    float animation_progress;    // Progression de l'animation (0.0 à 1.0)
    bool is_animating;           // TRUE si animation en cours

    // ─────────────────────────────────────────────────────────────────────────
    // STYLE ET COULEURS
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color bg_off_color;      // Couleur de fond OFF (gris)
    SDL_Color bg_on_color;       // Couleur de fond ON (vert)
    SDL_Color thumb_color;       // Couleur du curseur (blanc)
    SDL_Color text_color;        // Couleur du texte
    SDL_Color bg_hover_color;    // Couleur du fond au survol (transparent)

    // ─────────────────────────────────────────────────────────────────────────
    // POSITION ET DIMENSIONS (RELATIVES AU CONTENEUR)
    // ─────────────────────────────────────────────────────────────────────────
    int x, y;                    // Position de base (coin sup. gauche)
    int name_x;                  // Position X du début du texte
    int toggle_x;                // Position X du toggle
    int text_center_y;           // Centre vertical du texte
    int text_height;             // Hauteur du texte

    // Dimensions fixes du toggle
    int toggle_width;            // Largeur totale du bouton toggle
    int toggle_height;           // Hauteur totale du bouton toggle
    int thumb_size;              // Diamètre du curseur

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION
    // ─────────────────────────────────────────────────────────────────────────
    bool whole_widget_hovered;   // TRUE si souris sur le widget
    bool toggle_hovered;         // TRUE si souris sur le bouton toggle

    // ─────────────────────────────────────────────────────────────────────────
    // CALLBACK
    // ─────────────────────────────────────────────────────────────────────────
    void (*on_value_changed)(bool new_value);  // Appelé à chaque basculement

} ToggleWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS
// ════════════════════════════════════════════════════════════════════════════

// Crée un nouveau widget toggle
ToggleWidget* create_toggle_widget(const char* name, int x, int y, bool start_state,
                                   int toggle_width, int toggle_height, int thumb_size,
                                   int text_size, TTF_Font* font);

// Met à jour l'animation du widget
void update_toggle_widget(ToggleWidget* widget, float delta_time);

// Rend le widget à l'écran
void render_toggle_widget(SDL_Renderer* renderer, ToggleWidget* widget, TTF_Font* font,
                          int offset_x, int offset_y);

// Gère les événements souris du widget
void handle_toggle_widget_events(ToggleWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y);

// Bascule la valeur du widget (avec animation)
void toggle_widget_value(ToggleWidget* widget);

// Définit le callback appelé quand la valeur change
void set_toggle_value_changed_callback(ToggleWidget* widget, void (*callback)(bool));

// Libère la mémoire du widget
void free_toggle_widget(ToggleWidget* widget);

#endif
