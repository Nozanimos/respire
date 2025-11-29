// SPDX-License-Identifier: GPL-3.0-or-later
// stats_panel.h
#ifndef __STATS_PANEL_H__
#define __STATS_PANEL_H__

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <time.h>

// ════════════════════════════════════════════════════════════════════════
// STRUCTURES DE DONNÉES STATISTIQUES
// ════════════════════════════════════════════════════════════════════════

// Une entrée d'exercice (sauvegardée en binaire)
typedef struct {
    time_t timestamp;           // Date et heure de l'exercice
    int session_count;          // Nombre de sessions dans cet exercice
    float* session_times;       // Temps de chaque session (en secondes)
} ExerciseEntry;

// Collection d'exercices (chargée depuis les fichiers)
typedef struct {
    ExerciseEntry* entries;     // Tableau d'exercices
    int count;                  // Nombre d'exercices chargés
    int capacity;               // Capacité du tableau
} ExerciseHistory;

// ════════════════════════════════════════════════════════════════════════
// STRUCTURE DU PANNEAU DE STATISTIQUES
// ════════════════════════════════════════════════════════════════════════

typedef enum {
    STATS_CLOSED,
    STATS_OPENING,
    STATS_OPEN,
    STATS_CLOSING
} StatsPanelState;

typedef struct {
    // État et animation
    StatsPanelState state;
    float animation_progress;   // 0.0 = fermé, 1.0 = ouvert
    float animation_speed;      // Vitesse d'animation

    // Dimensions et position
    int panel_width;
    int panel_height;
    int current_x;              // Position X actuelle (animée)
    int target_x;               // Position X cible
    int y;                      // Position Y (fixe)

    // Données de l'exercice en cours (à sauvegarder)
    float* current_session_times;
    int current_session_count;

    // Historique chargé depuis les fichiers
    ExerciseHistory history;

    // Textures et rendu
    SDL_Texture* graph_texture; // Texture du graphique
    bool needs_redraw;          // Flag pour recalcul du graphique

    // Interaction et navigation
    int scroll_offset;          // Offset de scroll (pour historique long)
    int selected_exercise_index; // Index de l'exercice sélectionné (-1 = dernier)
    bool is_already_saved;      // L'exercice actuel est déjà sauvegardé

    // Boutons
    SDL_Rect save_button;
    SDL_Rect cancel_button;
    SDL_Rect reset_button;
    bool save_hovered;
    bool cancel_hovered;
    bool reset_hovered;

} StatsPanel;

// ════════════════════════════════════════════════════════════════════════
// PROTOTYPES
// ════════════════════════════════════════════════════════════════════════

/**
 * Créer le panneau de statistiques
 * @param screen_width Largeur de l'écran
 * @param screen_height Hauteur de l'écran
 * @param session_times Tableau des temps de session de l'exercice actuel
 * @param session_count Nombre de sessions dans l'exercice actuel
 * @return Pointeur vers le panneau créé
 */
StatsPanel* create_stats_panel(int screen_width, int screen_height,
                                float* session_times, int session_count);

/**
 * Ouvrir le panneau (démarre l'animation)
 * @param panel Panneau à ouvrir
 */
void open_stats_panel(StatsPanel* panel);

/**
 * Fermer le panneau (démarre l'animation de fermeture)
 * @param panel Panneau à fermer
 */
void close_stats_panel(StatsPanel* panel);

/**
 * Mettre à jour l'animation du panneau
 * @param panel Panneau à mettre à jour
 * @param delta_time Temps écoulé depuis la dernière frame (secondes)
 */
void update_stats_panel(StatsPanel* panel, float delta_time);

/**
 * Dessiner le panneau
 * @param renderer Renderer SDL
 * @param panel Panneau à dessiner
 */
void render_stats_panel(SDL_Renderer* renderer, StatsPanel* panel);

/**
 * Gérer les événements du panneau
 * @param panel Panneau
 * @param event Événement SDL
 */
void handle_stats_panel_event(StatsPanel* panel, SDL_Event* event);

/**
 * Sauvegarder l'exercice actuel dans un fichier binaire
 * @param panel Panneau contenant les données
 * @return true si succès
 */
bool save_exercise_to_file(StatsPanel* panel);

/**
 * Charger l'historique depuis les fichiers binaires
 * @param history Structure à remplir
 * @return Nombre d'exercices chargés
 */
int load_exercise_history(ExerciseHistory* history);

/**
 * Réinitialiser l'historique (supprimer tous les fichiers)
 * @return true si succès
 */
bool reset_exercise_history(void);

/**
 * Libérer la mémoire du panneau
 * @param panel Panneau à détruire
 */
void destroy_stats_panel(StatsPanel* panel);

#endif
