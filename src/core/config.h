// SPDX-License-Identifier: GPL-3.0-or-later
// config.h
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>
#include <SDL2/SDL2_gfxPrimitives.h>


// nombre d'hexagones
#define NB_HX 4
#define NB_SIDE 6

// Configuration centralisée des angles
#define ANGLE_1     20.0
#define ANGLE_2     30.0
#define ANGLE_3     40.0
#define ANGLE_4     60.0

// Configuration animation
#define TARGET_FPS          60
#define BREATH_DURATION     3.0f

// Fichier de configuration
#define CONFIG_FILE "../config/respiration.conf"

// Tableaux pour les angles et directions des hexagones (utilisant les defines ci-dessus)
static const double HEXAGON_ANGLES[] = {ANGLE_1, ANGLE_2, ANGLE_3, ANGLE_4};
static const bool HEXAGON_DIRECTIONS[] = {true, false, true, false}; // Horaire, Anti-horaire

// ════════════════════════════════════════════════════════════════════════════
// STRUCTURE DE CONFIGURATION
// ════════════════════════════════════════════════════════════════════════════
// Contient UNIQUEMENT les valeurs métier utilisées par l'application
typedef struct {
    // Paramètres respiration
    int nb_session;             // Nombre de cycles (1-16)
    float breath_duration;      // Durée d'un cycle en secondes (1.0-10.0)
    bool alternate_cycles;      // Alterner sens rotation entre cycles
    int Nb_respiration;         // Nombre de respirations (10-60)

    // Configuration de rétention (nouveau système)
    int retention_pattern;      // Pattern: 0=pleins, 1=vides, 2=alterné, 3=2v-1p, 4=1v-2p, 5=custom
    bool retention_start_empty; // Pour patterns alternés: commencer par vide (true) ou plein (false)

    // Nouveau système de pattern personnalisé (actif quand retention_pattern == 5)
    int pattern_seq1_type;      // Séquence 1: 0=pleins, 1=vides
    int pattern_seq1_count;     // Séquence 1: nombre de sessions (1-10)
    int pattern_seq2_type;      // Séquence 2: 0=pleins, 1=vides
    int pattern_seq2_count;     // Séquence 2: nombre de sessions (1-10)

    // Ancien système (gardé pour compatibilité fichier config)
    int retention_type;         // DEPRECATED: 0=poumons pleins, 1=poumons vides, 2=alternée

    // Timer de démarrage
    int start_duration;         // Secondes avant démarrage (3-60)

    // Paramètres d'affichage (non sauvegardés dans le fichier)
    int screen_width;
    int screen_height;
    bool fullscreen;

} AppConfig;

// Forward declaration pour éviter include circulaire
struct WidgetList;
typedef struct WidgetList WidgetList;

// ════════════════════════════════════════════════════════════════════════════
// PROTOTYPES
// ════════════════════════════════════════════════════════════════════════════

/**
 * Charge la configuration depuis le fichier respiration.conf
 * Appelée au démarrage de l'application AVANT la création des widgets
 * @param config Structure à remplir avec les valeurs du fichier
 */
void load_config(AppConfig* config);

/**
 * Sauvegarde la configuration dans le fichier respiration.conf
 * @param config Structure contenant les valeurs à sauvegarder
 */
void save_config(const AppConfig* config);

/**
 * Synchronise AppConfig → Widgets
 * Parcourt la widget_list et initialise chaque widget avec sa valeur depuis config
 * Fonction GÉNÉRIQUE : ajouter un nouveau widget = ajouter 1 case dans le switch
 * @param config Configuration source
 * @param list Liste des widgets à synchroniser
 */
void sync_config_to_widgets(AppConfig* config, WidgetList* list);

/**
 * Synchronise Widgets → AppConfig → Fichier
 * Parcourt la widget_list, récupère les valeurs et sauvegarde
 * Fonction GÉNÉRIQUE : ajouter un nouveau widget = ajouter 1 case dans le switch
 * @param list Liste des widgets sources
 * @param config Configuration destination
 */
void sync_widgets_to_config(WidgetList* list, AppConfig* config);

#endif
