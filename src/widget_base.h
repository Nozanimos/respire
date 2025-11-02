#ifndef __WIDGET_BASE_H__
#define __WIDGET_BASE_H__

#include <stdbool.h>
#include <SDL2/SDL_ttf.h>

// ════════════════════════════════════════════════════════════════════════════
//  WIDGET BASE - Structure commune à TOUS les widgets
// ════════════════════════════════════════════════════════════════════════════
// Cette structure définit le "conteneur" de base de chaque widget.
//
// PRINCIPE FONDAMENTAL :
// ----------------------
// Chaque widget est une BOÎTE (bounding box) avec :
//   - UNE SEULE position (x, y) RELATIVE au panneau parent
//   - UNE SEULE dimension (width, height)
//   - Tous les éléments internes en coordonnées LOCALES (offsets)
//
// COORDONNÉES :
// -------------
//   Position RELATIVE : par rapport au panneau (ou conteneur parent)
//   Position LOCALE   : par rapport au coin (0,0) du widget lui-même
//   Position ABSOLUE  : sur l'écran (= relative + locale + offset du panneau)
//
// EXEMPLE :
// ---------
//   Panneau à x=100
//   Widget à base.x=20 (relatif au panneau)
//   Texte à local_x=0 (relatif au widget)
//   → Position écran du texte = 100 + 20 + 0 = 120 pixels
//
// RESCALING :
// -----------
//   On scale base.x, base.y, base.width, base.height
//   Les offsets locaux sont RECALCULÉS en remésurant le texte avec la nouvelle police

typedef struct {
    // ─────────────────────────────────────────────────────────────────────────
    // POSITION DU WIDGET (relative au panneau parent)
    // ─────────────────────────────────────────────────────────────────────────
    int x, y;                    // Position actuelle (après scaling)
    int base_x, base_y;          // Position de référence (scale = 1.0, panel = 500px)

    // ─────────────────────────────────────────────────────────────────────────
    // DIMENSIONS DU WIDGET (bounding box complète)
    // ─────────────────────────────────────────────────────────────────────────
    int width, height;           // Dimensions actuelles (après scaling)
    int base_width, base_height; // Dimensions de référence (scale = 1.0)

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT D'INTERACTION
    // ─────────────────────────────────────────────────────────────────────────
    bool hovered;                // TRUE si souris survole le widget entier
    bool enabled;                // TRUE si le widget est actif (cliquable)

} WidgetBase;

// ════════════════════════════════════════════════════════════════════════════
//  GESTIONNAIRE DE CACHE DE POLICES
// ════════════════════════════════════════════════════════════════════════════
// Ce système permet de charger plusieurs tailles de police sans les recharger
// à chaque frame, optimisant ainsi les performances.
//
// FONCTIONNEMENT :
//   1. Première demande de taille 18px → charge et cache
//   2. Demande suivante de 18px → réutilise du cache (instantané)
//   3. Demande de 16px → charge et cache cette nouvelle taille
//
// LIMITE : 10 tailles différentes en cache (largement suffisant)

#define MAX_CACHED_FONTS 10      // Nombre max de polices en cache
#define MIN_FONT_SIZE 12         // Taille minimum

// Structure d'une police en cache
typedef struct {
    TTF_Font* font;              // Pointeur vers la police SDL_ttf
    int size;                    // Taille en pixels de cette police
    bool in_use;                 // TRUE si ce slot est utilisé
} CachedFont;

// Cache global (partagé entre tous les widgets)
// Note : Déclaré ici, défini dans widget.c
extern CachedFont g_font_cache[MAX_CACHED_FONTS];
extern int g_font_cache_count;
extern char g_font_path[256];    // Chemin vers la police par défaut

// ════════════════════════════════════════════════════════════════════════════
//  FONCTIONS DU GESTIONNAIRE DE POLICES
// ════════════════════════════════════════════════════════════════════════════

// Initialise le gestionnaire avec le chemin de la police
// À appeler UNE FOIS au démarrage de l'application
void init_font_manager(const char* font_path);

// Obtient une police à la taille demandée (crée ou réutilise du cache)
// Applique automatiquement le minimum de MIN_FONT_SIZE
// Retourne NULL en cas d'erreur
TTF_Font* get_font_for_size(int size);

// Libère toutes les polices du cache
// À appeler à la fermeture de l'application
void cleanup_font_manager(void);

// ════════════════════════════════════════════════════════════════════════════
//  FONCTION UTILITAIRE - Rescaling de la base
// ════════════════════════════════════════════════════════════════════════════
// Cette fonction est appelée par tous les widgets pour rescaler leur position
// et dimensions de base. Les éléments internes sont ensuite recalculés par
// chaque widget selon ses propres offsets locaux.

static inline void rescale_widget_base(WidgetBase* base, float panel_ratio) {
    if (!base) return;

    // Scaler la position relative au panneau
    base->x = (int)(base->base_x * panel_ratio);
    base->y = (int)(base->base_y * panel_ratio);

    // Scaler les dimensions
    base->width = (int)(base->base_width * panel_ratio);
    base->height = (int)(base->base_height * panel_ratio);
}

// ════════════════════════════════════════════════════════════════════════════
//  HIT DETECTION - Test si un point est dans le widget
// ════════════════════════════════════════════════════════════════════════════
// Teste si les coordonnées (mx, my) sont à l'intérieur de la bounding box
// du widget. Prend en compte l'offset du panneau parent.

static inline bool widget_contains_point(WidgetBase* base, int mx, int my,
                                         int panel_offset_x, int panel_offset_y) {
    if (!base) return false;

    // Position absolue du widget à l'écran
    int abs_x = panel_offset_x + base->x;
    int abs_y = panel_offset_y + base->y;

    // Test d'inclusion
    return (mx >= abs_x && mx < abs_x + base->width &&
            my >= abs_y && my < abs_y + base->height);
}

#endif // __WIDGET_BASE_H__
