#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "geometry.h"
#include "hexagone_list.h"
#include "renderer.h"
#include "config.h"
#include "debug.h"


void init_debug_mode(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            debug_file = fopen("debug.txt", "w");
            if (debug_file) {
                // ✅ CORRECTION : Une seule redirection
                freopen("debug.txt", "w", stdout);
                // stderr reste séparé pour les vraies erreurs

                setbuf(stdout, NULL);

                time_t now = time(NULL);
                debug_printf("=== DÉBUT SESSION DEBUG - %s ===\n", ctime(&now));
                debug_printf("✅ Mode debug activé - logs dans debug.txt\n");
            }
            break;
        }
    }
}

void cleanup_debug_mode() {
    if (debug_file) {
        time_t now = time(NULL);
        fprintf(debug_file, "=== FIN SESSION DEBUG - %s ===\n", ctime(&now));
        fclose(debug_file);
        debug_file = NULL;
    }
}

/*------------------------------------------- MAIN --------------------------------------------*/

int main(int argc, char **argv) {

    // Initialiser le mode debug si demandé
    init_debug_mode(argc, argv);


    /*------------------------------------------------------------*/


    AppState app = {0};
    HexagoneList* hex_list = NULL;
    AppConfig config;
    SDL_Event event;
    int done = 1;

    // Charger la configuration
    load_config(&config);

    // === INITIALISATION ===
    if (!initialize_app(&app, "Respiration guidée", "../img/nenuphar.jpg")) {
        fprintf(stderr, "Échec initialisation - arrêt\n");
        return EXIT_FAILURE;
    }

    // === CRÉATION DES HEXAGONES ===
    // NOUVEAU : Calcul de la taille du container et ratio
    int container_size = (app.screen_width < app.screen_height) ? app.screen_width : app.screen_height;
    float size_ratio = 0.75f; // 75% de la taille du container

    hex_list = create_all_hexagones(app.screen_width/2, app.screen_height/2, container_size, size_ratio);

    // === PRÉ-CALCULS ===
    precompute_all_cycles(hex_list, TARGET_FPS, config.breath_duration);
    print_rotation_frame_requirements(hex_list, TARGET_FPS, config.breath_duration);

    const int FRAME_DELAY = 1000 / TARGET_FPS;
    Uint32 frame_start;
    int frame_time;
    int frame_count = 0;
    Uint32 last_fps_time = SDL_GetTicks();

    debug_printf("🔄 INIT Prévisualisation - Cadre: (50,80), Centre: (50,50), Taille: 100, Ratio: 0.70\n");

    debug_printf("Démarrage de l'application...\n");
    debug_printf("Boucle principale - ESC pour quitter\n");

    while (done) {
        frame_start = SDL_GetTicks();

        // Gestion événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                done = 0;
                }

                // GESTION DU PANNEAU
                if (app.settings_panel) {
                    handle_settings_panel_event(app.settings_panel, &event, &config);
                }
        }

        // Mise à jour animations hexagones
        HexagoneNode* node = hex_list->first;
        while (node) {
            apply_precomputed_frame(node);
            node = node->next;
        }

        // Mise à jour animation panneau
        if (app.settings_panel) {
            update_settings_panel(app.settings_panel, (float)FRAME_DELAY / 1000.0f);
        }

        // RENDU COMPLET (utiliser render_app au lieu de render_hexagones)
        render_app(&app);

        // Régulation FPS
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }

        // Affichage FPS
        frame_count++;
        if (SDL_GetTicks() - last_fps_time >= 1000) {
            frame_count = 0;
            last_fps_time = SDL_GetTicks();
        }
    }

    // === NETTOYAGE ===
    debug_printf("Nettoyage...\n");
    cleanup_debug_mode();
    free_hexagone_list(hex_list);
    cleanup_app(&app);

    debug_printf("Application terminée\n");
    return EXIT_SUCCESS;
}
