// SPDX-License-Identifier: GPL-3.0-or-later
#include "renderer.h"
#include "precompute_list.h"
#include "widget_base.h"
#include "json_config_loader.h"
#include "session_card.h"
#include "debug.h"
#include "../instances/technique_instance.h"
#include "../instances/whm/whm.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H




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

// ════════════════════════════════════════════════════════════════════════════
// CRÉATION DU TITRE DE L'ÉCRAN D'ACCUEIL (Style Cairo métallisé)
// ════════════════════════════════════════════════════════════════════════════
static SDL_Texture* create_wim_title_texture(SDL_Renderer* renderer, const char* font_path) {
    // Initialiser FreeType
    FT_Library ft_library;
    FT_Face ft_face;

    if (FT_Init_FreeType(&ft_library)) {
        debug_printf("❌ Erreur initialisation FreeType pour titre\n");
        return NULL;
    }

    if (FT_New_Face(ft_library, font_path, 0, &ft_face)) {
        debug_printf("❌ Erreur chargement police pour titre\n");
        FT_Done_FreeType(ft_library);
        return NULL;
    }

    // Créer face Cairo
    cairo_font_face_t* cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);

    // Mesurer le texte pour calculer la taille de la surface
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    cairo_set_font_face(temp_cr, cairo_face);
    cairo_set_font_size(temp_cr, 30.0);

    cairo_text_extents_t ext1, ext2;
    cairo_text_extents(temp_cr, "Technique", &ext1);
    cairo_text_extents(temp_cr, "Wim Hof", &ext2);

    int width = (int)(fmax(ext1.width, ext2.width) + 200);
    int height = (int)(ext1.height + ext2.height + 25);  // Espace entre les lignes

    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);

    // Créer la surface finale
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    // Fond transparent
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);

    // Antialiasing
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    cairo_set_font_face(cr, cairo_face);
    cairo_set_font_size(cr, 30.0);

    // Position du texte "Technique"
    double y1 = 10 + ext1.height;
    double x1 = (width - ext1.width) / 2;

    // Gradient pour "Technique"
    cairo_pattern_t* grad1 = cairo_pattern_create_linear(0, y1 - ext1.height, 0, y1);
    cairo_pattern_add_color_stop_rgba(grad1, 0.0,  0.40, 0.45, 0.52, 1.0);
    cairo_pattern_add_color_stop_rgba(grad1, 0.25, 0.60, 0.65, 0.72, 1.0);
    cairo_pattern_add_color_stop_rgba(grad1, 0.7,  0.30, 0.35, 0.42, 1.0);
    cairo_pattern_add_color_stop_rgba(grad1, 1.0,  0.24, 0.28, 0.36, 1.0);

    cairo_move_to(cr, x1, y1);
    cairo_set_source(cr, grad1);
    cairo_show_text(cr, "Technique");
    cairo_pattern_destroy(grad1);

    // Position du texte "Wim Hof"
    double y2 = y1 + 10 + ext2.height;
    double x2 = (width - ext2.width) / 2;

    // Gradient pour "Wim Hof"
    cairo_pattern_t* grad2 = cairo_pattern_create_linear(0, y2 - ext2.height, 0, y2);
    cairo_pattern_add_color_stop_rgba(grad2, 0.0,  0.40, 0.45, 0.52, 1.0);
    cairo_pattern_add_color_stop_rgba(grad2, 0.25, 0.60, 0.65, 0.72, 1.0);
    cairo_pattern_add_color_stop_rgba(grad2, 0.7,  0.30, 0.35, 0.42, 1.0);
    cairo_pattern_add_color_stop_rgba(grad2, 1.0,  0.24, 0.28, 0.36, 1.0);

    cairo_move_to(cr, x2, y2);
    cairo_set_source(cr, grad2);
    cairo_show_text(cr, "Wim Hof");
    cairo_pattern_destroy(grad2);

    cairo_surface_flush(surface);

    // Convertir en SDL_Texture
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);

    SDL_Surface* sdl_surface = SDL_CreateRGBSurfaceWithFormatFrom(
        data, width, height, 32, stride, SDL_PIXELFORMAT_ARGB8888
    );

    SDL_Texture* texture = NULL;
    if (sdl_surface) {
        texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
        SDL_FreeSurface(sdl_surface);
    }

    // Nettoyage
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(cairo_face);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    return texture;
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

    // ═══════════════════════════════════════════════════════════════════════════
    // INITIALISATION FPS ADAPTATIF
    // ═══════════════════════════════════════════════════════════════════════════
    app->last_interaction_time = SDL_GetTicks();
    app->editor_has_focus = false;
    app->last_editor_event = 0;

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉCRAN D'ACCUEIL (Technique Wim Hof)
    // ═══════════════════════════════════════════════════════════════════════════
    app->waiting_to_start = true;  // Commence sur l'écran d'accueil

    // Charger l'image wim.png
    SDL_Surface* wim_surface = IMG_Load("../img/wim.png");
    if (!wim_surface) {
        debug_printf("⚠️  Impossible de charger wim.png: %s\n", IMG_GetError());
        app->wim_image = NULL;
    } else {
        app->wim_image = SDL_CreateTextureFromSurface(app->renderer, wim_surface);
        SDL_FreeSurface(wim_surface);

        if (!app->wim_image) {
            debug_printf("⚠️  Impossible de créer texture wim.png\n");
        }
    }

    // Créer le titre "Technique\nWim Hof" en Cairo
    app->wim_title = create_wim_title_texture(app->renderer, font_path);
    if (!app->wim_title) {
        debug_printf("⚠️  Impossible de créer titre Wim Hof\n");
    }

    debug_printf("Application initialisée: %dx%d\n", app->screen_width, app->screen_height);
    debug_printf("✅ Écran d'accueil Wim Hof créé\n");
    return true;
}

// Gestion des événements de l'application
void handle_app_events(AppState* app, SDL_Event* event) {
    if (!app) return;

    // ─────────────────────────────────────────────────────────────────────────
    // PRIORITÉ 1 : Éditeur JSON (si ouvert)
    // ─────────────────────────────────────────────────────────────────────────
    if (app->json_editor && app->json_editor->est_ouvert) {
        debug_printf("📝 JSON Editor OUVERT - traite événement type=%d\n", event->type);
        if (gerer_evenements_json_editor(app->json_editor, event)) {
            // L'éditeur a consommé l'événement → marquer pour FPS adaptatif
            app->last_editor_event = SDL_GetTicks();
            debug_printf("📝 JSON Editor a CONSOMMÉ l'événement\n");
            return;
        }
        debug_printf("📝 JSON Editor n'a PAS consommé l'événement\n");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // PRIORITÉ 2 : Écran d'accueil - Démarrage au clic sur l'image
    // ─────────────────────────────────────────────────────────────────────────
    if (app->waiting_to_start && event->type == SDL_MOUSEBUTTONDOWN) {
        debug_printf("🖱️  CLIC DÉTECTÉ : waiting_to_start=%d, type=%d\n",
                     app->waiting_to_start, event->type);

        if (event->button.button == SDL_BUTTON_LEFT) {
            int mouse_x = event->button.x;
            int mouse_y = event->button.y;

            // Calculer la zone cliquable avec la MÊME formule que le rendu
            // (exactement comme dans render_app() lignes 905-933)
            int title_w = 200, title_h = 60;
            if (app->wim_title) {
                SDL_QueryTexture(app->wim_title, NULL, NULL, &title_w, &title_h);
            }
            int total_height = title_h + 20 + 250;
            int start_y = (app->screen_height - total_height) / 2;

            SDL_Rect clickable_rect = {
                (app->screen_width - 250) / 2,
                start_y + title_h + 20,
                250,
                250
            };

            debug_printf("🖱️  Clic gauche à (%d,%d) - Zone cliquable: (%d,%d) %dx%d\n",
                         mouse_x, mouse_y,
                         clickable_rect.x, clickable_rect.y,
                         clickable_rect.w, clickable_rect.h);

            // Vérifier si le clic est dans la zone de l'image
            if (mouse_x >= clickable_rect.x &&
                mouse_x < clickable_rect.x + clickable_rect.w &&
                mouse_y >= clickable_rect.y &&
                mouse_y < clickable_rect.y + clickable_rect.h) {

                debug_printf("🚀 CLIC VALIDE - Démarrage technique Wim Hof...\n");

                // ═════════════════════════════════════════════════════════════════
                // ÉTAPE 1 : RECHARGER LA CONFIGURATION
                // ═════════════════════════════════════════════════════════════════
                load_config(&app->config);
                debug_printf("🔄 Configuration rechargée\n");

                // ═════════════════════════════════════════════════════════════════
                // ÉTAPE 2 : FERMER L'ÉDITEUR JSON ET LE PANNEAU SETTINGS
                // ═════════════════════════════════════════════════════════════════
                if (app->json_editor && app->json_editor->est_ouvert) {
                    app->json_editor->est_ouvert = false;
                    debug_printf("📝 Éditeur JSON fermé\n");
                }
                if (app->settings_panel) {
                    app->settings_panel->state = PANEL_CLOSED;
                    debug_printf("⚙️  Panneau settings fermé\n");
                }

                // ═════════════════════════════════════════════════════════════════
                // ÉTAPE 3 : PRÉ-CALCULS (~100 MB) - PARTIE DU CORE
                // ═════════════════════════════════════════════════════════════════
                // ⚠️  Libérer les anciennes données précompilées avant de réallouer
                // (évite memory leak si on reclique plusieurs fois)
                if (app->hexagones) {
                    free_precomputed_data(app->hexagones);
                    debug_printf("🗑️  Anciennes données précompilées libérées\n");
                }

                debug_printf("🔢 Lancement des pré-calculs (breath_duration=%.1fs)...\n",
                             app->config.breath_duration);
                Uint32 precompute_start = SDL_GetTicks();

                precompute_all_cycles(app->hexagones, TARGET_FPS, app->config.breath_duration);

                HexagoneNode* node = app->hexagones->first;
                while (node) {
                    precompute_counter_frames(
                        node,
                        node->total_cycles,
                        TARGET_FPS,
                        app->config.breath_duration,
                        app->config.Nb_respiration
                    );
                    node = node->next;
                }

                Uint32 precompute_time = SDL_GetTicks() - precompute_start;
                debug_printf("✅ Pré-calculs terminés en %u ms\n", precompute_time);

                // ═════════════════════════════════════════════════════════════════
                // ÉTAPE 4 : CRÉER L'INSTANCE DE LA TECHNIQUE WIM HOF
                // ═════════════════════════════════════════════════════════════════
                // Détruire l'instance précédente si elle existe
                if (app->active_technique) {
                    debug_printf("🗑️  Destruction ancienne instance WHM\n");
                    technique_destroy((TechniqueInstance*)app->active_technique);
                    app->active_technique = NULL;
                }

                // Créer la nouvelle instance
                TechniqueInstance* whm = whm_create(app->renderer);
                if (!whm) {
                    debug_printf("❌ Échec création instance WHM\n");
                    return;
                }

                // Configurer l'instance WHM avec les hexagones existants
                whm_set_hexagones(whm, app->hexagones);
                whm_set_screen_info(whm, app->screen_width, app->screen_height, app->scale_factor);
                whm_create_counter(whm, app->renderer);

                // Figer les hexagones à la frame 0 (= scale_max selon le cosinus)
                // IMPORTANT: Il faut appliquer la frame 0 pour copier les vx/vy dans node->data
                node = app->hexagones->first;
                while (node) {
                    // Positionner sur frame 0 AVANT de dégeler
                    node->current_cycle = 0;

                    // Dégeler temporairement pour que apply_precomputed_frame() fonctionne
                    node->is_frozen = false;

                    // Appliquer la frame 0 (copie vx/vy dans node->data)
                    // NOTE: apply_precomputed_frame() incrémente current_cycle, donc on finit à 1
                    // mais ce n'est pas grave car on fige juste après
                    apply_precomputed_frame(node);

                    // Re-figer immédiatement
                    node->is_frozen = true;

                    debug_printf("🎯 Hexagone %d: frame 0 appliquée, figé à current_cycle=%d\n",
                               node->data->element_id, node->current_cycle);

                    node = node->next;
                }

                debug_printf("✅ Hexagones figés avec frame 0 (scale_max) appliquée\n");

                app->active_technique = whm;
                app->waiting_to_start = false;

                debug_printf("✅ Instance WHM créée et démarrée\n");
                return;
            } else {
                debug_printf("❌ Clic HORS de la zone cliquable !\n");
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // PRIORITÉ 3 : Événements globaux
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

            // 🎯 TRACKER LE FOCUS DE L'ÉDITEUR JSON (pour FPS adaptatif)
            if (app->json_editor && app->json_editor->window) {
                Uint32 editor_window_id = SDL_GetWindowID(app->json_editor->window);
                if (event->window.windowID == editor_window_id) {
                    if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                        app->editor_has_focus = true;
                        debug_printf("🎯 JSON Editor a pris le focus\n");
                    } else if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                        app->editor_has_focus = false;
                        debug_printf("🎯 JSON Editor a perdu le focus\n");
                    }
                }
            }

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

                    debug_printf("📐 Ancien centre: (%d,%d), Nouveau centre: (%d,%d)\n",
                                 old_center_x, old_center_y, new_center_x, new_center_y);

                    // IMPORTANT : Parcourir TOUS les hexagones
                    HexagoneNode* node = app->hexagones->first;
                    int hex_count = 0;

                    // ═══════════════════════════════════════════════════════════════
                    // UTILISER DIRECTEMENT LE SCALE_FACTOR
                    // ═══════════════════════════════════════════════════════════════
                    // Au lieu de calculer un ratio entre ancien/nouveau container,
                    // on utilise directement le scale_factor qui est déjà calculé
                    // par rapport à la résolution de référence (1280x720).
                    // Cela permet de scaler correctement même si on change juste
                    // la largeur ou la hauteur.
                    // ═══════════════════════════════════════════════════════════════
                    debug_printf("📏 Application du scale_factor : %.3f\n", app->scale_factor);

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
                        // On applique directement le scale_factor calculé
                        // ─────────────────────────────────────────────────────────────
                        hex->current_scale = app->scale_factor;

                        debug_printf("  ✅ Hexagone %d - Centre:(%d,%d) Scale:%.3f\n",
                                     hex_count, hex->center_x, hex->center_y,
                                     hex->current_scale);

                        hex_count++;
                        node = node->next;
                    }

                    debug_printf("✅ %d hexagones redimensionnés (scale_factor: %.3f)\n",
                                 hex_count, app->scale_factor);
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

                // ═══════════════════════════════════════════════════════════════
                // ÉTAPE 3 : REPOSITIONNER LA CARTE DE SESSION (si WHM actif)
                // ═══════════════════════════════════════════════════════════════
                if (app->active_technique) {
                    // Mettre à jour les infos d'écran dans WHM
                    whm_set_screen_info(app->active_technique, app->screen_width,
                                       app->screen_height, app->scale_factor);
                    debug_printf("✅ Infos d'écran mises à jour dans WHM\n");
                }

                // ═══════════════════════════════════════════════════════════════
                // ÉTAPE 4 : ZONE CLIQUABLE WIM HOF
                // ═══════════════════════════════════════════════════════════════
                // La zone cliquable sera automatiquement mise à jour lors du prochain
                // rendu de l'écran d'accueil (ligne 940: app->wim_clickable_rect = img_rect)
                // Pas besoin de la recalculer manuellement ici
            }
        }
        break;

        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_ESCAPE) {
                app->is_running = false;
            }
            // 🆕 ARRÊT DU CHRONOMÈTRE avec ESPACE
            else if (event->key.keysym.sym == SDLK_SPACE && app->chrono_phase && app->session_stopwatch) {
                // Arrêter le chronomètre
                stopwatch_stop(app->session_stopwatch);

                // Récupérer le temps écoulé
                int elapsed_seconds = stopwatch_get_elapsed_seconds(app->session_stopwatch);
                float elapsed_time = (float)elapsed_seconds;

                // Stocker le temps dans le tableau (avec réallocation si nécessaire)
                if (app->session_times && app->session_count < app->session_capacity) {
                    app->session_times[app->session_count] = elapsed_time;
                    app->session_count++;
                    debug_printf("✅ Session %d terminée: %.0f secondes (stocké)\n",
                                 app->session_count, elapsed_time);
                } else if (app->session_times) {
                    // Réallocation du tableau (doubler la capacité)
                    int new_capacity = app->session_capacity * 2;
                    float* new_array = realloc(app->session_times, new_capacity * sizeof(float));
                    if (new_array) {
                        app->session_times = new_array;
                        app->session_capacity = new_capacity;
                        app->session_times[app->session_count] = elapsed_time;
                        app->session_count++;
                        debug_printf("✅ Session %d terminée: %.0f secondes (tableau étendu à %d)\n",
                                     app->session_count, elapsed_time, new_capacity);
                    } else {
                        debug_printf("⚠️ Échec réallocation tableau - temps non stocké\n");
                    }
                }

                // Désactiver la phase chrono et activer la phase inspiration
                app->chrono_phase = false;
                app->inspiration_phase = true;

                debug_printf("⏹️  Chronomètre arrêté par ESPACE\n");
                debug_printf("🫁 Phase INSPIRATION activée - animation scale_min → scale_max\n");
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
            // 🎯 MARQUER INTERACTION POUR FPS ADAPTATIF
            app->last_interaction_time = SDL_GetTicks();

            // Transmettre ces événements aux panneaux UNIQUEMENT si on est sur l'écran d'accueil
            // Pendant l'animation principale, aucun panneau ne doit recevoir d'événements
            if (app->waiting_to_start) {
                if (app->settings_panel) {
                    // Définir les pointeurs pour mise à jour lors de "Appliquer"
                    set_timers_for_callbacks(&app->session_timer, &app->session_stopwatch,
                                            &app->retention_timer, &app->breath_counter,
                                            &app->total_sessions, &app->hexagones,
                                            &app->screen_width, &app->screen_height);
                    handle_settings_panel_event(app->settings_panel, event, &app->config);
                }
            }
            // Le panneau stats est accessible après l'animation (fin de toutes les sessions)
            else if (app->stats_panel) {
                handle_stats_panel_event(app->stats_panel, event);
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            // 🎯 MARQUER INTERACTION POUR FPS ADAPTATIF
            app->last_interaction_time = SDL_GetTicks();
            // 🆕 ARRÊT DU CHRONOMÈTRE avec CLIC GAUCHE (priorité sur le panneau)
            if (event->button.button == SDL_BUTTON_LEFT && app->chrono_phase && app->session_stopwatch) {
                // Arrêter le chronomètre
                stopwatch_stop(app->session_stopwatch);

                // Récupérer le temps écoulé
                int elapsed_seconds = stopwatch_get_elapsed_seconds(app->session_stopwatch);
                float elapsed_time = (float)elapsed_seconds;

                // Stocker le temps dans le tableau (avec réallocation si nécessaire)
                if (app->session_times && app->session_count < app->session_capacity) {
                    app->session_times[app->session_count] = elapsed_time;
                    app->session_count++;
                    debug_printf("✅ Session %d terminée: %.0f secondes (stocké)\n",
                                 app->session_count, elapsed_time);
                } else if (app->session_times) {
                    // Réallocation du tableau (doubler la capacité)
                    int new_capacity = app->session_capacity * 2;
                    float* new_array = realloc(app->session_times, new_capacity * sizeof(float));
                    if (new_array) {
                        app->session_times = new_array;
                        app->session_capacity = new_capacity;
                        app->session_times[app->session_count] = elapsed_time;
                        app->session_count++;
                        debug_printf("✅ Session %d terminée: %.0f secondes (tableau étendu à %d)\n",
                                     app->session_count, elapsed_time, new_capacity);
                    } else {
                        debug_printf("⚠️ Échec réallocation tableau - temps non stocké\n");
                    }
                }

                // Désactiver la phase chrono et activer la phase inspiration
                app->chrono_phase = false;
                app->inspiration_phase = true;

                debug_printf("⏹️  Chronomètre arrêté par CLIC GAUCHE\n");
                debug_printf("🫁 Phase INSPIRATION activée - animation scale_min → scale_max\n");
            }
            // Sinon, transmettre l'événement aux panneaux UNIQUEMENT si on est sur l'écran d'accueil
            // Pendant l'animation principale, aucun panneau ne doit recevoir d'événements
            else if (app->waiting_to_start) {
                if (app->settings_panel) {
                    // Définir les pointeurs pour mise à jour lors de "Appliquer"
                    set_timers_for_callbacks(&app->session_timer, &app->session_stopwatch,
                                            &app->retention_timer, &app->breath_counter,
                                            &app->total_sessions, &app->hexagones,
                                            &app->screen_width, &app->screen_height);
                    handle_settings_panel_event(app->settings_panel, event, &app->config);
                }
            }
            // Le panneau stats est accessible après l'animation (fin de toutes les sessions)
            else if (app->stats_panel) {
                handle_stats_panel_event(app->stats_panel, event);
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

    // ═══════════════════════════════════════════════════════════════════════
    // RENDU DE L'INSTANCE DE TECHNIQUE ACTIVE
    // ═══════════════════════════════════════════════════════════════════════
    if (app->active_technique) {
        TechniqueInstance* instance = (TechniqueInstance*)app->active_technique;

        // Déléguer le rendu à l'instance
        if (instance->render) {
            instance->render(instance, app->renderer);
        }

        // Continuer pour rendre les panneaux (settings, JSON editor)
        // puis faire Present à la fin
        goto render_panels;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // ÉCRAN D'ACCUEIL (Technique Wim Hof)
    // ═══════════════════════════════════════════════════════════════════════
    if (app->waiting_to_start) {
        debug_verbose("🎨 RENDU écran d'accueil (waiting_to_start=%d)\n", app->waiting_to_start);
        // Calculer les positions pour centrer le tout
        int title_w, title_h;
        if (app->wim_title) {
            SDL_QueryTexture(app->wim_title, NULL, NULL, &title_w, &title_h);
        } else {
            title_w = 200;
            title_h = 60;
        }

        int total_height = title_h + 20 + 250;  // titre + marge + image
        int start_y = (app->screen_height - total_height) / 2;

        // Rendre le titre au-dessus de l'image
        if (app->wim_title) {
            SDL_Rect title_rect = {
                (app->screen_width - title_w) / 2,
                start_y,
                title_w,
                title_h
            };
            SDL_RenderCopy(app->renderer, app->wim_title, NULL, &title_rect);
        }

        // Rendre l'image wim.png (250x250 au centre)
        debug_verbose("🖼️  Vérification wim_image: %p\n", (void*)app->wim_image);
        if (app->wim_image) {
            SDL_Rect img_rect = {
                (app->screen_width - 250) / 2,
                start_y + title_h + 20,
                250,
                250
            };
            debug_verbose("🖼️  Avant rendu - img_rect: pos(%d,%d) size(%dx%d)\n",
                         img_rect.x, img_rect.y, img_rect.w, img_rect.h);
            SDL_RenderCopy(app->renderer, app->wim_image, NULL, &img_rect);
        } else {
            debug_printf("⚠️  wim_image est NULL - impossible de rendre l'écran d'accueil !\n");
        }

        // Ne PAS faire SDL_RenderPresent ici !
        // On continue pour rendre le panneau settings et l'éditeur JSON
        // Le RenderPresent est fait à la fin de la fonction (ligne 1030)
    }
    // Si on est sur l'écran d'accueil, on a déjà rendu l'image Wim Hof
    // Maintenant on continue pour rendre le panneau settings et l'éditeur JSON

    // 2. Dessine tous les hexagones (sauf si la session de comptage est terminée et qu'on n'est PAS sur l'écran d'accueil)
    if (app->hexagones) {
        HexagoneNode* node = app->hexagones->first;
        while (node) {
            // 🎯 Ne dessiner l'hexagone que si :
            // - On est en phase timer (avant le compteur)
            // - OU le compteur est actif (is_active = true)
            // - OU on est en phase reappear (réapparition douce)
            // - OU on est en phase chrono (chronomètre actif, hexagones figés)
            // - OU on est en phase inspiration (animation scale_min → scale_max)
            // - OU on est en phase rétention (poumons pleins, timer 15s)
            bool should_render = app->timer_phase ||
            (app->breath_counter && app->breath_counter->is_active) ||
            app->reappear_phase ||
            app->chrono_phase ||
            app->inspiration_phase ||
            app->retention_phase;

            if (should_render) {
                make_hexagone(app->renderer, node->data);
            }
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
            // Calculer le rayon RÉEL (coordonnées relatives × current_scale)
            float dx = first_node->data->vx[0] * first_node->data->current_scale;
            float dy = first_node->data->vy[0] * first_node->data->current_scale;
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            debug_printf("📐 Timer session: vx[0]=%.1f vy[0]=%.1f scale=%.3f → hex_radius=%d\n",
                         (float)first_node->data->vx[0], (float)first_node->data->vy[0],
                         first_node->data->current_scale, hex_radius);

            // Rendu du timer (la taille de police s'adapte automatiquement au rayon)
            timer_render(app->session_timer, app->renderer,
                         hex_center_x, hex_center_y, hex_radius);
        }
    }

    // 🆕 Dessine le compteur SI on est en phase compteur (après le timer)
    if (app->counter_phase && app->breath_counter && app->hexagones && app->hexagones->first) {
        HexagoneNode* first_node = app->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les infos de position de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            int dx = first_node->data->vx[0];
            int dy = first_node->data->vy[0];
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            // 🆕 Passer le nœud hexagone ET le scale_factor
            counter_render(app->breath_counter, app->renderer,
                           hex_center_x, hex_center_y, hex_radius, first_node,
                           app->scale_factor);
        }
    }

    // 🆕 Dessine le chronomètre SI on est en phase chrono (après la réapparition)
    if (app->chrono_phase && app->session_stopwatch && app->hexagones && app->hexagones->first) {
        HexagoneNode* first_node = app->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les infos de position de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            // Calculer le rayon RÉEL (coordonnées relatives × current_scale)
            float dx = first_node->data->vx[0] * first_node->data->current_scale;
            float dy = first_node->data->vy[0] * first_node->data->current_scale;
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            debug_printf("📐 Stopwatch: vx[0]=%.1f vy[0]=%.1f scale=%.3f → hex_radius=%d\n",
                         (float)first_node->data->vx[0], (float)first_node->data->vy[0],
                         first_node->data->current_scale, hex_radius);

            // Rendu du chronomètre (la taille de police s'adapte automatiquement au rayon)
            stopwatch_render(app->session_stopwatch, app->renderer,
                             hex_center_x, hex_center_y, hex_radius);
        }
    }

    // 🆕 Dessine le timer de rétention SI on est en phase rétention (après inspiration)
    if (app->retention_phase && app->retention_timer && app->hexagones && app->hexagones->first) {
        HexagoneNode* first_node = app->hexagones->first;
        if (first_node && first_node->data) {
            // Récupérer les infos de position de l'hexagone
            int hex_center_x = first_node->data->center_x;
            int hex_center_y = first_node->data->center_y;
            // Calculer le rayon RÉEL (coordonnées relatives × current_scale)
            float dx = first_node->data->vx[0] * first_node->data->current_scale;
            float dy = first_node->data->vy[0] * first_node->data->current_scale;
            int hex_radius = (int)sqrt(dx*dx + dy*dy);

            debug_printf("📐 Timer rétention: vx[0]=%.1f vy[0]=%.1f scale=%.3f → hex_radius=%d\n",
                         (float)first_node->data->vx[0], (float)first_node->data->vy[0],
                         first_node->data->current_scale, hex_radius);

            // Rendu du timer de rétention (la taille de police s'adapte automatiquement au rayon)
            timer_render(app->retention_timer, app->renderer,
                         hex_center_x, hex_center_y, hex_radius);
        }
    }

    // 🆕 Dessine la carte de session SI on est en phase carte
    if (app->session_card_phase && app->session_card) {
        session_card_render(app->session_card, app->renderer);
    }

render_panels:
    // 3. Dessine le panneau settings (par dessus) UNIQUEMENT sur l'écran d'accueil
    // Pendant l'animation principale, le panneau est masqué pour éviter les distractions
    if (app->waiting_to_start && app->settings_panel) {
        render_settings_panel(app->renderer, app->settings_panel);
    }

    // 3.5. Dessine le panneau stats (par dessus tout) UNIQUEMENT après l'animation
    // Le panneau stats s'affiche à la fin de toutes les sessions
    if (!app->waiting_to_start && app->stats_panel) {
        render_stats_panel(app->renderer, app->stats_panel);
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

    // 2. Dessine tous les hexagones (sauf si la session de comptage est terminée)
    if (app->hexagones) {
        HexagoneNode* node = app->hexagones->first;
        while (node) {
            // 🎯 Ne dessiner l'hexagone que si :
            // - On est en phase timer (avant le compteur)
            // - OU on est en phase compteur (compteur actif)
            // Si le compteur est terminé (counter_phase = false après avoir été true), ne plus afficher
            if (app->timer_phase || app->counter_phase) {
                make_hexagone(app->renderer, node->data);
            }
            node = node->next;
        }
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

    // Libère les textures de l'écran d'accueil
    if (app->wim_image) {
        SDL_DestroyTexture(app->wim_image);
        app->wim_image = NULL;
    }
    if (app->wim_title) {
        SDL_DestroyTexture(app->wim_title);
        app->wim_title = NULL;
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
// SYSTÈME FPS ADAPTATIF
// ════════════════════════════════════════════════════════════════════════════
// Détermine si on doit utiliser 60 FPS (true) ou 15 FPS (false)
// Critères pour 60 FPS :
//   - Animations actives (hexagones, compteur, timer, chrono, carte session)
//   - Interaction récente fenêtre principale (< 2 secondes)
//   - JSON Editor : focus ET (autosave activé OU événement récent < 2s)
//   - Panneau settings/stats ouvert
// Critères pour 15 FPS (économie CPU) :
//   - Aucune animation
//   - Pas d'interaction récente
//   - JSON Editor sans focus ou sans autosave et sans interaction
//   - Panneaux fermés
// ════════════════════════════════════════════════════════════════════════════
bool should_use_high_fps(AppState* app) {
    if (!app) return true;  // Par sécurité

    Uint32 current_time = SDL_GetTicks();
    const Uint32 INTERACTION_TIMEOUT = 2000;  // 2 secondes

    // ═══════════════════════════════════════════════════════════════════════
    // 1. ANIMATION PRINCIPALE → TOUJOURS 60 FPS
    // ═══════════════════════════════════════════════════════════════════════
    // L'animation principale comprend toute la séquence :
    //   [timer] → [carte session] → [respiration compteur] → [chronomètre] → [rétention] → [stats]
    // Cette séquence complète doit TOUJOURS tourner à 60 FPS pour être fluide,
    // même pendant le chronomètre où les hexagones sont figés
    //
    // Note : L'écran d'accueil (waiting_to_start = true) reste à 15 FPS pour économiser le CPU

    if (app->timer_phase) return true;           // Timer avant session
    if (app->session_card_phase) return true;    // Carte de session animée
    if (app->counter_phase) return true;         // Compteur de respirations (hexagones respirent)
    if (app->chrono_phase) return true;          // Chronomètre (hexagones figés mais timer anime)
    if (app->reappear_phase) return true;        // Réapparition douce
    if (app->inspiration_phase) return true;     // Phase inspiration
    if (app->retention_phase) return true;       // Phase rétention

    // ═══════════════════════════════════════════════════════════════════════
    // 2. INTERACTION RÉCENTE FENÊTRE PRINCIPALE → 60 FPS pendant 2s
    // ═══════════════════════════════════════════════════════════════════════
    // Cela inclut :
    //   - Clic sur le bouton gear du panneau config
    //   - Mouvement de souris sur les widgets
    //   - Molette de scroll
    //   - Tout événement SDL_MOUSEMOTION, SDL_MOUSEWHEEL, SDL_MOUSEBUTTONDOWN
    if (current_time - app->last_interaction_time < INTERACTION_TIMEOUT) {
        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 3. PANNEAUX OUVERTS/EN ANIMATION → 60 FPS
    // ═══════════════════════════════════════════════════════════════════════
    // Dès qu'un panneau est en cours d'ouverture, ouvert, ou en cours de fermeture
    // → 60 FPS pour animation slide fluide
    if (app->settings_panel) {
        PanelState panel_state = app->settings_panel->state;
        if (panel_state == PANEL_OPEN || panel_state == PANEL_OPENING || panel_state == PANEL_CLOSING) {
            return true;
        }
    }

    if (app->stats_panel) {
        StatsPanelState stats_state = app->stats_panel->state;
        if (stats_state == STATS_OPEN || stats_state == STATS_OPENING || stats_state == STATS_CLOSING) {
            return true;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 4. JSON EDITOR : 60 FPS SI focus ET activité récente
    // ═══════════════════════════════════════════════════════════════════════
    // Logique subtile :
    //   - JSON fermé → 15 FPS (pas de rendu)
    //   - JSON ouvert MAIS pas de focus → 15 FPS (idle)
    //   - JSON avec focus MAIS pas d'activité → 15 FPS après 2s (idle)
    //   - JSON avec focus ET autosave actif → 60 FPS (hot reload)
    //   - JSON avec focus ET activité récente (< 2s) → 60 FPS (frappe clavier, etc.)
    if (app->json_editor && app->json_editor->est_ouvert && app->editor_has_focus) {
        // Si autosave activé → 60 FPS pour fluidité du hot reload
        if (app->json_editor->auto_save_enabled) {
            return true;
        }

        // Si événement récent dans l'éditeur (clavier, souris) → 60 FPS
        if (current_time - app->last_editor_event < INTERACTION_TIMEOUT) {
            return true;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 5. SINON → 15 FPS (mode économie CPU)
    // ═══════════════════════════════════════════════════════════════════════
    // Exemples de situations à 15 FPS :
    //   - Écran d'accueil sans interaction
    //   - Panneaux fermés sans interaction
    //   - JSON ouvert mais sans focus ou inactif depuis > 2s
    return false;
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
