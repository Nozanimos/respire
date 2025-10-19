#ifndef __WIDGET_H__
#define __WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "geometry.h"

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE D'UN WIDGET DE CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════
// Widget interactif pour modifier une valeur numérique via flèches ou molette
//
// LAYOUT VISUEL :
//   [option_name]  ↑↓  [value]
//        ↑         ↑    ↑
//      name_x   arrows_x value_x
//
// COORDONNÉES :
//   Toutes les positions (x, y, name_x, etc.) sont RELATIVES au conteneur
//   parent (panneau). Les fonctions de rendu/événements reçoivent l'offset
//   pour les convertir en coordonnées absolues.
typedef struct ConfigWidget {
    // ─────────────────────────────────────────────────────────────────────────
    // IDENTITÉ ET VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    char option_name[50];    // Nom affiché (ex: "Durée respiration")
    int value;               // Valeur actuelle
    int min_value;           // Valeur minimale autorisée
    int max_value;           // Valeur maximale autorisée
    int increment;           // Pas d'incrémentation (généralement 1)

    // ─────────────────────────────────────────────────────────────────────────
    // ÉLÉMENTS GRAPHIQUES
    // ─────────────────────────────────────────────────────────────────────────
    Triangle* down_arrow;    // Flèche bas (décrémenter)
    Triangle* up_arrow;      // Flèche haut (incrémenter)

    // ─────────────────────────────────────────────────────────────────────────
    // STYLE ET COULEURS
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color color;         // Couleur normale des flèches
    SDL_Color hover_color;   // Couleur des flèches au survol
    SDL_Color text_color;    // Couleur du texte
    SDL_Color bg_hover_color;// Couleur du fond au survol

    // ─────────────────────────────────────────────────────────────────────────
    // POSITION ET DIMENSIONS (RELATIVES AU CONTENEUR)
    // ─────────────────────────────────────────────────────────────────────────
    int x, y;                // Position de base (coin sup. gauche)
    int name_x;              // Position X du début du texte
    int arrows_x;            // Position X des flèches
    int up_arrow_y;          // Position Y de la flèche haut (NOUVEAU)
    int down_arrow_y;        // Position Y de la flèche bas (NOUVEAU)
    int value_x;             // Position X de la valeur numérique
    int text_center_y;       // Centre vertical du texte (pour aligner flèches)
    int text_height;         // Hauteur du texte (pour le fond de survol)

    int arrow_size;          // Taille des flèches en pixels
    int text_size;           // Taille de référence du texte

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION
    // ─────────────────────────────────────────────────────────────────────────
    bool whole_widget_hovered; // TRUE si souris sur le widget (pour molette)
    bool up_hovered;           // TRUE si souris sur flèche haut
    bool down_hovered;         // TRUE si souris sur flèche bas

    // ─────────────────────────────────────────────────────────────────────────
    // CALLBACK
    // ─────────────────────────────────────────────────────────────────────────
    void (*on_value_changed)(int new_value);  // Appelé à chaque modification

} ConfigWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS
// ════════════════════════════════════════════════════════════════════════════

// Crée un nouveau widget de configuration
ConfigWidget* create_config_widget(const char* name, int x, int y, int min_val, int max_val,
                                   int start_val, int increment, int arrow_size, int text_size,
                                   TTF_Font* font);

// Rend le widget à l'écran
// offset_x/y : position absolue du conteneur parent
void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget, TTF_Font* font,
                          int offset_x, int offset_y);

// Gère les événements souris du widget
// offset_x/y : position absolue du conteneur parent
void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y);

// Définit le callback appelé quand la valeur change
void set_widget_value_changed_callback(ConfigWidget* widget, void (*callback)(int));

// Libère la mémoire du widget
void free_config_widget(ConfigWidget* widget);

// ════════════════════════════════════════════════════════════════════════════
//  FONCTIONS HELPER (déclarées ici pour être utilisables par les widgets)
// ════════════════════════════════════════════════════════════════════════════

// Affiche du texte à l'écran
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, Uint32 color);

// Vérifie si un point est dans un rectangle
bool is_point_in_rect(int x, int y, SDL_Rect rect);

#endif
