#ifndef __TOGGLE_WIDGET_H__
#define __TOGGLE_WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "widget_base.h"

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE D'UN WIDGET TOGGLE (ON/OFF)
// ════════════════════════════════════════════════════════════════════════════
// Widget interactif de type interrupteur avec animation visuelle
//
// LAYOUT VISUEL :
//   [option_name]  [◯---] → [option_name]  [---●]
//        ↑             ↑           ↑           ↑
//   local_text_x  local_toggle_x (OFF)    (ON avec animation)
//
// ARCHITECTURE :
//   - base : conteneur avec position (x,y) relative au panneau
//   - Tous les éléments en coordonnées LOCALES (offsets)
//   - Position écran = panneau.x + base.x + local_x
//
// RESCALING INTELLIGENT :
//   - Change la taille de la police selon le ratio
//   - Remesure le texte avec TTF_SizeUTF8()
//   - Recalcule les offsets proportionnellement
//
// ANIMATION :
//   - Cercle blanc qui glisse de gauche à droite
//   - Fond vert quand actif, gris quand inactif
//   - Transition fluide avec interpolation

typedef struct ToggleWidget {
    // ─────────────────────────────────────────────────────────────────────────
    // BASE DU WIDGET (héritage de WidgetBase)
    // ─────────────────────────────────────────────────────────────────────────
    WidgetBase base;             // Position, dimensions, état

    // ─────────────────────────────────────────────────────────────────────────
    // IDENTITÉ ET VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    char option_name[50];        // Nom affiché (ex: "Cycles alternés")
    bool value;                  // Valeur actuelle (true = ON, false = OFF)

    // ─────────────────────────────────────────────────────────────────────────
    // CONFIGURATION DE LA POLICE (pour rescaling intelligent)
    // ─────────────────────────────────────────────────────────────────────────
    int base_text_size;          // Taille de police de référence (scale 1.0)
    int current_text_size;       // Taille actuelle après scaling

    // ─────────────────────────────────────────────────────────────────────────
    // ESPACEMENT DE BASE (pour rescaling proportionnel)
    // ─────────────────────────────────────────────────────────────────────────
    int base_espace_apres_texte; // Marge texte → toggle (ex: 20px)

    // ─────────────────────────────────────────────────────────────────────────
    // LAYOUT INTERNE (coordonnées LOCALES au widget)
    // ─────────────────────────────────────────────────────────────────────────
    int local_text_x;            // Offset du texte (généralement 0)
    int local_text_y;            // Offset vertical du texte
    int local_toggle_x;          // Offset du bouton toggle
    int local_toggle_y;          // Offset vertical du toggle

    // ─────────────────────────────────────────────────────────────────────────
    // DIMENSIONS DES SOUS-ÉLÉMENTS
    // ─────────────────────────────────────────────────────────────────────────
    int toggle_width;            // Largeur du bouton toggle
    int toggle_height;           // Hauteur du bouton toggle
    int thumb_size;              // Diamètre du curseur circulaire
    int text_height;             // Hauteur du texte

    // Dimensions de base pour rescaling
    int base_toggle_width;
    int base_toggle_height;
    int base_thumb_size;

    // ─────────────────────────────────────────────────────────────────────────
    // POSITION DU THUMB DANS LE TOGGLE (coordonnées LOCALES au toggle)
    // ─────────────────────────────────────────────────────────────────────────
    int thumb_local_x;           // Position X du thumb dans le toggle
    int thumb_local_y;           // Position Y du thumb dans le toggle (centré)

    // ─────────────────────────────────────────────────────────────────────────
    // ANIMATION
    // ─────────────────────────────────────────────────────────────────────────
    float animation_progress;    // Progression de l'animation (0.0 à 1.0)
    bool is_animating;           // TRUE si animation en cours

    // ─────────────────────────────────────────────────────────────────────────
    // STYLE ET COULEURS
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color bg_off_color;      // Couleur de fond OFF (gris)
    SDL_Color bg_on_color;       // Couleur de fond ON (vert)
    SDL_Color thumb_color;       // Couleur du curseur (blanc)
    SDL_Color text_color;        // Couleur du texte
    SDL_Color bg_hover_color;    // Couleur de fond au survol

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION (spécifique au toggle)
    // ─────────────────────────────────────────────────────────────────────────
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

// Rend le widget à l'écran (la police doit être à current_text_size)
void render_toggle_widget(SDL_Renderer* renderer, ToggleWidget* widget, TTF_Font* font,
                          int offset_x, int offset_y);

// Gère les événements souris du widget
void handle_toggle_widget_events(ToggleWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y);

// Bascule la valeur du widget (avec animation)
void toggle_widget_value(ToggleWidget* widget);

// Définit le callback appelé quand la valeur change
void set_toggle_value_changed_callback(ToggleWidget* widget, void (*callback)(bool));

// Recalcule les positions du widget selon le ratio du panneau
// REMESURE le texte avec la nouvelle taille de police
void rescale_toggle_widget(ToggleWidget* widget, float panel_ratio);

// Libère la mémoire du widget
void free_toggle_widget(ToggleWidget* widget);

#endif // __TOGGLE_WIDGET_H__
