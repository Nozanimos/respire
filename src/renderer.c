// renderer.c
#include "renderer.h"
#include <stdio.h>
#include "config.h"
#include "settings_panel.h"


// Initialise toute la partie SDL et graphique
bool initialize_app(AppState* app, const char* title, const char* image_path) {
    // 1. Initialisation SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("ERREUR SDL_Init: %s", SDL_GetError());
        return false;
    }

    // 2. Création fenêtre plein écran
    app->window = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   0, 0,  // Taille ignorée en FULLSCREEN_DESKTOP
                                   SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!app->window) {
        SDL_Log("ERREUR Fenêtre: %s", SDL_GetError());
        return false;
    }

    // 3. Création renderer
    app->renderer = SDL_CreateRenderer(
        app->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!app->renderer) {
        SDL_Log("ERREUR Renderer: %s", SDL_GetError());
        return false;
    }

    // 4. Chargement image de fond
    SDL_Surface* surface = IMG_Load(image_path);
    if (!surface) {
        SDL_Log("ERREUR Chargement image %s: %s", image_path, SDL_GetError());
        return false;
    }

    app->background = SDL_CreateTextureFromSurface(app->renderer, surface);
    SDL_FreeSurface(surface);
    if (!app->background) {
        SDL_Log("ERREUR Texture: %s", SDL_GetError());
        return false;
    }

    // 5. Récupération taille écran pour usage futur
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    app->screen_width = dm.w;
    app->screen_height = dm.h;

    // 6. Initialisation des autres champs
    app->hexagones = NULL;
    app->is_running = true;
    app->settings_panel = create_settings_panel(app->renderer, app->screen_width, app->screen_height);

    // Chargement de la configuration
    load_config(&app->config);

    printf("Application initialisée: %dx%d\n", app->screen_width, app->screen_height);
    return true;
}

// Gestion des événements de l'application
void handle_app_events(AppState* app, SDL_Event* event) {
    if (!app) return;

    switch (event->type) {
        case SDL_QUIT:
            app->is_running = false;
            break;

        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_ESCAPE) {
                app->is_running = false;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            // Passe l'événement au panneau de configuration
            if (app->settings_panel) {
                handle_settings_panel_event(app->settings_panel, event, &app->config);
            }
            break;
    }
}

// Mise à jour de l'application
void update_app(AppState* app, float delta_time) {
    if (!app) return;

    // Mise à jour des animations hexagones
    if (app->hexagones) {
        HexagoneNode* node = app->hexagones->first;
        while (node) {
            apply_precomputed_frame(node);
            node = node->next;
        }
    }

    // Mise à jour animation panneau
    if (app->settings_panel) {
        update_settings_panel(app->settings_panel, delta_time);
    }
}

// Rendu complet de l'application
void render_app(AppState* app) {
    if (!app || !app->renderer) return;

    // 1. Efface l'écran avec le fond
    SDL_RenderCopy(app->renderer, app->background, NULL, NULL);

    // 2. Dessine tous les hexagones
    if (app->hexagones) {
        HexagoneNode* node = app->hexagones->first;
        while (node) {
            make_hexagone(app->renderer, node->data);
            node = node->next;
        }
    }

    // 3. Dessine le panneau settings (par dessus)
    if (app->settings_panel) {
        render_settings_panel(app->renderer, app->settings_panel);
    }

    // 4. Présentation
    SDL_RenderPresent(app->renderer);
}

// Régulation FPS
void regulate_fps(Uint32 frame_start) {
    const int FRAME_DELAY = 1000 / TARGET_FPS;
    int frame_time = SDL_GetTicks() - frame_start;
    if (frame_time < FRAME_DELAY) {
        SDL_Delay(FRAME_DELAY - frame_time);
    }
}

void render_hexagones(AppState* app, HexagoneList* hex_list) {
    if (!app || !hex_list) return;

    // 1. Dessine le fond
    SDL_RenderCopy(app->renderer, app->background, NULL, NULL);

    // 2. Dessine tous les hexagones
    HexagoneNode* node = hex_list->first;
    while (node) {
        make_hexagone(app->renderer, node->data);
        node = node->next;
    }

    // 3. Met à jour l'affichage
    SDL_RenderPresent(app->renderer);
}

// Nettoie toutes les ressources graphiques
void cleanup_app(AppState* app) {
    if (!app) return;

    // Libère le panneau de settings
    if (app->settings_panel) {
        free_settings_panel(app->settings_panel);
    }

    // Libère les textures SDL
    if (app->background) {
        SDL_DestroyTexture(app->background);
    }
    if (app->renderer) {
        SDL_DestroyRenderer(app->renderer);
    }
    if (app->window) {
        SDL_DestroyWindow(app->window);
    }

    SDL_Quit();
    printf("Application nettoyée\n");
}
