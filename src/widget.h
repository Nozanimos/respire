#ifndef __WIDGET_H__
#define __WIDGET_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "geometry.h"

// Structure pour un widget de configuration
typedef struct ConfigWidget {
    char option_name[50];
    int value;
    int min_value;
    int max_value;
    int increment;

    // Éléments graphiques
    Triangle* down_arrow;
    Triangle* up_arrow;

    // Style
    SDL_Color color;
    SDL_Color hover_color;
    SDL_Color text_color;

    // Position et état
    int x, y;
    int arrow_size;
    int text_size;
    bool up_hovered;
    bool down_hovered;

    // Stocker les positions calculées
    int name_x, arrows_x, value_x;
    int text_center_y;

    // Callback pour les changements de valeur
    void (*on_value_changed)(int new_value);

} ConfigWidget;

// Prototypes
ConfigWidget* create_config_widget(const char* name, int x, int y, int min_val, int max_val,
                                   int start_val, int increment, int arrow_size, int text_size, TTF_Font* font);
void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget, TTF_Font* font);
void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event);
void free_config_widget(ConfigWidget* widget);
void set_widget_value_changed_callback(ConfigWidget* widget, void (*callback)(int));

// Helper functions (déclarées ici pour être utilisées dans les widgets)
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, Uint32 color);
bool is_point_in_rect(int x, int y, SDL_Rect rect);

#endif
