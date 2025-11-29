// SPDX-License-Identifier: GPL-3.0-or-later
// settings_panel.h
#ifndef __SETTINGS_PANEL_H__
#define __SETTINGS_PANEL_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include "config.h"
#include "precompute_list.h"
#include "widget.h"
#include "toggle_widget.h"
#include "widget_list.h"

typedef enum {
    PANEL_CLOSED,
    PANEL_OPENING,
    PANEL_OPEN,
    PANEL_CLOSING
} PanelState;

// === STRUCTURES UI ===
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

// === STRUCTURE PRÉVISUALISATION ===
typedef struct {
    HexagoneList* hex_list;
    int center_x, center_y;
    int container_size;
    float size_ratio;
    Uint32 last_update;
    double current_time;
    int frame_x, frame_y;
} PreviewSystem;

// === STRUCTURE DU PANNEAU DE CONFIGURATION ===
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

    // ════════════════════════════════════════════════════════════════════════
    // FACTEUR D'ÉCHELLE RESPONSIVE
    // ════════════════════════════════════════════════════════════════════════
    // Copie du scale_factor de AppState pour faciliter l'accès
    // Mis à jour lors du redimensionnement de la fenêtre
    // ════════════════════════════════════════════════════════════════════════
    float scale_factor;
    float panel_ratio;

    // Éléments UI
    AppConfig temp_config;
    TTF_Font* font_title;
    TTF_Font* font;
    TTF_Font* font_small;

    WidgetList* widget_list;

    UIButton apply_button;
    UIButton cancel_button;

    // Système de prévisualisation
    PreviewSystem preview_system;

    // ═══════════════════════════════════════════════════════════════════════════
    // HOT RELOAD DU JSON ET GESTION FENÊTRE
    // ═══════════════════════════════════════════════════════════════════════════
    const char* json_config_path;   // Chemin vers widgets_config.json
    time_t last_json_mtime;         // Timestamp de dernière modification
    float json_check_interval;      // Intervalle de vérification (secondes)
    float time_since_last_check;    // Temps écoulé depuis dernière vérification
    SDL_Renderer* renderer;         // Nécessaire pour recharger les widgets
    SDL_Window* window;             // Nécessaire pour SDL_SetWindowMinimumSize
    int screen_width;               // Largeur de l'écran (mise à jour lors du resize)
    int screen_height;              // Hauteur de l'écran (mise à jour lors du resize)

    // ═══════════════════════════════════════════════════════════════════════════
    // POSITIONS DES BARRES DE SÉPARATION (RESPONSIVE)
    // ═══════════════════════════════════════════════════════════════════════════
    // Stockées ici pour suivre le panel_ratio, comme le preview
    int separator_y;          // Position Y de la barre (scalée avec panel_ratio)
    int separator_start_x;    // Position X de début de barre
    int separator_end_x;      // Position X de fin de barre

    // ═══════════════════════════════════════════════════════════════════════════
    // SCROLL VERTICAL ET LAYOUT RESPONSIVE
    // ═══════════════════════════════════════════════════════════════════════════
    int scroll_offset;        // Décalage vertical du scroll (en pixels)
    int content_height;       // Hauteur totale du contenu (calculée automatiquement)
    int max_scroll;           // Scroll maximum possible
    bool layout_mode_column;  // true = mode colonne (étroit), false = mode 2 colonnes (large)
    int layout_threshold_width; // Largeur en dessous de laquelle on passe en mode colonne
    bool widgets_stacked;     // true = widgets empilés (fenêtre réduite), false = positions originales
    int min_width_for_unstack; // Largeur minimale calculée pour dépiler (bbox des widgets JSON)

    // ═══════════════════════════════════════════════════════════════════════════
    // MÉMOIRE DE LA LARGEUR AU MOMENT DE L'EMPILEMENT
    // ═══════════════════════════════════════════════════════════════════════════
    // Sauvegarde de panel_width AU MOMENT où on empile les widgets
    // Pour dépiler, on vérifie si panel_width >= panel_width_when_stacked + MARGE
    // Cela évite la boucle infinie de pile/dépile
    // ═══════════════════════════════════════════════════════════════════════════
    int panel_width_when_stacked;  // Largeur du panneau au moment de l'empilement (0 = jamais empilé)
    bool layout_dirty;             // Flag pour recalculer le layout (évite recalculs multiples par frame)

    // Anciens éléments (à supprimer progressivement)
    SDL_Texture* apply_button_texture;
    SDL_Texture* cancel_button_texture;
    SDL_Rect apply_button_rect;
    SDL_Rect cancel_button_rect;

} SettingsPanel;

// Prédéclarations des types (pour éviter include circulaire)
typedef struct TimerState TimerState;
typedef struct StopwatchState StopwatchState;
typedef struct CounterState BreathCounter;

// PROTOTYPES
SettingsPanel* create_settings_panel(SDL_Renderer* renderer, SDL_Window* window, int screen_width, int screen_height, float scale_factor);
void update_settings_panel(SettingsPanel* panel, float delta_time);
void render_settings_panel(SDL_Renderer* renderer, SettingsPanel* panel);
void handle_settings_panel_event(SettingsPanel* panel, SDL_Event* event, AppConfig* main_config);
void free_settings_panel(SettingsPanel* panel);

// Définir les pointeurs vers timers/compteurs/hexagones pour mise à jour lors de "Appliquer"
void set_timers_for_callbacks(TimerState** session_timer, StopwatchState** session_stopwatch,
                              TimerState** retention_timer, BreathCounter** breath_counter,
                              int* total_sessions, HexagoneList** hexagones,
                              int* screen_width, int* screen_height);

// PROTOTYPES POUR LA PRÉVISUALISATION
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
bool is_point_in_rect(int x, int y, SDL_Rect rect);

// Adaptation d'échelle
void update_panel_scale(SettingsPanel* panel, int screen_width, int screen_height, float scale_factor);

// Hot reload du JSON
void reload_widgets_from_json(SettingsPanel* panel, int screen_width, int screen_height);
void check_json_hot_reload(SettingsPanel* panel, float delta_time, int screen_width, int screen_height);

// Calcul de la largeur minimale de fenêtre
int get_minimum_window_width(SettingsPanel* panel);
void update_window_minimum_size(SettingsPanel* panel, SDL_Window* window);

// Layout responsive et scroll
void recalculate_widget_layout(SettingsPanel* panel);
void handle_panel_scroll(SettingsPanel* panel, SDL_Event* event);

// Fonction pour la version finale (hardcodée, sans JSON)
// À décommenter quand on basculera vers la version sans JSON Editor
// void init_widgets_hardcoded(SettingsPanel* panel);

#endif
