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

// Fonction utilitaire pour calculer largeur minimale
static int calculate_required_width_for_json_layout(SettingsPanel* panel);

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
    panel->widgets_stacked = false;       // Initialement, widgets aux positions originales
    panel->panel_width_when_stacked = 0;  // 0 = jamais empilé
    panel->layout_dirty = true;           // Nécessite un recalcul initial

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

    // Calculer la largeur minimale nécessaire pour afficher le layout JSON
    panel->min_width_for_unstack = calculate_required_width_for_json_layout(panel);
    debug_printf("✅ Largeur minimale pour dépiler: %dpx\n", panel->min_width_for_unstack);

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
                // Réinitialiser la mémoire de l'empilement quand le panneau se ferme
                panel->panel_width_when_stacked = 0;
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

    // Vérifier si les boutons entrent en collision (espacement < 20px) ou sortent du panneau
    const int MIN_SPACING = 20;
    // Utiliser la macro BUTTON_MARGIN déjà définie (ligne 15)

    bool buttons_should_stack = (scaled_spacing < MIN_SPACING) ||
                                 (total_buttons_width > panel_width - (2 * BUTTON_MARGIN));

    if (buttons_should_stack) {
        // Empiler verticalement : Appliquer au-dessus, Annuler en dessous
        int button_center_x = (panel_width - scaled_button_width) / 2;
        const int STACK_SPACING = 10;  // Espacement vertical entre boutons empilés

        // Appliquer (au-dessus)
        panel->apply_button.rect.x = button_center_x;
        panel->apply_button.rect.y = screen_height - scaled_bottom_margin - scaled_button_height - STACK_SPACING;
        panel->apply_button.rect.w = scaled_button_width;
        panel->apply_button.rect.h = scaled_button_height;

        // Annuler (en dessous)
        panel->cancel_button.rect.x = button_center_x;
        panel->cancel_button.rect.y = screen_height - scaled_bottom_margin;
        panel->cancel_button.rect.w = scaled_button_width;
        panel->cancel_button.rect.h = scaled_button_height;
    } else {
        // Côte à côte (comportement normal)
        int buttons_start_x = (panel_width - total_buttons_width) / 2;

        panel->apply_button.rect.x = buttons_start_x;
        panel->apply_button.rect.y = screen_height - scaled_bottom_margin;
        panel->apply_button.rect.w = scaled_button_width;
        panel->apply_button.rect.h = scaled_button_height;

        panel->cancel_button.rect.x = buttons_start_x + scaled_button_width + scaled_spacing;
        panel->cancel_button.rect.y = screen_height - scaled_bottom_margin;
        panel->cancel_button.rect.w = scaled_button_width;
        panel->cancel_button.rect.h = scaled_button_height;
    }

    // Mise à jour du preview (avec panel_ratio)
    const int BASE_PREVIEW_FRAME_X = 50;
    const int BASE_PREVIEW_FRAME_Y = 80;
    const int BASE_PREVIEW_SIZE = 100;

    panel->preview_system.frame_x = (int)(BASE_PREVIEW_FRAME_X * panel_ratio);
    panel->preview_system.frame_y = (int)(BASE_PREVIEW_FRAME_Y * panel_ratio);
    panel->preview_system.container_size = (int)(BASE_PREVIEW_SIZE * panel_ratio);
    panel->preview_system.center_x = panel->preview_system.container_size / 2;
    panel->preview_system.center_y = panel->preview_system.container_size / 2;

    // ═════════════════════════════════════════════════════════════════════════
    // POSITIONNEMENT DES BOUTONS DE LA WIDGET LIST (y_anchor)
    // ═════════════════════════════════════════════════════════════════════════
    WidgetNode* node = panel->widget_list->first;
    while (node) {
        if (node->type == WIDGET_TYPE_BUTTON && node->widget.button_widget) {
            ButtonWidget* btn = node->widget.button_widget;

            // Gérer l'ancrage en Y
            if (btn->y_anchor == BUTTON_ANCHOR_BOTTOM) {
                // Position relative au bas du panneau
                btn->base.y = screen_height - btn->base_y - btn->base_height / 2;
            } else {
                // Position relative au haut (comportement par défaut)
                btn->base.y = btn->base_y;
            }
        }
        node = node->next;
    }

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
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // RESCALE ET POSITIONNEMENT CENTRALISÉ DES WIDGETS - SUPPRIMÉ
    // ═════════════════════════════════════════════════════════════════════════
    // L'empilement est géré uniquement par recalculate_widget_layout() plus bas
    // Plus besoin de rescale_and_layout_widgets() qui repositionnait automatiquement
    // Le Selector est initialisé dès le chargement JSON, pas besoin de rescale ici

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

    // Marquer le layout comme nécessitant un recalcul
    panel->layout_dirty = true;

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

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE POUR STOCKER LES RECTANGLES DE COLLISION
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    WidgetNode* node;
    WidgetType type;
    int x, y, width, height;
} WidgetRect;

// ════════════════════════════════════════════════════════════════════════════
//  DÉTECTION DE COLLISION ENTRE DEUX RECTANGLES
// ════════════════════════════════════════════════════════════════════════════
static bool rects_collide(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return !(x1 + w1 <= x2 || x2 + w2 <= x1 || y1 + h1 <= y2 || y2 + h2 <= y1);
}

// ════════════════════════════════════════════════════════════════════════════
//  OBTENIR LE RECTANGLE DE COLLISION D'UN WIDGET
// ════════════════════════════════════════════════════════════════════════════
static bool get_widget_rect(WidgetNode* node, WidgetRect* rect) {
    if (!node || !rect) return false;

    rect->node = node;
    rect->type = node->type;

    switch (node->type) {
        case WIDGET_TYPE_LABEL:
            if (node->widget.label_widget) {
                LabelWidget* w = node->widget.label_widget;
                rect->x = w->base.x;
                rect->y = w->base.y;
                rect->width = w->base.width;
                rect->height = w->base.height;
                return true;
            }
            break;

        case WIDGET_TYPE_PREVIEW:
            if (node->widget.preview_widget) {
                PreviewWidget* w = node->widget.preview_widget;
                rect->x = w->base.x;
                rect->y = w->base.y;
                rect->width = w->base_frame_size;
                rect->height = w->base_frame_size;
                return true;
            }
            break;

        case WIDGET_TYPE_INCREMENT:
            if (node->widget.increment_widget) {
                ConfigWidget* w = node->widget.increment_widget;
                rect->x = w->base.x;
                rect->y = w->base.y;
                rect->width = w->base.width > 0 ? w->base.width : 300;
                rect->height = w->base.height > 0 ? w->base.height : 30;
                return true;
            }
            break;

        case WIDGET_TYPE_SELECTOR:
            if (node->widget.selector_widget) {
                SelectorWidget* w = node->widget.selector_widget;
                rect->x = w->base.x;
                rect->y = w->base.y;
                rect->width = w->base.width;
                rect->height = w->base.height;
                return true;
            }
            break;

        case WIDGET_TYPE_TOGGLE:
            if (node->widget.toggle_widget) {
                ToggleWidget* w = node->widget.toggle_widget;
                rect->x = w->base.x;
                rect->y = w->base.y;
                rect->width = w->base.base_width;
                rect->height = w->base.base_height;
                return true;
            }
            break;

        case WIDGET_TYPE_SEPARATOR:
            if (node->widget.separator_widget) {
                SeparatorWidget* w = node->widget.separator_widget;
                rect->x = w->base.x;
                rect->y = w->base.y;
                rect->width = w->base.width;
                rect->height = w->base.height;
                return true;
            }
            break;

        case WIDGET_TYPE_BUTTON:
            if (node->widget.button_widget) {
                ButtonWidget* w = node->widget.button_widget;
                // Inclure TOUS les boutons dans la détection de collision (y compris BOTTOM)
                // pour qu'ils puissent être empilés en mode réduit
                rect->x = w->base.x;
                rect->y = w->base.y;
                rect->width = w->base.width;
                rect->height = w->base.height;
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

// Calculer la largeur minimale nécessaire basée sur la bbox des widgets aux positions JSON
static int calculate_required_width_for_json_layout(SettingsPanel* panel) {
    if (!panel || !panel->widget_list) return 400;

    int max_right_edge = 0;  // Bord droit le plus à droite
    WidgetNode* node = panel->widget_list->first;

    while (node) {
        int widget_right = 0;

        switch (node->type) {
            case WIDGET_TYPE_INCREMENT:
                if (node->widget.increment_widget) {
                    ConfigWidget* w = node->widget.increment_widget;
                    // Position JSON + largeur
                    widget_right = w->base.base_x + (w->base.width > 0 ? w->base.width : 300);
                }
                break;
            case WIDGET_TYPE_SELECTOR:
                if (node->widget.selector_widget) {
                    SelectorWidget* w = node->widget.selector_widget;
                    widget_right = w->base.base_x + w->base.width;
                }
                break;
            case WIDGET_TYPE_SEPARATOR:
                if (node->widget.separator_widget) {
                    SeparatorWidget* w = node->widget.separator_widget;
                    // Séparateur: start_margin + largeur définie dans JSON
                    widget_right = w->base_start_margin + w->base_width;
                }
                break;
            case WIDGET_TYPE_LABEL:
                if (node->widget.label_widget) {
                    LabelWidget* w = node->widget.label_widget;
                    widget_right = w->base.base_x + w->base.width;
                }
                break;
            case WIDGET_TYPE_PREVIEW:
                if (node->widget.preview_widget) {
                    PreviewWidget* w = node->widget.preview_widget;
                    widget_right = w->base.base_x + w->base_frame_size;
                }
                break;
            default:
                break;
        }

        if (widget_right > max_right_edge) {
            max_right_edge = widget_right;
        }

        node = node->next;
    }

    // Ajouter une petite marge de sécurité (20px)
    int required_width = max_right_edge + 20;

    debug_printf("📐 Largeur minimale calculée pour layout JSON: %dpx\n", required_width);
    return required_width;
}

// ═══════════════════════════════════════════════════════════════════════════
// FONCTIONS HELPER POUR LE LAYOUT
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Restaure les positions JSON originales de tous les widgets (dépilement)
 */
static void restore_json_positions(SettingsPanel* panel) {
    if (!panel || !panel->widget_list) return;

    int panel_width = panel->rect.w;
    WidgetNode* node = panel->widget_list->first;

    debug_printf("📍 Restauration des positions JSON...\n");

    while (node) {
        switch (node->type) {
            case WIDGET_TYPE_LABEL:
                if (node->widget.label_widget) {
                    LabelWidget* w = node->widget.label_widget;
                    w->base.y = w->base.base_y;
                    // Appliquer l'alignement selon le JSON
                    switch (w->alignment) {
                        case LABEL_ALIGN_LEFT:
                            w->base.x = w->base.base_x;
                            break;
                        case LABEL_ALIGN_CENTER:
                            w->base.x = (panel_width - w->base.width) / 2;
                            break;
                        case LABEL_ALIGN_RIGHT:
                            w->base.x = panel_width - w->base.width - 20;
                            break;
                    }
                }
                break;

            case WIDGET_TYPE_PREVIEW:
                if (node->widget.preview_widget) {
                    PreviewWidget* w = node->widget.preview_widget;
                    w->base.x = w->base.base_x;
                    w->base.y = w->base.base_y;
                }
                break;

            case WIDGET_TYPE_INCREMENT:
                if (node->widget.increment_widget) {
                    ConfigWidget* w = node->widget.increment_widget;
                    w->base.x = w->base.base_x;
                    w->base.y = w->base.base_y;
                }
                break;

            case WIDGET_TYPE_SELECTOR:
                if (node->widget.selector_widget) {
                    SelectorWidget* w = node->widget.selector_widget;
                    w->base.x = w->base.base_x;
                    w->base.y = w->base.base_y;
                }
                break;

            case WIDGET_TYPE_TOGGLE:
                if (node->widget.toggle_widget) {
                    ToggleWidget* w = node->widget.toggle_widget;
                    w->base.x = w->base.base_x;
                    w->base.y = w->base.base_y;
                }
                break;

            case WIDGET_TYPE_SEPARATOR:
                if (node->widget.separator_widget) {
                    SeparatorWidget* w = node->widget.separator_widget;
                    w->base.y = w->base.base_y;
                    w->base.x = w->base_start_margin;
                    w->base.width = panel_width - w->base_start_margin - w->base_end_margin;
                }
                break;

            case WIDGET_TYPE_BUTTON:
                if (node->widget.button_widget) {
                    ButtonWidget* w = node->widget.button_widget;
                    w->base.x = w->base_x;  // Position JSON de base
                    // Gérer l'ancrage vertical des boutons
                    if (w->y_anchor == BUTTON_ANCHOR_BOTTOM) {
                        w->base.y = panel->screen_height - w->base_y - w->base_height / 2;
                    } else {
                        w->base.y = w->base_y;  // Position JSON de base
                    }
                }
                break;

            default:
                break;
        }
        node = node->next;
    }
}

/**
 * Empile les widgets verticalement pour éviter les collisions (empilement)
 * Utilise la liste de rectangles déjà calculée pour déterminer l'ordre
 *
 * LOGIQUE SPÉCIALE POUR LES SÉPARATEURS :
 * - Si widget au-dessus = LABEL (titre) → garder position Y fixe (base_y)
 * - Si widget au-dessus = widget callback → empiler sous le widget (current_y + 5px)
 */
static void stack_widgets_vertically(SettingsPanel* panel, WidgetRect* rects, int rect_count) {
    if (!panel || !rects) return;

    const int COLLISION_SPACING = 10;
    const int SEPARATOR_EXTRA_SPACING = 5;  // Espacement supplémentaire après widgets callback
    int panel_width = panel->rect.w;
    int center_x = panel_width / 2;
    int current_y = 50;  // Marge du haut

    debug_printf("🔧 Empilement vertical des widgets (avec centrage)...\n");

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAPE 1: Trouver le container_width maximum des widgets INCREMENT
    // ─────────────────────────────────────────────────────────────────────────
    int max_increment_width = 0;
    for (int i = 0; i < rect_count; i++) {
        if (rects[i].type == WIDGET_TYPE_INCREMENT) {
            if (rects[i].width > max_increment_width) {
                max_increment_width = rects[i].width;
            }
        }
    }

    // Calculer la position de départ pour centrer les INCREMENT
    int increment_start_x = (panel_width - max_increment_width) / 2;
    debug_printf("   📐 Max INCREMENT width=%d, centré à x=%d\n",
                 max_increment_width, increment_start_x);

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAPE 2: Empiler les widgets
    // ─────────────────────────────────────────────────────────────────────────
    for (int i = 0; i < rect_count; i++) {
        WidgetRect* r = &rects[i];

        switch (r->type) {
            case WIDGET_TYPE_LABEL:
                if (r->node->widget.label_widget) {
                    LabelWidget* w = r->node->widget.label_widget;
                    // Labels: appliquer alignement selon JSON, Y fixe
                    switch (w->alignment) {
                        case LABEL_ALIGN_LEFT:
                            w->base.x = 20;  // Marge gauche
                            break;
                        case LABEL_ALIGN_CENTER:
                            w->base.x = center_x - (w->base.width / 2);
                            break;
                        case LABEL_ALIGN_RIGHT:
                            w->base.x = panel_width - w->base.width - 20;
                            break;
                    }
                }
                break;

            case WIDGET_TYPE_PREVIEW:
                if (r->node->widget.preview_widget) {
                    PreviewWidget* w = r->node->widget.preview_widget;
                    // Centrer le preview, garder Y fixe
                    w->base.x = center_x - (w->base_frame_size / 2);
                    // Avancer current_y pour widgets suivants
                    current_y = w->base.y + w->base_frame_size + COLLISION_SPACING;
                    debug_printf("   📦 PREVIEW centré (x=%d)\n", w->base.x);
                }
                break;

            case WIDGET_TYPE_INCREMENT:
                if (r->node->widget.increment_widget) {
                    ConfigWidget* w = r->node->widget.increment_widget;
                    // Centrer l'INCREMENT (avec justification gardée grâce au container_width)
                    w->base.x = increment_start_x;
                    w->base.y = current_y;
                    debug_printf("   🔢 INCREMENT '%s' centré (x=%d, y=%d)\n",
                                w->option_name, w->base.x, w->base.y);
                    current_y += r->height + COLLISION_SPACING;
                }
                break;

            case WIDGET_TYPE_TOGGLE:
                if (r->node->widget.toggle_widget) {
                    ToggleWidget* w = r->node->widget.toggle_widget;
                    // Aligner le toggle à DROITE avec les INCREMENT
                    // Bord droit du toggle = bord droit des increment
                    int toggle_width = w->base.base_width;  // Largeur totale (texte + switch)
                    w->base.x = increment_start_x + max_increment_width - toggle_width;
                    w->base.y = current_y;
                    debug_printf("   🎚️  TOGGLE aligné à droite (x=%d, y=%d, right=%d)\n",
                                w->base.x, w->base.y, w->base.x + toggle_width);
                    current_y += r->height + COLLISION_SPACING;
                }
                break;

            case WIDGET_TYPE_SELECTOR:
                if (r->node->widget.selector_widget) {
                    SelectorWidget* w = r->node->widget.selector_widget;
                    // Centrer le SELECTOR
                    w->base.x = increment_start_x;
                    w->base.y = current_y;
                    debug_printf("   📋 SELECTOR centré (x=%d, y=%d)\n", w->base.x, w->base.y);
                    current_y += r->height + COLLISION_SPACING + 10;  // +10 pour callbacks
                }
                break;

            case WIDGET_TYPE_SEPARATOR:
                if (r->node->widget.separator_widget) {
                    SeparatorWidget* sep_w = r->node->widget.separator_widget;

                    // ═════════════════════════════════════════════════════════════
                    // TROUVER LE WIDGET AU-DESSUS DANS LA LISTE (pas en Y)
                    // ═════════════════════════════════════════════════════════════
                    // Parcourir vers le haut dans la liste (i-1, i-2, i-3...)
                    // jusqu'à trouver un widget qui n'est PAS un séparateur

                    WidgetType widget_above_type = WIDGET_TYPE_LABEL;  // Par défaut
                    int widget_above_index = -1;

                    // Remonter dans la liste jusqu'à trouver un widget non-separator
                    for (int j = i - 1; j >= 0; j--) {
                        if (rects[j].type != WIDGET_TYPE_SEPARATOR) {
                            widget_above_type = rects[j].type;
                            widget_above_index = j;
                            break;
                        }
                    }

                    debug_printf("   🔎 Séparateur [%d] → widget au-dessus dans liste = [%d] type=%d\n",
                                i, widget_above_index, widget_above_type);

                    // ═════════════════════════════════════════════════════════════
                    // LOGIQUE SÉPARATEUR SELON WIDGET AU-DESSUS (dans la liste)
                    // ═════════════════════════════════════════════════════════════

                    if (widget_above_type == WIDGET_TYPE_LABEL) {
                        // Widget au-dessus = LABEL (titre) → Position Y fixe
                        // Exemple : séparateur "Sessions"
                        // Ne PAS modifier sep_w->base.y, garder position JSON
                        debug_printf("   📏 Séparateur après LABEL → Y fixe (base_y=%d)\n",
                                    sep_w->base.base_y);
                    } else {
                        // Widget au-dessus = widget callback → Empiler juste en-dessous
                        current_y += SEPARATOR_EXTRA_SPACING;
                        sep_w->base.y = current_y;
                        debug_printf("   📏 Séparateur après widget callback type=%d → Y=%d (+%dpx)\n",
                                    widget_above_type, current_y, SEPARATOR_EXTRA_SPACING);
                        current_y += r->height + COLLISION_SPACING;
                    }

                    // Position X centrée, largeur adaptée
                    sep_w->base.x = increment_start_x;
                    sep_w->base.width = max_increment_width;
                    debug_printf("   ➖ SEPARATOR centré (x=%d, w=%d)\n",
                                sep_w->base.x, sep_w->base.width);
                }
                break;

            case WIDGET_TYPE_BUTTON:
                if (r->node->widget.button_widget) {
                    ButtonWidget* w = r->node->widget.button_widget;
                    // Centrer le BUTTON
                    w->base.x = center_x - (w->base.width / 2);
                    w->base.y = current_y;
                    debug_printf("   🔘 BUTTON '%s' centré (x=%d, y=%d)\n",
                                w->text, w->base.x, w->base.y);
                    current_y += r->height + COLLISION_SPACING;
                }
                break;

            default:
                break;
        }
    }
}

void recalculate_widget_layout(SettingsPanel* panel) {
    if (!panel || !panel->widget_list) return;

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 0: VÉRIFIER SI RECALCUL NÉCESSAIRE
    // ═══════════════════════════════════════════════════════════════════════════
    // Le flag layout_dirty évite les recalculs multiples par frame
    // Il est positionné à true lors des resize ou autres changements de layout
    // ═══════════════════════════════════════════════════════════════════════════
    if (!panel->layout_dirty) {
        return;  // Déjà à jour, pas besoin de recalculer
    }

    debug_printf("\n🔄 === RECALCULATE_WIDGET_LAYOUT (layout_dirty=true) ===\n");

    const int UNSTACK_MARGIN = 80;     // Marge d'hystérésis pour éviter oscillations
    int panel_width = panel->rect.w;
    WidgetNode* node;

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 1: DÉCISION DE DÉPILEMENT AVEC MÉMOIRE PERSISTANTE
    // ═══════════════════════════════════════════════════════════════════════════
    // Si widgets empilés ET largeur suffisante → dépiler
    // Condition: panel_width >= panel_width_when_stacked + UNSTACK_MARGIN
    // La marge évite les oscillations pile/dépile dues au scaling
    // ═══════════════════════════════════════════════════════════════════════════
    if (panel->widgets_stacked &&
        panel->panel_width_when_stacked > 0 &&
        panel_width >= panel->panel_width_when_stacked + UNSTACK_MARGIN) {

        debug_printf("🔄 DÉPILEMENT: panel_width=%dpx >= (saved_width=%dpx + marge=%dpx)\n",
                    panel_width, panel->panel_width_when_stacked, UNSTACK_MARGIN);

        // Restaurer positions JSON originales (helper function)
        restore_json_positions(panel);

        // ───────────────────────────────────────────────────────────────────────
        // Repositionner les UIButton (apply_button, cancel_button)
        // ───────────────────────────────────────────────────────────────────────
        int scaled_button_width = scale_value(BUTTON_WIDTH, panel->scale_factor);
        int scaled_button_height = scale_value(BUTTON_HEIGHT, panel->scale_factor);
        int scaled_spacing = scale_value(10, panel->scale_factor);
        int scaled_bottom_margin = scale_value(50, panel->scale_factor);

        int total_buttons_width = scaled_button_width * 2 + scaled_spacing;
        const int MIN_SPACING = 20;

        bool buttons_should_stack = (scaled_spacing < MIN_SPACING) ||
                                     (total_buttons_width > panel_width - (2 * BUTTON_MARGIN));

        if (buttons_should_stack) {
            // Empiler verticalement
            int button_center_x = (panel_width - scaled_button_width) / 2;
            const int STACK_SPACING = 10;

            panel->apply_button.rect.x = button_center_x;
            panel->apply_button.rect.y = panel->screen_height - scaled_bottom_margin - scaled_button_height - STACK_SPACING;
            panel->apply_button.rect.w = scaled_button_width;
            panel->apply_button.rect.h = scaled_button_height;

            panel->cancel_button.rect.x = button_center_x;
            panel->cancel_button.rect.y = panel->screen_height - scaled_bottom_margin;
            panel->cancel_button.rect.w = scaled_button_width;
            panel->cancel_button.rect.h = scaled_button_height;

            debug_printf("   🔘 UIButton empilés verticalement (x=%d)\n", button_center_x);
        } else {
            // Côte à côte (comportement normal)
            int buttons_start_x = (panel_width - total_buttons_width) / 2;

            panel->apply_button.rect.x = buttons_start_x;
            panel->apply_button.rect.y = panel->screen_height - scaled_bottom_margin;
            panel->apply_button.rect.w = scaled_button_width;
            panel->apply_button.rect.h = scaled_button_height;

            panel->cancel_button.rect.x = buttons_start_x + scaled_button_width + scaled_spacing;
            panel->cancel_button.rect.y = panel->screen_height - scaled_bottom_margin;
            panel->cancel_button.rect.w = scaled_button_width;
            panel->cancel_button.rect.h = scaled_button_height;

            debug_printf("   🔘 UIButton côte à côte (apply_x=%d, cancel_x=%d)\n",
                        buttons_start_x, buttons_start_x + scaled_button_width + scaled_spacing);
        }

        // Marquer comme dépilé
        panel->widgets_stacked = false;

        debug_printf("✅ Widgets dépilés - positions JSON restaurées\n");
        debug_printf("   📌 panel_width_when_stacked=%dpx (gardé en mémoire)\n",
                    panel->panel_width_when_stacked);

        // Recalculer les hauteurs et terminer
        goto calculate_heights;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 0bis: CALCULER LES GROUPES ET LARGEURS DES WIDGETS INCREMENT
    // ═══════════════════════════════════════════════════════════════════════════
    // Pour la détection de collision, on doit utiliser la largeur incluant l'alignement

    const int GROUP_SPACING_THRESHOLD = 30;  // Espacement dans JSON

    typedef struct {
        ConfigWidget* widget;
        int y_position;
        int text_width;
        int group_id;
        int container_width_for_group;
    } IncrementLayoutInfo;

    IncrementLayoutInfo increment_infos[50];
    int increment_count = 0;

    // Collecter les widgets INCREMENT
    node = panel->widget_list->first;
    while (node && increment_count < 50) {
        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            ConfigWidget* w = node->widget.increment_widget;
            TTF_Font* font = get_font_for_size(w->current_text_size);
            int text_width = 0;
            if (font) {
                TTF_SizeUTF8(font, w->option_name, &text_width, NULL);
            }

            increment_infos[increment_count].widget = w;
            increment_infos[increment_count].y_position = w->base.y;
            increment_infos[increment_count].text_width = text_width;
            increment_infos[increment_count].group_id = -1;
            increment_infos[increment_count].container_width_for_group = 0;
            increment_count++;
        }
        node = node->next;
    }

    // Trier par position Y
    for (int i = 0; i < increment_count - 1; i++) {
        for (int j = i + 1; j < increment_count; j++) {
            if (increment_infos[j].y_position < increment_infos[i].y_position) {
                IncrementLayoutInfo temp = increment_infos[i];
                increment_infos[i] = increment_infos[j];
                increment_infos[j] = temp;
            }
        }
    }

    // Regrouper par proximité verticale
    int current_group = 0;
    for (int i = 0; i < increment_count; i++) {
        if (increment_infos[i].group_id == -1) {
            increment_infos[i].group_id = current_group;
            int last_y = increment_infos[i].y_position;

            for (int j = i + 1; j < increment_count; j++) {
                int y_diff = increment_infos[j].y_position - last_y;
                if (y_diff > 0 && y_diff <= GROUP_SPACING_THRESHOLD &&
                    increment_infos[j].group_id == -1) {
                    increment_infos[j].group_id = current_group;
                    last_y = increment_infos[j].y_position;
                }
            }
            current_group++;
        }
    }

    // Calculer container_width pour chaque groupe
    for (int g = 0; g < current_group; g++) {
        int max_text_width = 0;
        ConfigWidget* longest_widget = NULL;

        for (int i = 0; i < increment_count; i++) {
            if (increment_infos[i].group_id == g && increment_infos[i].text_width > max_text_width) {
                max_text_width = increment_infos[i].text_width;
                longest_widget = increment_infos[i].widget;
            }
        }

        int container_width = 0;
        if (longest_widget) {
            container_width = longest_widget->local_arrows_x + longest_widget->arrow_size + 60;
        }

        for (int i = 0; i < increment_count; i++) {
            if (increment_infos[i].group_id == g) {
                increment_infos[i].container_width_for_group = container_width;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 1: CONSTRUIRE LA LISTE DES RECTANGLES DE COLLISION
    // ═══════════════════════════════════════════════════════════════════════════
    WidgetRect rects[50];  // Maximum 50 widgets
    int rect_count = 0;

    node = panel->widget_list->first;
    while (node && rect_count < 50) {
        // Pour les widgets INCREMENT, utiliser le container_width calculé
        if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
            ConfigWidget* w = node->widget.increment_widget;
            rects[rect_count].node = node;
            rects[rect_count].type = node->type;
            rects[rect_count].x = w->base.x;
            rects[rect_count].y = w->base.y;

            // Trouver le container_width pour ce widget
            int container_width = w->base.width;  // Défaut
            for (int i = 0; i < increment_count; i++) {
                if (increment_infos[i].widget == w) {
                    container_width = increment_infos[i].container_width_for_group;
                    break;
                }
            }

            rects[rect_count].width = container_width;
            rects[rect_count].height = w->base.height > 0 ? w->base.height : 30;
            rect_count++;
        } else {
            // Autres widgets: utiliser get_widget_rect normal
            if (get_widget_rect(node, &rects[rect_count])) {
                rect_count++;
            }
        }
        node = node->next;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 2: DÉTERMINER SI ON DOIT RÉORGANISER
    // ═══════════════════════════════════════════════════════════════════════════
    // Critère 1: Largeur de la fenêtre (si trop étroit, forcer l'empilement)
    // Critère 2: Détection de collision (si collision, réorganiser)

    bool needs_reorganization = false;

    // Vérifier si le panneau est trop étroit
    if (panel_width < panel->layout_threshold_width) {
        debug_printf("📱 Panneau étroit (%dpx < %dpx) - empilement forcé\n",
                     panel_width, panel->layout_threshold_width);
        needs_reorganization = true;
    } else {
        // Panneau large: vérifier les collisions
        for (int i = 0; i < rect_count && !needs_reorganization; i++) {
            for (int j = i + 1; j < rect_count; j++) {
                if (rects_collide(rects[i].x, rects[i].y, rects[i].width, rects[i].height,
                                rects[j].x, rects[j].y, rects[j].width, rects[j].height)) {
                    debug_printf("⚠️ COLLISION entre widget[%d] (type=%d, x=%d, y=%d, w=%d, h=%d) "
                                "et widget[%d] (type=%d, x=%d, y=%d, w=%d, h=%d)\n",
                                i, rects[i].type, rects[i].x, rects[i].y, rects[i].width, rects[i].height,
                                j, rects[j].type, rects[j].x, rects[j].y, rects[j].width, rects[j].height);
                    needs_reorganization = true;
                    break;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 3: RÉORGANISER SI NÉCESSAIRE (empilement)
    // ═══════════════════════════════════════════════════════════════════════════
    // Si needs_reorganization=true, empiler. Sinon, garder les positions restaurées.
    // ═══════════════════════════════════════════════════════════════════════════
    if (needs_reorganization) {
        debug_printf("🔧 EMPILEMENT: collisions détectées ou panneau trop étroit\n");

        // Marquer que les widgets sont maintenant empilés
        panel->widgets_stacked = true;

        // ═════════════════════════════════════════════════════════════════════════
        // SAUVEGARDER LA LARGEUR UNE SEULE FOIS (FLAG)
        // ═════════════════════════════════════════════════════════════════════════
        // Si panel_width_when_stacked == 0, c'est le PREMIER empilement
        // → Sauvegarder la largeur actuelle comme référence
        // Sinon, c'est un ré-empilement après dépilement → garder l'ancienne valeur
        // ═════════════════════════════════════════════════════════════════════════
        if (panel->panel_width_when_stacked == 0) {
            panel->panel_width_when_stacked = panel_width;
            debug_printf("   💾 SAUVEGARDE panel_width_when_stacked=%dpx (PREMIER empilement)\n",
                        panel->panel_width_when_stacked);
        } else {
            debug_printf("   ♻️  panel_width_when_stacked=%dpx déjà sauvegardé (ré-empilement)\n",
                        panel->panel_width_when_stacked);
        }

        // Empiler les widgets verticalement (helper function)
        stack_widgets_vertically(panel, rects, rect_count);

    } else {
        debug_printf("✅ Aucune collision détectée - conserver positions actuelles\n");
        // Note: Ne pas modifier widgets_stacked ici!
        // Si widgets_stacked = true, on le garde car le dépilement n'a pas eu lieu
        // Ne passer à widgets_stacked = false QUE lors du dépilement explicite
    }

calculate_heights:
    ;  // Statement vide nécessaire pour le label
    // ═══════════════════════════════════════════════════════════════════════════
    // ÉTAPE 4: CALCULER LA HAUTEUR TOTALE DU CONTENU ET LE MAX_SCROLL
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
    int available_height = panel->screen_height - 70;
    panel->max_scroll = panel->content_height - available_height;
    if (panel->max_scroll < 0) {
        panel->max_scroll = 0;
    }

    // S'assurer que le scroll actuel ne dépasse pas le max
    if (panel->scroll_offset > panel->max_scroll) {
        panel->scroll_offset = panel->max_scroll;
    }

    // Marquer le layout comme à jour
    panel->layout_dirty = false;
    debug_printf("✅ Recalcul terminé - layout_dirty=false\n\n");
}
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
    const int ABSOLUTE_MIN = 200;
    if (min_window_width < ABSOLUTE_MIN) {
        min_window_width = ABSOLUTE_MIN;
    }

    debug_printf("📐 Largeur minimale fenêtre calculée: %d px (panneau: %d px)\n",
                 min_window_width, min_panel_width);

    return min_window_width;
}
