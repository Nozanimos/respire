#ifndef WHM_H
#define WHM_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "../technique_instance.h"
#include "../../core/timer.h"
#include "../../core/counter.h"
#include "../../core/chronometre.h"
#include "../../core/session_card.h"
#include "../../core/precompute_list.h"
#include "../../core/geometry.h"
#include "whm_session_controller.h"

/**
 * Structure de données spécifique à la méthode Wim Hof
 * Contient tous les états des phases et les composants
 */
typedef struct {
    // ════════════════════════════════════════════════════════════════════════
    // PHASES DE LA TECHNIQUE WIM HOF (machine à états)
    // ════════════════════════════════════════════════════════════════════════
    bool timer_phase;           // Phase timer avant démarrage session
    bool session_card_phase;    // Phase carte de session animée
    bool counter_phase;         // Phase compteur de respirations
    bool reappear_phase;        // Phase de réapparition douce de l'hexagone
    bool chrono_phase;          // Phase chronomètre actif (méditation)
    bool inspiration_phase;     // Phase d'inspiration (scale_min → scale_max)
    bool retention_phase;       // Phase de rétention (poumons pleins)

    // ════════════════════════════════════════════════════════════════════════
    // COMPOSANTS DE LA TECHNIQUE
    // ════════════════════════════════════════════════════════════════════════
    TimerState* session_timer;      // Timer avant démarrage session
    CounterState* breath_counter;   // Compteur de respirations
    StopwatchState* session_stopwatch;  // Chronomètre pour mesurer le temps de méditation
    TimerState* retention_timer;    // Timer de rétention (15 secondes poumons pleins)
    SessionCardState* session_card; // Carte affichant le numéro de session

    // ════════════════════════════════════════════════════════════════════════
    // CONTRÔLEUR DE SESSION (nouveau système)
    // ════════════════════════════════════════════════════════════════════════
    SessionController* session_controller;  // Gère le flow et les transitions de sessions

    // ════════════════════════════════════════════════════════════════════════
    // RÉFÉRENCES AUX RESSOURCES PARTAGÉES (core)
    // ════════════════════════════════════════════════════════════════════════
    SDL_Renderer* renderer;     // Référence au renderer SDL (pour session_card_reset)
    HexagoneList* hexagones;    // Référence aux hexagones (créés par le core)
    int screen_width;           // Pour calculs de position
    int screen_height;
    float scale_factor;         // Facteur d'échelle responsive

    // ════════════════════════════════════════════════════════════════════════
    // FPS ADAPTATIF
    // ════════════════════════════════════════════════════════════════════════
    bool needs_high_fps;        // true si animation active (60 FPS), false sinon (15 FPS)

} WHMData;

/**
 * Créer une instance de la technique Wim Hof
 * @param renderer Le renderer SDL (partagé avec le core)
 * @return L'instance de la technique WHM
 */
TechniqueInstance* whm_create(SDL_Renderer* renderer);

/**
 * Définir les hexagones (appelé par le core après création)
 * @param instance L'instance WHM
 * @param hexagones La liste des hexagones créée par le core
 */
void whm_set_hexagones(TechniqueInstance* instance, HexagoneList* hexagones);

/**
 * Définir les dimensions de l'écran (pour calculs)
 * @param instance L'instance WHM
 * @param width Largeur écran
 * @param height Hauteur écran
 * @param scale_factor Facteur d'échelle responsive
 */
void whm_set_screen_info(TechniqueInstance* instance, int width, int height, float scale_factor);

/**
 * Créer le compteur de respirations (nécessite le renderer)
 * @param instance L'instance WHM
 * @param renderer Le renderer SDL
 */
void whm_create_counter(TechniqueInstance* instance, SDL_Renderer* renderer);

#endif // WHM_H
