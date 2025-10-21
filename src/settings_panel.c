#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
//#include <stdio.h>
#include "settings_panel.h"
#include "debug.h"



#define PANEL_WIDTH 500
#define ANIMATION_DURATION 0.3f
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 40
#define BUTTON_MARGIN 20

// ✅ CALLBACKS pour les widgets (ajouter en haut du fichier, avant create_settings_panel)

static SettingsPanel* current_panel_for_callbacks = NULL;

static void duration_value_changed(int new_value) {
    if (!current_panel_for_callbacks) return;

    current_panel_for_callbacks->temp_config.breath_duration = new_value;
    debug_printf("🔄 Durée respiration changée: %d secondes\n", new_value);

    // Mettre à jour la prévisualisation en temps réel
    update_preview_for_new_duration(current_panel_for_callbacks, new_value);
}

static void cycles_value_changed(int new_value) {
    if (!current_panel_for_callbacks) return;

    current_panel_for_callbacks->temp_config.breath_cycles = new_value;
    debug_printf("🔄 Cycles changés: %d\n", new_value);
}

static void alternate_cycles_changed(bool new_value) {
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

    // Réinitialiser les paramètres
    panel->preview_system.center_x = 50;
    panel->preview_system.center_y = 50;
    panel->preview_system.container_size = 100;
    panel->preview_system.size_ratio = 0.90f;

    debug_printf("🔄 Paramètres preview réinitialisés - Centre: (%d,%d), Container: %d, Ratio: %.2f\n",
                 panel->preview_system.center_x, panel->preview_system.center_y,
                 panel->preview_system.container_size, panel->preview_system.size_ratio);

    // Recréer les hexagones
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


SettingsPanel* create_settings_panel(SDL_Renderer* renderer, int screen_width, int screen_height) {
    SettingsPanel* panel = malloc(sizeof(SettingsPanel));
    if (!panel) return NULL;

    // INITIALISATION EXPLICITE de tous les membres
    memset(panel, 0, sizeof(SettingsPanel));


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

    // Calcul de la largeur totale approximative du widget le plus large
    int largeur_max_widget = 180 + 20 + 40 + 20; // texte_max + flèches + valeur + marges
    int widget_x = (PANEL_WIDTH - largeur_max_widget) / 2; // Centrage horizontal

    // ══════════════════════════════════════════════════════════════════════════
    // CRÉATION DE LA LISTE DE WIDGETS
    // ══════════════════════════════════════════════════════════════════════════
    panel->widget_list = create_widget_list();

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

    // ──────────────────────────────────────────────────────────────────────────
    // DEBUG : Afficher le contenu de la liste
    // ──────────────────────────────────────────────────────────────────────────
    debug_print_widget_list(panel->widget_list);

    // Créer les boutons UI
    panel->apply_button = create_button("Appliquer", 200, screen_height-50, 120, 30);
    panel->cancel_button = create_button("Annuler", 330, screen_height-50, 120, 30);

    // Initialisation de base
    panel->state = PANEL_CLOSED;
    panel->rect = (SDL_Rect){screen_width, 0, PANEL_WIDTH, screen_height};
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

    // Position de l'engrenage en haut à droite
    panel->gear_rect = (SDL_Rect){screen_width - 60, 20, 40, 40};


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
    panel->current_x = panel->target_x - (int)(PANEL_WIDTH * eased);
    panel->rect.x = panel->current_x;

    // ══════════════════════════════════════════════════════════════════════════
    // MISE À JOUR DES ANIMATIONS DES WIDGETS (EN UNE LIGNE !)
    // ══════════════════════════════════════════════════════════════════════════
    if (panel->state == PANEL_OPEN) {
        update_preview_animation(panel);
        update_widget_list_animations(panel->widget_list, delta_time);
    }

    /*// Animation de prévisualisation
    if (panel->state == PANEL_OPEN) {
        update_preview_animation(panel);
    }*/
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

        // === ESPACE RÉSERVÉ POUR L'ANIMATION ===
        // Dessiner un cadre pour la prévisualisation
        rectangleColor(renderer,
                       panel_x + 50, panel_y + 80,
                       panel_x + 150, panel_y + 180,
                       0xFFFFFFFF);

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

                // Réinitialiser la prévisualisation avec la config actuelle
                reinitialiser_preview_system(&panel->preview_system);
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
