// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __SESSION_CARD_H__
#define __SESSION_CARD_H__

#include <stdbool.h>
#include <SDL2/SDL.h>

// PHASES D'ANIMATION DE LA CARTE
typedef enum {
    CARD_ENTERING,      // Carte entre depuis la gauche (0-1s)
    CARD_PAUSED,        // Carte au centre (1-4s)
    CARD_EXITING,       // Carte sort vers la droite (4-5s)
    CARD_FINISHED       // Animation terminée
} SessionCardPhase;

// STRUCTURE SESSION CARD STATE
// Gère l'affichage animé de la carte de session
typedef struct {
    // État de l'animation
    SessionCardPhase phase;
    float elapsed_time;         // Temps écoulé depuis le début de la phase actuelle (en secondes)

    // Timings (en secondes)
    float enter_duration;       // Durée de l'entrée (défaut: 1s)
    float pause_duration;       // Durée de la pause au centre (défaut: 3s)
    float exit_duration;        // Durée de la sortie (défaut: 1s)

    // Position et dimensions
    int card_width;             // Largeur de la carte
    int card_height;            // Hauteur de la carte
    int current_x;              // Position X actuelle (animée)
    int current_y;              // Position Y (fixe, centrée verticalement)
    int center_x;               // Position X du centre de l'écran
    int screen_width;           // Largeur de l'écran
    float scale_factor;         // 🆕 Facteur d'échelle actuel (pour recalcul lors du reset)

    // Données à afficher
    int session_number;         // Numéro de session (1, 2, 3...)

    // Textures
    SDL_Texture* card_texture;  // Texture de la carte complète (background + texte)

    // Police et couleur
    const char* font_path;
    SDL_Color text_color;

} SessionCardState;

// PROTOTYPES

/**
 * Créer et initialiser une nouvelle carte de session
 * @param session_number Numéro de session à afficher
 * @param screen_width Largeur de l'écran
 * @param screen_height Hauteur de l'écran
 * @param font_path Chemin vers la police TTF
 * @param scale_factor Facteur d'échelle de la fenêtre (responsive)
 * @return Pointeur vers le SessionCardState créé, NULL si erreur
 */
SessionCardState* session_card_create(int session_number, int screen_width,
                                      int screen_height, const char* font_path,
                                      float scale_factor);

/**
 * Démarrer l'animation de la carte
 * @param card Pointeur vers la carte à démarrer
 */
void session_card_start(SessionCardState* card);

/**
 * Mettre à jour l'animation de la carte
 * @param card Pointeur vers la carte
 * @param delta_time Temps écoulé depuis la dernière frame (en secondes)
 * @return true si l'animation est toujours active, false si terminée
 */
bool session_card_update(SessionCardState* card, float delta_time);

/**
 * Dessiner la carte à sa position actuelle
 * @param card Pointeur vers la carte
 * @param renderer Renderer SDL2
 */
void session_card_render(SessionCardState* card, SDL_Renderer* renderer);

/**
 * Mettre à jour les dimensions d'écran et repositionner la carte
 * @param card Pointeur vers la carte
 * @param new_screen_width Nouvelle largeur de l'écran
 * @param new_screen_height Nouvelle hauteur de l'écran
 * @param scale_factor Nouveau facteur d'échelle
 */
void session_card_update_screen_size(SessionCardState* card, int new_screen_width,
                                      int new_screen_height, float scale_factor);

/**
 * Réinitialiser la carte pour une nouvelle session
 * @param card Pointeur vers la carte
 * @param session_number Nouveau numéro de session
 * @param renderer Renderer SDL2 (pour recréer la texture)
 */
void session_card_reset(SessionCardState* card, int session_number, SDL_Renderer* renderer);

/**
 * Vérifier si l'animation est terminée
 * @param card Pointeur vers la carte
 * @return true si l'animation est terminée
 */
bool session_card_is_finished(SessionCardState* card);

/**
 * Libérer la mémoire de la carte
 * @param card Pointeur vers la carte à détruire
 */
void session_card_destroy(SessionCardState* card);

#endif
