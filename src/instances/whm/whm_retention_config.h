#ifndef WHM_RETENTION_CONFIG_H
#define WHM_RETENTION_CONFIG_H

#include <stdbool.h>

/**
 * Patterns de rétention disponibles pour la méthode Wim Hof
 */
typedef enum {
    RETENTION_PATTERN_FULL,          // Toujours poumons pleins
    RETENTION_PATTERN_EMPTY,         // Toujours poumons vides
    RETENTION_PATTERN_ALTERNATING,   // Alterne 1-1 (ex: vide, plein, vide, plein...)
    RETENTION_PATTERN_2EMPTY_1FULL,  // 2 vides, 1 plein (ex: vide, vide, plein, vide, vide, plein...)
    RETENTION_PATTERN_1EMPTY_2FULL,  // 1 vide, 2 pleins (ex: vide, plein, plein, vide, plein, plein...)
    RETENTION_PATTERN_CUSTOM,        // Pattern personnalisé avec rollers
} RetentionPattern;

/**
 * Configuration de la rétention
 */
typedef struct {
    RetentionPattern pattern;    // Pattern choisi
    bool start_with_empty;       // Pour patterns alternés : commencer par vide (true) ou plein (false)

    // NOUVEAUX champs pour RETENTION_PATTERN_CUSTOM
    int custom_seq1_type;        // Séquence 1: 0=pleins, 1=vides
    int custom_seq1_count;       // Séquence 1: nombre de sessions (1-10)
    int custom_seq2_type;        // Séquence 2: 0=pleins, 1=vides
    int custom_seq2_count;       // Séquence 2: nombre de sessions (1-10)
} RetentionConfig;

/**
 * Créer une configuration de rétention
 * @param pattern Pattern à utiliser
 * @param start_with_empty true pour commencer par poumons vides, false pour poumons pleins
 * @return Configuration de rétention
 */
RetentionConfig retention_config_create(RetentionPattern pattern, bool start_with_empty);

/**
 * Détermine si une session donnée doit utiliser poumons vides
 * @param config Configuration de rétention
 * @param session_number Numéro de la session (commence à 1)
 * @return true si poumons vides, false si poumons pleins
 */
bool retention_config_should_use_empty_lungs(const RetentionConfig* config, int session_number);

/**
 * Obtenir le nom du pattern (pour affichage)
 * @param pattern Pattern
 * @return Nom du pattern
 */
const char* retention_pattern_get_name(RetentionPattern pattern);

/**
 * Créer une configuration de rétention personnalisée (pattern custom)
 * @param seq1_type Séquence 1: 0=pleins, 1=vides
 * @param seq1_count Séquence 1: nombre de sessions (1-10)
 * @param seq2_type Séquence 2: 0=pleins, 1=vides
 * @param seq2_count Séquence 2: nombre de sessions (1-10)
 * @return Configuration de rétention en mode custom
 */
RetentionConfig retention_config_create_custom(int seq1_type, int seq1_count,
                                               int seq2_type, int seq2_count);

/**
 * Convertir l'ancien retention_type (0,1,2) vers le nouveau système
 * Pour compatibilité avec l'ancienne config
 * @param old_type Ancien type (0=plein, 1=vide, 2=alterné)
 * @return Configuration de rétention correspondante
 */
RetentionConfig retention_config_from_legacy_type(int old_type);

#endif // WHM_RETENTION_CONFIG_H
