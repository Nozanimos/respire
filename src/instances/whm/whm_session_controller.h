#ifndef WHM_SESSION_CONTROLLER_H
#define WHM_SESSION_CONTROLLER_H

#include <stdbool.h>
#include "whm_retention_config.h"

/**
 * Étapes d'une session Wim Hof
 * Chaque session passe par ces étapes dans l'ordre
 */
typedef enum {
    SESSION_STAGE_PAUSE,           // Pause avant respiration (timer initial, uniquement session 1)
    SESSION_STAGE_CARD,            // Affichage carte session animée
    SESSION_STAGE_BREATHING,       // Compteur de respirations
    SESSION_STAGE_REAPPEAR,        // Réapparition douce de l'hexagone
    SESSION_STAGE_MEDITATION,      // Chronomètre méditation (apnée poumons vides)
    SESSION_STAGE_RETENTION_ANIM,  // Animation vers position de rétention (poumons pleins/vides)
    SESSION_STAGE_RETENTION_HOLD,  // Rétention 15s (poumons pleins ou vides selon pattern)
    SESSION_STAGE_COMPLETE,        // Session terminée (attend passage à session suivante)
    SESSION_STAGE_ALL_DONE,        // Toutes les sessions terminées
} SessionStage;

/**
 * Contrôleur de session
 * Gère le flow complet des sessions et les transitions
 */
typedef struct {
    // Gestion des sessions
    int current_session;          // Numéro de session actuelle (1-based)
    int total_sessions;           // Nombre total de sessions à effectuer
    SessionStage current_stage;   // Étape actuelle dans la session

    // Configuration de rétention
    RetentionConfig retention_config;

    // Historique des temps de méditation
    float* session_times;         // Tableau des temps de chaque session (en secondes)
    int session_count;            // Nombre de sessions enregistrées
    int session_capacity;         // Capacité du tableau

} SessionController;

/**
 * Créer un contrôleur de session
 * @param total_sessions Nombre total de sessions
 * @param retention_config Configuration de rétention
 * @return Contrôleur de session nouvellement créé
 */
SessionController* session_controller_create(int total_sessions, RetentionConfig retention_config);

/**
 * Passer à l'étape suivante
 * @param ctrl Contrôleur de session
 */
void session_controller_advance_stage(SessionController* ctrl);

/**
 * Passer à la session suivante (réinitialise l'étape à SESSION_STAGE_CARD)
 * @param ctrl Contrôleur de session
 */
void session_controller_next_session(SessionController* ctrl);

/**
 * Vérifier si c'est la dernière session
 * @param ctrl Contrôleur de session
 * @return true si c'est la dernière session
 */
bool session_controller_is_last_session(const SessionController* ctrl);

/**
 * Vérifier si toutes les sessions sont terminées
 * @param ctrl Contrôleur de session
 * @return true si toutes les sessions sont terminées
 */
bool session_controller_is_all_done(const SessionController* ctrl);

/**
 * Déterminer si la session actuelle doit utiliser poumons vides
 * @param ctrl Contrôleur de session
 * @return true si poumons vides, false si poumons pleins
 */
bool session_controller_should_use_empty_lungs(const SessionController* ctrl);

/**
 * Enregistrer le temps de méditation de la session actuelle
 * @param ctrl Contrôleur de session
 * @param time Temps en secondes
 */
void session_controller_record_time(SessionController* ctrl, float time);

/**
 * Obtenir le nom de l'étape actuelle (pour debug)
 * @param stage Étape
 * @return Nom de l'étape
 */
const char* session_stage_get_name(SessionStage stage);

/**
 * Détruire le contrôleur de session
 * @param ctrl Contrôleur de session
 */
void session_controller_destroy(SessionController* ctrl);

#endif // WHM_SESSION_CONTROLLER_H
