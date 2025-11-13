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
//  CRÉATION D'UN WIDGET DE CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════
ConfigWidget* create_config_widget(const char* name, int x, int y,
                                   int min_val, int max_val, int start_val,
                                   int increment, int arrow_size, int text_size,
                                   TTF_Font* font) {
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
    widget->arrow_size = arrow_size;
    widget->base_arrow_size = arrow_size;
    widget->up_arrow_hovered = false;
    widget->down_arrow_hovered = false;

    // Stocker la taille de police de base
    widget->base_text_size = text_size;
    widget->current_text_size = text_size;

    // ─────────────────────────────────────────────────────────────────────────
    // ESPACEMENTS DE BASE (calculés proportionnellement à la taille de police)
    // ─────────────────────────────────────────────────────────────────────────
    // Au lieu de valeurs fixes, on base tout sur text_size pour que les espacements
    // soient cohérents quelle que soit la taille de police définie dans le JSON
    widget->base_espace_apres_texte = (int)(text_size * 1.1);    // ~110% de la hauteur du texte
    widget->base_espace_entre_fleches = (int)(text_size * 0.28); // ~28% de la hauteur du texte
    widget->base_espace_apres_fleches = (int)(text_size * 0.83); // ~83% de la hauteur du texte

    // ─────────────────────────────────────────────────────────────────────────
    // COULEURS DU WIDGET
    // ─────────────────────────────────────────────────────────────────────────
    widget->bg_hover_color = (SDL_Color){0, 0, 0, 50};        // Fond noir transparent
    widget->color = (SDL_Color){0, 0, 0, 255};                // Flèches et texte noirs
    widget->hover_color = (SDL_Color){255, 255, 150, 255};    // Jaune pâle au survol

    // ─────────────────────────────────────────────────────────────────────────
    // MESURE DU TEXTE (pour calculer les dimensions)
    // ─────────────────────────────────────────────────────────────────────────
    int text_width = 0;
    int text_height = 0;

    // IMPORTANT : Utiliser get_font_for_size() pour obtenir la police à la bonne
    // taille (celle définie dans le JSON), pas le paramètre 'font' qui pourrait
    // être à une taille différente (comme font_normal qui est 20px)
    TTF_Font* correct_font = get_font_for_size(text_size);
    if (correct_font) {
        // Utiliser la police correcte pour mesurer précisément
        TTF_SizeUTF8(correct_font, name, &text_width, &text_height);
    } else if (font) {
        // Fallback sur le font passé en paramètre si get_font_for_size échoue
        TTF_SizeUTF8(font, name, &text_width, &text_height);
    } else {
        // Estimation grossière si pas de police du tout
        text_width = strlen(name) * (text_size / 2);
        text_height = text_size;
    }

    widget->text_height = text_height;

    // ─────────────────────────────────────────────────────────────────────────
    // LAYOUT INTERNE (coordonnées LOCALES au widget)
    // ─────────────────────────────────────────────────────────────────────────
    widget->local_text_x = 0;
    widget->local_text_y = 0;

    widget->local_arrows_x = text_width + widget->base_espace_apres_texte;

    int total_arrows_height = arrow_size * 2 + widget->base_espace_entre_fleches;
    widget->local_arrows_y = (text_height - total_arrows_height) / 2 + arrow_size / 2;

    widget->local_value_x = widget->local_arrows_x + arrow_size + widget->base_espace_apres_fleches;
    widget->local_value_y = 0;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DE LA BOUNDING BOX TOTALE
    // ─────────────────────────────────────────────────────────────────────────
    char value_str[16];
    snprintf(value_str, sizeof(value_str), "%d", widget->value);
    int value_width = strlen(value_str) * (text_size / 2);  // Estimation

    int total_width = widget->local_value_x + value_width + 10;  // +10 = marge droite
    int total_height = text_height > total_arrows_height ? text_height : total_arrows_height;

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

    debug_subsection("Création ConfigWidget");
    debug_printf("  Nom : %s\n", name);
    debug_printf("  Position : (%d, %d)\n", x, y);
    debug_printf("  Taille : %dx%d\n", total_width, total_height);
    debug_printf("  Police : %dpx\n", text_size);
    debug_printf("  Largeur texte mesuré : %dpx\n", text_width);
    debug_printf("  Layout : texte@%d, flèches@%d, valeur@%d\n",
                 widget->local_text_x, widget->local_arrows_x, widget->local_value_x);
    debug_blank_line();

    return widget;
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void render_config_widget(SDL_Renderer* renderer, ConfigWidget* widget,
                          int offset_x, int offset_y, int container_width) {
    if (!widget || !renderer) return;

    int widget_screen_x = offset_x + widget->base.x;
    int widget_screen_y = offset_y + widget->base.y;

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DES POSITIONS POUR L'ALIGNEMENT EN COLONNES
    // ─────────────────────────────────────────────────────────────────────────
    // Si container_width > 0, on aligne les flèches+valeur à droite
    // Sinon, on utilise le layout normal (séquentiel)
    int arrows_x_offset = widget->local_arrows_x;  // Position par défaut
    int value_x_offset = widget->local_value_x;    // Position par défaut

    if (container_width > 0) {
        // Distance depuis le bord droit pour les flèches+valeur
        // On réserve: arrow_size + espace + largeur_valeur (estimée à 40px)
        const int RIGHT_MARGIN = 10;  // Marge depuis le bord droit
        const int ESTIMATED_VALUE_WIDTH = 40;  // Largeur estimée pour la valeur

        int arrows_value_width = widget->arrow_size + widget->base_espace_apres_fleches + ESTIMATED_VALUE_WIDTH;

        // Positionner les flèches à partir de la droite
        arrows_x_offset = container_width - arrows_value_width - RIGHT_MARGIN;
        value_x_offset = arrows_x_offset + widget->arrow_size + widget->base_espace_apres_fleches;

        // S'assurer que les flèches ne se superposent pas au texte
        // Laisser au moins un petit espace après le texte
        int min_arrows_x = widget->local_arrows_x;
        if (arrows_x_offset < min_arrows_x) {
            arrows_x_offset = min_arrows_x;
            value_x_offset = widget->local_value_x;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // FOND AU SURVOL (rectangle arrondi)
    // ─────────────────────────────────────────────────────────────────────────
    if (widget->base.hovered) {
        // Utiliser container_width si disponible, sinon widget->base.width
        // container_width reflète la largeur réelle du groupe (incluant la valeur actuelle)
        int hover_width = (container_width > 0) ? container_width : widget->base.width;

        roundedBoxRGBA(renderer,
                       widget_screen_x - 5,
                       widget_screen_y - 5,
                       widget_screen_x + hover_width + 5,
                       widget_screen_y + widget->base.height + 5,
                       5,
                       widget->bg_hover_color.r,
                       widget->bg_hover_color.g,
                       widget->bg_hover_color.b,
                       widget->bg_hover_color.a);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // OBTENIR LA POLICE À LA BONNE TAILLE (celle utilisée pour mesurer)
    // ─────────────────────────────────────────────────────────────────────────
    TTF_Font* correct_font = get_font_for_size(widget->current_text_size);
    if (correct_font) {
        SDL_Surface* text_surface = TTF_RenderUTF8_Blended(correct_font, widget->option_name, widget->color);
        if (text_surface) {
            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            if (text_texture) {
                SDL_Rect text_rect = {
                    widget_screen_x + widget->local_text_x,
                    widget_screen_y + widget->local_text_y,
                    text_surface->w,
                    text_surface->h
                };
                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DES FLÈCHES (▲ et ▼)
    // ─────────────────────────────────────────────────────────────────────────
    int arrows_screen_x = widget_screen_x + arrows_x_offset;
    int arrows_screen_y = widget_screen_y + widget->local_arrows_y;

    SDL_Color up_color = widget->up_arrow_hovered ? widget->hover_color : widget->color;
    SDL_Color down_color = widget->down_arrow_hovered ? widget->hover_color : widget->color;

    int up_y = arrows_screen_y;
    Triangle* up_arrow = create_up_arrow(arrows_screen_x, up_y, widget->arrow_size, up_color);
    if (up_arrow) {
        draw_triangle(renderer, up_arrow);
        free_triangle(up_arrow);
    }

    int down_y = arrows_screen_y + widget->arrow_size + widget->base_espace_entre_fleches;
    Triangle* down_arrow = create_down_arrow(arrows_screen_x, down_y, widget->arrow_size, down_color);
    if (down_arrow) {
        draw_triangle(renderer, down_arrow);
        free_triangle(down_arrow);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RENDU DE LA VALEUR
    // ─────────────────────────────────────────────────────────────────────────
    char value_str[16];
    snprintf(value_str, sizeof(value_str), "%d", widget->value);

    if (correct_font) {
        SDL_Surface* value_surface = TTF_RenderUTF8_Blended(correct_font, value_str, widget->color);
        if (value_surface) {
            SDL_Texture* value_texture = SDL_CreateTextureFromSurface(renderer, value_surface);
            if (value_texture) {
                SDL_Rect value_rect = {
                    widget_screen_x + value_x_offset,
                    widget_screen_y + widget->local_value_y,
                    value_surface->w,
                    value_surface->h
                };
                SDL_RenderCopy(renderer, value_texture, NULL, &value_rect);
                SDL_DestroyTexture(value_texture);
            }
            SDL_FreeSurface(value_surface);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════
void handle_config_widget_events(ConfigWidget* widget, SDL_Event* event,
                                 int offset_x, int offset_y) {
    if (!widget || !event) return;

    int widget_screen_x = offset_x + widget->base.x;
    int widget_screen_y = offset_y + widget->base.y;

    if (event->type == SDL_MOUSEMOTION) {
        int mx = event->motion.x;
        int my = event->motion.y;

        widget->base.hovered = widget_contains_point(&widget->base, mx, my, offset_x, offset_y);

        int arrows_screen_x = widget_screen_x + widget->local_arrows_x;
        int arrows_screen_y = widget_screen_y + widget->local_arrows_y;

        int up_y = arrows_screen_y - widget->arrow_size / 2;
        widget->up_arrow_hovered = (mx >= arrows_screen_x - widget->arrow_size / 2 &&
                                    mx <= arrows_screen_x + widget->arrow_size / 2 &&
                                    my >= up_y &&
                                    my <= up_y + widget->arrow_size);

        int down_y = arrows_screen_y + widget->arrow_size / 2 + widget->base_espace_entre_fleches;
        widget->down_arrow_hovered = (mx >= arrows_screen_x - widget->arrow_size / 2 &&
                                      mx <= arrows_screen_x + widget->arrow_size / 2 &&
                                      my >= down_y &&
                                      my <= down_y + widget->arrow_size);
    }
    else if (event->type == SDL_MOUSEWHEEL) {
        // Support de la molette
        if (widget->base.hovered) {
            if (event->wheel.y > 0) {
                if (widget->value < widget->max_value) {
                    widget->value += widget->increment;
                    if (widget->value > widget->max_value) widget->value = widget->max_value;
                    if (widget->on_value_changed) widget->on_value_changed(widget->value);
                }
            } else if (event->wheel.y < 0) {
                if (widget->value > widget->min_value) {
                    widget->value -= widget->increment;
                    if (widget->value < widget->min_value) widget->value = widget->min_value;
                    if (widget->on_value_changed) widget->on_value_changed(widget->value);
                }
            }
        }
    }
    else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        if (widget->up_arrow_hovered && widget->value < widget->max_value) {
            widget->value += widget->increment;
            if (widget->value > widget->max_value) widget->value = widget->max_value;
            if (widget->on_value_changed) widget->on_value_changed(widget->value);
        }

        if (widget->down_arrow_hovered && widget->value > widget->min_value) {
            widget->value -= widget->increment;
            if (widget->value < widget->min_value) widget->value = widget->min_value;
            if (widget->on_value_changed) widget->on_value_changed(widget->value);
        }
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

    debug_subsection("Rescale CONFIG (intelligent)");
    debug_printf("  Widget : %s\n", widget->option_name);
    debug_printf("  Ratio : %.2f\n", panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 1. SCALER LA BASE (position du widget)
    // ─────────────────────────────────────────────────────────────────────────
    rescale_widget_base(&widget->base, panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 2. CALCULER LA NOUVELLE TAILLE DE POLICE
    // ─────────────────────────────────────────────────────────────────────────
    int new_text_size = (int)(widget->base_text_size * panel_ratio);
    widget->current_text_size = new_text_size;  // Sera ajusté par get_font_for_size

    debug_printf("  Police : %dpx → %dpx\n", widget->base_text_size, new_text_size);

    // ─────────────────────────────────────────────────────────────────────────
    // 3. OBTENIR LA POLICE À CETTE TAILLE (avec minimum garanti)
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
    int scaled_espace_fleches = (int)(widget->base_espace_apres_fleches * panel_ratio);

    // ─────────────────────────────────────────────────────────────────────────
    // 6. RECALCULER LES OFFSETS AVEC LES VRAIES DIMENSIONS
    // ─────────────────────────────────────────────────────────────────────────
    widget->local_arrows_x = new_text_width + scaled_espace_texte;

    widget->arrow_size = (int)(widget->base_arrow_size * panel_ratio);
    if (widget->arrow_size < 8) widget->arrow_size = 8;

    widget->local_value_x = widget->local_arrows_x + widget->arrow_size + scaled_espace_fleches;

    int total_arrows_height = widget->arrow_size * 2 + widget->base_espace_entre_fleches;
    widget->local_arrows_y = (widget->text_height - total_arrows_height) / 2 + widget->arrow_size / 2;

    debug_printf("  ✓ Offsets : texte@%d, flèches@%d, valeur@%d\n",
                 widget->local_text_x, widget->local_arrows_x, widget->local_value_x);
    debug_blank_line();
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION DU WIDGET
// ════════════════════════════════════════════════════════════════════════════
void free_config_widget(ConfigWidget* widget) {
    if (!widget) return;
    free(widget);
}
