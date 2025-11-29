// SPDX-License-Identifier: GPL-3.0-or-later
#include "whm.h"
#include "core/config.h"
#include "core/debug.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ════════════════════════════════════════════════════════════════════════
// PROTOTYPES PRIVÉS
// ════════════════════════════════════════════════════════════════════════
static void whm_init(TechniqueInstance* self, SDL_Renderer* renderer);
static void whm_handle_event(TechniqueInstance* self, SDL_Event* event);
static void whm_update(TechniqueInstance* self, float delta_time);
static void whm_render(TechniqueInstance* self, SDL_Renderer* renderer);
static void whm_cleanup(TechniqueInstance* self);

// ════════════════════════════════════════════════════════════════════════
// CRÉATION DE L'INSTANCE
// ════════════════════════════════════════════════════════════════════════

/**
 * Créer une instance de la technique Wim Hof
 */
TechniqueInstance* whm_create(SDL_Renderer* renderer) {
    TechniqueInstance* instance = malloc(sizeof(TechniqueInstance));
    if (!instance) {
        fprintf(stderr, "Échec allocation mémoire pour instance WHM\n");
        return NULL;
    }

    WHMData* data = malloc(sizeof(WHMData));
    if (!data) {
        fprintf(stderr, "Échec allocation mémoire pour WHMData\n");
        free(instance);
        return NULL;
    }

    // Initialiser la structure WHMData à zéro
    memset(data, 0, sizeof(WHMData));

    // Stocker le renderer pour session_card_reset()
    data->renderer = renderer;

    // Configurer l'instance
    instance->name = "whm";
    instance->technique_data = data;
    instance->init = whm_init;
    instance->handle_event = whm_handle_event;
    instance->update = whm_update;
    instance->render = whm_render;
    instance->cleanup = whm_cleanup;
    instance->is_finished = false;
    instance->needs_high_fps = false;

    // Appel de l'initialisation
    whm_init(instance, renderer);

    return instance;
}

/**
 * Définir les hexagones (appelé par le core après création)
 */
void whm_set_hexagones(TechniqueInstance* instance, HexagoneList* hexagones) {
    if (!instance || !instance->technique_data) return;
    WHMData* data = (WHMData*)instance->technique_data;
    data->hexagones = hexagones;
}

/**
 * Définir les dimensions de l'écran
 */
void whm_set_screen_info(TechniqueInstance* instance, int width, int height, float scale_factor) {
    if (!instance || !instance->technique_data) return;
    WHMData* data = (WHMData*)instance->technique_data;
    data->screen_width = width;
    data->screen_height = height;
    data->scale_factor = scale_factor;

    // Maintenant qu'on a les dimensions, créer les composants qui en ont besoin
    AppConfig config;
    load_config(&config);

    // Créer la carte de session
    if (!data->session_card && data->hexagones && data->session_controller) {
        data->session_card = session_card_create(
            data->session_controller->current_session,
            width,
            height,
            FONT_ARIAL_BOLD,
            scale_factor
        );
        debug_printf("✅ [WHM] Carte de session créée\n");
    }

    // 🆕 Mettre à jour la session_card si elle existe déjà (responsive lors redimensionnement)
    if (data->session_card) {
        session_card_update_screen_size(data->session_card, width, height, scale_factor);
        debug_printf("✅ [WHM] Session_card mise à jour responsive: %dx%d, scale=%.2f\n",
                   width, height, scale_factor);
    }

    // Créer le compteur de respirations (nécessite renderer, qu'on n'a pas ici)
    // On le créera dans renderer.c lors du clic sur Wim
    debug_printf("✅ [WHM] Infos écran configurées: %dx%d, scale=%.2f\n",
                 width, height, scale_factor);
}

/**
 * Créer le compteur de respirations
 */
void whm_create_counter(TechniqueInstance* instance, SDL_Renderer* renderer) {
    if (!instance || !instance->technique_data) return;
    WHMData* data = (WHMData*)instance->technique_data;

    // Ne créer que si pas déjà créé
    if (data->breath_counter) return;

    // Charger la config
    AppConfig config;
    load_config(&config);

    // Calculer les paramètres
    int container_size = (data->screen_width < data->screen_height)
        ? data->screen_width : data->screen_height;
    float size_ratio = 0.75f;
    int smallest_hex_radius = (int)(container_size * size_ratio * 0.5f);
    int counter_font_size = (int)(smallest_hex_radius * 0.7f);

    // Récupérer les scales depuis le premier hexagone
    if (!data->hexagones || !data->hexagones->first) {
        debug_printf("⚠️  [WHM] Pas d'hexagones pour créer le compteur\n");
        return;
    }

    double scale_min = data->hexagones->first->animation->scale_min;
    double scale_max = data->hexagones->first->animation->scale_max;

    // 🆕 Déterminer le type de rétention depuis le session_controller
    // (utilise le nouveau système retention_pattern au lieu de retention_type)
    int retention_type_for_counter = 0;  // Par défaut : poumons pleins
    if (data->session_controller) {
        retention_type_for_counter = session_controller_should_use_empty_lungs(data->session_controller) ? 1 : 0;
        debug_printf("🔍 [WHM] Type rétention pour compteur: %s (depuis session_controller)\n",
                   retention_type_for_counter == 0 ? "poumons PLEINS" : "poumons VIDES");
    }

    // Créer le compteur
    data->breath_counter = counter_create(
        renderer,
        config.Nb_respiration,
        retention_type_for_counter,
        FONT_ARIAL_BOLD,
        counter_font_size,
        scale_min,
        scale_max,
        TARGET_FPS,
        config.breath_duration
    );

    if (data->breath_counter) {
        data->breath_counter->is_active = false;
        debug_printf("✅ [WHM] Compteur créé: 0/%d respirations\n", config.Nb_respiration);
    } else {
        debug_printf("❌ [WHM] Échec création compteur\n");
    }
}

// ════════════════════════════════════════════════════════════════════════
// IMPLÉMENTATION DES FONCTIONS DE CYCLE DE VIE
// ════════════════════════════════════════════════════════════════════════

/**
 * Initialisation de la technique WHM
 */
static void whm_init(TechniqueInstance* self, SDL_Renderer* renderer) {
    (void)renderer;  // Pas utilisé pour l'instant
    WHMData* data = (WHMData*)self->technique_data;

    // Charger la configuration
    AppConfig config;
    load_config(&config);

    // ════════════════════════════════════════════════════════════════════
    // CRÉATION DU TIMER DE SESSION
    // ════════════════════════════════════════════════════════════════════
    int timer_duration = config.start_duration;
    int timer_font_size = 48;  // Sera ajusté plus tard avec screen_info

    data->session_timer = breathing_timer_create(timer_duration, FONT_ARIAL_BOLD, timer_font_size);
    if (data->session_timer) {
        timer_start(data->session_timer);
        data->timer_phase = true;
        debug_printf("✅ [WHM] Timer créé: %d secondes\n", timer_duration);
    } else {
        fprintf(stderr, "⚠️  [WHM] Échec création timer - démarrage direct\n");
        data->timer_phase = false;
    }

    // ════════════════════════════════════════════════════════════════════
    // CRÉATION DU COMPTEUR DE RESPIRATIONS
    // ════════════════════════════════════════════════════════════════════
    // Note: le compteur sera créé plus tard avec les bonnes dimensions
    // car il nécessite le renderer et les scale_min/max des hexagones
    data->breath_counter = NULL;
    data->counter_phase = false;

    // ════════════════════════════════════════════════════════════════════
    // CRÉATION DU CHRONOMÈTRE
    // ════════════════════════════════════════════════════════════════════
    int chrono_font_size = 36;
    data->session_stopwatch = stopwatch_create(FONT_ARIAL_BOLD, chrono_font_size);
    data->chrono_phase = false;

    // ════════════════════════════════════════════════════════════════════
    // CRÉATION DU TIMER DE RÉTENTION (15 secondes par défaut)
    // ════════════════════════════════════════════════════════════════════
    int retention_duration = 15;  // Durée fixe de rétention
    data->retention_timer = breathing_timer_create(retention_duration, FONT_ARIAL_BOLD, timer_font_size);
    data->retention_phase = false;

    // ════════════════════════════════════════════════════════════════════
    // CRÉATION DE LA CARTE DE SESSION
    // ════════════════════════════════════════════════════════════════════
    // Note: sera créée plus tard avec les bonnes dimensions d'écran
    data->session_card = NULL;
    data->session_card_phase = false;

    // ════════════════════════════════════════════════════════════════════
    // CRÉATION DU CONTRÔLEUR DE SESSION
    // ════════════════════════════════════════════════════════════════════
    RetentionConfig retention_config;

    // Si nouveau système configuré (retention_pattern), utiliser ça
    // Sinon, convertir depuis l'ancien retention_type
    if (config.retention_pattern >= 0 && config.retention_pattern <= 4) {
        retention_config = retention_config_create(
            (RetentionPattern)config.retention_pattern,
            config.retention_start_empty
        );
    } else {
        // Fallback : convertir l'ancien système
        retention_config = retention_config_from_legacy_type(config.retention_type);
    }

    data->session_controller = session_controller_create(
        config.nb_session,
        retention_config
    );

    if (!data->session_controller) {
        fprintf(stderr, "❌ [WHM] Échec création session controller\n");
    }

    // FPS par défaut
    self->needs_high_fps = true;  // Animation active au démarrage

    debug_printf("✅ [WHM] Technique initialisée (session 1/%d, pattern: %s)\n",
                 config.nb_session,
                 retention_pattern_get_name(retention_config.pattern));
}

/**
 * Gestion des événements
 */
static void whm_handle_event(TechniqueInstance* self, SDL_Event* event) {
    WHMData* data = (WHMData*)self->technique_data;

    if (event->type == SDL_KEYDOWN) {
        // ESC : retour à l'écran d'accueil
        if (event->key.keysym.sym == SDLK_ESCAPE) {
            debug_printf("🔙 [WHM] ESC pressé - retour à l'accueil\n");
            self->is_finished = true;
            return;
        }

        // ESPACE : arrêter le chronomètre
        if (event->key.keysym.sym == SDLK_SPACE) {
            if (data->chrono_phase && data->session_stopwatch) {
                // Arrêter le chronomètre et passer à l'inspiration
                stopwatch_stop(data->session_stopwatch);
                float elapsed = (float)data->session_stopwatch->elapsed_seconds;

                // Stocker le temps de la session
                if (data->session_controller) {
                    session_controller_record_time(data->session_controller, elapsed);
                }

                // Passer à la phase d'inspiration
                data->chrono_phase = false;
                data->inspiration_phase = true;
                self->needs_high_fps = true;

                debug_printf("⏸️  [WHM] Chrono arrêté (ESPACE): %.1fs - début inspiration\n", elapsed);
            }
        }
    }

    // CLIC GAUCHE : arrêter le chronomètre
    if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        if (data->chrono_phase && data->session_stopwatch) {
            // Arrêter le chronomètre et passer à l'inspiration
            stopwatch_stop(data->session_stopwatch);
            float elapsed = (float)data->session_stopwatch->elapsed_seconds;

            // Stocker le temps de la session
            if (data->session_controller) {
                session_controller_record_time(data->session_controller, elapsed);
            }

            // Passer à la phase d'inspiration
            data->chrono_phase = false;
            data->inspiration_phase = true;
            self->needs_high_fps = true;

            debug_printf("⏸️  [WHM] Chrono arrêté (CLIC): %.1fs - début inspiration\n", elapsed);
        }
    }
}

/**
 * Mise à jour de l'état (appelée chaque frame)
 */
static void whm_update(TechniqueInstance* self, float delta_time) {
    WHMData* data = (WHMData*)self->technique_data;

    // ════════════════════════════════════════════════════════════════════
    // PHASE 1 : TIMER AVANT SESSION
    // ════════════════════════════════════════════════════════════════════
    if (data->timer_phase && data->session_timer) {
        bool timer_running = timer_update(data->session_timer);

        if (!timer_running) {
            debug_printf("⏱️  [WHM] Timer terminé - début carte de session\n");
            data->timer_phase = false;
            data->session_card_phase = true;

            // Démarrer l'animation de la carte
            if (data->session_card) {
                session_card_start(data->session_card);
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // PHASE 2 : CARTE DE SESSION
    // ════════════════════════════════════════════════════════════════════
    if (data->session_card_phase && data->session_card) {
        bool card_running = session_card_update(data->session_card, delta_time);

        if (!card_running) {
            debug_printf("🎴 [WHM] Carte de session terminée - début respiration\n");
            data->session_card_phase = false;
            data->counter_phase = true;

            // 🆕 POSITIONNER LA "TÊTE DE LECTURE" SUR SCALE_MIN (poumons vides)
            // CRITIQUE : La respiration Wim Hof démarre TOUJOURS poumons vides
            if (data->hexagones) {
                HexagoneNode* node = data->hexagones->first;
                while (node) {
                    // Chercher la première frame avec scale_min
                    bool frame_found = false;
                    for (int frame = 0; frame < node->total_cycles && !frame_found; frame++) {
                        if (node->precomputed_counter_frames &&
                            node->precomputed_counter_frames[frame].is_at_scale_min) {
                            // Positionner la tête de lecture sur cette frame
                            node->current_cycle = frame;
                            frame_found = true;
                            debug_printf("🎯 [WHM] Hexagone %d positionné sur scale_min (frame %d)\n",
                                       node->data->element_id, frame);
                        }
                    }

                    // Dégeler l'animation
                    node->is_frozen = false;
                    node = node->next;
                }
            }

            // Démarrer le compteur
            if (data->breath_counter) {
                data->breath_counter->is_active = true;
                debug_printf("🫁 [WHM] Compteur activé - lecture depuis scale_min\n");
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // PHASE 3 : COMPTEUR DE RESPIRATIONS
    // ════════════════════════════════════════════════════════════════════
    if (data->counter_phase && data->breath_counter) {
        // Le compteur se met à jour automatiquement via le système de cache
        // Vérifier s'il est terminé
        if (!data->breath_counter->is_active) {
            bool is_empty_lungs = session_controller_should_use_empty_lungs(data->session_controller);
            debug_printf("💨 [WHM] Respirations terminées (rétention: %s) - début réapparition\n",
                        is_empty_lungs ? "poumons VIDES" : "poumons PLEINS");

            data->counter_phase = false;
            data->reappear_phase = true;

            // ⚠️ Le compteur a déjà géré la disparition (pas de 11ème chiffre)
            // La phase 4 va resynchroniser tous les hexagones
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // PHASE 4 : RÉAPPARITION DE L'HEXAGONE
    // ════════════════════════════════════════════════════════════════════
    if (data->reappear_phase) {
        static bool reappear_initialized = false;

        // 🎯 INITIALISATION : resynchroniser tous les hexagones
        // IMPORTANT : Cette phase fait TOUJOURS mid_scale → scale_max (inspire)
        // pour les deux types de rétention (poumons pleins ET poumons vides)
        if (!reappear_initialized) {
            if (data->hexagones) {
                HexagoneNode* node = data->hexagones->first;
                while (node) {
                    if (node->precomputed_counter_frames && node->total_cycles > 0) {
                        int start_frame = -1;
                        double scale_mid = 0.5;

                        // TOUJOURS chercher 0.5 → 1.0 (montée vers scale_max)
                        for (int i = node->total_cycles - 1; i >= 0; i--) {
                            if (node->precomputed_counter_frames[i].relative_breath_scale <= scale_mid) {
                                start_frame = i;
                                break;
                            }
                        }

                        if (start_frame >= 0) {
                            node->current_cycle = start_frame;
                            debug_printf("🎯 [WHM] Hexagone %d: sync frame %d (scale %.2f → 1.0 INSPIRE)\n",
                                       node->data->element_id, start_frame,
                                       node->precomputed_counter_frames[start_frame].relative_breath_scale);
                        }
                    }

                    // Ne pas figer : l'animation continue vers scale_max
                    node->is_frozen = false;
                    node = node->next;
                }
            }

            reappear_initialized = true;
            debug_printf("✅ [WHM] Hexagones resynchronisés - animation vers scale_max (INSPIRE)\n");
        }

        // 🎯 VÉRIFIER si tous les hexagones ont atteint scale_max
        bool all_at_target = true;

        if (data->hexagones) {
            HexagoneNode* node = data->hexagones->first;
            while (node) {
                if (node->precomputed_counter_frames && node->current_cycle < node->total_cycles) {
                    CounterFrame* frame = &node->precomputed_counter_frames[node->current_cycle];

                    // TOUJOURS vérifier scale_max
                    if (!frame->is_at_scale_max) {
                        all_at_target = false;
                        break;
                    }
                }
                node = node->next;
            }
        }

        // Si tous les hexagones sont à scale_max → passer en phase CHRONO
        if (all_at_target) {
            data->reappear_phase = false;
            data->chrono_phase = true;
            reappear_initialized = false;  // Reset pour la prochaine session

            // FIGER L'ANIMATION à scale_max
            if (data->hexagones) {
                HexagoneNode* node = data->hexagones->first;
                while (node) {
                    node->is_frozen = true;
                    node = node->next;
                }
            }

            // DÉMARRER LE CHRONOMÈTRE
            bool is_empty_lungs = session_controller_should_use_empty_lungs(data->session_controller);
            if (data->session_stopwatch) {
                stopwatch_start(data->session_stopwatch);
                debug_printf("⏱️  [WHM] Phase CHRONO activée - chronomètre démarré (rétention: %s)\n",
                           is_empty_lungs ? "poumons VIDES" : "poumons PLEINS");
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // PHASE 5 : CHRONOMÈTRE (méditation)
    // ════════════════════════════════════════════════════════════════════
    if (data->chrono_phase && data->session_stopwatch) {
        // Mettre à jour le chronomètre
        stopwatch_update(data->session_stopwatch);
        self->needs_high_fps = false;  // Économie d'énergie pendant la méditation
    }

    // ════════════════════════════════════════════════════════════════════
    // PHASE 6 : INSPIRATION/EXPIRATION (après arrêt du chronomètre)
    // ════════════════════════════════════════════════════════════════════
    // Animation selon le type de rétention configuré :
    // - Poumons PLEINS → EXPIRATION (scale_max → scale_min) + timer à min
    // - Poumons VIDES  → INSPIRATION (scale_min → scale_max) + timer à max
    if (data->inspiration_phase) {
        static bool inspiration_initialized = false;

        if (!inspiration_initialized) {
            bool is_empty_lungs = session_controller_should_use_empty_lungs(data->session_controller);

            if (data->hexagones) {
                HexagoneNode* node = data->hexagones->first;
                while (node) {
                    if (node->precomputed_counter_frames && node->total_cycles > 0) {
                        int target_frame = -1;

                        if (is_empty_lungs) {
                            // 🫁 POUMONS VIDES : INSPIRER (scale_min → scale_max)
                            // Chercher scale_min comme point de départ (déjà à cette position)
                            for (int i = node->total_cycles - 1; i >= 0; i--) {
                                if (node->precomputed_counter_frames[i].is_at_scale_min) {
                                    target_frame = i;
                                    break;
                                }
                            }

                            if (target_frame >= 0) {
                                node->current_cycle = target_frame;
                                debug_printf("🫁 [WHM] Hexagone %d: INSPIRE scale_min → scale_max (frame %d)\n",
                                           node->data->element_id, target_frame);
                            }
                        } else {
                            // 💨 POUMONS PLEINS : EXPIRER (scale_max → scale_min)
                            // Chercher scale_max comme point de départ (déjà à cette position)
                            for (int i = node->total_cycles - 1; i >= 0; i--) {
                                if (node->precomputed_counter_frames[i].is_at_scale_max) {
                                    target_frame = i;
                                    break;
                                }
                            }

                            if (target_frame >= 0) {
                                node->current_cycle = target_frame;
                                debug_printf("💨 [WHM] Hexagone %d: EXPIRE scale_max → scale_min (frame %d)\n",
                                           node->data->element_id, target_frame);
                            }
                        }
                    }

                    // Dégeler l'animation
                    node->is_frozen = false;
                    node = node->next;
                }
            }

            inspiration_initialized = true;
            debug_printf("🎬 [WHM] Animation %s démarrée\n",
                       is_empty_lungs ? "INSPIRATION (min → max)" : "EXPIRATION (max → min)");
        }

        // Vérifier si tous les hexagones ont atteint la cible
        bool all_at_target = true;
        bool is_empty_lungs = session_controller_should_use_empty_lungs(data->session_controller);

        if (data->hexagones) {
            HexagoneNode* node = data->hexagones->first;
            while (node) {
                if (node->precomputed_counter_frames && node->current_cycle < node->total_cycles) {
                    CounterFrame* frame = &node->precomputed_counter_frames[node->current_cycle];

                    if (is_empty_lungs) {
                        // POUMONS VIDES : attendre scale_max (après inspiration)
                        if (!frame->is_at_scale_max) {
                            all_at_target = false;
                            break;
                        }
                    } else {
                        // POUMONS PLEINS : attendre scale_min (après expiration)
                        if (!frame->is_at_scale_min) {
                            all_at_target = false;
                            break;
                        }
                    }
                }
                node = node->next;
            }
        }

        // Si tous à la cible → activer phase de rétention
        if (all_at_target) {
            data->inspiration_phase = false;
            data->retention_phase = true;
            inspiration_initialized = false;  // Reset pour la prochaine fois

            // Figer l'animation
            if (data->hexagones) {
                HexagoneNode* node = data->hexagones->first;
                while (node) {
                    node->is_frozen = true;
                    node = node->next;
                }
            }

            // Démarrer le timer de rétention (15 secondes)
            if (data->retention_timer) {
                timer_start(data->retention_timer);
                debug_printf("⏱️  [WHM] Phase RÉTENTION activée - timer 15s (figé à %s)\n",
                           is_empty_lungs ? "scale_max (poumons PLEINS après inspire)" :
                                          "scale_min (poumons VIDES après expire)");
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // PHASE 7 : RÉTENTION (poumons pleins OU vides, timer 15s)
    // ════════════════════════════════════════════════════════════════════
    if (data->retention_phase && data->retention_timer) {
        bool timer_running = timer_update(data->retention_timer);

        if (!timer_running) {
            // Timer terminé → fin de la rétention
            data->retention_phase = false;
            debug_printf("✅ [WHM] Phase RÉTENTION terminée\n");

            // 🆕 GESTION BOUCLE DE SESSIONS avec Session Controller
            if (!session_controller_is_last_session(data->session_controller)) {
                // Passer à la session suivante
                session_controller_next_session(data->session_controller);

                // Réinitialiser la carte avec le nouveau numéro
                if (data->session_card && data->renderer) {
                    int next_session = data->session_controller->current_session;
                    session_card_reset(data->session_card, next_session, data->renderer);
                    session_card_start(data->session_card);
                    data->session_card_phase = true;

                    debug_printf("🔄 [WHM] Nouvelle session: %d/%d (pattern: %s)\n",
                               next_session,
                               data->session_controller->total_sessions,
                               session_controller_should_use_empty_lungs(data->session_controller) ?
                                   "poumons vides" : "poumons pleins");
                }

                // Réinitialiser le compteur de respirations
                if (data->breath_counter) {
                    data->breath_counter->current_breath = 0;
                    data->breath_counter->was_at_min_last_frame = false;
                    data->breath_counter->waiting_for_scale_min = false;
                    data->breath_counter->was_at_max_last_frame = false;
                    // Mettre à jour le type de rétention pour la nouvelle session
                    data->breath_counter->retention_type = session_controller_should_use_empty_lungs(data->session_controller) ? 1 : 0;
                    debug_printf("🔄 [WHM] Compteur mis à jour: retention_type=%d (%s)\n",
                               data->breath_counter->retention_type,
                               data->breath_counter->retention_type == 0 ? "poumons PLEINS" : "poumons VIDES");
                }

                // Réinitialiser le timer de rétention pour la prochaine session
                if (data->retention_timer) {
                    timer_reset(data->retention_timer);
                }
            } else {
                // Toutes les sessions terminées
                debug_printf("🎉 [WHM] Toutes les sessions terminées (%d/%d)\n",
                           data->session_controller->current_session,
                           data->session_controller->total_sessions);

                // Libérer les données précompilées (~100 MB)
                if (data->hexagones) {
                    free_precomputed_data(data->hexagones);

                    // 🔥 FIX CRUCIAL: Repositionner les hexagones à scale_max pour le prochain stage
                    // Les hexagones sont actuellement à scale_min (fin rétention poumons pleins)
                    // On les repositionne à scale_max INVISIBLEMENT (aucune phase active)
                    // pour que le prochain clic sur Wim démarre avec un hexagone de bonne taille
                    HexagoneNode* node = data->hexagones->first;
                    while (node && node->data) {
                        // Recalculer les vx/vy de base à scale_max
                        // Les vx/vy sont des coordonnées relatives, on les multiplie par animation->scale_max
                        if (node->animation) {
                            // Recalculer les sommets à la taille de base
                            int container_size = (data->screen_width < data->screen_height) ?
                                               data->screen_width : data->screen_height;
                            recalculer_sommets(node->data, container_size);

                            // Les vx/vy sont maintenant à la taille de base
                            // current_scale sera appliqué au rendu (déjà = scale_factor)

                            debug_printf("🔄 [WHM] Hexagone %d repositionné pour prochain stage\n",
                                       node->data->element_id);
                        }
                        node = node->next;
                    }
                }

                // Marquer comme terminé pour retour à l'écran d'accueil
                // Le main.c affichera le stats_panel
                self->is_finished = true;
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // ANIMATION DES HEXAGONES
    // ════════════════════════════════════════════════════════════════════
    if (data->hexagones && !data->chrono_phase) {
        HexagoneNode* node = data->hexagones->first;
        while (node) {
            if (!node->is_frozen) {
                apply_precomputed_frame(node);
            }
            node = node->next;
        }
    }
}

/**
 * Rendu de la technique
 */
static void whm_render(TechniqueInstance* self, SDL_Renderer* renderer) {
    WHMData* data = (WHMData*)self->technique_data;

    // ════════════════════════════════════════════════════════════════════
    // RENDU DES HEXAGONES
    // ════════════════════════════════════════════════════════════════════
    if (data->hexagones && (data->timer_phase || data->counter_phase ||
                            data->reappear_phase || data->chrono_phase ||
                            data->inspiration_phase || data->retention_phase)) {
        HexagoneNode* node = data->hexagones->first;
        while (node) {
            make_hexagone(renderer, node->data);
            node = node->next;
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // RENDU DES COMPOSANTS (overlay)
    // ════════════════════════════════════════════════════════════════════

    // Timer
    if (data->timer_phase && data->session_timer && data->hexagones) {
        HexagoneNode* first_node = data->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les infos de position de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            // Calculer le rayon RÉEL (coordonnées relatives × current_scale)
            float dx = first_node->data->vx[0] * first_node->data->current_scale;
            float dy = first_node->data->vy[0] * first_node->data->current_scale;
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            timer_render(data->session_timer, renderer,
                        hex_center_x, hex_center_y, hex_radius);
        }
    }

    // Carte de session
    if (data->session_card_phase && data->session_card) {
        session_card_render(data->session_card, renderer);
    }

    // Compteur
    if (data->counter_phase && data->breath_counter && data->hexagones) {
        HexagoneNode* first_node = data->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les infos de position de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            // Calculer le rayon RÉEL (coordonnées relatives × current_scale)
            float dx = first_node->data->vx[0] * first_node->data->current_scale;
            float dy = first_node->data->vy[0] * first_node->data->current_scale;
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            counter_render(data->breath_counter, renderer,
                          hex_center_x, hex_center_y,
                          hex_radius, first_node, data->scale_factor);
        }
    }

    // Chronomètre
    if (data->chrono_phase && data->session_stopwatch && data->hexagones) {
        HexagoneNode* first_node = data->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les infos de position de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            // Calculer le rayon RÉEL (coordonnées relatives × current_scale)
            float dx = first_node->data->vx[0] * first_node->data->current_scale;
            float dy = first_node->data->vy[0] * first_node->data->current_scale;
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            stopwatch_render(data->session_stopwatch, renderer,
                           hex_center_x, hex_center_y, hex_radius);
        }
    }

    // Timer de rétention
    if (data->retention_phase && data->retention_timer && data->hexagones) {
        HexagoneNode* first_node = data->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les infos de position de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            // Calculer le rayon RÉEL (coordonnées relatives × current_scale)
            float dx = first_node->data->vx[0] * first_node->data->current_scale;
            float dy = first_node->data->vy[0] * first_node->data->current_scale;
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            timer_render(data->retention_timer, renderer,
                        hex_center_x, hex_center_y, hex_radius);
        }
    }
}

/**
 * Nettoyage de la technique
 */
static void whm_cleanup(TechniqueInstance* self) {
    if (!self || !self->technique_data) return;

    WHMData* data = (WHMData*)self->technique_data;

    debug_printf("🧹 [WHM] Nettoyage de la technique\n");

    // Libérer les composants
    if (data->session_timer) timer_destroy(data->session_timer);
    if (data->breath_counter) counter_destroy(data->breath_counter);
    if (data->session_stopwatch) stopwatch_destroy(data->session_stopwatch);
    if (data->retention_timer) timer_destroy(data->retention_timer);
    if (data->session_card) session_card_destroy(data->session_card);

    // Détruire le contrôleur de session
    if (data->session_controller) {
        session_controller_destroy(data->session_controller);
    }

    // Libérer la structure de données
    free(data);
    self->technique_data = NULL;
}
