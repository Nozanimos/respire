#include "technique_instance.h"
#include "core/memory/memory.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations des constructeurs de techniques
extern TechniqueInstance* whm_create(SDL_Renderer* renderer);

/**
 * Factory pour créer une instance de technique selon son nom
 */
TechniqueInstance* technique_create(const char* technique_name, SDL_Renderer* renderer) {
    if (strcmp(technique_name, "whm") == 0) {
        return whm_create(renderer);
    }
    // Future: box_breathing, 4-7-8, etc.

    return NULL;  // Technique inconnue
}

/**
 * Détruit proprement une instance de technique
 */
void technique_destroy(TechniqueInstance* instance) {
    if (!instance) return;

    if (instance->cleanup) {
        instance->cleanup(instance);
    }

    SAFE_FREE(instance);
}
