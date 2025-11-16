// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __WIDGET_H__
#define __WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "widget_base.h"

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE D'UN WIDGET D'INCRÉMENTATION (ConfigWidget) - STYLE ROLLER
// ════════════════════════════════════════════════════════════════════════════
// Widget interactif type "roller" mobile permettant d'incrémenter/décrémenter
//
// LAYOUT VISUEL :
//   [option_name]     [🎚️ valeur/mm:ss]
//        ↑                    ↑
//   local_text_x         roller_rect
//
// MODES D'AFFICHAGE :
//   - "numeric" : Affichage numérique classique (ex: 45)
//   - "time"    : Affichage temps mm:ss (ex: 01:30)
//
// INTERACTIONS :
//   - MOLETTE : incrémente/décrémente
//   - DRAG HORIZONTAL (numeric) : glissement pour changer valeur
//   - CLIC GAUCHE/DROITE (numeric) : -1/+1
//   - CLIC HAUT/BAS (time) : -1/+1 sur le champ survolé (mm ou ss)
//
// ARCHITECTURE :
//   - base : conteneur avec position (x,y) relative au panneau
//   - roller_rect : zone interactive du roller
//   - Position écran = panneau.x + base.x + local_x
//
// RESCALING INTELLIGENT :
//   - Change la taille de la police selon le ratio
//   - Remesure le texte avec TTF_SizeUTF8()
//   - Recalcule les offsets proportionnellement

typedef struct ConfigWidget {
    // ─────────────────────────────────────────────────────────────────────────
    // BASE DU WIDGET (héritage de WidgetBase)
    // ─────────────────────────────────────────────────────────────────────────
    WidgetBase base;             // Position, dimensions, état

    // ─────────────────────────────────────────────────────────────────────────
    // IDENTITÉ ET VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    char option_name[50];        // Nom affiché (ex: "Durée respiration")
    int value;                   // Valeur actuelle (stockée en secondes pour type "time")
    int min_value, max_value;    // Limites de la valeur
    int increment;               // Pas d'incrémentation
    char widget_display_type[16];// Type d'affichage : "numeric" ou "time"

    // ─────────────────────────────────────────────────────────────────────────
    // CONFIGURATION DE LA POLICE (pour rescaling intelligent)
    // ─────────────────────────────────────────────────────────────────────────
    int base_text_size;          // Taille de police de référence (scale 1.0)
    int current_text_size;       // Taille actuelle après scaling

    // ─────────────────────────────────────────────────────────────────────────
    // ESPACEMENTS DE BASE (pour rescaling proportionnel)
    // ─────────────────────────────────────────────────────────────────────────
    int base_espace_apres_texte;     // Marge texte → roller (ex: 20px)
    int base_roller_padding;         // Padding interne du roller (ex: 8px)

    // ─────────────────────────────────────────────────────────────────────────
    // LAYOUT INTERNE (coordonnées LOCALES au widget)
    // ─────────────────────────────────────────────────────────────────────────
    int local_text_x;            // Offset du texte (généralement 0)
    int local_text_y;            // Offset vertical du texte
    int local_roller_x;          // Offset du roller
    int local_roller_y;          // Offset vertical du roller
    SDL_Rect roller_rect;        // Rectangle du roller (coordonnées LOCALES)

    // ─────────────────────────────────────────────────────────────────────────
    // DIMENSIONS
    // ─────────────────────────────────────────────────────────────────────────
    int text_height;             // Hauteur du texte (pour centrage)
    int roller_width;            // Largeur du roller (calculée dynamiquement)
    int roller_height;           // Hauteur du roller (= text_height)

    // ─────────────────────────────────────────────────────────────────────────
    // STYLE ET COULEURS (nouveau design)
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color color;             // Couleur du texte du label
    SDL_Color roller_bg_color;   // Fond blanc alpha 200 : {255, 255, 255, 200}
    SDL_Color roller_text_color; // Bleu-gris foncé alpha 255 : {70, 80, 100, 255}
    SDL_Color roller_border_color; // Bordure du roller

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION DRAG
    // ─────────────────────────────────────────────────────────────────────────
    bool is_dragging;            // TRUE si drag en cours
    int drag_start_x;            // Position X de départ du drag
    int drag_start_value;        // Valeur de départ du drag

    // ─────────────────────────────────────────────────────────────────────────
    // MODE TIME : Gestion mm:ss
    // ─────────────────────────────────────────────────────────────────────────
    int selected_field;          // 0 = minutes, 1 = secondes (pour hover/interaction)
    SDL_Rect mm_rect;            // Zone des minutes (pour hover)
    SDL_Rect ss_rect;            // Zone des secondes (pour hover)

    // ─────────────────────────────────────────────────────────────────────────
    // CALLBACK
    // ─────────────────────────────────────────────────────────────────────────
    void (*on_value_changed)(int new_value);  // Appelé à chaque changement

} ConfigWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS
// ════════════════════════════════════════════════════════════════════════════

// Crée un nouveau widget de configuration (style roller)
// display_type : "numeric" ou "time" (NULL = "numeric" par défaut)
ConfigWidget* create_config_widget(const char* name, int x, int y,
                                   int min_val, int max_val, int start_val,
                                   int increment, int arrow_size, int text_size,
                                   TTF_Font* font, const char* display_type);

// Met à jour le widget (animations, états)
void update_config_widget(ConfigWidget* widget, float delta_time);

// Rend le widget à l'écran (la police doit être à current_text_size)
// Si container_width > 0, les flèches+valeur sont alignées à droite
void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget,
                          int offset_x, int offset_y, int container_width);

// Gère les événements souris du widget
// Si container_width > 0, utilise la même logique d'alignement que le rendu
void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y, int container_width);

// Définit le callback appelé quand la valeur change
void set_config_value_changed_callback(ConfigWidget* widget,
                                       void (*callback)(int));

// Recalcule les positions du widget selon le ratio du panneau
// REMESURE le texte avec la nouvelle taille de police
void rescale_config_widget(ConfigWidget* widget, float panel_ratio);

// Libère la mémoire du widget
void free_config_widget(ConfigWidget* widget);

#endif // __WIDGET_H__
