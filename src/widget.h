#ifndef __WIDGET_H__
#define __WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "widget_base.h"

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE D'UN WIDGET D'INCRÉMENTATION (ConfigWidget)
// ════════════════════════════════════════════════════════════════════════════
// Widget interactif permettant d'incrémenter/décrémenter une valeur numérique
//
// LAYOUT VISUEL :
//   [option_name]     ▲ ▼     [value]
//        ↑             ↑         ↑
//   local_text_x  local_arrows_x local_value_x
//
// ARCHITECTURE :
//   - base : conteneur avec position (x,y) relative au panneau
//   - Tous les éléments internes en coordonnées LOCALES (offsets)
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
    int value;                   // Valeur actuelle
    int min_value, max_value;    // Limites de la valeur
    int increment;               // Pas d'incrémentation

    // ─────────────────────────────────────────────────────────────────────────
    // CONFIGURATION DE LA POLICE (pour rescaling intelligent)
    // ─────────────────────────────────────────────────────────────────────────
    int base_text_size;          // Taille de police de référence (scale 1.0)
    int current_text_size;       // Taille actuelle après scaling

    // ─────────────────────────────────────────────────────────────────────────
    // ESPACEMENTS DE BASE (pour rescaling proportionnel)
    // ─────────────────────────────────────────────────────────────────────────
    // Ces valeurs définissent les marges entre les éléments à scale 1.0
    // Elles sont scalées proportionnellement lors du rescaling
    int base_espace_apres_texte;     // Marge texte → flèches (ex: 20px)
    int base_espace_entre_fleches;   // Espace vertical ▲ ↔ ▼ (ex: 5px)
    int base_espace_apres_fleches;   // Marge flèches → valeur (ex: 15px)

    // ─────────────────────────────────────────────────────────────────────────
    // LAYOUT INTERNE (coordonnées LOCALES au widget)
    // ─────────────────────────────────────────────────────────────────────────
    // Ces valeurs sont des OFFSETS par rapport à (base.x, base.y)
    // Elles sont RECALCULÉES lors du rescaling en remésurant le texte
    int local_text_x;            // Offset du texte (généralement 0)
    int local_text_y;            // Offset vertical du texte
    int local_arrows_x;          // Offset des flèches
    int local_arrows_y;          // Offset vertical des flèches (centre)
    int local_value_x;           // Offset de la valeur affichée
    int local_value_y;           // Offset vertical de la valeur

    // ─────────────────────────────────────────────────────────────────────────
    // DIMENSIONS DES SOUS-ÉLÉMENTS
    // ─────────────────────────────────────────────────────────────────────────
    int arrow_size;              // Taille des triangles (base et hauteur)
    int base_arrow_size;         // Taille de référence pour rescaling
    int text_height;             // Hauteur du texte (pour centrage)

    // ─────────────────────────────────────────────────────────────────────────
    // STYLE ET COULEURS
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color color;             // Couleur des flèches et du texte
    SDL_Color hover_color;       // Couleur au survol (jaune pâle)
    SDL_Color bg_hover_color;    // Couleur de fond au survol

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION (spécifique aux sous-éléments)
    // ─────────────────────────────────────────────────────────────────────────
    bool up_arrow_hovered;       // TRUE si souris sur flèche haut
    bool down_arrow_hovered;     // TRUE si souris sur flèche bas

    // ─────────────────────────────────────────────────────────────────────────
    // CALLBACK
    // ─────────────────────────────────────────────────────────────────────────
    void (*on_value_changed)(int new_value);  // Appelé à chaque changement

} ConfigWidget;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS
// ════════════════════════════════════════════════════════════════════════════

// Crée un nouveau widget de configuration
ConfigWidget* create_config_widget(const char* name, int x, int y,
                                   int min_val, int max_val, int start_val,
                                   int increment, int arrow_size, int text_size,
                                   TTF_Font* font);

// Met à jour le widget (animations, états)
void update_config_widget(ConfigWidget* widget, float delta_time);

// Rend le widget à l'écran (la police doit être à current_text_size)
void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget,
                          int offset_x, int offset_y);

// Gère les événements souris du widget
void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y);

// Définit le callback appelé quand la valeur change
void set_config_value_changed_callback(ConfigWidget* widget,
                                       void (*callback)(int));

// Recalcule les positions du widget selon le ratio du panneau
// REMESURE le texte avec la nouvelle taille de police
void rescale_config_widget(ConfigWidget* widget, float panel_ratio);

// Libère la mémoire du widget
void free_config_widget(ConfigWidget* widget);

#endif // __WIDGET_H__
