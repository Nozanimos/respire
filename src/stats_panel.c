// SPDX-License-Identifier: GPL-3.0-or-later
// stats_panel.c - Panneau de statistiques avec graphique
#include "stats_panel.h"
#include "debug.h"
#include "button_widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cairo/cairo.h>
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════
// CONSTANTES
// ════════════════════════════════════════════════════════════════════════
#define STATS_DIR "../config/stats"
#define ANIMATION_SPEED 3.0f    // Vitesse d'animation (unités par seconde)
#define GRAPH_MARGIN 60         // Marge pour les axes et labels
#define GRAPH_DAYS 5            // Nombre de jours affichés
#define BUTTON_HEIGHT 40
#define BUTTON_MARGIN 10

// Couleurs arc-en-ciel (RGB)
static const SDL_Color RAINBOW_COLORS[] = {
    {255, 0, 0, 255},       // Rouge
    {255, 127, 0, 255},     // Orange
    {255, 255, 0, 255},     // Jaune
    {0, 255, 0, 255},       // Vert
    {0, 0, 255, 255},       // Bleu
    {75, 0, 130, 255},      // Indigo
    {148, 0, 211, 255}      // Violet
};
#define RAINBOW_COUNT 7

// ════════════════════════════════════════════════════════════════════════
// UTILITAIRES FICHIERS BINAIRES
// ════════════════════════════════════════════════════════════════════════

// Créer le dossier stats s'il n'existe pas
static void ensure_stats_dir(void) {
    struct stat st = {0};
    if (stat(STATS_DIR, &st) == -1) {
        mkdir(STATS_DIR, 0700);
    }
}

// Générer un nom de fichier unique : stats_YYYYMMDD_HHMMSS.bin
static void generate_filename(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    snprintf(buffer, size, "%s/stats_%04d%02d%02d_%02d%02d%02d.bin",
             STATS_DIR,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);
}

// ════════════════════════════════════════════════════════════════════════
// SAUVEGARDE/CHARGEMENT BINAIRE
// ════════════════════════════════════════════════════════════════════════

bool save_exercise_to_file(StatsPanel* panel) {
    if (!panel || !panel->current_session_times || panel->current_session_count == 0) {
        return false;
    }

    ensure_stats_dir();

    char filename[256];
    generate_filename(filename, sizeof(filename));

    FILE* file = fopen(filename, "wb");
    if (!file) {
        debug_printf("❌ Erreur ouverture fichier: %s\n", filename);
        return false;
    }

    // Écrire timestamp
    time_t now = time(NULL);
    fwrite(&now, sizeof(time_t), 1, file);

    // Écrire nombre de sessions
    fwrite(&panel->current_session_count, sizeof(int), 1, file);

    // Écrire les temps
    fwrite(panel->current_session_times, sizeof(float), panel->current_session_count, file);

    fclose(file);
    debug_printf("✅ Exercice sauvegardé: %s (%d sessions)\n", filename, panel->current_session_count);

    return true;
}

int load_exercise_history(ExerciseHistory* history) {
    if (!history) return 0;

    // Initialiser
    history->entries = NULL;
    history->count = 0;
    history->capacity = 10;
    history->entries = malloc(history->capacity * sizeof(ExerciseEntry));

    if (!history->entries) return 0;

    // Lire tous les fichiers .bin du dossier
    DIR* dir = opendir(STATS_DIR);
    if (!dir) return 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Vérifier extension .bin
        size_t len = strlen(entry->d_name);
        if (len < 4 || strcmp(entry->d_name + len - 4, ".bin") != 0) {
            continue;
        }

        // Construire le chemin complet
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", STATS_DIR, entry->d_name);

        // Lire le fichier
        FILE* file = fopen(filepath, "rb");
        if (!file) continue;

        // Réallouer si nécessaire
        if (history->count >= history->capacity) {
            history->capacity *= 2;
            ExerciseEntry* new_entries = realloc(history->entries,
                                                 history->capacity * sizeof(ExerciseEntry));
            if (!new_entries) {
                fclose(file);
                break;
            }
            history->entries = new_entries;
        }

        ExerciseEntry* e = &history->entries[history->count];

        // Lire timestamp
        if (fread(&e->timestamp, sizeof(time_t), 1, file) != 1) {
            fclose(file);
            continue;
        }

        // Lire nombre de sessions
        if (fread(&e->session_count, sizeof(int), 1, file) != 1) {
            fclose(file);
            continue;
        }

        // Lire les temps
        e->session_times = malloc(e->session_count * sizeof(float));
        if (!e->session_times) {
            fclose(file);
            continue;
        }

        if (fread(e->session_times, sizeof(float), e->session_count, file) != (size_t)e->session_count) {
            free(e->session_times);
            fclose(file);
            continue;
        }

        fclose(file);
        history->count++;
    }

    closedir(dir);
    debug_printf("✅ Historique chargé: %d exercices\n", history->count);

    return history->count;
}

bool reset_exercise_history(void) {
    DIR* dir = opendir(STATS_DIR);
    if (!dir) return true; // Pas de dossier = déjà vide

    struct dirent* entry;
    int deleted = 0;

    while ((entry = readdir(dir)) != NULL) {
        // Vérifier extension .bin
        size_t len = strlen(entry->d_name);
        if (len < 4 || strcmp(entry->d_name + len - 4, ".bin") != 0) {
            continue;
        }

        // Supprimer le fichier
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", STATS_DIR, entry->d_name);
        if (remove(filepath) == 0) {
            deleted++;
        }
    }

    closedir(dir);
    debug_printf("🗑️ Historique réinitialisé: %d fichiers supprimés\n", deleted);

    return true;
}

// ════════════════════════════════════════════════════════════════════════
// RENDU DU GRAPHIQUE AVEC CAIRO
// ════════════════════════════════════════════════════════════════════════

// Créer la texture du graphique
static SDL_Texture* create_graph_texture(SDL_Renderer* renderer, StatsPanel* panel,
                                         int width, int height) {
    // Créer surface Cairo
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // Fond blanc
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    // Zone du graphique (avec marges pour les axes)
    int graph_x = GRAPH_MARGIN;
    int graph_y = GRAPH_MARGIN;
    int graph_width = width - 2 * GRAPH_MARGIN;
    int graph_height = height - 2 * GRAPH_MARGIN - 80; // Espace pour la date et boutons

    // Trouver le temps maximum pour l'échelle Y
    float max_time = 60.0f; // Minimum 1 minute
    for (int i = 0; i < panel->history.count; i++) {
        for (int j = 0; j < panel->history.entries[i].session_count; j++) {
            if (panel->history.entries[i].session_times[j] > max_time) {
                max_time = panel->history.entries[i].session_times[j];
            }
        }
    }

    // Arrondir max_time à la minute supérieure
    int max_minutes = (int)ceil(max_time / 60.0f);

    // ═══ GRILLE HORIZONTALE (minutes) ═══
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8); // Gris clair
    cairo_set_line_width(cr, 1.0);
    double dash[] = {5.0, 5.0};
    cairo_set_dash(cr, dash, 2, 0);

    for (int m = 1; m <= max_minutes; m++) {
        double y = graph_y + graph_height - (m * graph_height / (double)max_minutes);
        cairo_move_to(cr, graph_x, y);
        cairo_line_to(cr, graph_x + graph_width, y);
        cairo_stroke(cr);

        // Label (minutes)
        char label[16];
        snprintf(label, sizeof(label), "%d min", m);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_font_size(cr, 12);
        cairo_text_extents_t extents;
        cairo_text_extents(cr, label, &extents);
        cairo_move_to(cr, graph_x - extents.width - 5, y + 4);
        cairo_show_text(cr, label);
        cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    }

    // ═══ GRILLE VERTICALE (jours) ═══
    double day_width = graph_width / (double)GRAPH_DAYS;
    for (int d = 0; d < GRAPH_DAYS; d++) {
        double x = graph_x + d * day_width;
        cairo_move_to(cr, x, graph_y);
        cairo_line_to(cr, x, graph_y + graph_height);
        cairo_stroke(cr);
    }

    // ═══ AXES PRINCIPAUX ═══
    cairo_set_dash(cr, NULL, 0, 0); // Ligne continue
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 2.0);

    // Axe X
    cairo_move_to(cr, graph_x, graph_y + graph_height);
    cairo_line_to(cr, graph_x + graph_width, graph_y + graph_height);
    cairo_stroke(cr);

    // Axe Y
    cairo_move_to(cr, graph_x, graph_y);
    cairo_line_to(cr, graph_x, graph_y + graph_height);
    cairo_stroke(cr);

    // ═══ DESSINER LES RECTANGLES (exercices) ═══
    // Grouper par jour
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int today_day = tm_now->tm_yday;

    for (int i = 0; i < panel->history.count; i++) {
        ExerciseEntry* entry = &panel->history.entries[i];
        struct tm* tm_entry = localtime(&entry->timestamp);
        int entry_day = tm_entry->tm_yday;

        // Calculer le jour (0 = aujourd'hui, 1 = hier, etc.)
        int day_offset = today_day - entry_day;
        if (day_offset < 0) day_offset = 0;
        if (day_offset >= GRAPH_DAYS) continue;

        // Position X du jour
        double day_x = graph_x + (GRAPH_DAYS - 1 - day_offset) * day_width;

        // Dessiner les rectangles (du plus grand au plus petit pour empilement)
        for (int j = entry->session_count - 1; j >= 0; j--) {
            float session_time = entry->session_times[j];
            double rect_height = (session_time / 60.0) * (graph_height / (double)max_minutes);
            double rect_width = day_width * 0.6; // 60% de la largeur du jour
            double rect_x = day_x + day_width * 0.2; // Centré

            // Couleur arc-en-ciel
            SDL_Color color = RAINBOW_COLORS[j % RAINBOW_COUNT];
            cairo_set_source_rgb(cr, color.r / 255.0, color.g / 255.0, color.b / 255.0);

            // Dessiner rectangle arrondi
            double radius = 5.0;
            double rect_y = graph_y + graph_height - rect_height;

            cairo_new_sub_path(cr);
            cairo_arc(cr, rect_x + radius, rect_y + radius, radius, M_PI, 3 * M_PI / 2);
            cairo_arc(cr, rect_x + rect_width - radius, rect_y + radius, radius, 3 * M_PI / 2, 0);
            cairo_arc(cr, rect_x + rect_width - radius, rect_y + rect_height - radius, radius, 0, M_PI / 2);
            cairo_arc(cr, rect_x + radius, rect_y + rect_height - radius, radius, M_PI / 2, M_PI);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
    }

    // ═══ LABELS DES JOURS ═══
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_font_size(cr, 14);

    for (int d = 0; d < GRAPH_DAYS; d++) {
        time_t day_time = now - (GRAPH_DAYS - 1 - d) * 24 * 3600;
        struct tm* tm_day = localtime(&day_time);

        char date_label[32];
        snprintf(date_label, sizeof(date_label), "%02d/%02d", tm_day->tm_mday, tm_day->tm_mon + 1);

        double x = graph_x + d * day_width + day_width / 2;
        cairo_text_extents_t extents;
        cairo_text_extents(cr, date_label, &extents);
        cairo_move_to(cr, x - extents.width / 2, graph_y + graph_height + 20);
        cairo_show_text(cr, date_label);
    }

    // ═══ DATE DU JOUR EN HAUT ═══
    char today_label[64];
    const char* days[] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
    const char* months[] = {"janvier", "février", "mars", "avril", "mai", "juin",
                            "juillet", "août", "septembre", "octobre", "novembre", "décembre"};
    snprintf(today_label, sizeof(today_label), "%s %d %s",
             days[tm_now->tm_wday], tm_now->tm_mday, months[tm_now->tm_mon]);

    cairo_set_font_size(cr, 18);
    cairo_text_extents_t title_extents;
    cairo_text_extents(cr, today_label, &title_extents);
    cairo_move_to(cr, (width - title_extents.width) / 2, 30);
    cairo_show_text(cr, today_label);

    // Finaliser
    cairo_surface_flush(surface);

    // Convertir en texture SDL
    SDL_Surface* sdl_surface = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_ARGB8888
    );

    if (sdl_surface) {
        memcpy(sdl_surface->pixels,
               cairo_image_surface_get_data(surface),
               height * cairo_image_surface_get_stride(surface));

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
        SDL_FreeSurface(sdl_surface);

        cairo_destroy(cr);
        cairo_surface_destroy(surface);

        return texture;
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return NULL;
}

// ════════════════════════════════════════════════════════════════════════
// CRÉATION ET DESTRUCTION
// ════════════════════════════════════════════════════════════════════════

StatsPanel* create_stats_panel(int screen_width, int screen_height,
                                float* session_times, int session_count) {
    StatsPanel* panel = malloc(sizeof(StatsPanel));
    if (!panel) return NULL;

    // État initial
    panel->state = STATS_CLOSED;
    panel->animation_progress = 0.0f;
    panel->animation_speed = ANIMATION_SPEED;

    // Dimensions (même largeur que settings_panel)
    panel->panel_width = screen_width < 600 ? screen_width : 500;
    if (panel->panel_width > screen_width * 0.8) {
        panel->panel_width = screen_width * 0.8;
    }
    panel->panel_height = screen_height;

    // Position (hors écran à gauche)
    panel->current_x = -panel->panel_width;
    panel->target_x = 0;
    panel->y = 0;

    // Copier les données de l'exercice actuel
    panel->current_session_count = session_count;
    panel->current_session_times = malloc(session_count * sizeof(float));
    if (panel->current_session_times) {
        memcpy(panel->current_session_times, session_times, session_count * sizeof(float));
    }

    // Charger l'historique
    load_exercise_history(&panel->history);

    // Initialiser textures
    panel->graph_texture = NULL;
    panel->needs_redraw = true;

    // Boutons (en bas du panneau)
    int button_width = (panel->panel_width - 4 * BUTTON_MARGIN) / 3;
    int button_y = panel->panel_height - BUTTON_HEIGHT - BUTTON_MARGIN;

    panel->save_button = (SDL_Rect){BUTTON_MARGIN, button_y, button_width, BUTTON_HEIGHT};
    panel->cancel_button = (SDL_Rect){BUTTON_MARGIN * 2 + button_width, button_y, button_width, BUTTON_HEIGHT};
    panel->reset_button = (SDL_Rect){BUTTON_MARGIN * 3 + button_width * 2, button_y, button_width, BUTTON_HEIGHT};

    panel->save_hovered = false;
    panel->cancel_hovered = false;
    panel->reset_hovered = false;

    debug_printf("✅ Panneau stats créé (%dx%d, %d exercices dans l'historique)\n",
                 panel->panel_width, panel->panel_height, panel->history.count);

    return panel;
}

void destroy_stats_panel(StatsPanel* panel) {
    if (!panel) return;

    if (panel->current_session_times) {
        free(panel->current_session_times);
    }

    if (panel->graph_texture) {
        SDL_DestroyTexture(panel->graph_texture);
    }

    // Libérer l'historique
    for (int i = 0; i < panel->history.count; i++) {
        if (panel->history.entries[i].session_times) {
            free(panel->history.entries[i].session_times);
        }
    }
    if (panel->history.entries) {
        free(panel->history.entries);
    }

    free(panel);
    debug_printf("🗑️ Panneau stats détruit\n");
}

// ════════════════════════════════════════════════════════════════════════
// ANIMATION ET MISE À JOUR
// ════════════════════════════════════════════════════════════════════════

void open_stats_panel(StatsPanel* panel) {
    if (!panel) return;
    panel->state = STATS_OPENING;
    debug_printf("📊 Ouverture panneau stats\n");
}

void close_stats_panel(StatsPanel* panel) {
    if (!panel) return;
    panel->state = STATS_CLOSING;
    debug_printf("📊 Fermeture panneau stats\n");
}

void update_stats_panel(StatsPanel* panel, float delta_time) {
    if (!panel) return;

    if (panel->state == STATS_OPENING) {
        panel->animation_progress += panel->animation_speed * delta_time;
        if (panel->animation_progress >= 1.0f) {
            panel->animation_progress = 1.0f;
            panel->state = STATS_OPEN;
        }
        panel->current_x = -panel->panel_width + (int)(panel->animation_progress * panel->panel_width);
    }
    else if (panel->state == STATS_CLOSING) {
        panel->animation_progress -= panel->animation_speed * delta_time;
        if (panel->animation_progress <= 0.0f) {
            panel->animation_progress = 0.0f;
            panel->state = STATS_CLOSED;
        }
        panel->current_x = -panel->panel_width + (int)(panel->animation_progress * panel->panel_width);
    }
}

// ════════════════════════════════════════════════════════════════════════
// RENDU
// ════════════════════════════════════════════════════════════════════════

void render_stats_panel(SDL_Renderer* renderer, StatsPanel* panel) {
    if (!panel || panel->state == STATS_CLOSED) return;

    // Recréer le graphique si nécessaire
    if (panel->needs_redraw || !panel->graph_texture) {
        if (panel->graph_texture) {
            SDL_DestroyTexture(panel->graph_texture);
        }
        panel->graph_texture = create_graph_texture(renderer, panel,
                                                    panel->panel_width, panel->panel_height);
        panel->needs_redraw = false;
    }

    // Dessiner le graphique
    if (panel->graph_texture) {
        SDL_Rect dest = {panel->current_x, panel->y, panel->panel_width, panel->panel_height};
        SDL_RenderCopy(renderer, panel->graph_texture, NULL, &dest);
    }

    // Dessiner les boutons (rounded rectangles)
    SDL_Rect save_btn = {panel->current_x + panel->save_button.x, panel->save_button.y,
                         panel->save_button.w, panel->save_button.h};
    SDL_Rect cancel_btn = {panel->current_x + panel->cancel_button.x, panel->cancel_button.y,
                           panel->cancel_button.w, panel->cancel_button.h};
    SDL_Rect reset_btn = {panel->current_x + panel->reset_button.x, panel->reset_button.y,
                          panel->reset_button.w, panel->reset_button.h};

    // Enregistrer (vert ou vert clair si survolé)
    SDL_Color save_color = panel->save_hovered ? (SDL_Color){100, 255, 100, 255} : (SDL_Color){50, 200, 50, 255};
    roundedBoxRGBA(renderer, save_btn.x, save_btn.y, save_btn.x + save_btn.w, save_btn.y + save_btn.h,
                   8, save_color.r, save_color.g, save_color.b, save_color.a);

    // Annuler (gris ou gris clair si survolé)
    SDL_Color cancel_color = panel->cancel_hovered ? (SDL_Color){180, 180, 180, 255} : (SDL_Color){120, 120, 120, 255};
    roundedBoxRGBA(renderer, cancel_btn.x, cancel_btn.y, cancel_btn.x + cancel_btn.w, cancel_btn.y + cancel_btn.h,
                   8, cancel_color.r, cancel_color.g, cancel_color.b, cancel_color.a);

    // Réinitialiser (rouge ou rouge clair si survolé)
    SDL_Color reset_color = panel->reset_hovered ? (SDL_Color){255, 100, 100, 255} : (SDL_Color){200, 50, 50, 255};
    roundedBoxRGBA(renderer, reset_btn.x, reset_btn.y, reset_btn.x + reset_btn.w, reset_btn.y + reset_btn.h,
                   8, reset_color.r, reset_color.g, reset_color.b, reset_color.a);

    // TODO: Ajouter les labels des boutons avec TTF
}

// ════════════════════════════════════════════════════════════════════════
// GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════

void handle_stats_panel_event(StatsPanel* panel, SDL_Event* event) {
    if (!panel || panel->state != STATS_OPEN) return;

    if (event->type == SDL_MOUSEMOTION) {
        int mx = event->motion.x;
        int my = event->motion.y;

        // Ajuster les zones de clic avec current_x
        SDL_Rect save_zone = {panel->current_x + panel->save_button.x, panel->save_button.y,
                              panel->save_button.w, panel->save_button.h};
        SDL_Rect cancel_zone = {panel->current_x + panel->cancel_button.x, panel->cancel_button.y,
                                panel->cancel_button.w, panel->cancel_button.h};
        SDL_Rect reset_zone = {panel->current_x + panel->reset_button.x, panel->reset_button.y,
                               panel->reset_button.w, panel->reset_button.h};

        panel->save_hovered = (mx >= save_zone.x && mx < save_zone.x + save_zone.w &&
                               my >= save_zone.y && my < save_zone.y + save_zone.h);
        panel->cancel_hovered = (mx >= cancel_zone.x && mx < cancel_zone.x + cancel_zone.w &&
                                 my >= cancel_zone.y && my < cancel_zone.y + cancel_zone.h);
        panel->reset_hovered = (mx >= reset_zone.x && mx < reset_zone.x + reset_zone.w &&
                                my >= reset_zone.y && my < reset_zone.y + reset_zone.h);
    }
    else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        if (panel->save_hovered) {
            // Sauvegarder
            if (save_exercise_to_file(panel)) {
                // Recharger l'historique et redessiner
                for (int i = 0; i < panel->history.count; i++) {
                    if (panel->history.entries[i].session_times) {
                        free(panel->history.entries[i].session_times);
                    }
                }
                if (panel->history.entries) {
                    free(panel->history.entries);
                }
                load_exercise_history(&panel->history);
                panel->needs_redraw = true;
            }
        }
        else if (panel->cancel_hovered) {
            // Fermer sans sauvegarder
            close_stats_panel(panel);
        }
        else if (panel->reset_hovered) {
            // Réinitialiser l'historique
            if (reset_exercise_history()) {
                // Vider l'historique en mémoire
                for (int i = 0; i < panel->history.count; i++) {
                    if (panel->history.entries[i].session_times) {
                        free(panel->history.entries[i].session_times);
                    }
                }
                if (panel->history.entries) {
                    free(panel->history.entries);
                }
                panel->history.entries = NULL;
                panel->history.count = 0;
                panel->history.capacity = 0;
                panel->needs_redraw = true;
            }
        }
    }
}
