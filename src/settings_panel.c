#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "settings_panel.h"
#include "debug.h"



#define PANEL_WIDTH 500
#define ANIMATION_DURATION 0.3f
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 40
#define BUTTON_MARGIN 20

/* === NOUVELLES FONCTIONS PRÉVISUALISATION === */

void init_preview_system(SettingsPanel* panel, int x, int y, int size, float ratio) {
    panel->preview_system.frame_x = x;
    panel->preview_system.frame_y = y;
    panel->preview_system.center_x = size/2;
    panel->preview_system.center_y = size/2;
    panel->preview_system.container_size = size;
    panel->preview_system.size_ratio = ratio;
    panel->preview_system.last_update = SDL_GetTicks();
    panel->preview_system.current_time = 0.0;

    debug_printf("🔄 INIT Prévisualisation - Cadre: (%d,%d), Centre: (%d,%d), Taille: %d, Ratio: %.2f\n",
           x, y, panel->preview_system.center_x, panel->preview_system.center_y, size, ratio);

    // Créer les hexagones
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
    if (!panel->preview_system.hex_list) return;

    debug_printf("🔄 Mise à jour prévisualisation - nouvelle durée: %.1fs\n", new_duration);

    // Libérer l'ancien précalcul
    HexagoneNode* node = panel->preview_system.hex_list->first;
    while (node) {
        free(node->precomputed_vx);
        free(node->precomputed_vy);
        node->precomputed_vx = NULL;
        node->precomputed_vy = NULL;
        node = node->next;
    }

    // ✅ DEBUG : Afficher l'état avant réinitialisation
    node = panel->preview_system.hex_list->first;
    if (node && node->data) {
        Hexagon* first_hex = node->data;
        debug_printf("🔍 AVANT - Centre: (%d,%d), Scale: %.2f, vx[0]: %d, vy[0]: %d\n",
               first_hex->center_x, first_hex->center_y,
               first_hex->current_scale, first_hex->vx[0], first_hex->vy[0]);
    }

    // Réinitialiser les hexagones
    node = panel->preview_system.hex_list->first;
    while (node) {
        if (node->data) {
            // Réinitialiser la position ET l'échelle
            move_hexagon(node->data, panel->preview_system.center_x, panel->preview_system.center_y);
            scale_hexagon(node->data, 1.0f);
            node->current_cycle = 0;
        }
        node = node->next;
    }

    // ✅ DEBUG : Afficher l'état après réinitialisation
    node = panel->preview_system.hex_list->first;
    if (node && node->data) {
        Hexagon* first_hex = node->data;
        debug_printf("🔍 APRES - Centre: (%d,%d), Scale: %.2f, vx[0]: %d, vy[0]: %d\n",
               first_hex->center_x, first_hex->center_y,
               first_hex->current_scale, first_hex->vx[0], first_hex->vy[0]);
    }

    // Re-précalculer
    precompute_all_cycles(panel->preview_system.hex_list, TARGET_FPS, new_duration);

    // Réinitialiser le temps
    panel->preview_system.current_time = 0.0;
    panel->preview_system.last_update = SDL_GetTicks();

    debug_printf("✅ Prévisualisation réinitialisée avec nouvelle durée\n");
}

void render_preview(SDL_Renderer* renderer, PreviewSystem* preview, int offset_x, int offset_y) {
    if (!preview || !preview->hex_list) return;

    HexagoneNode* node = preview->hex_list->first;
    if (node && node->data) {
        // ✅ DEBUG : Afficher l'état pendant le rendu
        Hexagon* first_hex = node->data;
        debug_printf("🎨 RENDU - Centre: (%d,%d), Scale: %.2f, vx[0]: %d, vy[0]: %d, Offset: (%d,%d)\n",
               first_hex->center_x, first_hex->center_y,
               first_hex->current_scale, first_hex->vx[0], first_hex->vy[0],
               offset_x, offset_y);
    }

    while (node) {
        if (node->data) {
            // Sauvegarder la position et l'échelle originales
            int original_center_x = node->data->center_x;
            int original_center_y = node->data->center_y;
            float original_scale = node->data->current_scale;

            // Positionner au centre du cadre de prévisualisation
            int preview_center_x = offset_x + preview->frame_x + preview->container_size/2;
            int preview_center_y = offset_y + preview->frame_y + preview->container_size/2;

            transform_hexagon(node->data, preview_center_x, preview_center_y, 1.0f);

            // Rendre l'hexagone
            make_hexagone(renderer, node->data);

            // Restaurer la position et l'échelle originales
            transform_hexagon(node->data, original_center_x, original_center_y, original_scale);
        }
        node = node->next;
    }
}


SettingsPanel* create_settings_panel(SDL_Renderer* renderer, int screen_width, int screen_height) {
    SettingsPanel* panel = malloc(sizeof(SettingsPanel));
    if (!panel) return NULL;

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
        panel->font = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeSans.otf", 28);
    }
    if (!panel->font) {
        debug_printf("Erreur chargement police: %s\n", TTF_GetError());
        // Police normale
        panel->font = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeSans.otf", 20);
    }
    if (!panel->font_small) {
        debug_printf("Erreur chargement police: %s\n", TTF_GetError());
        // Police mini
        panel->font = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeSans.otf", 16);
    }

    // Chargement configuration temporaire
    load_config(&panel->temp_config);

    // === RÉORGANISATION DE L'ESPACE ===

    // Créer les sliders
    panel->duration_slider = create_slider(50, 240, 250, 1, 10, panel->temp_config.breath_duration);
    panel->cycles_slider = create_slider(50, 320, 250, 1, 20, panel->temp_config.breath_cycles);

    // Créer les boutons UI
    panel->apply_button = create_button("Appliquer", 200, screen_height-50, 120, 30);
    panel->cancel_button = create_button("Annuler", 330, screen_height-50, 120, 30);

    // Initialisation de base
    panel->state = PANEL_CLOSED;
    panel->rect = (SDL_Rect){screen_width, 0, PANEL_WIDTH, screen_height};
    panel->target_x = screen_width;
    panel->current_x = screen_width;
    panel->animation_progress = 0.0f;

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
    init_preview_system(panel, 50, 80, 100, 0.7f);

    debug_printf("Panneau de configuration créé\n");
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

    // Animation de prévisualisation
    if (panel->state == PANEL_OPEN) {
        update_preview_animation(panel);
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
        render_text(renderer, panel->font_title,"Configuration Respiration", panel_x + 140, panel_y + 40, 0xFFFFFFFF);

        // === ESPACE RÉSERVÉ POUR L'ANIMATION ===
        // Dessiner un cadre pour la prévisualisation
        rectangleColor(renderer,
                       panel_x + 50, panel_y + 80,
                       panel_x + 150, panel_y + 180,
                       0xFFFFFFFF);

        // Hexagone de prévisualisation
        render_preview(renderer, &panel->preview_system, panel_x, panel_y);

        // === SLIDER DURÉE RESPIRATION ===
        render_text(renderer, panel->font, "Vitesse de respiration", panel_x + 50, panel_y + 200, 0xFFFFFFFF);

        // Barre de séparation visuelle
        boxColor(renderer,
                 panel_x + 50, panel_y + 228,
                 panel_x + PANEL_WIDTH - 100, panel_y + 229,
                 0xFF666666);

        char duration_text[50];
        debug_printf(duration_text, sizeof(duration_text), "%d secondes", panel->duration_slider.current_value);
        render_text(renderer, panel->font_small, duration_text, panel_x + 50, panel_y + 255, 0xFF000000);

        render_slider(renderer, &panel->duration_slider, panel->font, panel_x, panel_y);

        // === SLIDER CYCLES ===
        render_text(renderer, panel->font, "Cycles", panel_x + 50, panel_y + 280, 0xFFFFFFFF);

        // Barre de séparation visuelle
        boxColor(renderer,
                 panel_x + 50, panel_y + 308,
                 panel_x + PANEL_WIDTH - 100, panel_y + 309,
                 0xFF666666);

        char cycles_text[50];
        debug_printf(cycles_text, sizeof(cycles_text), "%d cycles", panel->cycles_slider.current_value);
        render_text(renderer, panel->font_small, cycles_text, panel_x + 50, panel_y + 335, 0xFF000000);

        render_slider(renderer, &panel->cycles_slider, panel->font, panel_x, panel_y);

        // === BOUTONS ===
        render_button(renderer, &panel->apply_button, panel->font, panel_x, panel_y);
        render_button(renderer, &panel->cancel_button, panel->font, panel_x, panel_y);
    }
}

void handle_settings_panel_event(SettingsPanel* panel, SDL_Event* event, AppConfig* main_config) {
    if (!panel || !event) return;

    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;

        // Clic sur l'engrenage
        if (is_point_in_rect(x, y, panel->gear_rect)) {
            if (panel->state == PANEL_CLOSED) {
                panel->state = PANEL_OPENING;
                // Recharger la config actuelle dans la config temporaire
                load_config(&panel->temp_config);
                // NOUVEAU : Mettre à jour la prévisualisation avec la config actuelle
                update_preview_for_new_duration(panel, panel->temp_config.breath_duration);
                debug_printf("Ouverture du panneau de configuration\n");
            } else if (panel->state == PANEL_OPEN) {
                panel->state = PANEL_CLOSING;
                debug_printf("Fermeture du panneau de configuration\n");
            }
        }

        // Gestion des clics dans le panneau ouvert
        if (panel->state == PANEL_OPEN) {
            int panel_x = panel->rect.x;
            int panel_y = panel->rect.y;

            // === GESTION DES SLIDERS ===
            // Créer des rectangles absolus pour la détection
            SDL_Rect duration_slider_abs = {
                panel->duration_slider.thumb_rect.x + panel_x,
                panel->duration_slider.thumb_rect.y + panel_y,
                panel->duration_slider.thumb_rect.w,
                panel->duration_slider.thumb_rect.h
            };

            SDL_Rect cycles_slider_abs = {
                panel->cycles_slider.thumb_rect.x + panel_x,
                panel->cycles_slider.thumb_rect.y + panel_y,
                panel->cycles_slider.thumb_rect.w,
                panel->cycles_slider.thumb_rect.h
            };

            // Clic sur les curseurs des sliders
            if (is_point_in_rect(x, y, duration_slider_abs)) {
                panel->duration_slider.is_dragging = true;
                debug_printf("Début drag durée respiration\n");
            }
            if (is_point_in_rect(x, y, cycles_slider_abs)) {
                panel->cycles_slider.is_dragging = true;
                debug_printf("Début drag cycles\n");
            }

            // === GESTION DES BOUTONS ===
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

    // === GESTION DU DRAG CONTINU DES SLIDERS ===
    if (event->type == SDL_MOUSEMOTION) {
        if (panel->duration_slider.is_dragging) {
            int mouse_x = event->motion.x;
            int panel_x = panel->rect.x;

            // Calculer la nouvelle valeur
            float ratio = (float)(mouse_x - panel_x - panel->duration_slider.track_rect.x) /
            panel->duration_slider.track_rect.w;
            ratio = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);

            int new_value = panel->duration_slider.min_value +
            (int)(ratio * (panel->duration_slider.max_value - panel->duration_slider.min_value));

            // Mettre à jour seulement si la valeur a changé
            if (new_value != panel->duration_slider.current_value) {
                panel->duration_slider.current_value = new_value;

                // Mettre à jour la config temporaire
                panel->temp_config.breath_duration = panel->duration_slider.current_value;

                // Mettre à jour la prévisualisation en temps réel
                update_preview_for_new_duration(panel, panel->temp_config.breath_duration);

                update_slider_thumb_position(&panel->duration_slider);
            }
        }

        if (panel->cycles_slider.is_dragging) {
            int mouse_x = event->motion.x;
            int panel_x = panel->rect.x;

            float ratio = (float)(mouse_x - panel_x - panel->cycles_slider.track_rect.x) /
            panel->cycles_slider.track_rect.w;
            ratio = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);

            panel->cycles_slider.current_value = panel->cycles_slider.min_value +
            (int)(ratio * (panel->cycles_slider.max_value - panel->cycles_slider.min_value));

            // Mettre à jour la config temporaire
            panel->temp_config.breath_cycles = panel->cycles_slider.current_value;

            update_slider_thumb_position(&panel->cycles_slider);
        }
    }

    // === FIN DU DRAG ===
    if (event->type == SDL_MOUSEBUTTONUP) {
        panel->duration_slider.is_dragging = false;
        panel->cycles_slider.is_dragging = false;
    }
}

void free_settings_panel(SettingsPanel* panel) {
    if (!panel) return;
    // Libérer la prévisualisation
    if (panel->preview_system.hex_list) {
        free_hexagone_list(panel->preview_system.hex_list);
    }
    if (panel->font) TTF_CloseFont(panel->font);
    TTF_Quit();

    if (panel->gear_icon) SDL_DestroyTexture(panel->gear_icon);
    if (panel->background) SDL_DestroyTexture(panel->background);
    if (panel->apply_button_texture) SDL_DestroyTexture(panel->apply_button_texture);
    if (panel->cancel_button_texture) SDL_DestroyTexture(panel->cancel_button_texture);

    free(panel);
    debug_printf("Panneau de configuration libéré\n");
}

// Fonction helper pour détecter les clics
bool is_point_in_rect(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w &&
    y >= rect.y && y <= rect.y + rect.h);
}
