#ifndef __BUTTON_WIDGET_H__
#define __BUTTON_WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════════════════════
//  ENUMÉRATION POUR L'ANCRAGE VERTICAL
// ════════════════════════════════════════════════════════════════════════════
typedef enum {
    BUTTON_ANCHOR_TOP,      // Position Y relative au haut du panneau
    BUTTON_ANCHOR_BOTTOM    // Position Y relative au bas du panneau
} ButtonYAnchor;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE DU WIDGET BUTTON
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    // ─────────────────────────────────────────────────────────────────────────
    // BASE (position et dimensions relatives)
    // ─────────────────────────────────────────────────────────────────────────
    struct {
        int x, y;              // Position relative au panneau
        int width, height;     // Dimensions actuelles
        bool is_hovered;       // État de survol
    } base;

    // ─────────────────────────────────────────────────────────────────────────
    // TEXTE ET STYLE
    // ─────────────────────────────────────────────────────────────────────────
    char text[128];                  // Texte du bouton
    SDL_Color bg_color;              // Couleur de fond normale
    SDL_Color hover_color;           // Couleur de fond au survol
    SDL_Color text_color;            // Couleur du texte
    SDL_Color border_color;          // Couleur de la bordure
    int border_radius;               // Rayon des coins arrondis

    // ─────────────────────────────────────────────────────────────────────────
    // POLICE
    // ─────────────────────────────────────────────────────────────────────────
    int base_text_size;              // Taille de police de base
    int current_text_size;           // Taille de police actuelle (après scaling)

    // ─────────────────────────────────────────────────────────────────────────
    // DIMENSIONS ET POSITION DE BASE (pour rescaling)
    // ─────────────────────────────────────────────────────────────────────────
    int base_width;
    int base_height;
    int base_x;                      // Position X de base (du JSON)
    int base_y;                      // Position Y de base (du JSON)
    ButtonYAnchor y_anchor;          // Ancrage vertical (TOP ou BOTTOM)

    // ─────────────────────────────────────────────────────────────────────────
    // CALLBACK DE CLIC
    // ─────────────────────────────────────────────────────────────────────────
    void (*on_click)(void);          // Fonction appelée lors du clic

} ButtonWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES
// ════════════════════════════════════════════════════════════════════════════

// Crée un nouveau bouton
// x, y : position du CENTRE du bouton (relatif au panneau)
// width, height : dimensions du bouton
// text_size : taille de la police
// bg_color : couleur de fond
// y_anchor : ancrage vertical (TOP ou BOTTOM)
ButtonWidget* create_button_widget(const char* text, int x, int y,
                                   int width, int height, int text_size,
                                   SDL_Color bg_color, ButtonYAnchor y_anchor);

// Rendu du bouton
// offset_x, offset_y : offset du panneau parent
void render_button_widget(SDL_Renderer* renderer, ButtonWidget* button,
                          int offset_x, int offset_y);

// Gestion des événements (survol et clic)
// offset_x, offset_y : offset du panneau parent
void handle_button_widget_events(ButtonWidget* button, SDL_Event* event,
                                 int offset_x, int offset_y);

// Rescaling du bouton selon le ratio du panneau
void rescale_button_widget(ButtonWidget* button, float panel_ratio);

// Définit le callback de clic
void set_button_click_callback(ButtonWidget* button, void (*callback)(void));

// Libère le bouton
void free_button_widget(ButtonWidget* button);

#endif // __BUTTON_WIDGET_H__
