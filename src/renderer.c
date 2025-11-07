// SPDX-License-Identifier: GPL-3.0-or-later
#include "renderer.h"
#include "precompute_list.h"
#include "widget_base.h"
#include "json_config_loader.h"
#include "debug.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>




// ═════════════════════════════════════════════════════════════════════════════
// SYSTÈME D'ÉCHELLE RESPONSIVE
// ═════════════════════════════════════════════════════════════════════════════

// Résolution de référence (HD Ready)
#define REFERENCE_WIDTH  1280
#define REFERENCE_HEIGHT 720

// Limites du facteur d'échelle
#define MIN_SCALE 0.3f   // Très petits écrans (smartwatches, etc.)
#define MAX_SCALE 3.0f   // Très grands écrans (4K+)

// ─────────────────────────────────────────────────────────────────────────────
// CALCULE LE FACTEUR D'ÉCHELLE EN FONCTION DE LA TAILLE D'ÉCRAN
// ─────────────────────────────────────────────────────────────────────────────
// Cette fonction calcule un facteur d'échelle uniforme basé sur la résolution
// actuelle par rapport à la résolution de référence (1280x720).
//
// Logique :
//   1. Calcule le ratio largeur et hauteur séparément
//   2. Prend le MINIMUM des deux pour garder tout visible
//   3. Applique des limites (0.3 à 3.0)
//
// Exemples :
//   - 1280x720  → scale = 1.0  (référence)
//   - 1920x1080 → scale = 1.5  (Full HD)
//   - 3840x2160 → scale = 3.0  (4K, plafonné)
//   - 800x480   → scale = 0.625 (petit écran)
//   - 360x640   → scale = 0.28 (smartphone)
// ─────────────────────────────────────────────────────────────────────────────
float calculate_scale_factor(int width, int height) {
    // Calculer les ratios par rapport à la référence
    float width_ratio = (float)width / REFERENCE_WIDTH;
    float height_ratio = (float)height / REFERENCE_HEIGHT;

    // Prendre le minimum pour garantir que tout reste visible
    float scale = (width_ratio < height_ratio) ? width_ratio : height_ratio;

    // Appliquer les limites
    if (scale < MIN_SCALE) scale = MIN_SCALE;
    if (scale > MAX_SCALE) scale = MAX_SCALE;

    return scale;
}

// ─────────────────────────────────────────────────────────────────────────────
// APPLIQUE LE FACTEUR D'ÉCHELLE À UNE VALEUR
// ─────────────────────────────────────────────────────────────────────────────
// Fonction utilitaire pour scaler n'importe quelle dimension
// ─────────────────────────────────────────────────────────────────────────────
int scale_value(int value, float scale) {
    return (int)(value * scale);
}

// ─────────────────────────────────────────────────────────────────────────────
// CALCULE LA LARGEUR DU PANNEAU EN FONCTION DE L'ÉCRAN
// ─────────────────────────────────────────────────────────────────────────────
// Règles spéciales :
//   - Téléphone (< 600px) : 100% de la largeur
//   - Tablette/Desktop : 500px * scale, max 80% de l'écran
// ─────────────────────────────────────────────────────────────────────────────
int calculate_panel_width(int screen_width, float scale) {
    const int BASE_PANEL_WIDTH = 500;  // Largeur de référence

    // Cas 1 : Téléphone (écran très étroit)
    if (screen_width < 600) {
        return screen_width;  // Prendre toute la largeur
    }

    // Cas 2 : Tablette/Desktop
    int scaled_width = scale_value(BASE_PANEL_WIDTH, scale);
    int max_width = (int)(screen_width * 0.8f);  // Maximum 80% de l'écran

    // Retourner le minimum entre la largeur scalée et le maximum
    return (scaled_width < max_width) ? scaled_width : max_width;
}


// Initialise toute la partie SDL et graphique
bool initialize_app(AppState* app, const char* title, const char* image_path) {
    // 1. Initialisation SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("ERREUR SDL_Init: %s", SDL_GetError());
        return false;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 1.5 Initialisation TTF et gestionnaire de polices
    // ═══════════════════════════════════════════════════════════════════════════
    if (TTF_Init() == -1) {
        debug_printf("❌ Erreur TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    debug_section("INITIALISATION POLICES");

    // Initialiser le gestionnaire avec le chemin de la police
    const char* font_path = "../fonts/arial/ARIAL.TTF";
    init_font_manager(font_path);

    // Fallback si la police n'existe pas
    if (!get_font_for_size(18)) {
        debug_printf("⚠️ Police par défaut introuvable, essai fallback...\n");
        init_font_manager("/usr/share/fonts/gnu-free/FreeSans.otf");

        if (!get_font_for_size(18)) {
            debug_printf("❌ Aucune police disponible !\n");
            cleanup_font_manager();
            TTF_Quit();
            SDL_Quit();
            return false;
        }
    }

    debug_printf("✅ Gestionnaire de polices prêt\n");
    debug_blank_line();

    // 2. Création fenêtre plein écran
    app->window = SDL_CreateWindow(title,
                                   100, 100,  // Position sur l'écran
                                   1280, 720, // Taille fixe pour dev
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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

    // 5. Récupération taille FENÊTRE (pas écran) pour usage futur
    // ════════════════════════════════════════════════════════════════════════
    SDL_GetWindowSize(app->window, &app->screen_width, &app->screen_height);

    // ════════════════════════════════════════════════════════════════════════
    // 5b. CALCUL DU FACTEUR D'ÉCHELLE RESPONSIVE
    // ════════════════════════════════════════════════════════════════════════
    app->scale_factor = calculate_scale_factor(app->screen_width, app->screen_height);

    debug_printf("📐 Taille fenêtre : %dx%d\n", app->screen_width, app->screen_height);
    debug_printf("📏 Facteur d'échelle : %.2f\n", app->scale_factor);

    // 6. Initialisation des autres champs
    app->hexagones = NULL;
    app->is_running = true;
    app->settings_panel = create_settings_panel(
        app->renderer,
        app->window,       // ← Passer la fenêtre pour gérer la taille minimale
        app->screen_width,
        app->screen_height,
        app->scale_factor
    );

    // ════════════════════════════════════════════════════════════════════════
    // 6a. SYNCHRONISER CONFIG → WIDGETS
    // ════════════════════════════════════════════════════════════════════════
    // Les widgets sont créés avec les valeurs du JSON (valeur_depart)
    // Mais on doit les mettre à jour avec les valeurs de respiration.conf
    if (app->settings_panel && app->settings_panel->widget_list) {
        sync_config_to_widgets(&app->config, app->settings_panel->widget_list);
    }

    // ════════════════════════════════════════════════════════════════════════
    // 6b. DÉFINIR LA LARGEUR MINIMALE DE FENÊTRE
    // ════════════════════════════════════════════════════════════════════════
    // Empêcher que les widgets ne sortent de la fenêtre par la droite
    // en définissant une largeur minimale basée sur le plus grand widget
    if (app->settings_panel) {
        update_window_minimum_size(app->settings_panel, app->window);
    }

    // ════════════════════════════════════════════════════════════════════════
    // 6c. GÉNÉRATION AUTOMATIQUE DES TEMPLATES JSON
    // ════════════════════════════════════════════════════════════════════════
    // Générer templates.json si absent ou obsolète
    // Ce fichier contient des templates vierges pour chaque type de widget
    // utilisables dans l'éditeur JSON
    const char* widgets_config_path = "../config/widgets_config.json";
    const char* templates_output_path = "../src/json_editor/templates.json";

    // Toujours régénérer au démarrage pour garantir la synchronisation
    if (!generer_templates_json(widgets_config_path, templates_output_path)) {
        debug_printf("⚠️ Impossible de générer templates.json (non bloquant)\n");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DE LA FENÊTRE ÉDITEUR JSON
    // ─────────────────────────────────────────────────────────────────────────
    // ⚠️ IMPORTANT : Positionner la fenêtre de manière RESPONSIVE !
    //
    // PROBLÈME RÉSOLU : L'ancienne version utilisait une position fixe (1400px)
    // qui sortait de l'écran sur les petits moniteurs, causant un SDL_QUIT et
    // fermant immédiatement toute l'application !
    //
    // NOUVELLE LOGIQUE :
    // - Si l'écran est assez large (> 2000px) : placer à droite de la fenêtre
    // - Sinon : placer la fenêtre JSON au centre de l'écran
    // ─────────────────────────────────────────────────────────────────────────

    int editor_pos_x, editor_pos_y;

    // Récupérer la taille totale de l'écran (pas juste la fenêtre)
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);  // 0 = écran principal
    int screen_total_width = display_mode.w;
    int screen_total_height = display_mode.h;

    debug_printf("📺 Résolution écran détectée : %dx%d\n",
                 screen_total_width, screen_total_height);

    // ═════════════════════════════════════════════════════════════════════════
    // CHOIX INTELLIGENT DE LA POSITION
    // ═════════════════════════════════════════════════════════════════════════
    if (screen_total_width >= 2000) {
        // Écran large : placer à droite de la fenêtre principale
        int main_window_x, main_window_y;
        SDL_GetWindowPosition(app->window, &main_window_x, &main_window_y);

        editor_pos_x = main_window_x + app->screen_width + 20;  // 20px de marge
        editor_pos_y = main_window_y;

        debug_printf("🖥️ Écran large : JSON à droite de la fenêtre (%d, %d)\n",
                     editor_pos_x, editor_pos_y);
    } else {
        // Écran normal/petit : centrer la fenêtre JSON
        // EDITOR_WIDTH est défini dans json_editor.h (typiquement 600)
        // EDITOR_HEIGHT est défini dans json_editor.h (typiquement 800)
        const int JSON_EDITOR_WIDTH = 600;   // Valeur par défaut
        const int JSON_EDITOR_HEIGHT = 800;  // Valeur par défaut

        editor_pos_x = (screen_total_width - JSON_EDITOR_WIDTH) / 2;
        editor_pos_y = (screen_total_height - JSON_EDITOR_HEIGHT) / 2;

        // Sécurité : ne jamais sortir de l'écran
        if (editor_pos_x < 0) editor_pos_x = 50;
        if (editor_pos_y < 0) editor_pos_y = 50;

        debug_printf("💻 Écran standard : JSON centrée (%d, %d)\n",
                     editor_pos_x, editor_pos_y);
    }

    // Créer la fenêtre avec la position calculée
    app->json_editor = creer_json_editor(
        "../config/widgets_config.json",
        editor_pos_x,
        editor_pos_y
    );

    if (!app->json_editor) {
        debug_printf("⚠️ Impossible de créer l'éditeur JSON\n");
        // Ce n'est pas bloquant, on continue sans
    }

    // Chargement de la configuration
    load_config(&app->config);

    debug_printf("Application initialisée: %dx%d\n", app->screen_width, app->screen_height);
    return true;
}

// Gestion des événements de l'application
void handle_app_events(AppState* app, SDL_Event* event) {
    if (!app) return;

    // ─────────────────────────────────────────────────────────────────────────
    // PRIORITÉ 1 : Éditeur JSON (si ouvert)
    // ─────────────────────────────────────────────────────────────────────────
    if (app->json_editor && app->json_editor->est_ouvert) {
        if (gerer_evenements_json_editor(app->json_editor, event)) {
            return;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // PRIORITÉ 2 : Événements globaux
    // ─────────────────────────────────────────────────────────────────────────
    switch (event->type) {
        case SDL_QUIT:
            app->is_running = false;
            break;

        case SDL_WINDOWEVENT:
            // ════════════════════════════════════════════════════════════════
            // GESTION DES ÉVÉNEMENTS DE FENÊTRE
            // ════════════════════════════════════════════════════════════════
            // IMPORTANT : Filtrer UNIQUEMENT les événements de la fenêtre PRINCIPALE
            // pour éviter que l'éditeur JSON ne ferme l'application
            // ════════════════════════════════════════════════════════════════
        {
            // Récupérer l'ID de la fenêtre principale
            Uint32 main_window_id = SDL_GetWindowID(app->window);

            // IGNORER tous les événements qui ne concernent PAS la fenêtre principale
            if (event->window.windowID != main_window_id) {
                break;  // ← CRITIQUE : Ignorer les événements des autres fenêtres
            }

            // Maintenant on traite UNIQUEMENT les événements de la fenêtre principale
            if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
                // Fermeture de la fenêtre principale
                app->is_running = false;
                debug_printf("🚪 Fermeture de la fenêtre principale demandée\n");
            }
            else if (event->window.event == SDL_WINDOWEVENT_RESIZED ||
                event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                // 2. Redimensionnement de la fenêtre
                // ─────────────────────────────────────────────────────────────
                // Quand l'utilisateur redimensionne la fenêtre, on met à jour
                // les dimensions et on repositionne TOUS les éléments
                // ─────────────────────────────────────────────────────────────

                // Sauvegarder l'ancienne taille AVANT de la mettre à jour
                int old_width = app->screen_width;
                int old_height = app->screen_height;

                // Récupérer la nouvelle taille
                SDL_GetWindowSize(app->window, &app->screen_width, &app->screen_height);

                debug_printf("🔄 Fenêtre redimensionnée : %dx%d → %dx%d\n",
                             old_width, old_height,
                             app->screen_width, app->screen_height);

                // ═══════════════════════════════════════════════════════════════
                // RECALCULER LE FACTEUR D'ÉCHELLE
                // ═══════════════════════════════════════════════════════════════
                app->scale_factor = calculate_scale_factor(app->screen_width, app->screen_height);

                debug_printf("📏 Nouveau facteur d'échelle : %.2f\n", app->scale_factor);

                // ═══════════════════════════════════════════════════════════════
                // ÉTAPE 1 : RECENTRER L'HEXAGONE PRINCIPAL
                // ═══════════════════════════════════════════════════════════════
                if (app->hexagones && app->hexagones->first) {
                    // Calculer le nouveau centre de la fenêtre
                    int new_center_x = app->screen_width / 2;
                    int new_center_y = app->screen_height / 2;

                    // Calculer l'ancien centre (pour les offsets)
                    int old_center_x = old_width / 2;
                    int old_center_y = old_height / 2;

                    // Calculer la nouvelle taille du conteneur
                    int new_container_size = (app->screen_width < app->screen_height)
                    ? app->screen_width
                    : app->screen_height;

                    debug_printf("📐 Ancien centre: (%d,%d), Nouveau centre: (%d,%d)\n",
                                 old_center_x, old_center_y, new_center_x, new_center_y);

                    // IMPORTANT : Parcourir TOUS les hexagones
                    HexagoneNode* node = app->hexagones->first;
                    int hex_count = 0;

                    // ═══════════════════════════════════════════════════════════════
                    // CALCULER LE RATIO DE REDIMENSIONNEMENT
                    // ═══════════════════════════════════════════════════════════════
                    // Pour que les hexagones gardent leurs proportions, on calcule
                    // le ratio entre l'ancienne et la nouvelle taille de conteneur.
                    //
                    // Exemple : 1280x720 → 640x360 donne un ratio de 0.5
                    //           Les hexagones doivent être 2x plus petits
                    // ═══════════════════════════════════════════════════════════════
                    int old_container_size = (old_width < old_height) ? old_width : old_height;
                    float scale_ratio = (float)new_container_size / (float)old_container_size;

                    debug_printf("📏 Ratio redimensionnement : %.3f (container: %d→%d)\n",
                                 scale_ratio, old_container_size, new_container_size);

                    while (node && node->data) {
                        Hexagon* hex = node->data;

                        // ─────────────────────────────────────────────────────────────
                        // ÉTAPE 1 : REPOSITIONNER LE CENTRE
                        // ─────────────────────────────────────────────────────────────
                        // Calculer l'offset de CET hexagone par rapport à l'ANCIEN centre
                        int offset_x = hex->center_x - old_center_x;
                        int offset_y = hex->center_y - old_center_y;

                        // Appliquer le NOUVEAU centre + offset
                        hex->center_x = new_center_x + offset_x;
                        hex->center_y = new_center_y + offset_y;

                        // ─────────────────────────────────────────────────────────────
                        // ÉTAPE 2 : METTRE À JOUR L'ÉCHELLE
                        // ─────────────────────────────────────────────────────────────
                        // ⚠️ IMPORTANT : On ne recalcule PAS les sommets !
                        //
                        // Les hexagones utilisent un système de coordonnées RELATIVES :
                        // - vx[i], vy[i] = coordonnées relatives au centre (fixes)
                        // - current_scale = facteur d'échelle appliqué lors du rendu
                        //
                        // Dans make_hexagone() (geometry.c ligne 29) :
                        //   absolute_x = center_x + (vx[i] * current_scale)
                        //
                        // Donc pour redimensionner, on multiplie juste le scale !
                        // ─────────────────────────────────────────────────────────────
                        hex->current_scale *= scale_ratio;

                        debug_printf("  ✅ Hexagone %d - Centre:(%d,%d) Scale:%.3f\n",
                                     hex_count, hex->center_x, hex->center_y,
                                     hex->current_scale);

                        hex_count++;
                        node = node->next;
                    }

                    debug_printf("✅ %d hexagones redimensionnés (ratio: %.3f)\n",
                                 hex_count, scale_ratio);
                }

                // ═══════════════════════════════════════════════════════════════
                // ÉTAPE 2 : REPOSITIONNER LE PANNEAU DE CONFIGURATION
                // ═══════════════════════════════════════════════════════════════
                if (app->settings_panel) {
                    // Mettre à jour le scale du panneau
                    update_panel_scale(app->settings_panel,
                                       app->screen_width,
                                       app->screen_height,
                                       app->scale_factor);

                    debug_printf("✅ Panneau mis à jour avec nouveau scale\n");
                }
            }
        }
        break;

        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_ESCAPE) {
                app->is_running = false;
            }
            break;

            // ═════════════════════════════════════════════════════════════════
            // GESTION DES ÉVÉNEMENTS DU PANNEAU DE CONFIGURATION
            // ═════════════════════════════════════════════════════════════════
            // IMPORTANT : Les widgets ont besoin de 3 types d'événements :
            //   1. SDL_MOUSEMOTION    → détection du hovering (fond gris)
            //   2. SDL_MOUSEWHEEL     → modification valeur avec molette
            //   3. SDL_MOUSEBUTTONDOWN → clics sur flèches et boutons
            // ═════════════════════════════════════════════════════════════════
        case SDL_MOUSEMOTION:
        case SDL_MOUSEWHEEL:
        case SDL_MOUSEBUTTONDOWN:
            // Transmettre TOUS ces événements au panneau quand il existe
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

    // 2.5. Dessine le timer SI on est en phase timer
    if (app->timer_phase && app->session_timer && app->hexagones) {
        // Récupérer le premier hexagone (le plus grand) pour centrer le timer
        HexagoneNode* first_node = app->hexagones->first;
        if (first_node && first_node->data) {
            // Le rayon de l'hexagone est approximativement la distance du centre au sommet
            // On peut l'estimer via la taille actuelle de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            // Calculer le rayon approximatif (distance centre->sommet)
            // En utilisant les coordonnées relatives du premier point
            int dx = first_node->data->vx[0];
            int dy = first_node->data->vy[0];
            int hex_radius = (int)sqrt(dx*dx + dy*dy);
            // Rendre le timer centré sur l'hexagone
            timer_render(app->session_timer, app->renderer,
                         hex_center_x, hex_center_y, hex_radius);
        }
    }

    // 🆕 Dessine le compteur SI on est en phase compteur (après le timer)
    if (app->counter_phase && app->breath_counter && app->hexagones && app->hexagones->first) {
        HexagoneNode* first_node = app->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les mêmes infos que pour le timer
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            int dx = first_node->data->vx[0];
            int dy = first_node->data->vy[0];
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            // Utiliser le scale du premier hexagone pour l'effet fish-eye
            double current_scale = first_node->current_scale;

            counter_render(app->breath_counter, app->renderer,
                           hex_center_x, hex_center_y, hex_radius, current_scale);
        }
    }


    // 3. Dessine le panneau settings (par dessus)
    if (app->settings_panel) {
        render_settings_panel(app->renderer, app->settings_panel);
    }

    // 4. Présentation fenêtre principale
    SDL_RenderPresent(app->renderer);

    // ─────────────────────────────────────────────────────────────────────────
    // 5. RENDU DE LA FENÊTRE ÉDITEUR JSON (seulement si ouverte)
    // ─────────────────────────────────────────────────────────────────────────
    if (app->json_editor && app->json_editor->est_ouvert) {
        verifier_auto_save(app->json_editor);  // Auto-save pour hot reload
        rendre_json_editor(app->json_editor);
    } else if (app->json_editor && !app->json_editor->est_ouvert) {
        // ✅ Si la fenêtre est marquée comme fermée, la détruire
        detruire_json_editor(app->json_editor);
        app->json_editor = NULL;
        debug_printf("🗑️ Fenêtre JSON fermée\n");
    }
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

    // Libère l'éditeur JSON
    if (app->json_editor) {
        detruire_json_editor(app->json_editor);
        app->json_editor = NULL;
    }

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
    debug_printf("Application nettoyée\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  NOTES IMPORTANTES
// ════════════════════════════════════════════════════════════════════════════
//
// 🎯 FLUX DES ÉVÉNEMENTS :
//    1. L'éditeur JSON a la priorité (si ouvert)
//    2. Puis les événements globaux (ESC, fermeture)
//    3. Puis le panneau de configuration
//
// 🖼️ FLUX DE RENDU :
//    1. Fenêtre principale (hexagones + panneau)
//    2. Fenêtre éditeur JSON (indépendante)
//
// ⚠️ IMPORTANTE : SDL_TEXTINPUT
//    Pour que la saisie clavier fonctionne dans l'éditeur,
//    SDL_StartTextInput() est automatiquement activé par SDL.
//    Si tu veux désactiver la saisie ailleurs, utilise SDL_StopTextInput()
