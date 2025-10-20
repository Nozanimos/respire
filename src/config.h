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

typedef struct {
    // Paramètres respiration
    int breath_cycles;
    float breath_duration;
    bool alternate_cycles;

    // Paramètres d'affichage
    int screen_width;
    int screen_height;
    bool fullscreen;

    // Timer
    int startup_delay; // secondes avant démarrage

} AppConfig;


// Prototypes
void load_config(AppConfig* config);
void save_config(const AppConfig* config);
void apply_config(const AppConfig* config);

#endif
