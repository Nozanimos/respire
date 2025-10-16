// settings_panel.h
#ifndef __SETTINGS_PANEL_H__
#define __SETTINGS_PANEL_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdbool.h>
#include "config.h"
#include "hexagone_list.h"  // AJOUT

typedef enum {
    PANEL_CLOSED,
    PANEL_OPENING,
    PANEL_OPEN,
    PANEL_CLOSING
} PanelState;

// === STRUCTURES UI EXISTANTES ===
typedef struct {
    int min_value;
    int max_value;
    int current_value;
    SDL_Rect track_rect;
    SDL_Rect thumb_rect;
    bool is_dragging;
} Slider;

typedef struct {
    char text[50];
    SDL_Rect rect;
    SDL_Texture* texture;
    bool is_hovered;
} UIButton;

// === NOUVELLE STRUCTURE PRÉVISUALISATION ===
typedef struct {
    HexagoneList* hex_list;
    int center_x, center_y;
    int container_size;
    float size_ratio;
    Uint32 last_update;
    double current_time;
    int frame_x, frame_y;
} PreviewSystem;

typedef struct {
    PanelState state;
    SDL_Rect rect;
    SDL_Texture* background;
    SDL_Texture* gear_icon;
    SDL_Rect gear_rect;

    // Position animation
    int target_x;
    int current_x;
    float animation_progress;

    // Éléments UI
    AppConfig temp_config;
    TTF_Font* font_title;
    TTF_Font* font;
    TTF_Font* font_small;
    Slider duration_slider;
    Slider cycles_slider;
    UIButton apply_button;
    UIButton cancel_button;

    // NOUVEAU : Système de prévisualisation
    PreviewSystem preview_system;

    // Anciens éléments (à supprimer progressivement)
    SDL_Texture* apply_button_texture;
    SDL_Texture* cancel_button_texture;
    SDL_Rect apply_button_rect;
    SDL_Rect cancel_button_rect;

} SettingsPanel;

// Prototypes
SettingsPanel* create_settings_panel(SDL_Renderer* renderer, int screen_width, int screen_height);
void update_settings_panel(SettingsPanel* panel, float delta_time);
void render_settings_panel(SDL_Renderer* renderer, SettingsPanel* panel);
void handle_settings_panel_event(SettingsPanel* panel, SDL_Event* event, AppConfig* main_config);
void free_settings_panel(SettingsPanel* panel);

// NOUVEAUX PROTOTYPES POUR LA PRÉVISUALISATION
void reinitialiser_preview_system(PreviewSystem* preview);
void init_preview_system(SettingsPanel* panel, int x, int y, int size, float ratio);
void update_preview_animation(SettingsPanel* panel);
void update_preview_for_new_duration(SettingsPanel* panel, float new_duration);
void render_preview(SDL_Renderer* renderer, PreviewSystem* preview, int offset_x, int offset_y);

// Helper functions
bool is_point_in_rect(int x, int y, SDL_Rect rect);
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, Uint32 color);
UIButton create_button(const char* text, int x, int y, int width, int height);
void render_button(SDL_Renderer* renderer, UIButton* button, TTF_Font* font, int offset_x, int offset_y);
Slider create_slider(int x, int y, int width, int min_val, int max_val, int start_val);
void update_slider_thumb_position(Slider* slider);
void render_slider(SDL_Renderer* renderer, Slider* slider, TTF_Font* font, int offset_x, int offset_y);
bool handle_slider_event(Slider* slider, SDL_Event* event);

#endif
