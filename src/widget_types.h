#ifndef __WIDGET_TYPES_H__
#define __WIDGET_TYPES_H__

// ════════════════════════════════════════════════════════════════════════════
//  TYPES DE WIDGETS SUPPORTÉS
// ════════════════════════════════════════════════════════════════════════════
typedef enum {
    WIDGET_TYPE_INCREMENT,    // Widget numérique avec flèches
    WIDGET_TYPE_TOGGLE,       // Interrupteur ON/OFF
    WIDGET_TYPE_SLIDER,       // Curseur (pour plus tard)
    WIDGET_TYPE_BUTTON,       // Bouton simple (pour plus tard)
    WIDGET_TYPE_SELECTOR      // Sélecteur à options (pour plus tard)
} WidgetType;

// ════════════════════════════════════════════════════════════════════════════
//  CONFIGURATION GÉNÉRIQUE D'UN WIDGET
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    WidgetType type;
    const char* id;
    const char* display_name;
    int x, y;

    // Configuration spécifique au type
    union {
        struct { // WIDGET_TYPE_INCREMENT
            int min_value;
            int max_value;
            int default_value;
            int increment;
        } increment_config;

        struct { // WIDGET_TYPE_TOGGLE
            bool default_state;
        } toggle_config;
    };

} WidgetConfig;

#endif
