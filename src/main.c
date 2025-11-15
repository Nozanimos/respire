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
#include "chronometre.h"
#include "session_card.h"



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
        config.retention_type,          // Type de rétention (0=pleins, 1=vides)
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

    // === INITIALISATION TABLEAU DES TEMPS DE SESSION ===
    // Tableau dynamique qui va stocker les temps de chaque session
    // Capacité initiale : 10 sessions, puis réallocation si besoin
    app.session_times = malloc(10 * sizeof(float));
    if (!app.session_times) {
        fprintf(stderr, "⚠️ Échec allocation tableau sessions\n");
        app.session_count = 0;
        app.session_capacity = 0;
    } else {
        app.session_count = 0;      // Aucune session pour l'instant
        app.session_capacity = 10;   // Capacité de 10 sessions
        debug_printf("✅ Tableau sessions créé (capacité: %d)\n", app.session_capacity);
    }

    // === CRÉATION DU CHRONOMÈTRE ===
    // Le chronomètre démarre après la session de respiration pour mesurer le temps de méditation
    // Utilise la même police et taille que le timer
    app.session_stopwatch = stopwatch_create("../fonts/arial/ARIALBD.TTF", timer_font_size);
    if (!app.session_stopwatch) {
        fprintf(stderr, "⚠️ Échec création chronomètre\n");
        app.reappear_phase = false;
        app.chrono_phase = false;
        app.inspiration_phase = false;
        app.retention_phase = false;
    } else {
        // Le chronomètre ne démarre PAS immédiatement
        app.reappear_phase = false;
        app.chrono_phase = false;
        app.inspiration_phase = false;
        app.retention_phase = false;
        debug_printf("✅ Chronomètre créé (taille police: %d)\n", timer_font_size);
    }

    // === CRÉATION DU TIMER DE RÉTENTION (15 secondes) ===
    // Timer pour la phase de rétention après l'inspiration (poumons pleins)
    app.retention_timer = breathing_timer_create(15, "../fonts/arial/ARIALBD.TTF", timer_font_size);
    if (!app.retention_timer) {
        fprintf(stderr, "⚠️ Échec création timer de rétention\n");
        app.retention_phase = false;
    } else {
        debug_printf("✅ Timer de rétention créé: 15 secondes (taille police: %d)\n", timer_font_size);
    }

    // === CRÉATION CARTE DE SESSION ===
    // Carte animée affichant le numéro de session entre le timer et le compteur
    app.current_session = 1;  // Commencer à la session 1
    app.total_sessions = config.nb_session;  // Nombre total depuis la config

    app.session_card = session_card_create(
        app.current_session,
        app.screen_width,
        app.screen_height,
        "../fonts/arial/ARIALBD.TTF"
    );

    if (!app.session_card) {
        fprintf(stderr, "⚠️ Échec création carte de session\n");
        app.session_card_phase = false;
    } else {
        app.session_card_phase = false;  // Sera activée après le timer
        debug_printf("✅ Carte de session créée: session %d/%d\n",
                     app.current_session, app.total_sessions);
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
                // Timer terminé → lancer la carte de session
                app.timer_phase = false;
                app.session_card_phase = true;

                // Démarrer l'animation de la carte
                if (app.session_card) {
                    session_card_start(app.session_card);
                    debug_printf("🎬 Timer terminé → Carte de session (session %d/%d)\n",
                                 app.current_session, app.total_sessions);
                }
            }
        }

        // === GESTION CARTE DE SESSION ===
        if (app.session_card_phase && app.session_card) {
            // Mettre à jour l'animation (delta_time en secondes)
            // Utiliser un delta_time fixe basé sur TARGET_FPS (1/60 = ~0.0167s)
            float delta_time = 1.0f / TARGET_FPS;
            bool card_running = session_card_update(app.session_card, delta_time);

            if (!card_running) {
                // Carte terminée → démarrer l'animation et le compteur
                app.session_card_phase = false;

                // 🆕 POSITIONNER LA "TÊTE DE LECTURE" SUR SCALE_MIN (poumons vides)
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

                // 🆕 DÉMARRER LE COMPTEUR
                if (app.breath_counter) {
                    app.breath_counter->is_active = true;
                    app.counter_phase = true;
                    debug_printf("🫁 Compteur activé - lecture depuis précomputing (démarre sur scale_min)\n");
                }

                debug_printf("🎬 Carte terminée → Animation respiratoire (session %d)\n", app.current_session);
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
            // Le compteur se désactive automatiquement quand il atteint le scale final
            // après avoir complété toutes les respirations
            if (!app.breath_counter->is_active) {
                // Compteur désactivé → LANCER LA PHASE DE RÉAPPARITION
                app.counter_phase = false;
                app.reappear_phase = true;  // 🆕 Activer la phase de réapparition

                debug_printf("✅ Session terminée: %d/%d respirations\n",
                             app.breath_counter->current_breath, app.breath_counter->total_breaths);

                // 🆕 POSITIONNER LA TÊTE DE LECTURE À scale_max/2
                // On cherche la première frame où le scale est >= scale_max/2
                // puis on laisse l'animation jouer jusqu'à scale_max
                HexagoneNode* node = hex_list->first;
                while (node) {
                    if (node->precomputed_scales && node->total_cycles > 0) {
                        // Calculer scale_max (le plus grand scale dans le précompute)
                        double scale_max = 0.0;
                        for (int i = 0; i < node->total_cycles; i++) {
                            if (node->precomputed_scales[i] > scale_max) {
                                scale_max = node->precomputed_scales[i];
                            }
                        }

                        // Chercher la dernière séquence : scale_max/2 → scale_max
                        // On part de la fin et on remonte
                        double scale_mid = scale_max / 2.0;
                        int start_frame = -1;

                        // Trouver la dernière montée vers scale_max
                        for (int i = node->total_cycles - 1; i >= 0; i--) {
                            if (node->precomputed_scales[i] <= scale_mid) {
                                start_frame = i;
                                break;
                            }
                        }

                        // Si trouvé, positionner la tête de lecture
                        if (start_frame >= 0) {
                            node->current_cycle = start_frame;
                            debug_printf("🎯 Hexagone %d: tête de lecture → frame %d (scale %.2f → %.2f)\n",
                                         node->data->element_id, start_frame,
                                         node->precomputed_scales[start_frame], scale_max);
                        }
                    }

                    // Dégeler l'animation pour la réapparition
                    node->is_frozen = false;
                    node = node->next;
                }

                debug_printf("🎬 Phase REAPPEAR activée - animation scale_max/2 → scale_max\n");
            }
        }

        // === GESTION PHASE REAPPEAR (réapparition douce de l'hexagone) ===
        // L'animation joue depuis scale_max/2 jusqu'à scale_max pour un alignement parfait
        if (app.reappear_phase) {
            // Vérifier si tous les hexagones ont atteint scale_max
            bool all_at_scale_max = true;
            HexagoneNode* node = hex_list->first;

            while (node) {
                if (node->precomputed_counter_frames && node->current_cycle < node->total_cycles) {
                    // Vérifier si on est au scale_max (flag is_at_scale_max)
                    if (!node->precomputed_counter_frames[node->current_cycle].is_at_scale_max) {
                        all_at_scale_max = false;
                        break;
                    }
                }
                node = node->next;
            }

            // Si tous les hexagones sont à scale_max → passer en phase CHRONO
            if (all_at_scale_max) {
                app.reappear_phase = false;
                app.chrono_phase = true;

                // FIGER L'ANIMATION à scale_max
                node = hex_list->first;
                while (node) {
                    node->is_frozen = true;
                    node = node->next;
                }

                // DÉMARRER LE CHRONOMÈTRE
                if (app.session_stopwatch) {
                    stopwatch_start(app.session_stopwatch);
                    debug_printf("⏱️  Phase CHRONO activée - chronomètre démarré à 00:00\n");
                }
            }
        }

        // === MISE À JOUR DU CHRONOMÈTRE ===
        // Le chronomètre tourne pendant la phase CHRONO
        if (app.chrono_phase && app.session_stopwatch) {
            stopwatch_update(app.session_stopwatch);
        }

        // === GESTION PHASE INSPIRATION/EXPIRATION (après arrêt du chronomètre) ===
        // Animation selon le type de rétention configuré :
        // - retention_type=0 : Poumons pleins → EXPIRATION (scale_max → scale_min) + timer à min
        // - retention_type=1 : Poumons vides → INSPIRATION (scale_min → scale_max) + timer à max
        if (app.inspiration_phase) {
            static bool inspiration_initialized = false;
            if (!inspiration_initialized) {
                HexagoneNode* node = hex_list->first;
                bool is_full_lungs = (app.config.retention_type == 0);  // 0 = poumons pleins

                while (node) {
                    if (node->precomputed_counter_frames && node->total_cycles > 0) {
                        int target_frame = -1;

                        if (is_full_lungs) {
                            // Poumons pleins : on est à scale_max, chercher scale_max pour partir vers scale_min
                            for (int i = node->total_cycles - 1; i >= 0; i--) {
                                if (node->precomputed_counter_frames[i].is_at_scale_max) {
                                    target_frame = i;
                                    break;
                                }
                            }
                        } else {
                            // Poumons vides : on est à scale_min, chercher scale_min pour partir vers scale_max
                            for (int i = node->total_cycles - 1; i >= 0; i--) {
                                if (node->precomputed_counter_frames[i].is_at_scale_min) {
                                    target_frame = i;
                                    break;
                                }
                            }
                        }

                        // Si trouvé, positionner la tête de lecture
                        if (target_frame >= 0) {
                            node->current_cycle = target_frame;
                            debug_printf("🫁 Hexagone %d: tête de lecture → frame %d (%s)\n",
                                       node->data->element_id, target_frame,
                                       is_full_lungs ? "scale_max" : "scale_min");
                        }
                    }

                    // Dégeler l'animation
                    node->is_frozen = false;
                    node = node->next;
                }

                inspiration_initialized = true;
                debug_printf("🎬 Animation %s démarrée\n",
                           is_full_lungs ? "expiration (max → min)" : "inspiration (min → max)");
            }

            // Vérifier si tous les hexagones ont atteint la cible
            bool all_at_target = true;
            bool is_full_lungs = (app.config.retention_type == 0);
            HexagoneNode* node = hex_list->first;

            while (node) {
                if (node->precomputed_counter_frames && node->current_cycle < node->total_cycles) {
                    if (is_full_lungs) {
                        // Poumons pleins : attendre scale_min (expiration)
                        if (!node->precomputed_counter_frames[node->current_cycle].is_at_scale_min) {
                            all_at_target = false;
                            break;
                        }
                    } else {
                        // Poumons vides : attendre scale_max (inspiration)
                        if (!node->precomputed_counter_frames[node->current_cycle].is_at_scale_max) {
                            all_at_target = false;
                            break;
                        }
                    }
                }
                node = node->next;
            }

            // Si tous à la cible → activer phase de rétention
            if (all_at_target) {
                app.inspiration_phase = false;
                app.retention_phase = true;
                inspiration_initialized = false;  // Reset pour la prochaine fois

                // Figer l'animation
                node = hex_list->first;
                while (node) {
                    node->is_frozen = true;
                    node = node->next;
                }

                // Démarrer le timer de rétention (15 secondes)
                if (app.retention_timer) {
                    timer_start(app.retention_timer);
                    debug_printf("⏱️  Phase RÉTENTION activée - timer 15s (figé à %s)\n",
                               is_full_lungs ? "scale_min" : "scale_max");
                }
            }
        }

        // === GESTION PHASE RÉTENTION (poumons pleins OU vides, timer 15s) ===
        if (app.retention_phase && app.retention_timer) {
            bool timer_running = timer_update(app.retention_timer);

            if (!timer_running) {
                // Timer terminé → fin de la rétention
                app.retention_phase = false;
                debug_printf("✅ Phase RÉTENTION terminée\n");

                // 🆕 GESTION BOUCLE DE SESSIONS
                // Vérifier si on doit continuer avec une nouvelle session
                if (app.current_session < app.total_sessions) {
                    // Incrémenter le numéro de session
                    app.current_session++;

                    // Réinitialiser la carte avec le nouveau numéro
                    if (app.session_card) {
                        session_card_reset(app.session_card, app.current_session, app.renderer);
                        session_card_start(app.session_card);
                        app.session_card_phase = true;

                        debug_printf("🔄 Nouvelle session: %d/%d\n",
                                     app.current_session, app.total_sessions);
                    }

                    // Réinitialiser le compteur de respirations
                    if (app.breath_counter) {
                        app.breath_counter->current_breath = 0;
                        app.breath_counter->was_at_min_last_frame = false;
                        app.breath_counter->waiting_for_scale_min = false;
                        app.breath_counter->was_at_max_last_frame = false;
                    }

                    // Réinitialiser le timer de rétention pour la prochaine session
                    if (app.retention_timer) {
                        timer_reset(app.retention_timer);
                    }
                } else {
                    // Toutes les sessions terminées
                    debug_printf("🎉 Toutes les sessions terminées (%d/%d)\n",
                                 app.current_session, app.total_sessions);
                    // L'application continue de tourner, l'utilisateur peut interagir avec le panneau
                }
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

    // Libérer le chronomètre
    if (app.session_stopwatch) {
        stopwatch_destroy(app.session_stopwatch);
        app.session_stopwatch = NULL;
    }

    // Libérer le timer de rétention
    if (app.retention_timer) {
        timer_destroy(app.retention_timer);
        app.retention_timer = NULL;
    }

    // Libérer le tableau des temps de session
    if (app.session_times) {
        free(app.session_times);
        app.session_times = NULL;
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
