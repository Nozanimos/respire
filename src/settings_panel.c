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

// Forward declarations (évite include circulaire avec renderer.h)
extern float calculate_scale_factor(int width, int height);
extern int scale_value(int value, float scale);
extern int calculate_panel_width(int screen_width, float scale);

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACKS POUR LES WIDGETS
// ════════════════════════════════════════════════════════════════════════════
static SettingsPanel* current_panel_for_callbacks = NULL;

void duration_value_changed(int new_value) {
    if (!current_panel_for_callbacks) return;
    current_panel_for_callbacks->temp_config.breath_duration = new_value;
    debug_printf("🔄 Durée respiration changée: %d secondes\n", new_value);
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
}

// ════════════════════════════════════════════════════════════════════════════
//  FONCTIONS DE PRÉVISUALISATION
// ════════════════════════════════════════════════════════════════════════════

void reinitialiser_preview_system(PreviewSystem* preview) {
    if (!preview) return;
    preview->center_x = 50;
    preview->center_y = 50;
    preview->container_size = 100;
    preview->size_ratio = 0.70f;
    debug_printf("🔄 Paramètres preview réinitialisés - Centre: (%d,%d), Container: %d, Ratio: %.2f\n",
                 preview->center_x, preview->center_y, preview->container_size, preview->size_ratio);
}

void init_preview_system(SettingsPanel* panel, int x, int y, int size, float ratio) {
    panel->preview_system.frame_x = x;
    panel->preview_system.frame_y = y;
    panel->preview_system.center_x = size/2;
    panel->preview_system.center_y = size/2;
    panel->preview_system.container_size = size;
    panel->preview_system.size_ratio = ratio;
    panel->preview_system.last_update = SDL_GetTicks();
    panel->preview_system.current_time = 0.0;
    panel->preview_system.hex_list = NULL;

    panel->preview_system.hex_list = create_all_hexagones(
        panel->preview_system.center_x,
        panel->preview_system.center_y,
        panel->preview_system.container_size,
        panel->preview_system.size_ratio
    );

    if (!panel->preview_system.hex_list || !panel->preview_system.hex_list->first || !panel->preview_system.hex_list->first->data) {
        debug_printf("❌ ERREUR: Impossible de créer les hexagones de prévisualisation\n");
    }
}

void update_preview_animation(SettingsPanel* panel) {
    if (!panel || !panel->preview_system.hex_list) return;

    PreviewSystem* preview = &panel->preview_system;
    Uint32 current_ticks = SDL_GetTicks();
    float delta = (current_ticks - preview->last_update) / 1000.0f;
    preview->last_update = current_ticks;
    preview->current_time += delta;

    // Avancer d'une frame dans le précalcul
    HexagoneNode* node = preview->hex_list->first;
    while (node) {
        apply_precomputed_frame(node);
        node = node->next;
    }
}

void update_preview_for_new_duration(SettingsPanel* panel, float new_duration) {
    if (!panel) return;

    PreviewSystem* preview = &panel->preview_system;

    if (preview->hex_list) {
        free_hexagone_list(preview->hex_list);
        preview->hex_list = NULL;
    }

    // Recalculer les centres RELATIFS
    preview->center_x = preview->container_size / 2;
    preview->center_y = preview->container_size / 2;

    debug_printf("🔄 Recréation hexagones - Container: %d, Centre: (%d,%d), Ratio: %.2f\n",
                 preview->container_size, preview->center_x, preview->center_y,
                 preview->size_ratio);

    // Recréer les hexagones
    preview->hex_list = create_all_hexagones(
        preview->center_x,
        preview->center_y,
        preview->container_size,
        preview->size_ratio
    );

    if (!preview->hex_list) {
        debug_printf("❌ ERREUR: Impossible de recréer les hexagones\n");
        return;
    }

    // Re-précalculer les cycles avec la nouvelle durée
    precompute_all_cycles(preview->hex_list, TARGET_FPS, new_duration);

    // Réinitialiser le temps
    preview->current_time = 0.0;
    preview->last_update = SDL_GetTicks();

    debug_printf("✅ Prévisualisation COMPLÈTEMENT réinitialisée avec nouvelle durée\n");
}

void render_preview(SDL_Renderer* renderer, PreviewSystem* preview, int offset_x, int offset_y) {
    if (!preview || !preview->hex_list) return;

    HexagoneNode* node = preview->hex_list->first;
    while (node && node->data) {
        Hexagon* hex = node->data;

        // Positionner l'hexagone en coordonnées ABSOLUES (offset + relative)
        int abs_x = offset_x + preview->frame_x + preview->center_x;
        int abs_y = offset_y + preview->frame_y + preview->center_y;
        transform_hexagon(hex, abs_x, abs_y, hex->current_scale);

        make_hexagone(renderer, hex);

        // Restaurer la position relative (importante pour les prochains rendus)
        transform_hexagon(hex, preview->center_x, preview->center_y, 1.0f);

        node = node->next;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION DU PANNEAU
// ════════════════════════════════════════════════════════════════════════════

SettingsPanel* create_settings_panel(SDL_Renderer* renderer, int screen_width, int screen_height, float scale_factor) {
    SettingsPanel* panel = malloc(sizeof(SettingsPanel));
    if (!panel) return NULL;

    memset(panel, 0, sizeof(SettingsPanel));
    panel->scale_factor = scale_factor;
    panel->state = PANEL_CLOSED;

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

    const char* json_path = "../config/widgets_config.json";
    if (!charger_widgets_depuis_json(json_path, &ctx, panel->widget_list)) {
        debug_printf("⚠️ Échec chargement JSON, utilisation config par défaut\n");

        /*// FALLBACK : widgets hardcodés (avec scaling de base)
        int panel_width_scaled = calculate_panel_width(screen_width, scale_factor);
        int largeur_max_widget = scale_value(180, scale_factor) +
        scale_value(20, scale_factor) +
        scale_value(40, scale_factor) +
        scale_value(20, scale_factor);
        //int widget_x = (panel_width_scaled - largeur_max_widget) / 2;*/

        // ════════════════════════════════════════════════════════════════════════
        // ⚠️  WIDGETS DÉSACTIVÉS - Géré par JSON
        // ════════════════════════════════════════════════════════════════════════
        // Ce widget est maintenant chargé depuis widgets_config.json
        // Si tu veux revenir au hardcodé, décommente les lignes ci-dessous :
        /*
         *       add_increment_widget(panel->widget_list, "breath_duration", "Durée respiration",
         *                            widget_x, scale_value(240, scale_factor), 1, 10, 3, 1,
         *                            scale_value(6, scale_factor), scale_value(18, scale_factor),
         *                            panel->font, duration_value_changed);
         */

       /* add_increment_widget(panel->widget_list, "breath_cycles", "Cycles",
                             widget_x, scale_value(320, scale_factor), 1, 20, 1, 1,
                             scale_value(6, scale_factor), scale_value(18, scale_factor),
                             panel->font, cycles_value_changed);

        add_toggle_widget(panel->widget_list, "alternate_cycles", "Cycles alternés",
                          widget_x, scale_value(400, scale_factor), false,
                          scale_value(40, scale_factor), scale_value(18, scale_factor),
                          scale_value(18, scale_factor), scale_value(18, scale_factor),
                          alternate_cycles_changed);*/
    }

    debug_print_widget_list(panel->widget_list);

    // ════════════════════════════════════════════════════════════════════════
    // CRÉATION DES BOUTONS
    // ════════════════════════════════════════════════════════════════════════
    int scaled_button_width = scale_value(BUTTON_WIDTH, scale_factor);
    int scaled_button_height = scale_value(BUTTON_HEIGHT, scale_factor);

    panel->apply_button = create_button("Appliquer", 0, 0,
                                        scaled_button_width, scaled_button_height);
    panel->cancel_button = create_button("Annuler", 0, 0,
                                         scaled_button_width, scaled_button_height);

    debug_printf("📏 Boutons créés - Largeur: %d, Hauteur: %d\n",
                 scaled_button_width, scaled_button_height);

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
    // INITIALISATION DU SYSTÈME DE PRÉVISUALISATION
    // ════════════════════════════════════════════════════════════════════════
    init_preview_system(panel, 50, 80, 100, 0.90f);

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
        update_preview_animation(panel);
        update_widget_list_animations(panel->widget_list, delta_time);
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

        /*// Titre
        TTF_SetFontStyle(panel->font_title, TTF_STYLE_UNDERLINE);
        render_text(renderer, panel->font_title, "Configuration",
                    panel_x + scale_value(50, panel->scale_factor),
                    panel_y + scale_value(10, panel->scale_factor),
                    0xFF000000);
        TTF_SetFontStyle(panel->font_title, TTF_STYLE_NORMAL);*/

        // Cadre du preview
        int frame_x1 = panel_x + panel->preview_system.frame_x;
        int frame_y1 = panel_y + panel->preview_system.frame_y;
        int frame_x2 = frame_x1 + panel->preview_system.container_size;
        int frame_y2 = frame_y1 + panel->preview_system.container_size;
        rectangleColor(renderer, frame_x1, frame_y1, frame_x2, frame_y2, 0xFFFFFFFF);

        // Hexagone de prévisualisation
        render_preview(renderer, &panel->preview_system, panel_x, panel_y);

        /*// ═════════════════════════════════════════════════════════════════════════
        // BARRE DE SÉPARATION
        // ═════════════════════════════════════════════════════════════════════════
        // On la dessine tant que le panneau a au moins 80px de large
        // (assez d'espace pour voir la barre avec ses marges)
        if (panel->rect.w >= 80) {
            int bar_width = panel->separator_end_x - panel->separator_start_x;

            // Vérifier que les coordonnées sont cohérentes
            if (bar_width > 0 && panel->separator_start_x >= 0) {
                // Épaisseur de 1px (ou scalée selon panel_ratio pour rester visible)
                int thickness = (int)(panel->panel_ratio + 0.5f);
                if (thickness < 1) thickness = 1;


                // Ligne noire de la barre par-dessus
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                for (int i = 0; i < thickness; i++) {
                    SDL_RenderDrawLine(renderer,
                                       panel_x + panel->separator_start_x,
                                       panel_y + panel->separator_y + i,
                                       panel_x + panel->separator_end_x,
                                       panel_y + panel->separator_y + i);
                }
            }
        }*/

        // Widgets
        render_all_widgets(renderer, panel->widget_list, panel_x, panel_y);

        // Boutons
        render_button(renderer, &panel->apply_button, panel->font, panel_x, panel_y);
        render_button(renderer, &panel->cancel_button, panel->font, panel_x, panel_y);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════

void handle_settings_panel_event(SettingsPanel* panel, SDL_Event* event, AppConfig* main_config) {
    if (!panel || !event) return;

    current_panel_for_callbacks = panel;
    int panel_x = panel->rect.x;
    int panel_y = panel->rect.y;

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

                // On a besoin de screen_width, qu'on peut obtenir depuis gear_rect
                // (gear_rect.x = screen_width - gear_size - gear_margin)
                int screen_width = panel->gear_rect.x + panel->gear_rect.w +
                scale_value(20, panel->scale_factor);

                panel->target_x = screen_width - panel->rect.w;

                debug_printf("🎯 OUVERTURE - target_x=%d, screen_width=%d, panel_width=%d\n",
                             panel->target_x, screen_width, panel->rect.w);


                // Recharger la config et mettre à jour les widgets
                load_config(&panel->temp_config);
                set_widget_int_value(panel->widget_list, "breath_duration",
                                     panel->temp_config.breath_duration);
                set_widget_int_value(panel->widget_list, "breath_cycles",
                                     panel->temp_config.breath_cycles);
                set_widget_bool_value(panel->widget_list, "alternate_cycles",
                                      panel->temp_config.alternate_cycles);

                update_preview_for_new_duration(panel, panel->temp_config.breath_duration);
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
        if (event->type == SDL_MOUSEBUTTONDOWN) {
            int x = event->button.x;
            int y = event->button.y;

            // Boutons Appliquer/Annuler
            SDL_Rect abs_apply = panel->apply_button.rect;
            abs_apply.x += panel_x;
            abs_apply.y += panel_y;

            SDL_Rect abs_cancel = panel->cancel_button.rect;
            abs_cancel.x += panel_x;
            abs_cancel.y += panel_y;

            if (is_point_in_rect(x, y, abs_apply)) {
                save_config(&panel->temp_config);
                *main_config = panel->temp_config;
                panel->state = PANEL_CLOSING;
                debug_printf("✅ Configuration appliquée\n");
                return;
            }

            if (is_point_in_rect(x, y, abs_cancel)) {
                load_config(&panel->temp_config);
                panel->state = PANEL_CLOSING;
                debug_printf("❌ Modifications annulées\n");
                return;
            }
        }

        // Événements des widgets
        handle_widget_list_events(panel->widget_list, event, panel_x, panel_y);
    }

    if (event->type == SDL_MOUSEBUTTONUP) {
        current_panel_for_callbacks = NULL;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DU SCALE (resize fenêtre)
// ════════════════════════════════════════════════════════════════════════════

void update_panel_scale(SettingsPanel* panel, int screen_width, int screen_height, float scale_factor) {
    if (!panel) return;

    panel->scale_factor = scale_factor;

    // Calcul de la largeur du panneau
    int panel_width = (screen_width >= PANEL_WIDTH) ? PANEL_WIDTH : screen_width;
    panel->rect.w = panel_width;
    panel->rect.h = screen_height;

    // Calcul du ratio pour les éléments internes
    float panel_ratio = (float)panel_width / (float)PANEL_WIDTH;

    // Sauvegarder le ratio dans la structure pour l'utiliser ailleurs
    panel->panel_ratio = panel_ratio;

    // Positions selon l'état
    if (panel->state == PANEL_CLOSED) {
        panel->rect.x = screen_width;
        panel->target_x = screen_width;
        panel->current_x = screen_width;
    }
    else if (panel->state == PANEL_OPEN) {
        panel->rect.x = screen_width - panel_width;
        panel->target_x = screen_width - panel_width;
        panel->current_x = screen_width - panel_width;
    }
    else if (panel->state == PANEL_OPENING || panel->state == PANEL_CLOSING) {
        // Mettre à jour seulement la cible, pas current_x (géré par l'animation)
        panel->target_x = (panel->state == PANEL_OPENING)
        ? screen_width - panel_width
        : screen_width;
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

    debug_section("RESCALE DES WIDGETS");

    // ═════════════════════════════════════════════════════════════════════════
    // RESCALE DES WIDGETS
    // ═════════════════════════════════════════════════════════════════════════
    if (panel->widget_list) {
        WidgetNode* node = panel->widget_list->first;
        int widget_count = 0;

        while (node) {
            if (node->type == WIDGET_TYPE_INCREMENT && node->widget.increment_widget) {
                rescale_config_widget(node->widget.increment_widget, panel_ratio);
                widget_count++;
            }
            else if (node->type == WIDGET_TYPE_TOGGLE && node->widget.toggle_widget) {
                rescale_toggle_widget(node->widget.toggle_widget, panel_ratio);
                widget_count++;
            }
            node = node->next;
        }

        if (widget_count > 0) {
            debug_printf("✅ %d widgets rescalés (ratio: %.2f)\n", widget_count, panel_ratio);
        }
    }

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
