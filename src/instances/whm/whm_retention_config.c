// SPDX-License-Identifier: GPL-3.0-or-later
#include "whm_retention_config.h"

/**
 * Créer une configuration de rétention
 */
RetentionConfig retention_config_create(RetentionPattern pattern, bool start_with_empty) {
    RetentionConfig config;
    config.pattern = pattern;
    config.start_with_empty = start_with_empty;
    // Initialiser les champs custom avec des valeurs par défaut
    config.custom_seq1_type = 1;   // vides
    config.custom_seq1_count = 1;
    config.custom_seq2_type = 0;   // pleins
    config.custom_seq2_count = 2;
    return config;
}

/**
 * Créer une configuration de rétention personnalisée
 */
RetentionConfig retention_config_create_custom(int seq1_type, int seq1_count,
                                               int seq2_type, int seq2_count) {
    RetentionConfig config;
    config.pattern = RETENTION_PATTERN_CUSTOM;
    config.start_with_empty = false;  // Non utilisé en mode custom
    config.custom_seq1_type = seq1_type;
    config.custom_seq1_count = seq1_count;
    config.custom_seq2_type = seq2_type;
    config.custom_seq2_count = seq2_count;
    return config;
}

/**
 * Détermine si une session donnée doit utiliser poumons vides
 */
bool retention_config_should_use_empty_lungs(const RetentionConfig* config, int session_number) {
    // Les numéros de session commencent à 1
    int zero_based = session_number - 1;

    switch (config->pattern) {
        case RETENTION_PATTERN_FULL:
            // Toujours poumons pleins
            return false;

        case RETENTION_PATTERN_EMPTY:
            // Toujours poumons vides
            return true;

        case RETENTION_PATTERN_ALTERNATING:
            // Alterne 1-1
            if (config->start_with_empty) {
                // vide (0), plein (1), vide (2), plein (3)...
                return (zero_based % 2 == 0);
            } else {
                // plein (0), vide (1), plein (2), vide (3)...
                return (zero_based % 2 == 1);
            }

        case RETENTION_PATTERN_2EMPTY_1FULL:
            // Pattern : vide, vide, plein, vide, vide, plein...
            // Position dans le cycle de 3
            if (config->start_with_empty) {
                int pos = zero_based % 3;
                return (pos != 2);  // Plein uniquement à position 2
            } else {
                // Si on commence par plein : plein, vide, vide, plein, vide, vide...
                int pos = zero_based % 3;
                return (pos != 0);  // Plein uniquement à position 0
            }

        case RETENTION_PATTERN_1EMPTY_2FULL:
            // Pattern : vide, plein, plein, vide, plein, plein...
            // Position dans le cycle de 3
            if (config->start_with_empty) {
                int pos = zero_based % 3;
                return (pos == 0);  // Vide uniquement à position 0
            } else {
                // Si on commence par plein : plein, plein, vide, plein, plein, vide...
                int pos = zero_based % 3;
                return (pos == 2);  // Vide uniquement à position 2
            }

        case RETENTION_PATTERN_CUSTOM: {
            // Pattern personnalisé avec deux séquences alternantes
            int cycle_length = config->custom_seq1_count + config->custom_seq2_count;

            // Sécurité : si cycle invalide, fallback sur poumons vides
            if (cycle_length <= 0) {
                return true;
            }

            int pos = zero_based % cycle_length;

            if (pos < config->custom_seq1_count) {
                // Dans la séquence 1
                return (config->custom_seq1_type == 1);
            } else {
                // Dans la séquence 2
                return (config->custom_seq2_type == 1);
            }
        }

        default:
            return true;  // Par défaut poumons vides
    }
}

/**
 * Obtenir le nom du pattern (pour affichage)
 */
const char* retention_pattern_get_name(RetentionPattern pattern) {
    switch (pattern) {
        case RETENTION_PATTERN_FULL:
            return "Poumons pleins";
        case RETENTION_PATTERN_EMPTY:
            return "Poumons vides";
        case RETENTION_PATTERN_ALTERNATING:
            return "Alterné (1-1)";
        case RETENTION_PATTERN_2EMPTY_1FULL:
            return "2 vides → 1 plein";
        case RETENTION_PATTERN_1EMPTY_2FULL:
            return "1 vide → 2 pleins";
        case RETENTION_PATTERN_CUSTOM:
            return "Pattern personnalisé";
        default:
            return "Inconnu";
    }
}

/**
 * Convertir l'ancien retention_type vers le nouveau système
 */
RetentionConfig retention_config_from_legacy_type(int old_type) {
    RetentionConfig config;

    switch (old_type) {
        case 0:
            // Ancien type 0 = poumons pleins
            config.pattern = RETENTION_PATTERN_FULL;
            config.start_with_empty = false;
            break;

        case 1:
            // Ancien type 1 = poumons vides
            config.pattern = RETENTION_PATTERN_EMPTY;
            config.start_with_empty = true;
            break;

        case 2:
            // Ancien type 2 = alterné
            config.pattern = RETENTION_PATTERN_ALTERNATING;
            config.start_with_empty = true;  // Par défaut commencer par vide
            break;

        default:
            // Fallback : poumons vides
            config.pattern = RETENTION_PATTERN_EMPTY;
            config.start_with_empty = true;
            break;
    }

    return config;
}
