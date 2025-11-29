// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "core/geometry.h"
#include "core/precompute_list.h"
#include "core/renderer.h"
#include "core/config.h"
#include "core/debug.h"
#include "core/widget_base.h"
#include "core/timer.h"
#include "core/counter.h"
#include "core/chronometre.h"
#include "core/session_card.h"
#include "instances/technique_instance.h"
#include "instances/whm/whm.h"
#include "core/memory/memory.h"



void init_debug_mode(int argc, char **argv) {
    int debug_enabled = 0;

    // Vérifier les arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            debug_enabled = 1;
        }
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose_mode = 1;
        }
    }

    if (debug_enabled) {
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
            if (verbose_mode) {
                debug_printf("✅ Mode VERBOSE activé\n");
            }
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
    if (!initialize_app(&app, "Respiration guidée", IMG_NENUPHAR)) {
        fprintf(stderr, "Échec initialisation - arrêt\n");
        return EXIT_FAILURE;
    }

    // === CRÉATION DES HEXAGONES ===
    int container_size = (app.screen_width < app.screen_height) ? app.screen_width : app.screen_height;
    float size_ratio = 0.75f;

    hex_list = create_all_hexagones(app.screen_width/2, app.screen_height/2, container_size, size_ratio);

    // Assignement hex_list à app.hexagones
    app.hexagones = hex_list;

    // 🐛 FIX: Corriger current_scale des hexagones pour le responsive
    // Les hexagones sont créés avec current_scale = 1.0, mais ils doivent avoir scale_factor
    if (hex_list) {
        HexagoneNode* node = hex_list->first;
        while (node && node->data) {
            node->data->current_scale = app.scale_factor;
            node = node->next;
        }
        debug_printf("✅ Hexagones créés avec scale_factor=%.2f\n", app.scale_factor);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 🆕 OPTIMISATION : PRÉ-CALCULS DIFFÉRÉS
    // ═══════════════════════════════════════════════════════════════════════════
    // Les pré-calculs (~100 MB) ne sont PAS faits au démarrage !
    // Ils seront faits PENDANT le timer (après le clic sur l'image Wim Hof)
    // Avantages :
    //   - Démarrage instantané de l'application
    //   - 10+ secondes disponibles pour le calcul pendant le timer
    //   - Libération de la mémoire à la fin de l'animation
    //   - Économie de ressources pour les autres techniques de respiration futures
    // ═══════════════════════════════════════════════════════════════════════════

    debug_printf("✅ Hexagones créés et assignés à app.hexagones\n");
    debug_printf("📊 Nombre d'hexagones: %d\n", hex_list->count);
    debug_printf("⏳ Pré-calculs différés (se feront après le clic sur l'image)\n");


    // === CRÉATION DU TIMER ===
    // Récupérer la durée depuis la config (chargée depuis respiration.conf)
    int timer_duration = config.start_duration;

    // Calculer la taille de police adaptée à l'hexagone
    int smallest_hex_radius = (int)(container_size * size_ratio * 0.5f);  // Rayon du plus petit hexagone
    int timer_font_size = smallest_hex_radius / 2;  // Police = moitié du rayon

    app.session_timer = breathing_timer_create(timer_duration, FONT_ARIAL_BOLD, timer_font_size);
    if (!app.session_timer) {
        fprintf(stderr, "⚠️  Échec création timer - démarrage direct de l'animation\n");
        app.timer_phase = false;
    } else {
        // ⚠️ NE PAS DÉMARRER LE TIMER MAINTENANT !
        // Le timer sera démarré après le clic sur l'écran d'accueil
        app.timer_phase = false;  // Sera activé après le clic
        debug_printf("✅ Timer créé: %d secondes (sera démarré après clic)\n", timer_duration);

        // 🆕 FIGER L'ANIMATION POUR PLUS TARD
        HexagoneNode* node = hex_list->first;
        while (node) {
            node->is_frozen = true;  // Figer tous les hexagones
            node = node->next;
        }
        debug_printf("❄️  Animation figée (sera démarrée après clic)\n");
    }

    // === CRÉATION DU COMPTEUR DE RESPIRATIONS ===
    // 🆕 Le compteur utilise un CACHE DE TEXTURES prérendu avec Cairo
    // La taille de police est calculée dynamiquement selon la taille du plus petit hexagone
    int counter_font_size = (int)(smallest_hex_radius * 0.7f);

    // Récupérer les scales depuis le premier hexagone (tous ont les mêmes scales)
    double scale_min = hex_list->first->animation->scale_min;
    double scale_max = hex_list->first->animation->scale_max;

    // 🆕 Compteur optimisé - avec cache complet par frame
    app.breath_counter = counter_create(
        app.renderer,                   // Renderer SDL (pour créer le cache)
        config.Nb_respiration,          // Nombre max de respirations
        config.retention_type,          // Type de rétention (0=pleins, 1=vides)
        FONT_ARIAL_BOLD,   // Police (Arial Bold)
        counter_font_size,              // Taille dynamique basée sur l'hexagone
        scale_min,                      // Scale min pour le cache
        scale_max,                      // Scale max pour le cache
        TARGET_FPS,                     // FPS (60) pour calculer frames_per_cycle
        config.breath_duration          // Durée d'un cycle (3.0s) pour calculer frames_per_cycle
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
    app.session_times = SAFE_MALLOC(10 * sizeof(float));
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
    app.session_stopwatch = stopwatch_create(FONT_ARIAL_BOLD, timer_font_size);
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
    app.retention_timer = breathing_timer_create(15, FONT_ARIAL_BOLD, timer_font_size);
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

    // 🆕 PASSER LE SCALE_FACTOR pour le responsive
    app.session_card = session_card_create(
        app.current_session,
        app.screen_width,
        app.screen_height,
        FONT_ARIAL_BOLD,
        app.scale_factor
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

    // ═══════════════════════════════════════════════════════════════════════════
    // FPS ADAPTATIF : 60 FPS (animations) ou 15 FPS (idle/économie CPU)
    // ═══════════════════════════════════════════════════════════════════════════
    const int FRAME_DELAY_HIGH = 1000 / TARGET_FPS;  // 60 FPS = ~16ms
    const int FRAME_DELAY_LOW = 1000 / 15;           // 15 FPS = ~66ms
    int current_frame_delay = FRAME_DELAY_HIGH;      // Commence en 60 FPS
    bool was_high_fps = true;                        // Pour logger les changements

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
            // Si une technique est active, déléguer les événements à l'instance
            if (app.active_technique) {
                TechniqueInstance* instance = (TechniqueInstance*)app.active_technique;
                if (instance->handle_event) {
                    instance->handle_event(instance, &event);
                }
            }

            // Utiliser la fonction centralisée de renderer.c
            handle_app_events(&app, &event);

            // Vérifier si l'appli doit se fermer
            if (!app.is_running) {
                done = 0;
            }
        }

        // ═════════════════════════════════════════════════════════════════════
        // DÉLÉGATION À L'INSTANCE DE TECHNIQUE ACTIVE
        // ═════════════════════════════════════════════════════════════════════
        if (app.active_technique) {
            TechniqueInstance* instance = (TechniqueInstance*)app.active_technique;

            // Mettre à jour l'instance
            float delta_time = 1.0f / TARGET_FPS;
            if (instance->update) {
                instance->update(instance, delta_time);
            }

            // Vérifier si la technique est terminée
            if (instance->is_finished) {
                debug_printf("🔙 [MAIN] Technique terminée\n");

                // Récupérer les données de session avant de détruire l'instance
                WHMData* whm_data = (WHMData*)instance->technique_data;
                float* session_times = NULL;
                int session_count = 0;

                if (whm_data && whm_data->session_controller &&
                    whm_data->session_controller->session_times &&
                    whm_data->session_controller->session_count > 0) {
                    // Copier les temps de session depuis le controller
                    session_count = whm_data->session_controller->session_count;
                    session_times = SAFE_MALLOC(sizeof(float) * session_count);
                    if (session_times) {
                        memcpy(session_times, whm_data->session_controller->session_times,
                               sizeof(float) * session_count);
                    }
                }

                // Détruire l'instance
                technique_destroy(instance);
                app.active_technique = NULL;

                // Créer et ouvrir le panneau de statistiques si on a des sessions
                if (session_times && session_count > 0 && !app.stats_panel) {
                    app.stats_panel = create_stats_panel(
                        app.screen_width,
                        app.screen_height,
                        session_times,
                        session_count
                    );

                    if (app.stats_panel) {
                        open_stats_panel(app.stats_panel);
                        debug_printf("📊 [MAIN] Panneau de statistiques ouvert\n");
                    }

                    SAFE_FREE(session_times);
                } else {
                    // Pas de sessions → retour direct à l'écran d'accueil
                    app.waiting_to_start = true;
                    debug_printf("🏠 [MAIN] Retour direct à l'écran d'accueil\n");
                }
            }

            // Rendre l'app (qui déléguera le rendu à l'instance)
            render_app(&app);

            // Réguler les FPS selon les besoins de l'instance
            regulate_fps(frame_start);

            // Continuer la boucle (skip le code ancien ci-dessous)
            continue;
        }

        // ═════════════════════════════════════════════════════════════════════
        // MISE À JOUR DU PANNEAU STATS (s'il est ouvert)
        // ═════════════════════════════════════════════════════════════════════
        if (app.stats_panel) {
            float delta_time = 1.0f / TARGET_FPS;
            update_stats_panel(app.stats_panel, delta_time);

            // 🆕 RETOUR À L'ÉCRAN D'ACCUEIL quand le panneau stats est complètement fermé
            if (app.stats_panel->state == STATS_CLOSED && !app.waiting_to_start) {
                app.waiting_to_start = true;
                debug_printf("🏠 Retour à l'écran d'accueil (stats fermé)\n");

                // ═════════════════════════════════════════════════════════════════
                // FERMER LE JSON EDITOR
                // ═════════════════════════════════════════════════════════════════
                if (app.json_editor && app.json_editor->est_ouvert) {
                    app.json_editor->est_ouvert = false;
                    debug_printf("📝 JSON Editor fermé (retour écran d'accueil)\n");
                }

                // ═════════════════════════════════════════════════════════════════
                // RECHARGER LA CONFIGURATION
                // ═════════════════════════════════════════════════════════════════
                load_config(&app.config);
                debug_printf("🔄 Configuration rechargée depuis respiration.conf\n");

                // ═════════════════════════════════════════════════════════════════
                // RÉINITIALISER LES HEXAGONES PRINCIPAUX
                // ═════════════════════════════════════════════════════════════════
                // ⚠️  NE PAS free_precomputed_data ici !
                // Le precompute sera refait au prochain clic Wim dans renderer.c
                // On réinitialise juste l'état visuel des hexagones
                if (app.hexagones) {
                    HexagoneNode* node = app.hexagones->first;
                    while (node) {
                        node->data->center_x = app.screen_width / 2;
                        node->data->center_y = app.screen_height / 2;

                        // 🐛 FIX: Positionner sur scale_max pour que le timer soit bien visible
                        // Les precomputed_counter_frames existent encore (pas de free_precomputed_data)
                        bool found_scale_max = false;
                        if (node->precomputed_counter_frames && node->total_cycles > 0) {
                            for (int i = 0; i < node->total_cycles; i++) {
                                if (node->precomputed_counter_frames[i].is_at_scale_max) {
                                    node->current_cycle = i;
                                    found_scale_max = true;

                                    // 🔥 CRITIQUE: Appliquer la frame pour copier vx/vy !
                                    node->is_frozen = false;
                                    apply_precomputed_frame(node);
                                    node->is_frozen = true;
                                    break;
                                }
                            }
                        }

                        // Fallback
                        if (!found_scale_max) {
                            node->current_cycle = 0;
                            node->is_frozen = false;
                            apply_precomputed_frame(node);
                            node->is_frozen = true;
                        } else {
                            node->is_frozen = true;
                        }

                        node = node->next;
                    }
                }

                // ═════════════════════════════════════════════════════════════════
                // RÉINITIALISER LE PREVIEW DU PANNEAU SETTINGS
                // ═════════════════════════════════════════════════════════════════
                if (app.settings_panel && app.settings_panel->preview_system.hex_list) {
                    debug_printf("🗑️  Libération anciennes données preview...\n");
                    free_precomputed_data(app.settings_panel->preview_system.hex_list);

                    // Recalculer avec la nouvelle durée
                    debug_printf("🔢 Recalcul preview (breath_duration=%.1fs)...\n",
                                 app.config.breath_duration);
                    precompute_all_cycles(app.settings_panel->preview_system.hex_list,
                                         TARGET_FPS,
                                         app.config.breath_duration);

                    // Réinitialiser le temps d'animation
                    app.settings_panel->preview_system.current_time = 0.0;
                    app.settings_panel->preview_system.last_update = SDL_GetTicks();

                    debug_printf("✅ Preview réinitialisé\n");
                }

                // ═════════════════════════════════════════════════════════════════
                // DÉTRUIRE LE PANNEAU STATS
                // ═════════════════════════════════════════════════════════════════
                if (app.stats_panel) {
                    destroy_stats_panel(app.stats_panel);
                    app.stats_panel = NULL;
                    debug_printf("🗑️  Panneau stats détruit\n");
                }
            }
        }

        // ═════════════════════════════════════════════════════════════════════
        // CODE ANCIEN (sera supprimé progressivement)
        // ═════════════════════════════════════════════════════════════════════

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
                    // Utilise precomputed_counter_frames avec relative_breath_scale (0.0→1.0)
                    if (node->precomputed_counter_frames && node->total_cycles > 0) {
                        // Chercher la dernière séquence : 0.5 → 1.0 (milieu vers maximum)
                        // On part de la fin et on remonte
                        double scale_mid = 0.5;  // Milieu du cycle en valeur relative
                        int start_frame = -1;

                        // Trouver la dernière montée vers scale_max (relative = 1.0)
                        for (int i = node->total_cycles - 1; i >= 0; i--) {
                            if (node->precomputed_counter_frames[i].relative_breath_scale <= scale_mid) {
                                start_frame = i;
                                break;
                            }
                        }

                        // Si trouvé, positionner la tête de lecture
                        if (start_frame >= 0) {
                            node->current_cycle = start_frame;
                            debug_printf("🎯 Hexagone %d: tête de lecture → frame %d (scale %.2f → 1.0)\n",
                                         node->data->element_id, start_frame,
                                         node->precomputed_counter_frames[start_frame].relative_breath_scale);
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

                    // 🆕 LIBÉRER LES DONNÉES PRÉCOMPILÉES (~100 MB)
                    // Plus besoin des pré-calculs, libérer la mémoire pour économiser les ressources
                    if (app.hexagones) {
                        free_precomputed_data(app.hexagones);
                    }

                    // 🆕 CRÉER ET OUVRIR LE PANNEAU DE STATISTIQUES
                    // UNIQUEMENT si au moins une session a été comptée
                    if (!app.stats_panel && app.session_times && app.session_count > 0) {
                        app.stats_panel = create_stats_panel(
                            app.screen_width,
                            app.screen_height,
                            app.session_times,
                            app.session_count
                        );

                        if (app.stats_panel) {
                            open_stats_panel(app.stats_panel);
                            debug_printf("📊 Panneau de statistiques ouvert\n");
                        }
                    }
                    // 🆕 Si aucune session n'a été comptée, revenir directement à l'écran d'accueil
                    else if (app.session_count == 0) {
                        app.waiting_to_start = true;
                        debug_printf("🏠 Retour direct à l'écran d'accueil (aucune session comptée)\n");

                        // Fermer le JSON Editor
                        if (app.json_editor && app.json_editor->est_ouvert) {
                            app.json_editor->est_ouvert = false;
                            debug_printf("📝 JSON Editor fermé (retour écran d'accueil)\n");
                        }

                        // Recharger la configuration
                        load_config(&app.config);
                        debug_printf("🔄 Configuration rechargée depuis respiration.conf\n");

                        // Réinitialiser tous les timers
                        if (app.session_timer) {
                            timer_reset(app.session_timer);
                            app.session_timer->total_seconds = app.config.start_duration;
                            app.session_timer->remaining_seconds = app.config.start_duration;
                            debug_printf("🔄 Timer de session réinitialisé à %d secondes\n", app.config.start_duration);
                        }

                        if (app.session_stopwatch) {
                            stopwatch_reset(app.session_stopwatch);
                            debug_printf("🔄 Chronomètre réinitialisé\n");
                        }

                        if (app.retention_timer) {
                            timer_reset(app.retention_timer);
                            debug_printf("🔄 Timer de rétention réinitialisé\n");
                        }

                        // Réinitialiser le compteur de respirations
                        if (app.breath_counter) {
                            app.breath_counter->current_breath = 0;
                            app.breath_counter->was_at_min_last_frame = false;
                            app.breath_counter->waiting_for_scale_min = false;
                            app.breath_counter->was_at_max_last_frame = false;
                            app.breath_counter->is_active = false;
                            app.breath_counter->total_breaths = app.config.Nb_respiration;
                            debug_printf("🔄 Compteur réinitialisé à 0/%d respirations\n", app.config.Nb_respiration);
                        }

                        // Réinitialiser les phases pour la prochaine session
                        app.timer_phase = false;
                        app.session_card_phase = false;
                        app.counter_phase = false;
                        app.reappear_phase = false;
                        app.chrono_phase = false;
                        app.inspiration_phase = false;
                        app.retention_phase = false;

                        // Réinitialiser le compteur de sessions
                        app.current_session = 1;
                        app.total_sessions = app.config.nb_session;
                        debug_printf("🔄 Nombre de sessions mis à jour: %d\n", app.total_sessions);

                        // Réinitialiser les hexagones principaux
                        if (app.hexagones) {
                            free_precomputed_data(app.hexagones);
                            HexagoneNode* node = app.hexagones->first;
                            while (node) {
                                node->data->center_x = app.screen_width / 2;
                                node->data->center_y = app.screen_height / 2;
                                node->current_cycle = 0;
                                node->is_frozen = true;

                                // 🐛 FIX: Après free_precomputed_data, pas de frames précompilées
                                // Les vx/vy seront recalculés au prochain precompute_all_cycles
                                // On garde juste current_scale intact (scale_factor déjà appliqué)

                                node = node->next;
                            }
                        }

                        // Réinitialiser le preview du panneau settings
                        if (app.settings_panel && app.settings_panel->preview_system.hex_list) {
                            debug_printf("🗑️  Libération anciennes données preview...\n");
                            free_precomputed_data(app.settings_panel->preview_system.hex_list);

                            debug_printf("🔢 Recalcul preview (breath_duration=%.1fs)...\n",
                                         app.config.breath_duration);
                            precompute_all_cycles(app.settings_panel->preview_system.hex_list,
                                                 TARGET_FPS,
                                                 app.config.breath_duration);

                            app.settings_panel->preview_system.current_time = 0.0;
                            app.settings_panel->preview_system.last_update = SDL_GetTicks();
                            debug_printf("✅ Preview réinitialisé\n");
                        }

                        // Détruire le panneau stats s'il existe
                        if (app.stats_panel) {
                            destroy_stats_panel(app.stats_panel);
                            app.stats_panel = NULL;
                            debug_printf("🗑️  Panneau stats détruit\n");
                        }
                    }
                }
            }
        }

        // ═══════════════════════════════════════════════════════════════════════
        // FPS ADAPTATIF : Détecter si on a besoin de 60 FPS ou 15 FPS
        // ═══════════════════════════════════════════════════════════════════════
        bool use_high_fps = should_use_high_fps(&app);
        current_frame_delay = use_high_fps ? FRAME_DELAY_HIGH : FRAME_DELAY_LOW;

        // Logger les changements de FPS (pour debug)
        if (use_high_fps != was_high_fps) {
            debug_printf("🎯 FPS adaptatif : %s → %s (%d FPS)\n",
                        was_high_fps ? "60 FPS" : "15 FPS",
                        use_high_fps ? "60 FPS" : "15 FPS",
                        use_high_fps ? 60 : 15);
            was_high_fps = use_high_fps;
        }

        // Mise à jour animation panneau settings
        if (app.settings_panel) {
            update_settings_panel(app.settings_panel, (float)current_frame_delay / 1000.0f);
        }

        // Mise à jour animation panneau stats
        if (app.stats_panel) {
            update_stats_panel(app.stats_panel, (float)current_frame_delay / 1000.0f);

            // 🆕 RETOUR À L'ÉCRAN D'ACCUEIL quand le panneau stats est complètement fermé
            if (app.stats_panel->state == STATS_CLOSED && !app.waiting_to_start) {
                app.waiting_to_start = true;
                debug_printf("🏠 Retour à l'écran d'accueil (stats fermé)\n");

                // ═════════════════════════════════════════════════════════════════
                // FERMER LE JSON EDITOR
                // ═════════════════════════════════════════════════════════════════
                // S'assurer que le JSON editor est fermé pour ne pas consommer les clics
                if (app.json_editor && app.json_editor->est_ouvert) {
                    app.json_editor->est_ouvert = false;
                    debug_printf("📝 JSON Editor fermé (retour écran d'accueil)\n");
                }

                // ═════════════════════════════════════════════════════════════════
                // RECHARGER LA CONFIGURATION
                // ═════════════════════════════════════════════════════════════════
                // Recharger respiration.conf pour prendre en compte les modifications
                // faites dans le panneau settings AVANT le prochain clic
                load_config(&app.config);
                debug_printf("🔄 Configuration rechargée depuis respiration.conf\n");

                // ═════════════════════════════════════════════════════════════════
                // RÉINITIALISER TOUS LES TIMERS
                // ═════════════════════════════════════════════════════════════════
                if (app.session_timer) {
                    timer_reset(app.session_timer);
                    // Mettre à jour la durée avec la nouvelle config
                    app.session_timer->total_seconds = app.config.start_duration;
                    app.session_timer->remaining_seconds = app.config.start_duration;
                    debug_printf("🔄 Timer de session réinitialisé à %d secondes\n", app.config.start_duration);
                }

                if (app.session_stopwatch) {
                    stopwatch_reset(app.session_stopwatch);
                    debug_printf("🔄 Chronomètre réinitialisé\n");
                }

                if (app.retention_timer) {
                    timer_reset(app.retention_timer);
                    debug_printf("🔄 Timer de rétention réinitialisé\n");
                }

                // ═════════════════════════════════════════════════════════════════
                // RÉINITIALISER LE COMPTEUR DE RESPIRATIONS
                // ═════════════════════════════════════════════════════════════════
                if (app.breath_counter) {
                    app.breath_counter->current_breath = 0;
                    app.breath_counter->was_at_min_last_frame = false;
                    app.breath_counter->waiting_for_scale_min = false;
                    app.breath_counter->was_at_max_last_frame = false;
                    app.breath_counter->is_active = false;
                    // Mettre à jour le nombre de respirations avec la nouvelle config
                    app.breath_counter->total_breaths = app.config.Nb_respiration;
                    debug_printf("🔄 Compteur réinitialisé à 0/%d respirations\n", app.config.Nb_respiration);
                }

                // Réinitialiser les phases pour la prochaine session
                app.timer_phase = false;
                app.session_card_phase = false;
                app.counter_phase = false;
                app.reappear_phase = false;
                app.chrono_phase = false;
                app.inspiration_phase = false;
                app.retention_phase = false;

                // Réinitialiser le compteur de sessions
                app.current_session = 1;
                app.session_count = 0;
                app.total_sessions = app.config.nb_session;
                debug_printf("🔄 Nombre de sessions mis à jour: %d\n", app.total_sessions);

                // ═════════════════════════════════════════════════════════════════
                // RÉINITIALISER LES HEXAGONES PRINCIPAUX
                // ═════════════════════════════════════════════════════════════════
                // VERSION STABLE : Réinitialisation au lieu de destruction
                // (La destruction complète cause des crashs)
                if (app.hexagones) {
                    // Libérer les données précompilées
                    free_precomputed_data(app.hexagones);

                    // Réinitialiser chaque hexagone à son état d'origine
                    HexagoneNode* node = app.hexagones->first;
                    while (node) {
                        node->data->center_x = app.screen_width / 2;
                        node->data->center_y = app.screen_height / 2;
                        node->current_cycle = 0;
                        node->is_frozen = true;

                        // 🐛 FIX: Après free_precomputed_data, pas de frames précompilées
                        // Les vx/vy seront recalculés au prochain precompute_all_cycles
                        // On garde juste current_scale intact (scale_factor déjà appliqué)

                        node = node->next;
                    }
                }

                // ═════════════════════════════════════════════════════════════════
                // RÉINITIALISER LE PREVIEW DU PANNEAU SETTINGS
                // ═════════════════════════════════════════════════════════════════
                // Le PreviewSystem garde les données précompilées de l'ancien stage
                // Il faut libérer et recalculer avec la nouvelle durée
                if (app.settings_panel && app.settings_panel->preview_system.hex_list) {
                    debug_printf("🗑️  Libération anciennes données preview...\n");
                    free_precomputed_data(app.settings_panel->preview_system.hex_list);

                    // Recalculer avec la nouvelle durée
                    debug_printf("🔢 Recalcul preview (breath_duration=%.1fs)...\n",
                                 app.config.breath_duration);
                    precompute_all_cycles(app.settings_panel->preview_system.hex_list,
                                         TARGET_FPS,
                                         app.config.breath_duration);

                    // Réinitialiser le temps d'animation
                    app.settings_panel->preview_system.current_time = 0.0;
                    app.settings_panel->preview_system.last_update = SDL_GetTicks();

                    debug_printf("✅ Preview réinitialisé\n");
                }

                // ═════════════════════════════════════════════════════════════════
                // DÉTRUIRE LE PANNEAU STATS
                // ═════════════════════════════════════════════════════════════════
                // Le panneau stats doit être détruit pour permettre une nouvelle session
                if (app.stats_panel) {
                    destroy_stats_panel(app.stats_panel);
                    app.stats_panel = NULL;
                    debug_printf("🗑️  Panneau stats détruit\n");
                }

                // 🆕 La zone cliquable sera automatiquement mise à jour par le rendu
                // via la ligne: app->wim_clickable_rect = img_rect; dans render_app()
                // Pas besoin de la recalculer manuellement ici
            }
        }

        // RENDU COMPLET
        render_app(&app);

        // Régulation FPS adaptatif
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < current_frame_delay) {
            SDL_Delay(current_frame_delay - frame_time);
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
        SAFE_FREE(app.session_times);
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
