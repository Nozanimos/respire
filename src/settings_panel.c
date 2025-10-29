#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include "settings_panel.h"
#include "debug.h"
#include "json_config_loader.h"



#define PANEL_WIDTH 500
#define ANIMATION_DURATION 0.3f
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 40
#define BUTTON_MARGIN 20

// Forward declarations des fonctions de renderer.h (évite l'include circulaire)
extern float calculate_scale_factor(int width, int height);
extern int scale_value(int value, float scale);
extern int calculate_panel_width(int screen_width, float scale);

// ✅ CALLBACKS pour les widgets (ajouter en haut du fichier, avant create_settings_panel)

static SettingsPanel* current_panel_for_callbacks = NULL;

void duration_value_changed(int new_value) {
    if (!current_panel_for_callbacks) return;

    current_panel_for_callbacks->temp_config.breath_duration = new_value;
    debug_printf("🔄 Durée respiration changée: %d secondes\n", new_value);

    // Mettre à jour la prévisualisation en temps réel
    update_preview_for_new_duration(current_panel_for_callbacks, new_value);
}

void cycles_value_changed(int new_value) {
    if (!current_panel_for_callbacks) return;

    current_panel_for_callbacks->temp_config.breath_cycles = new_value;
    debug_printf("🔄 Cycles changés: %d\n", new_value);
}

void alternate_cycles_changed(bool new_value) {
    if (!current_panel_for_callbacks) return;

    current_panel_for_callbacks->temp_config.alternate_cycles = new_value;
    debug_printf("🔄 Cycles alternés changés: %s\n", new_value ? "ACTIF" : "INACTIF");

    // Ici tu pourras ajouter la logique pour affecter l'animation principale
}

/* === FONCTIONS DE PRÉVISUALISATION === */

void reinitialiser_preview_system(PreviewSystem* preview) {
    if (!preview) return;

    // Réinitialiser aux valeurs d'origine fixes
    preview->center_x = 50;  // container_size/2 = 100/2 = 50
    preview->center_y = 50;
    preview->container_size = 100;
    preview->size_ratio = 0.70f;

    debug_printf("🔄 Paramètres preview réinitialisés - Centre: (%d,%d), Container: %d, Ratio: %.2f\n",
                 preview->center_x, preview->center_y, preview->container_size, preview->size_ratio);
}

void init_preview_system(SettingsPanel* panel, int x, int y, int size, float ratio) {
    // Initialiser d'abord les paramètres de base
    panel->preview_system.frame_x = x;
    panel->preview_system.frame_y = y;
    panel->preview_system.center_x = size/2;
    panel->preview_system.center_y = size/2;
    panel->preview_system.container_size = size;
    panel->preview_system.size_ratio = ratio;
    panel->preview_system.last_update = SDL_GetTicks();
    panel->preview_system.current_time = 0.0;

    // Initialiser hex_list à NULL pour la première fois
    panel->preview_system.hex_list = NULL;

    debug_printf("🔄 INIT Prévisualisation - Cadre: (%d,%d), Centre: (%d,%d), Taille: %d, Ratio: %.2f\n",
                 x, y, panel->preview_system.center_x, panel->preview_system.center_y, size, ratio);

    // Créer les hexagones (sans tentative de libération préalable)
    panel->preview_system.hex_list = create_all_hexagones(
        panel->preview_system.center_x,
        panel->preview_system.center_y,
        panel->preview_system.container_size,
        panel->preview_system.size_ratio
    );

    if (panel->preview_system.hex_list && panel->preview_system.hex_list->first && panel->preview_system.hex_list->first->data) {
        Hexagon* first_hex = panel->preview_system.hex_list->first->data;
        debug_printf("🔍 INIT - Premier hexagone - Centre: (%d,%d), Scale: %.2f, vx[0]: %d, vy[0]: %d\n",
                     first_hex->center_x, first_hex->center_y,
                     first_hex->current_scale, first_hex->vx[0], first_hex->vy[0]);
    } else {
        debug_printf("❌ ERREUR: Impossible de créer les hexagones de prévisualisation\n");
        return;
    }

    if (panel->preview_system.hex_list) {
        precompute_all_cycles(panel->preview_system.hex_list, TARGET_FPS, panel->temp_config.breath_duration);
        debug_printf("✅ Prévisualisation initialisée\n");
    }
}


void update_preview_animation(SettingsPanel* panel) {
    if (!panel->preview_system.hex_list) return;

    Uint32 current_time = SDL_GetTicks();
    float delta_time = (current_time - panel->preview_system.last_update) / 1000.0f;
    panel->preview_system.last_update = current_time;
    panel->preview_system.current_time += delta_time;

    // Avancer d'une frame dans le précalcul
    HexagoneNode* node = panel->preview_system.hex_list->first;
    while (node) {
        apply_precomputed_frame(node);
        node = node->next;
    }
}

void update_preview_for_new_duration(SettingsPanel* panel, float new_duration) {
    if (!panel) return;

    debug_printf("🔄 Mise à jour prévisualisation - nouvelle durée: %.1fs\n", new_duration);

    // ✅ CORRECTION : Vérifier que la liste existe avant de la libérer
    if (panel->preview_system.hex_list) {
        free_hexagone_list(panel->preview_system.hex_list);
        panel->preview_system.hex_list = NULL;
    }

    // Les dimensions actuelles du preview sont déjà correctes
    // (mises à jour par update_panel_scale lors du redimensionnement)
    // On doit juste recalculer les centres RELATIFS

    panel->preview_system.center_x = panel->preview_system.container_size / 2;
panel->preview_system.center_y = panel->preview_system.container_size / 2;

debug_printf("🔄 Recréation hexagones - Container: %d, Centre: (%d,%d), Ratio: %.2f\n",
             panel->preview_system.container_size,
             panel->preview_system.center_x, panel->preview_system.center_y,
             panel->preview_system.size_ratio);

    // Recréer les hexagones avec les dimensions actuelles
    panel->preview_system.hex_list = create_all_hexagones(
        panel->preview_system.center_x,
        panel->preview_system.center_y,
        panel->preview_system.container_size,
        panel->preview_system.size_ratio
    );

    // ✅ DEBUG : Afficher l'état APRÈS création
    if (panel->preview_system.hex_list && panel->preview_system.hex_list->first && panel->preview_system.hex_list->first->data) {
        Hexagon* first_hex = panel->preview_system.hex_list->first->data;
        debug_printf("🔍 APRÈS CRÉATION - Centre: (%d,%d), Scale: %.2f, vx[0]: %d, vy[0]: %d\n",
                     first_hex->center_x, first_hex->center_y,
                     first_hex->current_scale, first_hex->vx[0], first_hex->vy[0]);
    } else {
        debug_printf("❌ ERREUR: Impossible de recréer les hexagones\n");
        return;
    }

    // Re-précalculer les cycles
    if (panel->preview_system.hex_list) {
        precompute_all_cycles(panel->preview_system.hex_list, TARGET_FPS, new_duration);
    }

    // Réinitialiser le temps
    panel->preview_system.current_time = 0.0;
    panel->preview_system.last_update = SDL_GetTicks();

    debug_printf("✅ Prévisualisation COMPLÈTEMENT réinitialisée avec nouvelle durée\n");
}

void render_preview(SDL_Renderer* renderer, PreviewSystem* preview, int offset_x, int offset_y) {
    if (!preview || !preview->hex_list) {
        debug_printf("❌ RENDU: Prévisualisation non initialisée\n");
        return;
    }

    HexagoneNode* node = preview->hex_list->first;
    if (!node || !node->data) {
        debug_printf("❌ RENDU: Aucun hexagone à afficher\n");
        return;
    }

    while (node) {
        if (node->data) {
            // Positionner au centre du cadre de prévisualisation
            int preview_center_x = offset_x + preview->frame_x + preview->container_size/2;
            int preview_center_y = offset_y + preview->frame_y + preview->container_size/2;

            // Appliquer la transformation
            transform_hexagon(node->data, preview_center_x, preview_center_y, 1.0f);

            // Rendre l'hexagone
            make_hexagone(renderer, node->data);

            // ✅ IMPORTANT : Restaurer immédiatement la position d'origine
            transform_hexagon(node->data, preview->center_x, preview->center_y, 1.0f);
        }
        node = node->next;
    }
}


SettingsPanel* create_settings_panel(SDL_Renderer* renderer, int screen_width, int screen_height, float scale_factor) {
    SettingsPanel* panel = malloc(sizeof(SettingsPanel));
    if (!panel) return NULL;

    // INITIALISATION EXPLICITE de tous les membres
    memset(panel, 0, sizeof(SettingsPanel));

    // ════════════════════════════════════════════════════════════════════════
    // STOCKER LE FACTEUR D'ÉCHELLE
    // ════════════════════════════════════════════════════════════════════════
    panel->scale_factor = scale_factor;

    debug_printf("🎨 Création panneau avec scale: %.2f\n", scale_factor);


    // ══════════════════════════════════════════════════════════════════════════
    // CHARGEMENT DES POLICES
    // ══════════════════════════════════════════════════════════════════════════

    // Initialiser SDL_ttf
    if (TTF_Init() == -1) {
        debug_printf("Erreur TTF_Init: %s\n", TTF_GetError());
    }

    panel->font_title = TTF_OpenFont("../fonts/arial/ARIAL.TTF", 28);
    panel->font = TTF_OpenFont("../fonts/arial/ARIAL.TTF", 20);
    panel->font_small = TTF_OpenFont("../fonts/arial/ARIAL.TTF", 16);

    if (!panel->font_title) {
        debug_printf("Erreur chargement police: %s\n", TTF_GetError());
        // Police titre
        panel->font = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeSans.otf", 24);
    }
    if (!panel->font) {
        debug_printf("Erreur chargement police: %s\n", TTF_GetError());
        // Police normale
        panel->font = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeSans.otf", 18);
    }
    if (!panel->font_small) {
        debug_printf("Erreur chargement police: %s\n", TTF_GetError());
        // Police mini
        panel->font = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeSans.otf", 14);
    }

    // Chargement configuration temporaire
    load_config(&panel->temp_config);

    // === RÉORGANISATION DE L'ESPACE ===


    // ══════════════════════════════════════════════════════════════════════════
    // CRÉATION DE LA LISTE DE WIDGETS
    // ══════════════════════════════════════════════════════════════════════════
    panel->widget_list = create_widget_list();

    // Préparer le contexte de chargement
    LoaderContext ctx = {
        .renderer = renderer,
        .font_titre = panel->font_title,
        .font_normal = panel->font,
        .font_petit = panel->font_small
    };

    // Charger les widgets depuis le JSON
    const char* json_path = "../config/widgets_config.json";
    if (!charger_widgets_depuis_json(json_path, &ctx, panel->widget_list)) {
        debug_printf("⚠️ Échec chargement JSON, utilisation config par défaut\n");

        // FALLBACK : Si le JSON n'existe pas ou est invalide,
        // créer les widgets en dur (ton ancien code)

        // Calcul largeur widget pour centrage
        int largeur_max_widget = 180 + 20 + 40 + 20;
        int widget_x = (PANEL_WIDTH - largeur_max_widget) / 2;

    // ──────────────────────────────────────────────────────────────────────────
    // WIDGET 1 : DURÉE DE RESPIRATION (Incrément)
    // ──────────────────────────────────────────────────────────────────────────
    add_increment_widget(
        panel->widget_list,              // Liste où ajouter le widget
        "breath_duration",               // ID unique (pour get/set programmatique)
        "Durée respiration",             // Nom affiché à l'écran
        widget_x,                        // Position X (relative au panneau)
        240,                             // Position Y (relative au panneau)
        1,                               // Valeur MIN (1 seconde minimum)
        10,                              // Valeur MAX (10 secondes maximum)
        3,                               // Valeur INITIALE (3 secondes par défaut)
        1,                               // INCRÉMENT (clic = +1 ou -1)
        6,                               // Taille des flèches ↑↓ en pixels
        18,                              // Taille de référence du texte
        panel->font,                     // Police TTF pour le rendu
        duration_value_changed           // Callback appelé à chaque changement
    );

    // ──────────────────────────────────────────────────────────────────────────
    // WIDGET 2 : NOMBRE DE CYCLES (Incrément)
    // ──────────────────────────────────────────────────────────────────────────
    add_increment_widget(
        panel->widget_list,              // Liste où ajouter le widget
        "breath_cycles",                 // ID unique
        "Cycles",                        // Nom affiché à l'écran
        widget_x,                        // Position X (relative au panneau)
        320,                             // Position Y (relative au panneau)
        1,                               // Valeur MIN (1 cycle minimum)
        20,                              // Valeur MAX (20 cycles maximum)
        1,                               // Valeur INITIALE (1 cycle par défaut)
        1,                               // INCRÉMENT (clic = +1 ou -1)
        6,                               // Taille des flèches ↑↓ en pixels
        18,                              // Taille de référence du texte
        panel->font,                     // Police TTF pour le rendu
        cycles_value_changed             // Callback appelé à chaque changement
    );

    // ──────────────────────────────────────────────────────────────────────────
    // WIDGET 3 : CYCLES ALTERNÉS (Toggle ON/OFF)
    // ──────────────────────────────────────────────────────────────────────────
    add_toggle_widget(
        panel->widget_list,              // Liste où ajouter le widget
        "alternate_cycles",              // ID unique
        "Cycles alternés",               // Nom affiché à l'écran
        widget_x,                        // Position X (relative au panneau)
        400,                             // Position Y (relative au panneau)
        false,                           // État INITIAL (false = OFF, true = ON)
        40,                              // Largeur du bouton toggle en pixels
        18,                              // Hauteur du bouton toggle en pixels
        18,                              // Diamètre du curseur circulaire
        18,                              // Taille de référence du texte
        panel->font,                     // Police TTF pour le rendu
        alternate_cycles_changed         // Callback appelé à chaque basculement
        );
    }

    // ──────────────────────────────────────────────────────────────────────────
    // DEBUG : Afficher le contenu de la liste
    // ──────────────────────────────────────────────────────────────────────────
    debug_print_widget_list(panel->widget_list);


    // ════════════════════════════════════════════════════════════════════════
    // CRÉATION DES BOUTONS AVEC SCALING
    // ════════════════════════════════════════════════════════════════════════
    // Les boutons sont scalés selon le facteur d'échelle pour s'adapter
    // à tous les types d'écrans (téléphone, tablette, desktop)

    // Constantes de base (non scalées)
    const int BASE_BUTTON_SPACING = 10;   // Espace entre les 2 boutons
    const int BASE_BOTTOM_MARGIN = 50;    // Pixels depuis le bas

    // Calculer la largeur du panneau (dynamique selon l'écran)
    int panel_width = calculate_panel_width(screen_width, scale_factor);

    // Calculer les dimensions scalées des boutons
    int scaled_button_width = (int)(BUTTON_WIDTH * scale_factor);
    int scaled_button_height = (int)(BUTTON_HEIGHT * scale_factor);
    int scaled_spacing = (int)(BASE_BUTTON_SPACING * scale_factor);
    int scaled_bottom_margin = (int)(BASE_BOTTOM_MARGIN * scale_factor);

    // Calculer le centrage des boutons dans le panneau
    int total_buttons_width_scaled = scaled_button_width * 2 + scaled_spacing;
    int buttons_start_x_scaled = (panel_width - total_buttons_width_scaled) / 2;

    debug_printf("📏 Calcul boutons - Panel: %d, Boutons: %dx%d, Start X: %d\n",
                 panel_width, scaled_button_width, scaled_button_height, buttons_start_x_scaled);

    // Création du bouton "Appliquer" (à gauche)
    panel->apply_button = create_button(
        "Appliquer",                                    // Texte
        buttons_start_x_scaled,                         // X (centré, scalé)
    screen_height - scaled_bottom_margin,           // Y (depuis le bas, scalé)
    scaled_button_width,                            // Largeur scalée
    scaled_button_height                            // Hauteur scalée
    );

    // Création du bouton "Annuler" (à droite)
    panel->cancel_button = create_button(
        "Annuler",                                                      // Texte
        buttons_start_x_scaled + scaled_button_width + scaled_spacing, // X (après le premier)
    screen_height - scaled_bottom_margin,                           // Y (même hauteur)
    scaled_button_width,                                            // Largeur scalée
    scaled_button_height                                            // Hauteur scalée
    );

    debug_printf("📏 Boutons créés - Largeur: %d, Hauteur: %d, Espacement: %d\n",
                 scaled_button_width, scaled_button_height, scaled_spacing);

    // ════════════════════════════════════════════════════════════════════════
    // INITIALISATION DU PANNEAU AVEC LARGEUR RESPONSIVE
    // ════════════════════════════════════════════════════════════════════════

    panel->state = PANEL_CLOSED;
    panel->rect = (SDL_Rect){screen_width, 0, panel_width, screen_height};

    debug_printf("📐 Panneau créé - Largeur: %d (scale: %.2f)\n", panel_width, scale_factor);
    panel->target_x = screen_width;
    panel->current_x = screen_width;
    panel->animation_progress = 0.0f;

    // Chargement du fond du panneau
    SDL_Surface* bg_surface = IMG_Load("../img/settings_bg.png");
    if (!bg_surface) {
        debug_printf("Erreur: Impossible de charger ../img/settings_bg.png: %s\n", IMG_GetError());
        // Fallback: fond gris semi-transparent
        bg_surface = SDL_CreateRGBSurface(0, PANEL_WIDTH, screen_height, 32, 0, 0, 0, 0);
        SDL_FillRect(bg_surface, NULL, SDL_MapRGBA(bg_surface->format, 50, 50, 60, 230));
    }
    panel->background = SDL_CreateTextureFromSurface(renderer, bg_surface);
    SDL_FreeSurface(bg_surface);

    // Chargement de l'icône engrenage
    SDL_Surface* gear_surface = IMG_Load("../img/settings.png");

    if (!gear_surface) {
        debug_printf("Erreur: Impossible de charger ../img/settings.png: %s\n", IMG_GetError());
        // Fallback: créer une surface simple
        gear_surface = SDL_CreateRGBSurface(0, 40, 40, 32, 0, 0, 0, 0);
        SDL_FillRect(gear_surface, NULL, SDL_MapRGBA(gear_surface->format, 200, 200, 200, 255));
    }

    panel->gear_icon = SDL_CreateTextureFromSurface(renderer, gear_surface);
    SDL_FreeSurface(gear_surface);

    // ════════════════════════════════════════════════════════════════════════
    // POSITION DE L'ENGRENAGE (scalé)
    // ════════════════════════════════════════════════════════════════════════
    int gear_size = (int)(40 * scale_factor);
    int gear_margin = (int)(20 * scale_factor);

    panel->gear_rect = (SDL_Rect){
        screen_width - gear_size - gear_margin,  // X (depuis la droite)
        gear_margin,                              // Y (depuis le haut)
        gear_size,                                // Largeur scalée
        gear_size                                 // Hauteur scalée
    };


    // Création des boutons (textures simples pour l'instant)
    SDL_Surface* apply_surface = SDL_CreateRGBSurface(0, BUTTON_WIDTH, BUTTON_HEIGHT, 32, 0, 0, 0, 0);
    SDL_FillRect(apply_surface, NULL, SDL_MapRGBA(apply_surface->format, 76, 175, 80, 255)); // Vert
    panel->apply_button_texture = SDL_CreateTextureFromSurface(renderer, apply_surface);

    SDL_Surface* cancel_surface = SDL_CreateRGBSurface(0, BUTTON_WIDTH, BUTTON_HEIGHT, 32, 0, 0, 0, 0);
    SDL_FillRect(cancel_surface, NULL, SDL_MapRGBA(cancel_surface->format, 244, 67, 54, 255)); // Rouge
    panel->cancel_button_texture = SDL_CreateTextureFromSurface(renderer, cancel_surface);

    SDL_FreeSurface(apply_surface);
    SDL_FreeSurface(cancel_surface);

    // Position des boutons (sera ajustée lors du rendu)
    panel->apply_button_rect = (SDL_Rect){0, 0, BUTTON_WIDTH, BUTTON_HEIGHT};
    panel->cancel_button_rect = (SDL_Rect){0, 0, BUTTON_WIDTH, BUTTON_HEIGHT};

    // Chargement configuration temporaire
    load_config(&panel->temp_config);
    init_preview_system(panel, 50, 80, 100, 0.90f);

    debug_printf("✅ Panneau de configuration créé avec widgets\n");
    return panel;
}

void update_settings_panel(SettingsPanel* panel, float delta_time) {
    if (!panel) return;

    switch(panel->state) {
        case PANEL_OPENING:
            panel->animation_progress += delta_time / ANIMATION_DURATION;
            if (panel->animation_progress >= 1.0f) {
                panel->animation_progress = 1.0f;
                panel->state = PANEL_OPEN;
            }
            break;

        case PANEL_CLOSING:
            panel->animation_progress -= delta_time / ANIMATION_DURATION;
            if (panel->animation_progress <= 0.0f) {
                panel->animation_progress = 0.0f;
                panel->state = PANEL_CLOSED;
            }
            break;

        default:
            break;
    }

    // Animation easing (cubique pour un effet smooth)
    float eased = panel->animation_progress * panel->animation_progress * panel->animation_progress;
    // ═══════════════════════════════════════════════════════════════════════════
    // ANIMATION DU PANNEAU
    // ═══════════════════════════════════════════════════════════════════════════
    // ⚠️ IMPORTANT : Utiliser panel->rect.w au lieu de la constante PANEL_WIDTH !
    //
    // Pourquoi ?
    // - PANEL_WIDTH = 500 (constante fixe)
    // - panel->rect.w = largeur actuelle après redimensionnement (ex: 441)
    //
    // Si on utilise PANEL_WIDTH, l'animation utilisera toujours 500px même si
    // le panneau a été redimensionné à 441px, causant des sauts visuels !
    // ═══════════════════════════════════════════════════════════════════════════
    //panel->current_x = panel->target_x - (int)(panel->rect.w * eased);

    // Mise à jour de la position en cours en fonction de la cible et de l'animation
    // ⚠️ CETTE LIGNE EST CRUCIALE ⚠️
    // SEULEMENT si l'état est en transition (OPENING ou CLOSING)
    if (panel->state == PANEL_OPENING || panel->state == PANEL_CLOSING) {
        // Ici, le déplacement dépend de la largeur du panneau pendant l'animation
        panel->current_x = panel->target_x - (int)(panel->rect.w * eased);
    }
    // Si l'état est stable (OPEN ou CLOSED), on suppose que panel->current_x
    // a été correctement mis à jour par update_panel_scale ou par la fin de l'animation précédente.
    // On ne le recalcule pas ici en fonction de rect.w.

    panel->rect.x = panel->current_x;

    // ══════════════════════════════════════════════════════════════════════════
    // MISE À JOUR DES ANIMATIONS DES WIDGETS (EN UNE LIGNE !)
    // ══════════════════════════════════════════════════════════════════════════
    if (panel->state == PANEL_OPEN) {
        update_preview_animation(panel);
        update_widget_list_animations(panel->widget_list, delta_time);
    }

}

void render_settings_panel(SDL_Renderer* renderer, SettingsPanel* panel) {
    if (!panel) return;


    // Icône engrenage (toujours visible)
    if (panel->gear_icon) {
        SDL_RenderCopy(renderer, panel->gear_icon, NULL, &panel->gear_rect);
    }

    // Panneau (seulement si ouvert)
    if (panel->state != PANEL_CLOSED) {
        // Fond
        SDL_RenderCopy(renderer, panel->background, NULL, &panel->rect);

        int panel_x = panel->rect.x;
        int panel_y = panel->rect.y;

        // === TITRE ===
        TTF_SetFontStyle(panel->font_title, TTF_STYLE_UNDERLINE);
        render_text(renderer, panel->font_title,"Configuration", panel_x + 50, panel_y + 10, 0xFF000000);

        // ═════════════════════════════════════════════════════════════════════════
        // CADRE BLANC DU PREVIEW
        // ═════════════════════════════════════════════════════════════════════════
        // ⚠️ IMPORTANT : Le cadre doit utiliser les MÊMES coordonnées que l'hexagone !
        //
        // Le cadre et l'hexagone partagent :
        // - frame_x, frame_y : position relative du cadre dans le panneau
        // - container_size : taille du cadre (et de l'hexagone qui est dedans)
        //
        // Ces valeurs sont mises à jour par update_panel_scale(), donc le cadre
        // et l'hexagone restent toujours synchronisés, même après redimensionnement.
        // ═════════════════════════════════════════════════════════════════════════

        // Calculer les coordonnées du cadre en utilisant les valeurs du preview_system
        int frame_x1 = panel_x + panel->preview_system.frame_x;
        int frame_y1 = panel_y + panel->preview_system.frame_y;
        int frame_x2 = frame_x1 + panel->preview_system.container_size;
        int frame_y2 = frame_y1 + panel->preview_system.container_size;

        // Dessiner le cadre blanc avec les coordonnées calculées
        rectangleColor(renderer, frame_x1, frame_y1, frame_x2, frame_y2, 0xFFFFFFFF);

        /*debug_printf("📐 Cadre preview : (%d,%d) → (%d,%d), taille: %d\n",
                     frame_x1, frame_y1, frame_x2, frame_y2,
                     panel->preview_system.container_size);*/

        // Hexagone de prévisualisation
        render_preview(renderer, &panel->preview_system, panel_x, panel_y);


        // ══════════════════════════════════════════════════════════════════════════
        // ✅ RENDU DE TOUS LES WIDGETS (EN UNE SEULE LIGNE !)
        // ══════════════════════════════════════════════════════════════════════════
        render_all_widgets(renderer, panel->widget_list, panel->font, panel_x, panel_y);


        // === BOUTONS ===
        render_button(renderer, &panel->apply_button, panel->font, panel_x, panel_y);
        render_button(renderer, &panel->cancel_button, panel->font, panel_x, panel_y);
    }
}

void handle_settings_panel_event(SettingsPanel* panel, SDL_Event* event, AppConfig* main_config) {
    if (!panel || !event) return;

    // SET le panel courant pour les callbacks
    current_panel_for_callbacks = panel;

    int panel_x = 0;
    int panel_y = 0;

    // ═════════════════════════════════════════════════════════════════════════
    // GESTION DES ÉVÉNEMENTS GLOBAUX (indépendants de l'état du panneau)
    // ═════════════════════════════════════════════════════════════════════════

    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;

        // ─────────────────────────────────────────────────────────────────────
        // CLIC SUR L'ENGRENAGE (ouvrir/fermer le panneau)
        // ─────────────────────────────────────────────────────────────────────
        if (is_point_in_rect(x, y, panel->gear_rect)) {
            if (panel->state == PANEL_CLOSED) {
                panel->state = PANEL_OPENING;
                // Recharger la config actuelle dans la config temporaire
                load_config(&panel->temp_config);

                // METTRE À JOUR les widgets avec les valeurs actuelles
                set_widget_int_value(panel->widget_list, "breath_duration", panel->temp_config.breath_duration);
                set_widget_int_value(panel->widget_list, "breath_cycles", panel->temp_config.breath_cycles);
                set_widget_bool_value(panel->widget_list, "alternate_cycles", panel->temp_config.alternate_cycles);

                // Recréer les hexagones avec la durée actuelle
                // (les dimensions sont déjà correctes, mises à jour par update_panel_scale)
                update_preview_for_new_duration(panel, panel->temp_config.breath_duration);

                debug_printf("🎯 Ouverture panneau - prévisualisation réinitialisée\n");
            } else if (panel->state == PANEL_OPEN) {
                panel->state = PANEL_CLOSING;
                debug_printf("Fermeture du panneau de configuration\n");
            }
            return;  // ✅ Sortir immédiatement après gestion de l'engrenage
        }
        // ✅ NETTOYER la référence au panel à la fin de la gestion d'événements
        if (event->type == SDL_MOUSEBUTTONUP) {
            current_panel_for_callbacks = NULL;
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // GESTION DES ÉVÉNEMENTS QUAND LE PANNEAU EST OUVERT
    // ═════════════════════════════════════════════════════════════════════════
    // IMPORTANT : Cette section doit gérer TOUS les types d'événements
    // (MOUSEMOTION, MOUSEBUTTONDOWN, MOUSEWHEEL, etc.)

    if (panel->state == PANEL_OPEN) {
        panel_x = panel->rect.x;
        panel_y = panel->rect.y;

        // ─────────────────────────────────────────────────────────────────────────
        // TRANSMETTRE TOUS LES ÉVÉNEMENTS AUX WIDGETS (EN UNE LIGNE !)
        // ─────────────────────────────────────────────────────────────────────────
        handle_widget_list_events(panel->widget_list, event, panel_x, panel_y);

        // ─────────────────────────────────────────────────────────────────────
        // GESTION DES BOUTONS (seulement pour les clics)
        // ─────────────────────────────────────────────────────────────────────

        if (event->type == SDL_MOUSEBUTTONDOWN) {
            int x = event->button.x;
            int y = event->button.y;

            SDL_Rect apply_abs_rect = {
                panel->apply_button.rect.x + panel_x,
                panel->apply_button.rect.y + panel_y,
                panel->apply_button.rect.w,
                panel->apply_button.rect.h
            };

            SDL_Rect cancel_abs_rect = {
                panel->cancel_button.rect.x + panel_x,
                panel->cancel_button.rect.y + panel_y,
                panel->cancel_button.rect.w,
                panel->cancel_button.rect.h
            };

            // Clic sur le bouton Appliquer
            if (is_point_in_rect(x, y, apply_abs_rect)) {
                // Appliquer la configuration temporaire
                *main_config = panel->temp_config;
                save_config(main_config);
                debug_printf("Configuration appliquée et sauvegardée\n");
                panel->state = PANEL_CLOSING;
            }

            // Clic sur le bouton Annuler
            if (is_point_in_rect(x, y, cancel_abs_rect)) {
                // Annuler les changements
                debug_printf("Changements annulés\n");
                panel->state = PANEL_CLOSING;
            }
        }
    }

    // ✅ NETTOYER la référence au panel à la fin de la gestion d'événements
    if (event->type == SDL_MOUSEBUTTONUP) {
        current_panel_for_callbacks = NULL;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// MISE À JOUR DE L'ÉCHELLE DU PANNEAU
// ═════════════════════════════════════════════════════════════════════════════
// Cette fonction recalcule et met à jour toutes les dimensions du panneau
// selon le nouveau facteur d'échelle. Appelée lors du redimensionnement.
// ═════════════════════════════════════════════════════════════════════════════
void update_panel_scale(SettingsPanel* panel, int screen_width, int screen_height, float scale_factor) {
    if (!panel) return;

    // Mettre à jour le facteur d'échelle
    panel->scale_factor = scale_factor;

    debug_printf("🔄 Mise à jour scale panneau: %.2f\n", scale_factor);

    // ─────────────────────────────────────────────────────────────────────────
    // 1. RECALCULER LA LARGEUR DU PANNEAU
    // ─────────────────────────────────────────────────────────────────────────
    // ⚠️ STRATÉGIE RESPONSIVE POUR LE PANNEAU :
    //
    // Le panneau doit garder sa largeur de BASE (500px) tant qu'il ne touche
    // pas le bord gauche de la fenêtre. Il ne doit rétrécir QUE lorsqu'il
    // entre en contact avec le bord gauche.
    //
    // Logique :
    // 1. Largeur souhaitée = 500px (constante de base)
    // 2. Espace disponible = largeur de l'écran (on veut pouvoir prendre tout l'écran si nécessaire)
    // 3. Largeur finale = min(largeur_souhaitée, espace_disponible)
    //
    // Exemples :
    // - Écran 1280px : panneau = 500px (il y a de la place) ✓
    // - Écran 600px  : panneau = 500px (il touche le bord mais c'est ok) ✓
    // - Écran 400px  : panneau = 400px (il DOIT rétrécir sinon déborde) ✓
    // - Écran 200px  : panneau = 200px (téléphone, plein écran) ✓
    // ─────────────────────────────────────────────────────────────────────────

    const int BASE_PANEL_WIDTH = PANEL_WIDTH;  // Largeur de base du panneau

    // Calculer la largeur finale
    int panel_width;
    if (screen_width >= BASE_PANEL_WIDTH) {
        // Il y a assez de place : garder la largeur de base
        panel_width = BASE_PANEL_WIDTH;
        debug_printf("📏 Panneau : largeur de base (%dpx) - espace disponible: %dpx\n",
                     panel_width, screen_width);
    } else {
        // Pas assez de place : prendre toute la largeur disponible
        panel_width = screen_width;
        debug_printf("📏 Panneau : largeur réduite (%dpx) - touche le bord gauche !\n",
                     panel_width);
    }

    panel->rect.w = panel_width;
    panel->rect.h = screen_height;

    // ═════════════════════════════════════════════════════════════════════════════
    // REPOSITIONNEMENT SELON L'ÉTAT DU PANNEAU
    // ═════════════════════════════════════════════════════════════════════════════
    // Le panneau peut être dans 4 états différents :
    // - PANEL_CLOSED  : Complètement hors écran (x = screen_width)
    // - PANEL_OPENING : En train de glisser vers la gauche (animation en cours)
    // - PANEL_OPEN    : Complètement visible, collé au bord droit
    // - PANEL_CLOSING : En train de glisser vers la droite (animation en cours)
    //
    // ⚠️ CRITIQUE : Il faut repositionner selon l'état actuel, sinon le panneau
    // saute toujours en position fermée lors du redimensionnement !
    // ═════════════════════════════════════════════════════════════════════════════

    if (panel->state == PANEL_CLOSED) {
        // Panneau fermé : hors écran à droite
        panel->rect.x = screen_width;
        panel->target_x = screen_width;
        panel->current_x = screen_width;

        debug_printf("📍 Panneau FERMÉ - Position: hors écran (%d)\n", screen_width);
    }
    else if (panel->state == PANEL_OPEN) {
        // Panneau ouvert : collé au bord droit, complètement visible
        panel->rect.x = screen_width - panel_width;
        panel->target_x = screen_width - panel_width;
        panel->current_x = screen_width - panel_width;

        debug_printf("📍 Panneau OUVERT - Position: (%d) largeur: %d\n",
                     panel->rect.x, panel_width);
    }
    else if (panel->state == PANEL_OPENING || panel->state == PANEL_CLOSING) {
        // En animation : recalculer la cible mais garder l'animation en cours
        panel->target_x = (panel->state == PANEL_OPENING)
        ? screen_width - panel_width   // Cible = visible
        : screen_width;                // Cible = hors écran

        // La position actuelle (current_x) continue son animation
        // Elle sera mise à jour automatiquement par update_settings_panel()

        debug_printf("📍 Panneau EN ANIMATION - Cible: %d\n", panel->target_x);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 2. REPOSITIONNER L'ENGRENAGE (scalé)
    // ─────────────────────────────────────────────────────────────────────────
    int gear_size = (int)(40 * scale_factor);
    int gear_margin = (int)(20 * scale_factor);

    panel->gear_rect.x = screen_width - gear_size - gear_margin;
    panel->gear_rect.y = gear_margin;
    panel->gear_rect.w = gear_size;
    panel->gear_rect.h = gear_size;

    // ─────────────────────────────────────────────────────────────────────────
    // 3. REPOSITIONNER LES BOUTONS (scalés)
    // ─────────────────────────────────────────────────────────────────────────
    int scaled_button_width = (int)(120 * scale_factor);   // BUTTON_WIDTH
    int scaled_button_height = (int)(40 * scale_factor);   // BUTTON_HEIGHT
    int scaled_spacing = (int)(10 * scale_factor);
    int scaled_bottom_margin = (int)(50 * scale_factor);

    // Recalculer le centrage
    int total_buttons_width = scaled_button_width * 2 + scaled_spacing;
    int buttons_start_x = (panel_width - total_buttons_width) / 2;

    // Bouton "Appliquer"
    panel->apply_button.rect.x = buttons_start_x;
    panel->apply_button.rect.y = screen_height - scaled_bottom_margin;
    panel->apply_button.rect.w = scaled_button_width;
    panel->apply_button.rect.h = scaled_button_height;

    // Bouton "Annuler"
    panel->cancel_button.rect.x = buttons_start_x + scaled_button_width + scaled_spacing;
    panel->cancel_button.rect.y = screen_height - scaled_bottom_margin;
    panel->cancel_button.rect.w = scaled_button_width;
    panel->cancel_button.rect.h = scaled_button_height;

    debug_printf("✅ Panneau rescalé - Largeur: %d, Boutons: %dx%d\n",
                 panel_width, scaled_button_width, scaled_button_height);

    // ═════════════════════════════════════════════════════════════════════════════
    // 4. REDIMENSIONNER LE SYSTÈME DE PRÉVISUALISATION
    // ═════════════════════════════════════════════════════════════════════════════
    // ⚠️ STRATÉGIE IMPORTANTE : Le preview suit le PANNEAU, pas la fenêtre !
    //
    // Le preview doit garder sa taille de base (100px) tant que le panneau
    // garde sa largeur de base (500px). Il ne doit rétrécir que lorsque
    // le panneau lui-même rétrécit.
    //
    // POURQUOI ?
    // - scale_factor est basé sur la fenêtre (1280px → 0.5 si fenêtre = 640px)
    // - Mais le panneau garde 500px jusqu'à ce qu'il touche le bord gauche
    // - Donc le preview doit suivre le PANNEAU, pas la fenêtre !
    //
    // SOLUTION :
    // Calculer un ratio basé sur la largeur actuelle du panneau par rapport
    // à sa largeur de base.
    // ═════════════════════════════════════════════════════════════════════════════

    // ─────────────────────────────────────────────────────────────────────────────
    // Dimensions de base du preview (à panel_width = 500px)
    // ─────────────────────────────────────────────────────────────────────────────
    const int BASE_PREVIEW_FRAME_X = 50;   // Position X dans le panneau
    const int BASE_PREVIEW_FRAME_Y = 80;   // Position Y dans le panneau
    const int BASE_PREVIEW_SIZE = 100;     // Taille du cadre carré

    // ─────────────────────────────────────────────────────────────────────────────
    // Calculer le ratio de redimensionnement du preview
    // ─────────────────────────────────────────────────────────────────────────────
    // Ce ratio dépend de la largeur du panneau, PAS de la fenêtre !
    //
    // Formule : ratio = largeur_actuelle_panneau / largeur_base_panneau
    //
    // Exemples :
    // - Panneau 500px : ratio = 500/500 = 1.0  → preview 100px ✓
    // - Panneau 400px : ratio = 400/500 = 0.8  → preview 80px  ✓
    // - Panneau 300px : ratio = 300/500 = 0.6  → preview 60px  ✓
    // ─────────────────────────────────────────────────────────────────────────────
    const int BASE_PANEL_WIDTH_FOR_PREVIEW = PANEL_WIDTH;  // Référence pour le calcul
    float panel_ratio = (float)panel_width / (float)BASE_PANEL_WIDTH_FOR_PREVIEW;

    debug_printf("📏 Ratio preview : %.2f (panneau: %d/%d)\n",
                 panel_ratio, panel_width, BASE_PANEL_WIDTH_FOR_PREVIEW);

    // ─────────────────────────────────────────────────────────────────────────────
    // Appliquer le ratio du panneau aux dimensions du preview
    // ─────────────────────────────────────────────────────────────────────────────
    panel->preview_system.frame_x = (int)(BASE_PREVIEW_FRAME_X * panel_ratio);
    panel->preview_system.frame_y = (int)(BASE_PREVIEW_FRAME_Y * panel_ratio);
    panel->preview_system.container_size = (int)(BASE_PREVIEW_SIZE * panel_ratio);

    // Le centre est toujours à container_size / 2 (centre du carré)
    panel->preview_system.center_x = panel->preview_system.container_size / 2;
    panel->preview_system.center_y = panel->preview_system.container_size / 2;

    debug_printf("📐 Preview redimensionné - Pos:(%d,%d) Taille:%d Centre:(%d,%d)\n",
                 panel->preview_system.frame_x,
                 panel->preview_system.frame_y,
                 panel->preview_system.container_size,
                 panel->preview_system.center_x,
                 panel->preview_system.center_y);

    // ─────────────────────────────────────────────────────────────────────────────
    // Redimensionner les hexagones du preview
    // ─────────────────────────────────────────────────────────────────────────────
    if (panel->preview_system.hex_list) {
        HexagoneNode* preview_node = panel->preview_system.hex_list->first;
        int preview_hex_count = 0;

        while (preview_node && preview_node->data) {
            Hexagon* hex = preview_node->data;

            // Repositionner au nouveau centre (relatif)
            hex->center_x = panel->preview_system.center_x;
            hex->center_y = panel->preview_system.center_y;

            // ═══════════════════════════════════════════════════════════════════════
            // MÉTHODE B : Recalculer les sommets ET mettre scale à 1.0
            // ═══════════════════════════════════════════════════════════════════════
            // Les sommets vx[i]/vy[i] sont recalculés à la bonne taille absolue
            // Donc on met current_scale à 1.0 pour ne PAS appliquer de scale en plus
            // ═══════════════════════════════════════════════════════════════════════

            // Recalculer les sommets avec la nouvelle taille de conteneur
            recalculer_sommets(hex, panel->preview_system.container_size);

            // ⚠️ CRITIQUE : Mettre le scale à 1.0 !
            // Sinon le scale précédent reste et l'hexagone ne change pas de taille
            hex->current_scale = 1.0f;

            /*debug_printf("  🔄 Preview hex %d - Container:%d, Scale:%.2f\n",
                         hex->element_id,
                         panel->preview_system.container_size,
                         hex->current_scale);*/

            preview_hex_count++;
            preview_node = preview_node->next;
        }

        debug_printf("✅ %d hexagones du preview redimensionnés (ratio: %.2f)\n",
                     preview_hex_count, panel_ratio);

        // ═════════════════════════════════════════════════════════════════════════
        // CRITIQUE : RE-PRÉCALCULER LES FRAMES D'ANIMATION !
        // ═════════════════════════════════════════════════════════════════════════
        // On vient de recalculer les sommets avec la nouvelle taille, MAIS les
        // frames d'animation ont été pré-calculées avec l'ancienne taille.
        //
        // À chaque frame, update_preview_animation() applique une frame pré-calculée
        // qui ÉCRASE les sommets qu'on vient de recalculer !
        //
        // Solution : re-précalculer toutes les frames avec la nouvelle taille.
        // ═════════════════════════════════════════════════════════════════════════

        if (panel->preview_system.hex_list) {
            precompute_all_cycles(panel->preview_system.hex_list,
                                  TARGET_FPS,
                                  panel->temp_config.breath_duration);

            debug_printf("🔄 Frames d'animation du preview recalculées\n");
        }
    }
}

void free_settings_panel(SettingsPanel* panel) {
    if (!panel) return;

    // ✅ LIBÉRER LA LISTE DE WIDGETS (qui libère automatiquement tous les widgets)
    if (panel->widget_list) {
        free_widget_list(panel->widget_list);
    }

    // Libérer la prévisualisation
    if (panel->preview_system.hex_list) {
        free_hexagone_list(panel->preview_system.hex_list);
    }

    // (garder le reste du nettoyage existant)
    if (panel->font_title) TTF_CloseFont(panel->font_title);
    if (panel->font) TTF_CloseFont(panel->font);
    if (panel->font_small) TTF_CloseFont(panel->font_small);
    TTF_Quit();

    if (panel->gear_icon) SDL_DestroyTexture(panel->gear_icon);
    if (panel->background) SDL_DestroyTexture(panel->background);
    if (panel->apply_button_texture) SDL_DestroyTexture(panel->apply_button_texture);
    if (panel->cancel_button_texture) SDL_DestroyTexture(panel->cancel_button_texture);

    free(panel);
    debug_printf("✅ Panneau de configuration libéré (avec widgets)\n");
}
