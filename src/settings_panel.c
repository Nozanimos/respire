// SPDX-License-Identifier: GPL-3.0-or-later
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include "settings_panel.h"
#include "preview_widget.h"
#include "button_widget.h"
#include "debug.h"
#include "json_config_loader.h"

#define PANEL_WIDTH 500
#define ANIMATION_DURATION 0.3f
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 40
#define BUTTON_MARGIN 20

// Forward declarations (évite include circulaire avec renderer.h)
extern float calculate_scale_factor(int width, int height);
extern int scale_value(int value, float scale);
extern int calculate_panel_width(int screen_width, float scale);

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACKS POUR LES WIDGETS
// ════════════════════════════════════════════════════════════════════════════
static SettingsPanel* current_panel_for_callbacks = NULL;
static AppConfig* current_main_config_for_callbacks = NULL;

void duration_value_changed(int new_value) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // ═══════════════════════════════════════════════════════════════════════════
    // APPLIQUER IMMÉDIATEMENT À LA CONFIGURATION PRINCIPALE
    // ═══════════════════════════════════════════════════════════════════════════
    current_main_config_for_callbacks->breath_duration = new_value;
    current_panel_for_callbacks->temp_config.breath_duration = new_value;

    // Sauvegarder immédiatement dans le fichier
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Durée respiration changée: %d secondes (sauvegardé)\n", new_value);

    // ═══════════════════════════════════════════════════════════════════════════
    // METTRE À JOUR LE PREVIEW DANS LA WIDGET LIST
    // ═══════════════════════════════════════════════════════════════════════════
    WidgetList* list = current_panel_for_callbacks->widget_list;
    if (list) {
        WidgetNode* node = list->first;
        while (node) {
            // Chercher le widget preview
            if (node->type == WIDGET_TYPE_PREVIEW && node->widget.preview_widget) {
                // Appeler la fonction du preview_widget.c
                preview_set_breath_duration(node->widget.preview_widget, (float)new_value);
                break;  // On a trouvé le preview, on peut sortir
            }
            node = node->next;
        }
    }
}

void cycles_value_changed(int new_value) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // Appliquer immédiatement
    current_main_config_for_callbacks->nb_session = new_value;
    current_panel_for_callbacks->temp_config.nb_session = new_value;

    // Sauvegarder immédiatement
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Cycles changés: %d (sauvegardé)\n", new_value);
}

void nb_breath(int new_value) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // Appliquer immédiatement
    current_main_config_for_callbacks->Nb_respiration = new_value;
    current_panel_for_callbacks->temp_config.Nb_respiration = new_value;

    // Sauvegarder immédiatement
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Nombre de respirations changé: %d (sauvegardé)\n", new_value);
}

void start_value_changed(int new_value) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // Appliquer immédiatement
    current_main_config_for_callbacks->start_duration = new_value;
    current_panel_for_callbacks->temp_config.start_duration = new_value;

    // Sauvegarder immédiatement
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Durée de démarrage changée: %d secondes (sauvegardé)\n", new_value);
}

void alternate_cycles_changed(bool new_value) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // Appliquer immédiatement
    current_main_config_for_callbacks->alternate_cycles = new_value;
    current_panel_for_callbacks->temp_config.alternate_cycles = new_value;

    // Sauvegarder immédiatement
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Cycles alternés changés: %s (sauvegardé)\n", new_value ? "ACTIF" : "INACTIF");
}

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACKS POUR LE SELECTOR TYPE DE RÉTENTION
// ════════════════════════════════════════════════════════════════════════════
void retention_full(void) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // Appliquer immédiatement : poumons pleins = 0
    current_main_config_for_callbacks->retention_type = 0;
    current_panel_for_callbacks->temp_config.retention_type = 0;

    // Sauvegarder immédiatement
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Type de rétention changé: POUMONS PLEINS (sauvegardé)\n");
}

void retention_empty(void) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // Appliquer immédiatement : poumons vides = 1
    current_main_config_for_callbacks->retention_type = 1;
    current_panel_for_callbacks->temp_config.retention_type = 1;

    // Sauvegarder immédiatement
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Type de rétention changé: POUMONS VIDES (sauvegardé)\n");
}

void retention_alternate(void) {
    if (!current_panel_for_callbacks || !current_main_config_for_callbacks) return;

    // Appliquer immédiatement : alternée = 2
    current_main_config_for_callbacks->retention_type = 2;
    current_panel_for_callbacks->temp_config.retention_type = 2;

    // Sauvegarder immédiatement
    save_config(current_main_config_for_callbacks);

    debug_printf("✅ Type de rétention changé: ALTERNÉE (sauvegardé)\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACKS POUR LES BOUTONS APPLIQUER/ANNULER
// ════════════════════════════════════════════════════════════════════════════
// NOTE : Les changements sont maintenant appliqués immédiatement lors de chaque
// modification de widget. Ces boutons servent simplement à fermer le panneau.
void apply_button_clicked(void) {
    if (!current_panel_for_callbacks) return;

    // Les changements sont déjà appliqués et sauvegardés
    // On ferme simplement le panneau
    current_panel_for_callbacks->state = PANEL_CLOSING;
    debug_printf("✅ Panneau fermé (changements déjà appliqués)\n");
}

void cancel_button_clicked(void) {
    if (!current_panel_for_callbacks) return;

    // Les changements sont déjà appliqués et sauvegardés
    // On ferme simplement le panneau
    current_panel_for_callbacks->state = PANEL_CLOSING;
    debug_printf("✅ Panneau fermé\n");
}
// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION DU PANNEAU
// ════════════════════════════════════════════════════════════════════════════

SettingsPanel* create_settings_panel(SDL_Renderer* renderer, SDL_Window* window, int screen_width, int screen_height, float scale_factor) {
    SettingsPanel* panel = malloc(sizeof(SettingsPanel));
    if (!panel) return NULL;

    memset(panel, 0, sizeof(SettingsPanel));
    panel->scale_factor = scale_factor;
    panel->state = PANEL_CLOSED;
    panel->renderer = renderer;
    panel->window = window;
    panel->screen_width = screen_width;
    panel->screen_height = screen_height;

    // Initialisation du hot reload
    panel->json_config_path = "../config/widgets_config.json";
    panel->json_check_interval = 0.5f;  // Vérifier toutes les 0.5 secondes
    panel->time_since_last_check = 0.0f;
    panel->last_json_mtime = 0;

    // Initialisation du scroll et layout responsive
    panel->scroll_offset = 0;
    panel->content_height = 0;
    panel->max_scroll = 0;
    panel->layout_mode_column = false;
    panel->layout_threshold_width = 350;  // Passer en mode colonne si largeur < 350px

    debug_printf("🎨 Création panneau avec scale: %.2f\n", scale_factor);

    // ════════════════════════════════════════════════════════════════════════
    // OBTENTION DES POLICES (depuis le gestionnaire centralisé)
    // ════════════════════════════════════════════════════════════════════════
    // Les tailles sont calculées avec le scale_factor mais le minimum de 16px
    // sera appliqué automatiquement par get_font_for_size()

    int font_title_size = scale_value(28, scale_factor);
    int font_normal_size = scale_value(20, scale_factor);
    int font_small_size = scale_value(16, scale_factor);

    panel->font_title = get_font_for_size(font_title_size);
    panel->font = get_font_for_size(font_normal_size);
    panel->font_small = get_font_for_size(font_small_size);

    if (!panel->font_title || !panel->font || !panel->font_small) {
        debug_printf("❌ Impossible d'obtenir les polices pour le panneau\n");
        free(panel);
        return NULL;
    }

    debug_subsection("Polices du panneau");
    debug_printf("  Titre : %dpx\n", font_title_size);
    debug_printf("  Normal : %dpx\n", font_normal_size);
    debug_printf("  Petit : %dpx\n", font_small_size);
    debug_blank_line();

    // ════════════════════════════════════════════════════════════════════════
    // CHARGEMENT DES WIDGETS DEPUIS JSON
    // ════════════════════════════════════════════════════════════════════════
    panel->widget_list = create_widget_list();
    load_config(&panel->temp_config);

    LoaderContext ctx = {
        .renderer = renderer,
        .font_titre = panel->font_title,
        .font_normal = panel->font,
        .font_petit = panel->font_small
    };

    if (!charger_widgets_depuis_json(panel->json_config_path, &ctx, panel->widget_list)) {
        debug_printf("⚠️ Échec chargement JSON, utilisation config par défaut\n");
    }

    // Initialiser le timestamp du fichier JSON
    struct stat file_stat;
    if (stat(panel->json_config_path, &file_stat) == 0) {
        panel->last_json_mtime = file_stat.st_mtime;
        debug_printf("📅 JSON timestamp initial: %ld\n", (long)panel->last_json_mtime);
    }

    debug_print_widget_list(panel->widget_list);

    // Synchroniser les widgets avec la config chargée (pour initialiser les valeurs)
    sync_config_to_widgets(&panel->temp_config, panel->widget_list);

    // ════════════════════════════════════════════════════════════════════════
    // CHARGEMENT DU FOND ET DE L'ICÔNE
    // ════════════════════════════════════════════════════════════════════════
    SDL_Surface* bg_surface = IMG_Load("../img/settings_bg.png");
    if (!bg_surface) {
        bg_surface = SDL_CreateRGBSurface(0, PANEL_WIDTH, screen_height, 32, 0, 0, 0, 0);
        SDL_FillRect(bg_surface, NULL, SDL_MapRGBA(bg_surface->format, 240, 240, 240, 255));
    }
    panel->background = SDL_CreateTextureFromSurface(renderer, bg_surface);
    SDL_FreeSurface(bg_surface);

    SDL_Surface* gear_surface = IMG_Load("../img/settings.png");
    if (!gear_surface) {
        gear_surface = SDL_CreateRGBSurface(0, 40, 40, 32, 0, 0, 0, 0);
        SDL_FillRect(gear_surface, NULL, SDL_MapRGBA(gear_surface->format, 128, 128, 128, 255));
    }
    panel->gear_icon = SDL_CreateTextureFromSurface(renderer, gear_surface);
    SDL_FreeSurface(gear_surface);

    int gear_size = scale_value(40, scale_factor);
    int gear_margin = scale_value(20, scale_factor);
    panel->gear_rect = (SDL_Rect){
        screen_width - gear_size - gear_margin,
        gear_margin,
        gear_size,
        gear_size
    };

    // ════════════════════════════════════════════════════════════════════════
    // CALCUL DES POSITIONS INITIALES (responsive)
    // ════════════════════════════════════════════════════════════════════════
    update_panel_scale(panel, screen_width, screen_height, scale_factor);

    debug_printf("✅ Panneau de configuration créé avec widgets\n");
    return panel;
}

// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DU PANNEAU (animation)
// ════════════════════════════════════════════════════════════════════════════

void update_settings_panel(SettingsPanel* panel, float delta_time) {
    if (!panel) return;

    // Mise à jour de l'animation
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

    // Interpolation de la position (easing cubique)
    float eased = panel->animation_progress * panel->animation_progress *
    panel->animation_progress;

    // Calcul de la position actuelle
    if (panel->state == PANEL_OPENING) {
        // Va de "hors écran" (target_x + rect.w) vers "visible" (target_x)
        // eased va de 0.0 → 1.0
        int start_x = panel->target_x + panel->rect.w;
        panel->current_x = start_x - (int)(panel->rect.w * eased);

    } else if (panel->state == PANEL_CLOSING) {
        // Va de "visible" (target_x) vers "hors écran" (target_x + rect.w)
        // eased va de 1.0 → 0.0, donc (1.0 - eased) va de 0.0 → 1.0
        panel->current_x = panel->target_x + (int)(panel->rect.w * (1.0f - eased));

    } else if (panel->state == PANEL_OPEN) {
        // ═════════════════════════════════════════════════════════════════════════
        // FORCER LA POSITION FINALE EXACTE
        // ═════════════════════════════════════════════════════════════════════════
        // Quand l'animation est terminée, s'assurer que le panneau est exactement
        // à sa position cible (évite les erreurs d'arrondi pendant l'animation)
        panel->current_x = panel->target_x;
    }

    panel->rect.x = panel->current_x;

    // Mise à jour des animations internes (preview, widgets)
    if (panel->state == PANEL_OPEN) {
        update_widget_list_animations(panel->widget_list, delta_time);
    }

    // Vérification du hot reload du JSON (seulement si le panneau est ouvert)
    if (panel->state == PANEL_OPEN) {
        check_json_hot_reload(panel, delta_time, panel->screen_width, panel->screen_height);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU PANNEAU
// ════════════════════════════════════════════════════════════════════════════

void render_settings_panel(SDL_Renderer* renderer, SettingsPanel* panel) {
    if (!panel) return;

    // Icône engrenage (toujours visible)
    if (panel->gear_icon) {
        SDL_RenderCopy(renderer, panel->gear_icon, NULL, &panel->gear_rect);
    }

    // Panneau (seulement si non fermé)
    if (panel->state != PANEL_CLOSED) {
        SDL_RenderCopy(renderer, panel->background, NULL, &panel->rect);

        int panel_x = panel->rect.x;
        int panel_y = panel->rect.y;

        // Widgets (avec scroll)
        render_all_widgets(renderer, panel->widget_list, panel_x, panel_y, panel->rect.w, panel->scroll_offset);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════

void handle_settings_panel_event(SettingsPanel* panel, SDL_Event* event, AppConfig* main_config) {
    if (!panel || !event) return;

    current_panel_for_callbacks = panel;
    current_main_config_for_callbacks = main_config;
    int panel_x = panel->rect.x;
    int panel_y = panel->rect.y;

    // ═════════════════════════════════════════════════════════════════════════
    // RACCOURCI CLAVIER F5 : FORCER LE RECHARGEMENT DU JSON
    // ═════════════════════════════════════════════════════════════════════════
    if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_F5) {
        debug_printf("⚡ F5 pressé : Rechargement forcé du JSON\n");
        reload_widgets_from_json(panel, panel->screen_width, panel->screen_height);
        return;
    }

    // Clic sur l'engrenage (ouvrir/fermer)
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;

        if (is_point_in_rect(x, y, panel->gear_rect)) {
            if (panel->state == PANEL_CLOSED) {
                panel->state = PANEL_OPENING;
                panel->animation_progress = 0.0f;

                // ═════════════════════════════════════════════════════════════════════════
                // CALCULER LA POSITION CIBLE (où le panneau doit aller)
                // ═════════════════════════════════════════════════════════════════════════
                // ⚠️ IMPORTANT : On doit mettre à jour target_x MAINTENANT !
                // Sans ça, l'animation utilise l'ancien target_x (hors écran)
                //
                // Position cible = screen_width - panel_width (collé au bord droit)
                // ═════════════════════════════════════════════════════════════════════════

                // Utiliser la vraie largeur d'écran stockée dans panel
                panel->target_x = panel->screen_width - panel->rect.w;

                debug_printf("🎯 OUVERTURE - target_x=%d, screen_width=%d, panel_width=%d\n",
                             panel->target_x, panel->screen_width, panel->rect.w);


                // Recharger la config et mettre à jour les widgets
                load_config(&panel->temp_config);

                // Synchroniser TOUS les widgets depuis la config (générique)
                sync_config_to_widgets(&panel->temp_config, panel->widget_list);

                // Mettre à jour la durée du preview widget via la widget_list
                if (panel->widget_list) {
                    WidgetNode* node = panel->widget_list->first;
                    while (node) {
                        if (node->type == WIDGET_TYPE_PREVIEW && node->widget.preview_widget) {
                            preview_set_breath_duration(node->widget.preview_widget,
                                                       (float)panel->temp_config.breath_duration);
                            break;
                        }
                        node = node->next;
                    }
                }
                debug_printf("🎯 Ouverture panneau\n");

            } else if (panel->state == PANEL_OPEN) {
                panel->state = PANEL_CLOSING;
                debug_printf("🎯 Fermeture panneau\n");
            }
            return;
        }
    }

    // Événements du panneau ouvert
    if (panel->state == PANEL_OPEN) {
        // Gestion du scroll (molette souris)
        handle_panel_scroll(panel, event);

        // Événements des widgets
        handle_widget_list_events(panel->widget_list, event, panel_x, panel_y);
    }

    if (event->type == SDL_MOUSEBUTTONUP) {
        current_panel_for_callbacks = NULL;
        current_main_config_for_callbacks = NULL;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DU SCALE (resize fenêtre)
// ════════════════════════════════════════════════════════════════════════════

void update_panel_scale(SettingsPanel* panel, int screen_width, int screen_height, float scale_factor) {
    if (!panel) return;

    panel->scale_factor = scale_factor;
    panel->screen_width = screen_width;
    panel->screen_height = screen_height;

    // ═════════════════════════════════════════════════════════════════════════
    // CALCUL DE LA LARGEUR MINIMALE NÉCESSAIRE (basée sur les widgets)
    // ═════════════════════════════════════════════════════════════════════════
    int min_panel_width = calculate_min_panel_width(panel->widget_list);

    // ═════════════════════════════════════════════════════════════════════════
    // CALCUL DE LA LARGEUR DU PANNEAU (limitée par le minimum)
    // ═════════════════════════════════════════════════════════════════════════
    int panel_width = (screen_width >= PANEL_WIDTH) ? PANEL_WIDTH : screen_width;

    // Ne jamais descendre en dessous de la largeur minimale
    if (panel_width < min_panel_width) {
        panel_width = min_panel_width;
    }

    panel->rect.w = panel_width;
    panel->rect.h = screen_height;

    // Calcul du ratio pour les éléments internes (garde pour compatibilité)
    float panel_ratio = (float)panel_width / (float)PANEL_WIDTH;

    // Sauvegarder le ratio dans la structure pour l'utiliser ailleurs
    panel->panel_ratio = panel_ratio;

    // ═════════════════════════════════════════════════════════════════════════
    // POSITIONS SELON L'ÉTAT
    // ═════════════════════════════════════════════════════════════════════════
    // ⚠️ IMPORTANT : Toujours mettre à jour target_x pour avoir la bonne cible
    // même lors d'un resize pendant une animation
    panel->target_x = screen_width - panel_width;  // Position ouverte

    if (panel->state == PANEL_CLOSED) {
        panel->rect.x = screen_width;
        panel->target_x = screen_width;
        panel->current_x = screen_width;
    }
    else if (panel->state == PANEL_OPEN) {
        // ═════════════════════════════════════════════════════════════════════
        // FIX: S'assurer que le panneau est bien collé au bord droit
        // ═════════════════════════════════════════════════════════════════════
        panel->rect.x = screen_width - panel_width;
        panel->current_x = screen_width - panel_width;
    }
    else if (panel->state == PANEL_OPENING || panel->state == PANEL_CLOSING) {
        // Mettre à jour seulement la cible, current_x est géré par l'animation
        // Mais recalculer la position actuelle basée sur l'animation_progress
        if (panel->state == PANEL_CLOSING) {
            panel->target_x = screen_width;
        }
        // Recalculer current_x basé sur animation_progress pour éviter les sauts
        float eased = panel->animation_progress * panel->animation_progress *
                      panel->animation_progress;
        if (panel->state == PANEL_OPENING) {
            int start_x = screen_width;
            panel->current_x = start_x - (int)(panel_width * eased);
        } else {
            panel->current_x = panel->target_x - panel_width + (int)(panel_width * (1.0f - eased));
        }
        panel->rect.x = panel->current_x;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // MISE À JOUR DE L'ENGRENAGE
    // ═════════════════════════════════════════════════════════════════════════
    // ⚠️ IMPORTANT : L'engrenage doit rester à sa taille de base (40px) tant
    // que la fenêtre fait plus de PANEL_WIDTH (500px) !
    // Il ne doit réduire QUE si la fenêtre devient plus petite.
    //
    // Solution : Utiliser le panel_ratio (et non scale_factor) !
    // ═════════════════════════════════════════════════════════════════════════
    const int BASE_GEAR_SIZE = 40;
    const int BASE_GEAR_MARGIN = 20;

    int gear_size = (int)(BASE_GEAR_SIZE * panel_ratio);
    int gear_margin = (int)(BASE_GEAR_MARGIN * panel_ratio);

    panel->gear_rect.x = screen_width - gear_size - gear_margin;
    panel->gear_rect.y = gear_margin;
    panel->gear_rect.w = gear_size;
    panel->gear_rect.h = gear_size;

    // Mise à jour des boutons (positions RELATIVES)
    int scaled_button_width = scale_value(BUTTON_WIDTH, scale_factor);
    int scaled_button_height = scale_value(BUTTON_HEIGHT, scale_factor);
    int scaled_spacing = scale_value(10, scale_factor);
    int scaled_bottom_margin = scale_value(50, scale_factor);

    int total_buttons_width = scaled_button_width * 2 + scaled_spacing;
    int buttons_start_x = (panel_width - total_buttons_width) / 2;

    panel->apply_button.rect.x = buttons_start_x;
    panel->apply_button.rect.y = screen_height - scaled_bottom_margin;
    panel->apply_button.rect.w = scaled_button_width;
    panel->apply_button.rect.h = scaled_button_height;

    panel->cancel_button.rect.x = buttons_start_x + scaled_button_width + scaled_spacing;
    panel->cancel_button.rect.y = screen_height - scaled_bottom_margin;
    panel->cancel_button.rect.w = scaled_button_width;
    panel->cancel_button.rect.h = scaled_button_height;

    // Mise à jour du preview (avec panel_ratio)
    const int BASE_PREVIEW_FRAME_X = 50;
    const int BASE_PREVIEW_FRAME_Y = 80;
    const int BASE_PREVIEW_SIZE = 100;

    panel->preview_system.frame_x = (int)(BASE_PREVIEW_FRAME_X * panel_ratio);
    panel->preview_system.frame_y = (int)(BASE_PREVIEW_FRAME_Y * panel_ratio);
    panel->preview_system.container_size = (int)(BASE_PREVIEW_SIZE * panel_ratio);
    panel->preview_system.center_x = panel->preview_system.container_size / 2;
    panel->preview_system.center_y = panel->preview_system.container_size / 2;

    // Redimensionner les hexagones du preview
    if (panel->preview_system.hex_list) {
        HexagoneNode* node = panel->preview_system.hex_list->first;
        int hex_count = 0;

        while (node && node->data) {
            Hexagon* hex = node->data;

            // Repositionner au nouveau centre (relatif)
            hex->center_x = panel->preview_system.center_x;
            hex->center_y = panel->preview_system.center_y;

            // Recalculer les sommets de base avec la nouvelle taille
            recalculer_sommets(hex, panel->preview_system.container_size);
            hex->current_scale = 1.0f;

            hex_count++;
            node = node->next;
        }

        // ═════════════════════════════════════════════════════════════════════════
        // RE-CALCULER TOUTES LES FRAMES ANIMÉES
        // ═════════════════════════════════════════════════════════════════════════
        // ⚠️ IMPORTANT : Les sommets de base (vx[], vy[]) ont changé, donc il faut
        // re-précalculer TOUTES les frames avec rotation + scale pour l'animation !
        // ═════════════════════════════════════════════════════════════════════════
        precompute_all_cycles(panel->preview_system.hex_list, TARGET_FPS,
                              panel->temp_config.breath_duration);

        if (hex_count > 0) {
            debug_printf("✅ %d hexagones du preview redimensionnés et frames recalculées (ratio: %.2f)\n",
                         hex_count, panel_ratio);
        }if (hex_count > 0) {
            debug_printf("✅ %d hexagones du preview redimensionnés et frames recalculées (ratio: %.2f)\n",
                         hex_count, panel_ratio);
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // RESCALE ET POSITIONNEMENT CENTRALISÉ DES WIDGETS
    // ═════════════════════════════════════════════════════════════════════════
    // Utilise la fonction centralisée qui gère tout le scaling et le positionnement
    rescale_and_layout_widgets(panel->widget_list, panel_width, screen_width, screen_height);

    // Barre de séparation

    // ═════════════════════════════════════════════════════════════════════════
    // MISE À JOUR DE LA BARRE DE SÉPARATION
    // ═════════════════════════════════════════════════════════════════════════
    // Position en pourcentage de la hauteur pour rester centrée
    const float SEPARATOR_HEIGHT_RATIO = 0.30f;  // 30% de la hauteur (après le preview)
    const int BASE_SEPARATOR_MARGIN = 20;

    panel->separator_y = (int)(screen_height * SEPARATOR_HEIGHT_RATIO);
    panel->separator_start_x = (int)(BASE_SEPARATOR_MARGIN * panel_ratio);
    panel->separator_end_x = panel_width - (int)(BASE_SEPARATOR_MARGIN * panel_ratio);

    // Recalculer le layout responsive après le resize
    recalculate_widget_layout(panel);
}

// ════════════════════════════════════════════════════════════════════════════
//  RECALCUL DU LAYOUT RESPONSIVE DES WIDGETS
// ════════════════════════════════════════════════════════════════════════════
// Cette fonction repositionne automatiquement les widgets en fonction de la
// largeur disponible:
//   - Mode large (>= threshold): preview à gauche, widgets à droite
//   - Mode étroit (< threshold): preview en haut centré, widgets en dessous centrés
//
// Calcule également la hauteur totale du contenu pour le scroll
// ════════════════════════════════════════════════════════════════════════════

void recalculate_widget_layout(SettingsPanel* panel) {
    if (!panel || !panel->widget_list) return;

    int panel_width = panel->rect.w;
    bool use_column_mode = (panel_width < panel->layout_threshold_width);
    panel->layout_mode_column = use_column_mode;

    // Marges et espacements
    const int MARGIN_TOP = 50;
    const int MARGIN_LEFT = 20;
    const int MARGIN_RIGHT = 20;
    const int WIDGET_SPACING_Y = 30;
    const int SECTION_SPACING_Y = 15;
    const int PREVIEW_MARGIN_BOTTOM = 20;

    int current_y = MARGIN_TOP;
    int center_x = panel_width / 2;
    WidgetNode* node = panel->widget_list->first;

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 1: REPOSITIONNER LES WIDGETS
    // ═══════════════════════════════════════════════════════════════════════════

    if (use_column_mode) {
        // MODE COLONNE (étroit): Tout en vertical, centré
        debug_printf("📱 Layout responsive: MODE COLONNE (largeur=%d)\n", panel_width);

        node = panel->widget_list->first;
        current_y = MARGIN_TOP;

        while (node) {
            switch (node->type) {
                case WIDGET_TYPE_LABEL: {
                    if (node->widget.label_widget) {
                        LabelWidget* w = node->widget.label_widget;
                        // Centrer le titre
                        w->base.x = center_x - (w->base.width / 2);
                        w->base.y = current_y;
                        current_y += w->base.height + SECTION_SPACING_Y;
                    }
                    break;
                }

                case WIDGET_TYPE_SEPARATOR: {
                    if (node->widget.separator_widget) {
                        SeparatorWidget* w = node->widget.separator_widget;
                        w->base.x = MARGIN_LEFT;
                        w->base.y = current_y;
                        w->base.width = panel_width - MARGIN_LEFT - MARGIN_RIGHT;
                        current_y += w->base.height + SECTION_SPACING_Y;
                    }
                    break;
                }

                case WIDGET_TYPE_PREVIEW: {
                    if (node->widget.preview_widget) {
                        PreviewWidget* w = node->widget.preview_widget;
                        // Centrer le preview
                        w->base.x = center_x - (w->base_frame_size / 2);
                        w->base.y = current_y;
                        current_y += w->base_frame_size + PREVIEW_MARGIN_BOTTOM;
                    }
                    break;
                }

                case WIDGET_TYPE_INCREMENT: {
                    if (node->widget.increment_widget) {
                        ConfigWidget* w = node->widget.increment_widget;
                        // Centrer les widgets increment
                        int widget_width = w->local_value_x + 50;
                        w->base.x = center_x - (widget_width / 2);
                        w->base.y = current_y;
                        current_y += 30 + WIDGET_SPACING_Y;
                    }
                    break;
                }

                case WIDGET_TYPE_SELECTOR: {
                    if (node->widget.selector_widget) {
                        SelectorWidget* w = node->widget.selector_widget;
                        // Centrer les selectors
                        w->base.x = center_x - (w->base.width / 2);
                        w->base.y = current_y;
                        current_y += w->base.height + WIDGET_SPACING_Y;
                    }
                    break;
                }

                case WIDGET_TYPE_TOGGLE: {
                    if (node->widget.toggle_widget) {
                        ToggleWidget* w = node->widget.toggle_widget;
                        // Centrer les toggles
                        w->base.x = center_x - (w->base.base_width / 2);
                        w->base.y = current_y;
                        current_y += w->base.base_height + WIDGET_SPACING_Y;
                    }
                    break;
                }

                case WIDGET_TYPE_BUTTON: {
                    if (node->widget.button_widget) {
                        ButtonWidget* w = node->widget.button_widget;
                        // Les boutons gardent leur position relative au bas ou se centrent
                        if (w->y_anchor == BUTTON_ANCHOR_BOTTOM) {
                            // Laisser le y tel quel (relatif au bas)
                            w->base.x = center_x - (w->base_width / 2);
                        } else {
                            w->base.x = center_x - (w->base_width / 2);
                            w->base.y = current_y;
                            current_y += w->base_height + WIDGET_SPACING_Y;
                        }
                    }
                    break;
                }

                default:
                    break;
            }
            node = node->next;
        }

    } else {
        // MODE 2 COLONNES (large): Preview à gauche, widgets à droite
        debug_printf("🖥️  Layout responsive: MODE 2 COLONNES (largeur=%d)\n", panel_width);

        // On garde les positions JSON originales
        // (pas de modification nécessaire, les positions JSON sont déjà bonnes pour ce mode)
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 3: CALCULER LA HAUTEUR TOTALE DU CONTENU ET LE MAX_SCROLL
    // ═══════════════════════════════════════════════════════════════════════════
    int max_y = 0;
    node = panel->widget_list->first;
    while (node) {
        int widget_bottom = 0;

        switch (node->type) {
            case WIDGET_TYPE_LABEL:
                if (node->widget.label_widget) {
                    widget_bottom = node->widget.label_widget->base.y + node->widget.label_widget->base.height;
                }
                break;
            case WIDGET_TYPE_PREVIEW:
                if (node->widget.preview_widget) {
                    widget_bottom = node->widget.preview_widget->base.y + node->widget.preview_widget->base_frame_size;
                }
                break;
            case WIDGET_TYPE_INCREMENT:
                if (node->widget.increment_widget) {
                    widget_bottom = node->widget.increment_widget->base.y + 30;
                }
                break;
            case WIDGET_TYPE_SELECTOR:
                if (node->widget.selector_widget) {
                    widget_bottom = node->widget.selector_widget->base.y + node->widget.selector_widget->base.height;
                }
                break;
            case WIDGET_TYPE_TOGGLE:
                if (node->widget.toggle_widget) {
                    widget_bottom = node->widget.toggle_widget->base.y + node->widget.toggle_widget->base.base_height;
                }
                break;
            case WIDGET_TYPE_SEPARATOR:
                if (node->widget.separator_widget) {
                    widget_bottom = node->widget.separator_widget->base.y + node->widget.separator_widget->base.height;
                }
                break;
            case WIDGET_TYPE_BUTTON:
                if (node->widget.button_widget) {
                    ButtonWidget* w = node->widget.button_widget;
                    // Les boutons avec anchor bottom ne comptent pas dans le content_height
                    if (w->y_anchor == BUTTON_ANCHOR_TOP) {
                        widget_bottom = w->base.y + w->base_height;
                    }
                }
                break;
            default:
                break;
        }

        if (widget_bottom > max_y) {
            max_y = widget_bottom;
        }
        node = node->next;
    }

    // Ajouter une marge en bas
    panel->content_height = max_y + 60;

    // Calculer le scroll maximum
    // Les boutons ancrés au bas prennent 70px
    int available_height = panel->screen_height - 70;
    panel->max_scroll = panel->content_height - available_height;
    if (panel->max_scroll < 0) {
        panel->max_scroll = 0;
    }

    // S'assurer que le scroll actuel ne dépasse pas le max
    if (panel->scroll_offset > panel->max_scroll) {
        panel->scroll_offset = panel->max_scroll;
    }

    debug_printf("📏 Content height: %d, Available height: %d, Max scroll: %d\n",
                 panel->content_height, available_height, panel->max_scroll);
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DU SCROLL (MOLETTE SOURIS)
// ════════════════════════════════════════════════════════════════════════════

void handle_panel_scroll(SettingsPanel* panel, SDL_Event* event) {
    if (!panel || !event) return;
    if (event->type != SDL_MOUSEWHEEL) return;

    // Sensibilité du scroll (pixels par cran de molette)
    const int SCROLL_SPEED = 30;

    // Scroll vers le haut (event->wheel.y > 0) ou vers le bas (event->wheel.y < 0)
    panel->scroll_offset -= event->wheel.y * SCROLL_SPEED;

    // Limiter le scroll
    if (panel->scroll_offset < 0) {
        panel->scroll_offset = 0;
    }
    if (panel->scroll_offset > panel->max_scroll) {
        panel->scroll_offset = panel->max_scroll;
    }

    debug_printf("🖱️ Scroll: offset=%d, max=%d\n", panel->scroll_offset, panel->max_scroll);
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION DE LA MÉMOIRE
// ════════════════════════════════════════════════════════════════════════════

void free_settings_panel(SettingsPanel* panel) {
    if (!panel) return;

    if (panel->preview_system.hex_list) {
        free_hexagone_list(panel->preview_system.hex_list);
    }

    if (panel->widget_list) {
        free_widget_list(panel->widget_list);
    }

    if (panel->background) SDL_DestroyTexture(panel->background);
    if (panel->gear_icon) SDL_DestroyTexture(panel->gear_icon);
    if (panel->apply_button_texture) SDL_DestroyTexture(panel->apply_button_texture);
    if (panel->cancel_button_texture) SDL_DestroyTexture(panel->cancel_button_texture);

    free(panel);
}

// ════════════════════════════════════════════════════════════════════════════
//  INITIALISATION DES WIDGETS EN DUR (VERSION FINALE SANS JSON)
// ════════════════════════════════════════════════════════════════════════════
// ⚠️  FONCTION POUR LA VERSION FINALE DU PROJET
// ⚠️  Cette fonction sera utilisée quand on débranchera le JSON Editor
//
// INSTRUCTIONS POUR BASCULER VERS LA VERSION HARDCODÉE :
// -------------------------------------------------------
// 1. Générer le fichier src/generated_widgets.c via le JSON Editor
//    (Menu contextuel → "Générer Code C")
//
// 2. Copier la fonction init_widgets_from_json() depuis generated_widgets.c
//    et la coller dans ce fichier (settings_panel.c)
//
// 3. Dans init_settings_panel() (ligne ~161) et reload_widgets_from_json() (ligne ~621),
//    remplacer l'appel à charger_widgets_depuis_json() par :
//    init_widgets_from_json(panel->widget_list, panel->font);
//
// 4. Supprimer ou commenter tout le code lié au JSON Editor :
//    - json_config_loader.c / .h
//    - json_editor/ (tout le dossier)
//    - Les includes et appels liés au JSON
//
// 5. Mettre à jour le Makefile pour ne plus compiler ces fichiers
//
// AVANTAGES DE LA VERSION HARDCODÉE :
// - Application plus légère (pas de dépendance cJSON)
// - Chargement instantané (pas de parsing JSON)
// - Moins de code à maintenir en production
// - Parfait pour le build final en fullscreen
//
/*
 init_widgets_hardcoded(SettingsPanel* panel) {
 if (!panel || !panel->widget_list) {
     debug_printf("❌ Panel invalide pour init_widgets_hardcoded\n");
     return;
     }

     // Appeler la fonction générée depuis generated_widgets.c
     // (à copier/coller ici depuis le fichier généré)
     init_widgets_from_json(panel->widget_list, panel->font);

     debug_printf("✅ Widgets initialisés en mode HARDCODÉ (sans JSON)\n");
     }
     */

// ════════════════════════════════════════════════════════════════════════════
//  HOT RELOAD DES WIDGETS DEPUIS LE JSON
// ════════════════════════════════════════════════════════════════════════════
void reload_widgets_from_json(SettingsPanel* panel, int screen_width, int screen_height) {
    if (!panel) return;

    debug_printf("🔄 RECHARGEMENT des widgets depuis JSON...\n");

    // Libérer l'ancienne liste de widgets
    if (panel->widget_list) {
        free_widget_list(panel->widget_list);
        panel->widget_list = NULL;
    }

    // Créer une nouvelle liste
    panel->widget_list = create_widget_list();
    if (!panel->widget_list) {
        debug_printf("❌ Impossible de créer la nouvelle widget_list\n");
        return;
    }

    // Charger depuis le JSON
    LoaderContext ctx = {
        .renderer = panel->renderer,
        .font_titre = panel->font_title,
        .font_normal = panel->font,
        .font_petit = panel->font_small
    };

    if (!charger_widgets_depuis_json(panel->json_config_path, &ctx, panel->widget_list)) {
        debug_printf("❌ Échec rechargement JSON\n");
        return;
    }

    // Mettre à jour le timestamp
    struct stat file_stat;
    if (stat(panel->json_config_path, &file_stat) == 0) {
        panel->last_json_mtime = file_stat.st_mtime;
        debug_printf("📅 JSON timestamp mis à jour: %ld\n", (long)panel->last_json_mtime);
    }

    // Recalculer les positions et dimensions
    update_panel_scale(panel, screen_width, screen_height, panel->scale_factor);

    // Mettre à jour la largeur minimale de fenêtre
    update_window_minimum_size(panel, panel->window);

    debug_printf("✅ Widgets rechargés avec succès\n");
    debug_print_widget_list(panel->widget_list);
}

// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DE LA LARGEUR MINIMALE DE FENÊTRE
// ════════════════════════════════════════════════════════════════════════════
void update_window_minimum_size(SettingsPanel* panel, SDL_Window* window) {
    if (!panel || !window) return;

    int min_width = get_minimum_window_width(panel);
    const int MIN_HEIGHT = 400;

    SDL_SetWindowMinimumSize(window, min_width, MIN_HEIGHT);
    debug_printf("🔄 Taille minimale fenêtre mise à jour: %dx%d\n", min_width, MIN_HEIGHT);
}

// ════════════════════════════════════════════════════════════════════════════
//  VÉRIFICATION PÉRIODIQUE DU FICHIER JSON
// ════════════════════════════════════════════════════════════════════════════
void check_json_hot_reload(SettingsPanel* panel, float delta_time, int screen_width, int screen_height) {
    if (!panel) return;

    // Accumuler le temps
    panel->time_since_last_check += delta_time;

    // Vérifier seulement si l'intervalle est écoulé
    if (panel->time_since_last_check < panel->json_check_interval) {
        return;
    }

    // Réinitialiser le timer
    panel->time_since_last_check = 0.0f;

    // Obtenir le timestamp actuel du fichier
    struct stat file_stat;
    if (stat(panel->json_config_path, &file_stat) != 0) {
        // Fichier non accessible (peut-être supprimé)
        return;
    }

    // Comparer avec le timestamp précédent
    if (file_stat.st_mtime != panel->last_json_mtime) {
        debug_printf("🔥 HOT RELOAD: Fichier JSON modifié détecté!\n");
        reload_widgets_from_json(panel, screen_width, screen_height);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  CALCUL DE LA LARGEUR MINIMALE DE FENÊTRE
// ════════════════════════════════════════════════════════════════════════════
int get_minimum_window_width(SettingsPanel* panel) {
    if (!panel || !panel->widget_list) {
        return 400;  // Valeur par défaut minimale
    }

    // Utiliser la fonction de widget_list.c qui calcule la largeur min du panneau
    int min_panel_width = calculate_min_panel_width(panel->widget_list);

    // La largeur minimale de fenêtre doit être au moins égale à la largeur du panneau
    // + un petit buffer pour éviter les problèmes d'arrondi
    const int BUFFER = 50;
    int min_window_width = min_panel_width + BUFFER;

    // Assurer une largeur minimale absolue (pour éviter des fenêtres trop petites)
    const int ABSOLUTE_MIN = 400;
    if (min_window_width < ABSOLUTE_MIN) {
        min_window_width = ABSOLUTE_MIN;
    }

    debug_printf("📐 Largeur minimale fenêtre calculée: %d px (panneau: %d px)\n",
                 min_window_width, min_panel_width);

    return min_window_width;
}
