// SPDX-License-Identifier: GPL-3.0-or-later
#include "whm_session_controller.h"
#include <stdlib.h>
#include <string.h>

/**
 * Créer un contrôleur de session
 */
SessionController* session_controller_create(int total_sessions, RetentionConfig retention_config) {
    SessionController* ctrl = malloc(sizeof(SessionController));
    if (!ctrl) {
        return NULL;
    }

    ctrl->current_session = 1;
    ctrl->total_sessions = total_sessions;
    ctrl->current_stage = SESSION_STAGE_PAUSE;  // Commence par la pause initiale
    ctrl->retention_config = retention_config;

    // Allouer le tableau pour stocker les temps de méditation
    ctrl->session_capacity = total_sessions;
    ctrl->session_times = calloc(ctrl->session_capacity, sizeof(float));
    ctrl->session_count = 0;

    if (!ctrl->session_times) {
        free(ctrl);
        return NULL;
    }

    return ctrl;
}

/**
 * Passer à l'étape suivante
 */
void session_controller_advance_stage(SessionController* ctrl) {
    if (!ctrl) return;

    switch (ctrl->current_stage) {
        case SESSION_STAGE_PAUSE:
            ctrl->current_stage = SESSION_STAGE_CARD;
            break;

        case SESSION_STAGE_CARD:
            ctrl->current_stage = SESSION_STAGE_BREATHING;
            break;

        case SESSION_STAGE_BREATHING:
            ctrl->current_stage = SESSION_STAGE_REAPPEAR;
            break;

        case SESSION_STAGE_REAPPEAR:
            ctrl->current_stage = SESSION_STAGE_MEDITATION;
            break;

        case SESSION_STAGE_MEDITATION:
            ctrl->current_stage = SESSION_STAGE_RETENTION_ANIM;
            break;

        case SESSION_STAGE_RETENTION_ANIM:
            ctrl->current_stage = SESSION_STAGE_RETENTION_HOLD;
            break;

        case SESSION_STAGE_RETENTION_HOLD:
            ctrl->current_stage = SESSION_STAGE_COMPLETE;
            break;

        case SESSION_STAGE_COMPLETE:
            // Reste en COMPLETE jusqu'à appel de session_controller_next_session()
            break;

        case SESSION_STAGE_ALL_DONE:
            // Terminé, ne fait rien
            break;
    }
}

/**
 * Passer à la session suivante
 */
void session_controller_next_session(SessionController* ctrl) {
    if (!ctrl) return;

    if (ctrl->current_session < ctrl->total_sessions) {
        // Passer à la session suivante
        ctrl->current_session++;
        ctrl->current_stage = SESSION_STAGE_CARD;  // Les sessions suivantes commencent par la carte
    } else {
        // Toutes les sessions sont terminées
        ctrl->current_stage = SESSION_STAGE_ALL_DONE;
    }
}

/**
 * Vérifier si c'est la dernière session
 */
bool session_controller_is_last_session(const SessionController* ctrl) {
    if (!ctrl) return false;
    return ctrl->current_session >= ctrl->total_sessions;
}

/**
 * Vérifier si toutes les sessions sont terminées
 */
bool session_controller_is_all_done(const SessionController* ctrl) {
    if (!ctrl) return true;
    return ctrl->current_stage == SESSION_STAGE_ALL_DONE;
}

/**
 * Déterminer si la session actuelle doit utiliser poumons vides
 */
bool session_controller_should_use_empty_lungs(const SessionController* ctrl) {
    if (!ctrl) return true;
    return retention_config_should_use_empty_lungs(&ctrl->retention_config, ctrl->current_session);
}

/**
 * Enregistrer le temps de méditation de la session actuelle
 */
void session_controller_record_time(SessionController* ctrl, float time) {
    if (!ctrl || !ctrl->session_times) return;

    // Agrandir le tableau si nécessaire
    if (ctrl->session_count >= ctrl->session_capacity) {
        int new_capacity = ctrl->session_capacity * 2;
        float* new_times = realloc(ctrl->session_times, new_capacity * sizeof(float));
        if (!new_times) {
            return;  // Échec allocation, on abandonne
        }
        ctrl->session_times = new_times;
        ctrl->session_capacity = new_capacity;
    }

    // Enregistrer le temps
    ctrl->session_times[ctrl->session_count] = time;
    ctrl->session_count++;
}

/**
 * Obtenir le nom de l'étape actuelle (pour debug)
 */
const char* session_stage_get_name(SessionStage stage) {
    switch (stage) {
        case SESSION_STAGE_PAUSE:
            return "PAUSE";
        case SESSION_STAGE_CARD:
            return "CARD";
        case SESSION_STAGE_BREATHING:
            return "BREATHING";
        case SESSION_STAGE_REAPPEAR:
            return "REAPPEAR";
        case SESSION_STAGE_MEDITATION:
            return "MEDITATION";
        case SESSION_STAGE_RETENTION_ANIM:
            return "RETENTION_ANIM";
        case SESSION_STAGE_RETENTION_HOLD:
            return "RETENTION_HOLD";
        case SESSION_STAGE_COMPLETE:
            return "COMPLETE";
        case SESSION_STAGE_ALL_DONE:
            return "ALL_DONE";
        default:
            return "UNKNOWN";
    }
}

/**
 * Détruire le contrôleur de session
 */
void session_controller_destroy(SessionController* ctrl) {
    if (!ctrl) return;

    if (ctrl->session_times) {
        free(ctrl->session_times);
    }

    free(ctrl);
}
