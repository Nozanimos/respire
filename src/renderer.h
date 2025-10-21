// renderer.h
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "hexagone_list.h"
#include "geometry.h"
#include "config.h"
#include "settings_panel.h"
#include "json_editor_window.h"

// Structure qui contient TOUT l'état de l'application graphique
typedef struct {
    SDL_Window* window;           // Fenêtre principale
    SDL_Renderer* renderer;       // Moteur de rendu
    SDL_Texture* background;      // Texture de fond
    int screen_width;             // Largeur écran
    int screen_height;            // Hauteur écran

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

#endif
