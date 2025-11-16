// SPDX-License-Identifier: GPL-3.0-or-later
#include "widget.h"
#include "geometry.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════════
//  GESTIONNAIRE DE CACHE DE POLICES - Variables globales
// ════════════════════════════════════════════════════════════════════════════
CachedFont g_font_cache[MAX_CACHED_FONTS] = {0};
int g_font_cache_count = 0;
char g_font_path[256] = "";

// ════════════════════════════════════════════════════════════════════════════
//  INITIALISATION DU GESTIONNAIRE
// ════════════════════════════════════════════════════════════════════════════
void init_font_manager(const char* font_path) {
    if (!font_path) return;

    snprintf(g_font_path, sizeof(g_font_path), "%s", font_path);
    g_font_cache_count = 0;

    // Marquer tous les slots comme non utilisés
    for (int i = 0; i < MAX_CACHED_FONTS; i++) {
        g_font_cache[i].font = NULL;
        g_font_cache[i].size = 0;
        g_font_cache[i].in_use = false;
    }

    debug_section("GESTIONNAIRE DE POLICES");
    debug_printf("✅ Initialisé avec : %s\n", font_path);
    debug_printf("   Cache : %d slots disponibles\n", MAX_CACHED_FONTS);
    debug_printf("   Taille minimum : %dpx\n", MIN_FONT_SIZE);
    debug_blank_line();
}

// ════════════════════════════════════════════════════════════════════════════
//  OBTENIR UNE POLICE (avec cache)
// ════════════════════════════════════════════════════════════════════════════
TTF_Font* get_font_for_size(int size) {
    if (g_font_path[0] == '\0') {
        debug_printf("⚠️ Gestionnaire de polices non initialisé\n");
        return NULL;
    }

    // Appliquer le minimum de taille
    if (size < MIN_FONT_SIZE) {
        debug_printf("📏 Taille %dpx → %dpx (min garanti)\n", size, MIN_FONT_SIZE);
        size = MIN_FONT_SIZE;
    }

    // Chercher dans le cache
    for (int i = 0; i < MAX_CACHED_FONTS; i++) {
        if (g_font_cache[i].in_use && g_font_cache[i].size == size) {
            // Trouvé ! Réutiliser
            return g_font_cache[i].font;
        }
    }

    // Pas trouvé → charger une nouvelle taille
    if (g_font_cache_count < MAX_CACHED_FONTS) {
        TTF_Font* new_font = TTF_OpenFont(g_font_path, size);
        if (!new_font) {
            debug_printf("❌ Impossible de charger police taille %dpx : %s\n",
                        size, TTF_GetError());
            return NULL;
        }

        // Ajouter au cache
        g_font_cache[g_font_cache_count].font = new_font;
        g_font_cache[g_font_cache_count].size = size;
        g_font_cache[g_font_cache_count].in_use = true;
        g_font_cache_count++;

        debug_printf("🔤 Police %dpx chargée et cachée (slot %d/%d)\n",
                    size, g_font_cache_count, MAX_CACHED_FONTS);

        return new_font;
    }

    // Cache plein ! Réutiliser le premier slot (stratégie simple)
    debug_printf("⚠️ Cache plein, réutilisation slot 0\n");
    TTF_CloseFont(g_font_cache[0].font);
    g_font_cache[0].font = TTF_OpenFont(g_font_path, size);
    g_font_cache[0].size = size;
    return g_font_cache[0].font;
}

// ════════════════════════════════════════════════════════════════════════════
//  NETTOYAGE DU GESTIONNAIRE
// ════════════════════════════════════════════════════════════════════════════
void cleanup_font_manager(void) {
    debug_section("NETTOYAGE GESTIONNAIRE DE POLICES");

    for (int i = 0; i < MAX_CACHED_FONTS; i++) {
        if (g_font_cache[i].in_use && g_font_cache[i].font) {
            debug_printf("🗑️ Libération police %dpx\n", g_font_cache[i].size);
            TTF_CloseFont(g_font_cache[i].font);
            g_font_cache[i].font = NULL;
            g_font_cache[i].in_use = false;
        }
    }

    g_font_cache_count = 0;
    debug_blank_line();
}

// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION D'UN WIDGET DE CONFIGURATION (STYLE ROLLER)
// ════════════════════════════════════════════════════════════════════════════
ConfigWidget* create_config_widget(const char* name, int x, int y,
                                   int min_val, int max_val, int start_val,
                                   int increment, int arrow_size, int text_size,
                                   TTF_Font* font, const char* display_type) {
    (void)arrow_size;  // Paramètre conservé pour compatibilité mais non utilisé
    (void)font;        // Paramètre conservé pour compatibilité mais non utilisé

    ConfigWidget* widget = malloc(sizeof(ConfigWidget));
    if (!widget) {
        debug_printf("❌ Erreur allocation ConfigWidget: %s\n", name);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // VALIDATION ET INITIALISATION DES VALEURS
    // ─────────────────────────────────────────────────────────────────────────
    if (start_val < min_val) start_val = min_val;
    if (start_val > max_val) start_val = max_val;

    snprintf(widget->option_name, sizeof(widget->option_name), "%s", name);
    widget->value = start_val;
    widget->min_value = min_val;
    widget->max_value = max_val;
    widget->increment = increment;

    // Type d'affichage : "numeric" par défaut, ou "time"
    if (display_type && strcmp(display_type, "time") == 0) {
        snprintf(widget->widget_display_type, sizeof(widget->widget_display_type), "time");
    } else {
        snprintf(widget->widget_display_type, sizeof(widget->widget_display_type), "numeric");
    }

    // Stocker la taille de police de base
    widget->base_text_size = text_size;
    widget->current_text_size = text_size;

    // ─────────────────────────────────────────────────────────────────────────
    // ESPACEMENTS DE BASE (pour le roller)
    // ─────────────────────────────────────────────────────────────────────────
    widget->base_espace_apres_texte = (int)(text_size * 0.7);  // Marge texte → roller
    widget->base_roller_padding = (int)(text_size * 0.4);      // Padding interne du roller

    // ─────────────────────────────────────────────────────────────────────────
    // COULEURS DU ROLLER (nouveau design)
    // ─────────────────────────────────────────────────────────────────────────
    widget->color = (SDL_Color){0, 0, 0, 255};                      // Texte label noir
    widget->roller_bg_color = (SDL_Color){255, 255, 255, 200};      // Fond blanc alpha 200
    widget->roller_text_color = (SDL_Color){70, 80, 100, 255};      // Bleu-gris foncé
    widget->roller_border_color = (SDL_Color){150, 150, 150, 180};  // Bordure grise

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION DRAG
    // ─────────────────────────────────────────────────────────────────────────
    widget->is_dragging = false;
    widget->drag_start_x = 0;
    widget->drag_start_value = 0;

    // ─────────────────────────────────────────────────────────────────────────
    // MODE TIME : Initialisation
    // ─────────────────────────────────────────────────────────────────────────
    widget->selected_field = -1;  // -1 = aucun champ sélectionné
    widget->mm_rect = (SDL_Rect){0, 0, 0, 0};
    widget->ss_rect = (SDL_Rect){0, 0, 0, 0};

    // ─────────────────────────────────────────────────────────────────────────
    // MESURE DU TEXTE (pour calculer les dimensions)
    // ─────────────────────────────────────────────────────────────────────────
    int text_width = 0;
    int text_height = 0;

    TTF_Font* correct_font = get_font_for_size(text_size);
    if (correct_font) {
        TTF_SizeUTF8(correct_font, name, &text_width, &text_height);
    } else {
        text_width = strlen(name) * (text_size / 2);
        text_height = text_size;
    }

    widget->text_height = text_height;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DE LA TAILLE DU ROLLER
    // ─────────────────────────────────────────────────────────────────────────
    // Largeur du roller : dépend du type
    int roller_content_width = 0;
    if (strcmp(widget->widget_display_type, "time") == 0) {
        // Mode time : "00:00" = 5 caractères
        char time_sample[] = "00:00";
        if (correct_font) {
            TTF_SizeUTF8(correct_font, time_sample, &roller_content_width, NULL);
        } else {
            roller_content_width = strlen(time_sample) * (text_size / 2);
        }
    } else {
        // Mode numeric : estimer la largeur max possible
        char max_value_str[16];
        snprintf(max_value_str, sizeof(max_value_str), "%d", max_val);
        if (correct_font) {
            TTF_SizeUTF8(correct_font, max_value_str, &roller_content_width, NULL);
        } else {
            roller_content_width = strlen(max_value_str) * (text_size / 2);
        }
    }

    widget->roller_width = roller_content_width + (widget->base_roller_padding * 2);
    widget->roller_height = text_height + 4;  // +4 pour un peu de padding vertical

    // ─────────────────────────────────────────────────────────────────────────
    // LAYOUT INTERNE (coordonnées LOCALES au widget)
    // ─────────────────────────────────────────────────────────────────────────
    widget->local_text_x = 0;
    widget->local_text_y = 0;

    widget->local_roller_x = text_width + widget->base_espace_apres_texte;
    widget->local_roller_y = 0;

    // Rectangle du roller (coordonnées locales)
    widget->roller_rect.x = widget->local_roller_x;
    widget->roller_rect.y = widget->local_roller_y;
    widget->roller_rect.w = widget->roller_width;
    widget->roller_rect.h = widget->roller_height;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DE LA BOUNDING BOX TOTALE
    // ─────────────────────────────────────────────────────────────────────────
    int total_width = widget->local_roller_x + widget->roller_width + 5;  // +5 = marge droite
    int total_height = text_height > widget->roller_height ? text_height : widget->roller_height;

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DE LA BASE (WidgetBase)
    // ─────────────────────────────────────────────────────────────────────────
    widget->base.x = x;
    widget->base.y = y;
    widget->base.base_x = x;
    widget->base.base_y = y;
    widget->base.width = total_width;
    widget->base.height = total_height;
    widget->base.base_width = total_width;
    widget->base.base_height = total_height;
    widget->base.hovered = false;
    widget->base.enabled = true;

    widget->on_value_changed = NULL;

    debug_subsection("Création ConfigWidget ROLLER");
    debug_printf("  Nom : %s\n", name);
    debug_printf("  Type : %s\n", widget->widget_display_type);
    debug_printf("  Position : (%d, %d)\n", x, y);
    debug_printf("  Taille : %dx%d\n", total_width, total_height);
    debug_printf("  Police : %dpx\n", text_size);
    debug_printf("  Roller : %dx%d\n", widget->roller_width, widget->roller_height);
    debug_blank_line();

    return widget;
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU WIDGET ROLLER
// ════════════════════════════════════════════════════════════════════════════
void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget,
                          int offset_x, int offset_y, int container_width) {
    if (!widget || !renderer) return;

    (void)container_width;  // Pas utilisé dans le nouveau design

    int widget_screen_x = offset_x + widget->base.x;
    int widget_screen_y = offset_y + widget->base.y;

    // ─────────────────────────────────────────────────────────────────────────
    // OBTENIR LA POLICE À LA BONNE TAILLE
    // ─────────────────────────────────────────────────────────────────────────
    TTF_Font* correct_font = get_font_for_size(widget->current_text_size);
    if (!correct_font) return;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. RENDU DU LABEL (option_name)
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Surface* label_surface = TTF_RenderUTF8_Blended(correct_font, widget->option_name, widget->color);
    if (label_surface) {
        SDL_Texture* label_texture = SDL_CreateTextureFromSurface(renderer, label_surface);
        if (label_texture) {
            SDL_Rect label_rect = {
                widget_screen_x + widget->local_text_x,
                widget_screen_y + widget->local_text_y,
                label_surface->w,
                label_surface->h
            };
            SDL_RenderCopy(renderer, label_texture, NULL, &label_rect);
            SDL_DestroyTexture(label_texture);
        }
        SDL_FreeSurface(label_surface);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 2. PRÉPARATION DU TEXTE À AFFICHER DANS LE ROLLER
    // ─────────────────────────────────────────────────────────────────────────
    char display_str[32];
    if (strcmp(widget->widget_display_type, "time") == 0) {
        // Mode TIME : afficher mm:ss
        int total_seconds = widget->value;
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        snprintf(display_str, sizeof(display_str), "%02d:%02d", minutes, seconds);
    } else {
        // Mode NUMERIC : afficher la valeur
        snprintf(display_str, sizeof(display_str), "%d", widget->value);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. RENDU DU ROLLER (fond arrondi avec Cairo)
    // ─────────────────────────────────────────────────────────────────────────
    int roller_screen_x = widget_screen_x + widget->roller_rect.x;
    int roller_screen_y = widget_screen_y + widget->roller_rect.y;

    // Dessiner le fond arrondi avec bordure
    draw_rounded_rect(renderer,
                      roller_screen_x,
                      roller_screen_y,
                      widget->roller_rect.w,
                      widget->roller_rect.h,
                      4,  // Rayon des coins arrondis
                      widget->roller_bg_color,
                      widget->roller_border_color);

    // ─────────────────────────────────────────────────────────────────────────
    // 4. RENDU DU TEXTE DANS LE ROLLER
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Surface* value_surface = TTF_RenderUTF8_Blended(correct_font, display_str, widget->roller_text_color);
    if (value_surface) {
        SDL_Texture* value_texture = SDL_CreateTextureFromSurface(renderer, value_surface);
        if (value_texture) {
            // Centrer le texte dans le roller
            int text_x = roller_screen_x + (widget->roller_rect.w - value_surface->w) / 2;
            int text_y = roller_screen_y + (widget->roller_rect.h - value_surface->h) / 2;

            SDL_Rect value_rect = {
                text_x,
                text_y,
                value_surface->w,
                value_surface->h
            };
            SDL_RenderCopy(renderer, value_texture, NULL, &value_rect);
            SDL_DestroyTexture(value_texture);
        }
        SDL_FreeSurface(value_surface);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5. MODE TIME : Calculer les zones des champs mm et ss (pour hover/interaction)
    // ─────────────────────────────────────────────────────────────────────────
    if (strcmp(widget->widget_display_type, "time") == 0) {
        // Mesurer la largeur de "00:" et "00"
        int mm_width = 0, ss_width = 0;
        TTF_SizeUTF8(correct_font, "00", &mm_width, NULL);
        TTF_SizeUTF8(correct_font, "00", &ss_width, NULL);

        int colon_width = 0;
        TTF_SizeUTF8(correct_font, ":", &colon_width, NULL);

        // Centrer le texte total
        int total_text_width = mm_width + colon_width + ss_width;
        int text_start_x = roller_screen_x + (widget->roller_rect.w - total_text_width) / 2;

        // Zone des minutes
        widget->mm_rect.x = text_start_x - roller_screen_x;
        widget->mm_rect.y = 0;
        widget->mm_rect.w = mm_width;
        widget->mm_rect.h = widget->roller_rect.h;

        // Zone des secondes
        widget->ss_rect.x = text_start_x + mm_width + colon_width - roller_screen_x;
        widget->ss_rect.y = 0;
        widget->ss_rect.w = ss_width;
        widget->ss_rect.h = widget->roller_rect.h;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS ROLLER
// ════════════════════════════════════════════════════════════════════════════
void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y, int container_width) {
    if (!widget || !event) return;

    (void)container_width;  // Pas utilisé dans le nouveau design

    int widget_screen_x = offset_x + widget->base.x;
    int widget_screen_y = offset_y + widget->base.y;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DU RECTANGLE DU ROLLER EN COORDONNÉES ÉCRAN
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Rect roller_screen_rect = {
        widget_screen_x + widget->roller_rect.x,
        widget_screen_y + widget->roller_rect.y,
        widget->roller_rect.w,
        widget->roller_rect.h
    };

    // ═════════════════════════════════════════════════════════════════════════
    // ÉVÉNEMENT: MOUVEMENT DE SOURIS
    // ═════════════════════════════════════════════════════════════════════════
    if (event->type == SDL_MOUSEMOTION) {
        int mx = event->motion.x;
        int my = event->motion.y;

        // Test de survol du roller
        bool on_roller = (mx >= roller_screen_rect.x && mx < roller_screen_rect.x + roller_screen_rect.w &&
                         my >= roller_screen_rect.y && my < roller_screen_rect.y + roller_screen_rect.h);

        widget->base.hovered = on_roller;

        // ─────────────────────────────────────────────────────────────────────
        // MODE TIME : Déterminer quel champ est survolé (mm ou ss)
        // ─────────────────────────────────────────────────────────────────────
        if (strcmp(widget->widget_display_type, "time") == 0 && on_roller) {
            int relative_x = mx - roller_screen_rect.x;

            // Tester si on est sur mm ou ss
            if (relative_x >= widget->mm_rect.x && relative_x < widget->mm_rect.x + widget->mm_rect.w) {
                widget->selected_field = 0;  // Minutes
            } else if (relative_x >= widget->ss_rect.x && relative_x < widget->ss_rect.x + widget->ss_rect.w) {
                widget->selected_field = 1;  // Secondes
            } else {
                widget->selected_field = -1;  // Aucun champ
            }
        } else {
            widget->selected_field = -1;
        }

        // ─────────────────────────────────────────────────────────────────────
        // DRAG EN COURS (mode numeric uniquement)
        // ─────────────────────────────────────────────────────────────────────
        if (widget->is_dragging && strcmp(widget->widget_display_type, "numeric") == 0) {
            int delta_x = mx - widget->drag_start_x;

            // Sensibilité : 1 incrément tous les 10 pixels
            const int PIXELS_PER_INCREMENT = 10;
            int increments = delta_x / PIXELS_PER_INCREMENT;

            if (increments != 0) {
                int new_value = widget->drag_start_value + (increments * widget->increment);

                // Limiter aux bornes
                if (new_value < widget->min_value) new_value = widget->min_value;
                if (new_value > widget->max_value) new_value = widget->max_value;

                if (new_value != widget->value) {
                    widget->value = new_value;
                    if (widget->on_value_changed) widget->on_value_changed(widget->value);
                }
            }
        }
    }
    // ═════════════════════════════════════════════════════════════════════════
    // ÉVÉNEMENT: MOLETTE
    // ═════════════════════════════════════════════════════════════════════════
    else if (event->type == SDL_MOUSEWHEEL) {
        if (widget->base.hovered) {
            int direction = (event->wheel.y > 0) ? 1 : -1;  // 1 = haut, -1 = bas

            if (strcmp(widget->widget_display_type, "time") == 0) {
                // ─────────────────────────────────────────────────────────────
                // MODE TIME : Modifier le champ survolé (mm ou ss)
                // ─────────────────────────────────────────────────────────────
                int total_seconds = widget->value;
                int minutes = total_seconds / 60;
                int seconds = total_seconds % 60;

                if (widget->selected_field == 0) {
                    // Modifier les minutes
                    minutes += direction;
                    if (minutes < 0) minutes = 0;
                    if (minutes > 99) minutes = 99;  // Limite arbitraire
                } else if (widget->selected_field == 1) {
                    // Modifier les secondes
                    seconds += direction;
                    if (seconds < 0) seconds = 0;
                    if (seconds > 59) seconds = 59;
                }

                int new_value = minutes * 60 + seconds;

                // Appliquer les limites min/max
                if (new_value < widget->min_value) new_value = widget->min_value;
                if (new_value > widget->max_value) new_value = widget->max_value;

                if (new_value != widget->value) {
                    widget->value = new_value;
                    if (widget->on_value_changed) widget->on_value_changed(widget->value);
                }
            } else {
                // ─────────────────────────────────────────────────────────────
                // MODE NUMERIC : Incrémenter/décrémenter
                // ─────────────────────────────────────────────────────────────
                int new_value = widget->value + (direction * widget->increment);

                if (new_value >= widget->min_value && new_value <= widget->max_value) {
                    widget->value = new_value;
                    if (widget->on_value_changed) widget->on_value_changed(widget->value);
                }
            }
        }
    }
    // ═════════════════════════════════════════════════════════════════════════
    // ÉVÉNEMENT: CLIC SOURIS (bouton enfoncé)
    // ═════════════════════════════════════════════════════════════════════════
    else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        int mx = event->button.x;
        int my = event->button.y;

        // Tester si on clique sur le roller
        bool on_roller = (mx >= roller_screen_rect.x && mx < roller_screen_rect.x + roller_screen_rect.w &&
                         my >= roller_screen_rect.y && my < roller_screen_rect.y + roller_screen_rect.h);

        if (on_roller) {
            if (strcmp(widget->widget_display_type, "time") == 0) {
                // ─────────────────────────────────────────────────────────────
                // MODE TIME : Clic haut/bas du champ pour ±1
                // ─────────────────────────────────────────────────────────────
                int relative_y = my - roller_screen_rect.y;
                int direction = (relative_y < roller_screen_rect.h / 2) ? 1 : -1;  // Haut = +1, Bas = -1

                int total_seconds = widget->value;
                int minutes = total_seconds / 60;
                int seconds = total_seconds % 60;

                if (widget->selected_field == 0) {
                    // Modifier les minutes
                    minutes += direction;
                    if (minutes < 0) minutes = 0;
                    if (minutes > 99) minutes = 99;
                } else if (widget->selected_field == 1) {
                    // Modifier les secondes
                    seconds += direction;
                    if (seconds < 0) seconds = 0;
                    if (seconds > 59) seconds = 59;
                }

                int new_value = minutes * 60 + seconds;
                if (new_value < widget->min_value) new_value = widget->min_value;
                if (new_value > widget->max_value) new_value = widget->max_value;

                if (new_value != widget->value) {
                    widget->value = new_value;
                    if (widget->on_value_changed) widget->on_value_changed(widget->value);
                }
            } else {
                // ─────────────────────────────────────────────────────────────
                // MODE NUMERIC : Démarrer le drag ou clic gauche/droite pour ±1
                // ─────────────────────────────────────────────────────────────
                int relative_x = mx - roller_screen_rect.x;
                int half_width = roller_screen_rect.w / 2;

                if (relative_x < half_width) {
                    // Clic gauche → décrémenter
                    int new_value = widget->value - widget->increment;
                    if (new_value >= widget->min_value) {
                        widget->value = new_value;
                        if (widget->on_value_changed) widget->on_value_changed(widget->value);
                    }
                } else {
                    // Clic droite → incrémenter
                    int new_value = widget->value + widget->increment;
                    if (new_value <= widget->max_value) {
                        widget->value = new_value;
                        if (widget->on_value_changed) widget->on_value_changed(widget->value);
                    }
                }

                // Démarrer le drag
                widget->is_dragging = true;
                widget->drag_start_x = mx;
                widget->drag_start_value = widget->value;
            }
        }
    }
    // ═════════════════════════════════════════════════════════════════════════
    // ÉVÉNEMENT: RELÂCHEMENT SOURIS
    // ═════════════════════════════════════════════════════════════════════════
    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT) {
        // Arrêter le drag
        widget->is_dragging = false;
    }
}


// ════════════════════════════════════════════════════════════════════════════
//  MISE À JOUR DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void update_config_widget(ConfigWidget* widget, float delta_time) {
    (void)widget;
    (void)delta_time;
}

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACK DE CHANGEMENT DE VALEUR
// ════════════════════════════════════════════════════════════════════════════
void set_config_value_changed_callback(ConfigWidget* widget, void (*callback)(int)) {
    if (widget) {
        widget->on_value_changed = callback;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  RESPONSIVE : RESCALE DU WIDGET (INTELLIGENT)
// ════════════════════════════════════════════════════════════════════════════
void rescale_config_widget(ConfigWidget* widget, float panel_ratio) {
    if (!widget) return;

    debug_subsection("Rescale CONFIG ROLLER");
    debug_printf("  Widget : %s\n", widget->option_name);
    debug_printf("  Type : %s\n", widget->widget_display_type);
    debug_printf("  Ratio : %.2f\n", panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 1. SCALER LA BASE (position du widget)
    // ─────────────────────────────────────────────────────────────────────────
    rescale_widget_base(&widget->base, panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 2. CALCULER LA NOUVELLE TAILLE DE POLICE
    // ─────────────────────────────────────────────────────────────────────────
    int new_text_size = (int)(widget->base_text_size * panel_ratio);
    widget->current_text_size = new_text_size;

    debug_printf("  Police : %dpx → %dpx\n", widget->base_text_size, new_text_size);

    // ─────────────────────────────────────────────────────────────────────────
    // 3. OBTENIR LA POLICE À CETTE TAILLE
    // ─────────────────────────────────────────────────────────────────────────
    TTF_Font* scaled_font = get_font_for_size(new_text_size);
    if (!scaled_font) {
        debug_printf("  ⚠️ Impossible d'obtenir police\n");
        debug_blank_line();
        return;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 4. REMESURER LE TEXTE AVEC LA NOUVELLE POLICE
    // ─────────────────────────────────────────────────────────────────────────
    int new_text_width = 0;
    int new_text_height = 0;
    TTF_SizeUTF8(scaled_font, widget->option_name, &new_text_width, &new_text_height);

    widget->text_height = new_text_height;

    debug_printf("  Texte remesuré : %dpx × %dpx\n", new_text_width, new_text_height);

    // ─────────────────────────────────────────────────────────────────────────
    // 5. SCALER LES ESPACEMENTS
    // ─────────────────────────────────────────────────────────────────────────
    int scaled_espace_texte = (int)(widget->base_espace_apres_texte * panel_ratio);
    int scaled_roller_padding = (int)(widget->base_roller_padding * panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 6. RECALCULER LA TAILLE DU ROLLER
    // ─────────────────────────────────────────────────────────────────────────
    int roller_content_width = 0;
    if (strcmp(widget->widget_display_type, "time") == 0) {
        // Mode time : "00:00"
        char time_sample[] = "00:00";
        TTF_SizeUTF8(scaled_font, time_sample, &roller_content_width, NULL);
    } else {
        // Mode numeric : largeur max possible
        char max_value_str[16];
        snprintf(max_value_str, sizeof(max_value_str), "%d", widget->max_value);
        TTF_SizeUTF8(scaled_font, max_value_str, &roller_content_width, NULL);
    }

    widget->roller_width = roller_content_width + (scaled_roller_padding * 2);
    widget->roller_height = new_text_height + 4;

    // ─────────────────────────────────────────────────────────────────────────
    // 7. RECALCULER LES OFFSETS
    // ─────────────────────────────────────────────────────────────────────────
    widget->local_roller_x = new_text_width + scaled_espace_texte;
    widget->local_roller_y = 0;

    // Mettre à jour le rectangle du roller
    widget->roller_rect.x = widget->local_roller_x;
    widget->roller_rect.y = widget->local_roller_y;
    widget->roller_rect.w = widget->roller_width;
    widget->roller_rect.h = widget->roller_height;

    debug_printf("  ✓ Roller : %dx%d @ (%d, %d)\n",
                 widget->roller_width, widget->roller_height,
                 widget->local_roller_x, widget->local_roller_y);
    debug_blank_line();
}


// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void free_config_widget(ConfigWidget* widget) {
    if (!widget) return;
    free(widget);
}
