// SPDX-License-Identifier: GPL-3.0-or-later
// renderer.h
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "precompute_list.h"
#include "geometry.h"
#include "config.h"
#include "settings_panel.h"
#include "json_editor/json_editor.h"
#include "timer.h"
#include "counter.h"
#include "chronometre.h"
#include "session_card.h"
#include "stats_panel.h"

// Structure qui contient TOUT l'état de l'application graphique
typedef struct {
    SDL_Window* window;           // Fenêtre principale
    SDL_Renderer* renderer;       // Moteur de rendu
    SDL_Texture* background;      // Texture de fond
    int screen_width;             // Largeur écran
    int screen_height;            // Hauteur écran

    // ════════════════════════════════════════════════════════════════════════
    // SYSTÈME D'ÉCHELLE RESPONSIVE
    // ════════════════════════════════════════════════════════════════════════
    // Facteur d'échelle calculé automatiquement selon la taille d'écran
    // Référence : 1280x720 (HD Ready) = facteur 1.0
    // Exemples :
    //   - Téléphone 360x640  → scale ≈ 0.28
    //   - Tablette  768x1024 → scale ≈ 0.60
    //   - Desktop   1920x1080 → scale ≈ 1.50
    //   - 4K        3840x2160 → scale = 3.00 (plafonné)
    // ════════════════════════════════════════════════════════════════════════
    float scale_factor;

    // État de l'application
    HexagoneList* hexagones;
    SettingsPanel* settings_panel;
    StatsPanel* stats_panel;
    JsonEditor* json_editor;
    TimerState* session_timer;      // Timer avant démarrage session

    // 🆕 Compteur de respirations (démarre après le timer)
    CounterState* breath_counter;
    bool counter_phase;
    bool timer_phase;               // true = phase timer, false = phase animation

    // 🆕 Chronomètre (démarre après la session de respiration)
    StopwatchState* session_stopwatch;  // Chronomètre pour mesurer le temps de méditation
    bool reappear_phase;            // Phase de réapparition douce de l'hexagone (scale_max/2 → scale_max)
    bool chrono_phase;              // Phase chronomètre actif (hexagones figés à scale_max)

    // 🆕 Phase inspiration + rétention (après le chronomètre)
    bool inspiration_phase;         // Phase d'inspiration (scale_min → scale_max)
    TimerState* retention_timer;    // Timer de rétention (15 secondes poumons pleins)
    bool retention_phase;           // Phase de rétention (poumons pleins, timer actif)

    // 🆕 Carte de session animée (entre timer_phase et counter_phase)
    SessionCardState* session_card;  // Carte affichant le numéro de session
    bool session_card_phase;         // Phase carte de session active

    // 🆕 Gestion des sessions multiples
    int current_session;             // Numéro de session en cours (1, 2, 3...)
    int total_sessions;              // Nombre total de sessions configurées

    // 🆕 Stockage des temps de session (pour statistiques futures)
    float* session_times;           // Tableau dynamique des temps de chaque session (en secondes)
    int session_count;              // Nombre de sessions effectuées
    int session_capacity;           // Capacité actuelle du tableau (pour réallocation)

    // 🆕 Écran d'accueil (Technique Wim Hof)
    bool waiting_to_start;           // true = écran d'accueil, false = animation lancée
    SDL_Texture* wim_image;          // Image wim.png (250x250)
    SDL_Texture* wim_title;          // Titre "Technique\nWim Hof" en Cairo

    // 🆕 SYSTÈME D'INSTANCES DE TECHNIQUES
    void* active_technique;          // Instance de technique active (TechniqueInstance*)

    AppConfig config;
    bool is_running;

    // ════════════════════════════════════════════════════════════════════════
    // SYSTÈME FPS ADAPTATIF
    // ════════════════════════════════════════════════════════════════════════
    Uint32 last_interaction_time;    // Timestamp de la dernière interaction utilisateur
    bool editor_has_focus;           // true si le JSON editor a le focus
    Uint32 last_editor_event;        // Timestamp du dernier événement dans l'éditeur

} AppState;

// Prototypes
bool initialize_app(AppState* app, const char* title, const char* image_path);
void render_hexagones(AppState* app, HexagoneList* hex_list);
void cleanup_app(AppState* app);
void handle_app_events(AppState* app, SDL_Event* event);
void update_app(AppState* app, float delta_time);
void render_app(AppState* app);
void regulate_fps(Uint32 frame_start);

// Fonctions d'échelle responsive
float calculate_scale_factor(int width, int height);
int scale_value(int value, float scale);
int calculate_panel_width(int screen_width, float scale);

// Système FPS adaptatif
bool should_use_high_fps(AppState* app);

#endif
