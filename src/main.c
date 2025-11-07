// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "geometry.h"
#include "precompute_list.h"
#include "renderer.h"
#include "config.h"
#include "debug.h"
#include "widget_base.h"
#include "timer.h"
#include "counter.h"



void init_debug_mode(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            // Configurer la locale pour UTF-8
            setlocale(LC_ALL, "");

            debug_file = fopen("debug.txt", "w");
            if (debug_file) {
                // Écrire le BOM UTF-8 pour garantir l'encodage
                fprintf(debug_file, "\xEF\xBB\xBF");
                // Marqueur d'encodage reconnu par les éditeurs (vim, VS Code, emacs, etc.)
                fprintf(debug_file, "# -*- coding: utf-8 -*-\n");

                // Une seule redirection
                freopen("debug.txt", "a", stdout);  // Mode 'a' pour ne pas écraser le BOM
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
    int container_size = (app.screen_width < app.screen_height) ? app.screen_width : app.screen_height;
    float size_ratio = 0.75f;

    hex_list = create_all_hexagones(app.screen_width/2, app.screen_height/2, container_size, size_ratio);

    // Assignement hex_list à app.hexagones
    app.hexagones = hex_list;

    // === PRÉ-CALCULS ===
    precompute_all_cycles(hex_list, TARGET_FPS, config.breath_duration);
    // 🆕 PRÉCOMPUTER LES FRAMES DU COMPTEUR pour tous les hexagones
    // On utilise le nombre de respirations depuis la config
    HexagoneNode* node = hex_list->first;
    while (node) {
        precompute_counter_frames(
            node,
            node->total_cycles,           // Nombre total de frames précalculées
            TARGET_FPS,                   // Images par seconde
            config.breath_duration,       // Durée d'un cycle complet
            config.Nb_respiration         // Nombre max de respirations à compter
        );
        node = node->next;
    }
    debug_printf("✅ Compteur précomputé pour %d hexagones\n", hex_list->count);
    print_rotation_frame_requirements(hex_list, TARGET_FPS, config.breath_duration);

    debug_printf("✅ Hexagones créés et assignés à app.hexagones\n");
    debug_printf("📊 Nombre d'hexagones: %d\n", hex_list->count);


    // === CRÉATION DU TIMER ===
    // Récupérer la durée depuis la config (chargée depuis respiration.conf)
    int timer_duration = config.start_duration;

    // Calculer la taille de police adaptée à l'hexagone
    int smallest_hex_radius = (int)(container_size * size_ratio * 0.5f);  // Rayon du plus petit hexagone
    int timer_font_size = smallest_hex_radius / 2;  // Police = moitié du rayon

    app.session_timer = breathing_timer_create(timer_duration, "../fonts/arial/ARIALBD.TTF", timer_font_size);
    if (!app.session_timer) {
        fprintf(stderr, "⚠️  Échec création timer - démarrage direct de l'animation\n");
        app.timer_phase = false;
    } else {
        app.timer_phase = true;
        timer_start(app.session_timer);
        debug_printf("✅ Timer créé: %d secondes\n", timer_duration);

        // 🆕 FIGER L'ANIMATION PENDANT LE TIMER
        HexagoneNode* node = hex_list->first;
        while (node) {
            node->is_frozen = true;  // Figer tous les hexagones
            node = node->next;
        }
        debug_printf("❄️  Animation figée pendant le timer\n");
    }

    // === CRÉATION DU COMPTEUR DE RESPIRATIONS ===
    // 🆕 Le compteur utilise SDL_TTF avec génération dynamique pour une qualité optimale
    // La taille de police est calculée dynamiquement selon la taille du plus petit hexagone
    int counter_font_size = (int)(smallest_hex_radius * 0.7f);

    // 🆕 Compteur simplifié - les données d'animation viennent du précomputing
    app.breath_counter = counter_create(
        config.Nb_respiration,          // Nombre max de respirations
        "../fonts/arial/ARIALBD.TTF",   // Police (Arial Bold)
        counter_font_size               // Taille dynamique basée sur l'hexagone
    );

    if (!app.breath_counter) {
        fprintf(stderr, "⚠️  Échec création compteur - respiration sans comptage\n");
        app.counter_phase = false;
    } else {
        // Le compteur ne démarre PAS immédiatement, il attend la fin du timer
        app.counter_phase = false;  // Sera activé après le timer
        debug_printf("✅ Compteur créé: 0/%d respirations (taille police: %d)\n",
                     config.Nb_respiration, counter_font_size);
    }

    /*------------------------------------------------------------*/

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
            // Utiliser la fonction centralisée de renderer.c
            handle_app_events(&app, &event);

            // Vérifier si l'appli doit se fermer
            if (!app.is_running) {
                done = 0;
            }
        }

        // === GESTION TIMER ===
        if (app.timer_phase) {
            // Phase 1 : TIMER - Countdown avant démarrage
            bool timer_running = timer_update(app.session_timer);

            if (!timer_running) {
                // Timer terminé → démarrer l'animation et le compteur
                app.timer_phase = false;

                // 🆕 POSITIONNER LA "TÊTE DE LECTURE" SUR SCALE_MIN (poumons vides)
                // Chercher la première frame où is_at_scale_min = true
                HexagoneNode* node = hex_list->first;
                while (node) {
                    // Chercher la première frame avec scale_min
                    bool frame_found = false;
                    for (int frame = 0; frame < node->total_cycles && !frame_found; frame++) {
                        if (node->precomputed_counter_frames &&
                            node->precomputed_counter_frames[frame].is_at_scale_min) {
                            // Positionner la tête de lecture sur cette frame
                            node->current_cycle = frame;
                        frame_found = true;
                        debug_printf("🎯 Hexagone %d positionné sur scale_min (frame %d)\n",
                                     node->data->element_id, frame);
                            }
                    }

                    // Dégeler l'animation
                    node->is_frozen = false;
                    node = node->next;
                }

                // 🆕 DÉMARRER LE COMPTEUR (activer simplement - données dans le précomputing)
                if (app.breath_counter) {
                    app.breath_counter->is_active = true;
                    app.counter_phase = true;
                    debug_printf("🫁 Compteur activé - lecture depuis précomputing (démarre sur scale_min)\n");
                }

                debug_printf("🎬 Timer terminé - animation positionnée sur scale_min (poumons vides)\n");
            }
        }

        // === ANIMATION (toujours active, sauf si figée) ===
        HexagoneNode* node = hex_list->first;
        while (node) {
            apply_precomputed_frame(node);  // Ne fait rien si is_frozen = true
            node = node->next;
        }

        // === VÉRIFICATION FIN DU COMPTEUR (le compteur se désactive lui-même) ===
        if (app.counter_phase && app.breath_counter) {
            // Le compteur se désactive automatiquement quand il atteint le scale_max
            // après avoir complété toutes les respirations
            if (!app.breath_counter->is_active) {
                // Compteur désactivé → figer l'animation à scale_max
                app.counter_phase = false;

                // 🆕 FIGER L'ANIMATION en position de repos (scale_max)
                HexagoneNode* node = hex_list->first;
                while (node) {
                    node->is_frozen = true;
                    node = node->next;
                }

                debug_printf("✅ Session terminée: %d/%d respirations - animation et compteur désactivés\n",
                             app.breath_counter->current_breath, app.breath_counter->total_breaths);
            }
        }

        // Mise à jour animation panneau
        if (app.settings_panel) {
            update_settings_panel(app.settings_panel, (float)FRAME_DELAY / 1000.0f);
        }

        // RENDU COMPLET
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
    }  // <-- FIN DU WHILE (done) - ACCOLADE IMPORTANTE !

    // === NETTOYAGE ===
    debug_printf("Nettoyage...\n");

    // Libérer le timer
    if (app.session_timer) {
        timer_destroy(app.session_timer);
        app.session_timer = NULL;
    }

    // Libérer le compteur
    if (app.breath_counter) {
        counter_destroy(app.breath_counter);
        app.breath_counter = NULL;
    }

    // Libérer les polices AVANT TTF_Quit
    cleanup_font_manager();
    TTF_Quit();

    cleanup_debug_mode();

    free_hexagone_list(hex_list);

    cleanup_app(&app);

    debug_printf("Application terminée\n");
    return EXIT_SUCCESS;
}  // <-- FIN DU main()
