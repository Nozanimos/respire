// SPDX-License-Identifier: GPL-3.0-or-later
// renderer.h
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "hexagone_list.h"
#include "geometry.h"
#include "config.h"
#include "settings_panel.h"
#include "./json_editor/json_editor.h"

// Structure qui contient TOUT l'état de l'application graphique
typedef struct {
    SDL_Window* window;           // Fenêtre principale
    SDL_Renderer* renderer;       // Moteur de rendu
    SDL_Texture* background;      // Texture de fond
    int screen_width;             // Largeur écran
    int screen_height;            // Hauteur écran

    // ════════════════════════════════════════════════════════════════════════
    // SYSTÈME D'ÉCHELLE RESPONSIVE
    // ════════════════════════════════════════════════════════════════════════
    // Facteur d'échelle calculé automatiquement selon la taille d'écran
    // Référence : 1280x720 (HD Ready) = facteur 1.0
    // Exemples :
    //   - Téléphone 360x640  → scale ≈ 0.28
    //   - Tablette  768x1024 → scale ≈ 0.60
    //   - Desktop   1920x1080 → scale ≈ 1.50
    //   - 4K        3840x2160 → scale = 3.00 (plafonné)
    // ════════════════════════════════════════════════════════════════════════
    float scale_factor;

    // État de l'application
    HexagoneList* hexagones;
    SettingsPanel* settings_panel;
    JsonEditor* json_editor;
    AppConfig config;
    bool is_running;

} AppState;

// Prototypes
bool initialize_app(AppState* app, const char* title, const char* image_path);
void render_hexagones(AppState* app, HexagoneList* hex_list);
void cleanup_app(AppState* app);
void handle_app_events(AppState* app, SDL_Event* event);
void update_app(AppState* app, float delta_time);
void render_app(AppState* app);
void regulate_fps(Uint32 frame_start);

// Fonctions d'échelle responsive
float calculate_scale_factor(int width, int height);
int scale_value(int value, float scale);
int calculate_panel_width(int screen_width, float scale);

#endif
